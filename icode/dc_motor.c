#include "dc_motor.h"

#include "ti_msp_dl_config.h"

/*
 * 方向反相开关（按实际接线调整）
 * 0: 正常
 * 1: 该侧前进/后退逻辑互换
 */
#ifndef DCMOTOR_LEFT_REVERSE
#define DCMOTOR_LEFT_REVERSE   (0)
#endif

#ifndef DCMOTOR_RIGHT_REVERSE
#define DCMOTOR_RIGHT_REVERSE  (0)
#endif

static DCMotor_Status_t s_motor = {
    .left_duty_percent = 0,
    .right_duty_percent = 0,
    .left_dir = DCMOTOR_DIR_STOP,
    .right_dir = DCMOTOR_DIR_STOP,
    .enabled = 0
};

static int16_t DCMotor_ClampDuty(int16_t duty)
{
    if (duty > 100) return 100;
    if (duty < -100) return -100;
    return duty;
}

static DCMotor_Direction_t DCMotor_ApplyReverse(
    DCMotor_Direction_t dir, uint8_t reverse)
{
    if (!reverse) {
        return dir;
    }
    if (dir == DCMOTOR_DIR_FORWARD) {
        return DCMOTOR_DIR_BACKWARD;
    }
    if (dir == DCMOTOR_DIR_BACKWARD) {
        return DCMOTOR_DIR_FORWARD;
    }
    return DCMOTOR_DIR_STOP;
}

static void DCMotor_SetLeftDirection(DCMotor_Direction_t dir)
{
    DCMotor_Direction_t realDir = DCMotor_ApplyReverse(dir, DCMOTOR_LEFT_REVERSE);

    switch (realDir)
    {
        case DCMOTOR_DIR_FORWARD:
            DL_GPIO_setPins(DC_Motor_AIN1_PORT, DC_Motor_AIN1_PIN);
            DL_GPIO_clearPins(DC_Motor_AIN2_PORT, DC_Motor_AIN2_PIN);
            break;
        case DCMOTOR_DIR_BACKWARD:
            DL_GPIO_clearPins(DC_Motor_AIN1_PORT, DC_Motor_AIN1_PIN);
            DL_GPIO_setPins(DC_Motor_AIN2_PORT, DC_Motor_AIN2_PIN);
            break;
        case DCMOTOR_DIR_STOP:
        default:
            DL_GPIO_clearPins(DC_Motor_AIN1_PORT, DC_Motor_AIN1_PIN);
            DL_GPIO_clearPins(DC_Motor_AIN2_PORT, DC_Motor_AIN2_PIN);
            break;
    }
}

static void DCMotor_SetRightDirection(DCMotor_Direction_t dir)
{
    DCMotor_Direction_t realDir = DCMotor_ApplyReverse(dir, DCMOTOR_RIGHT_REVERSE);

    switch (realDir)
    {
        case DCMOTOR_DIR_FORWARD:
            DL_GPIO_setPins(DC_Motor_BIN1_PORT, DC_Motor_BIN1_PIN);
            DL_GPIO_clearPins(DC_Motor_BIN2_PORT, DC_Motor_BIN2_PIN);
            break;
        case DCMOTOR_DIR_BACKWARD:
            DL_GPIO_clearPins(DC_Motor_BIN1_PORT, DC_Motor_BIN1_PIN);
            DL_GPIO_setPins(DC_Motor_BIN2_PORT, DC_Motor_BIN2_PIN);
            break;
        case DCMOTOR_DIR_STOP:
        default:
            DL_GPIO_clearPins(DC_Motor_BIN1_PORT, DC_Motor_BIN1_PIN);
            DL_GPIO_clearPins(DC_Motor_BIN2_PORT, DC_Motor_BIN2_PIN);
            break;
    }
}

static void DCMotor_SetPwmAbs(DL_TIMER_CC_INDEX ccIndex, uint16_t dutyAbs)
{
    uint32_t load;
    uint32_t compare;

    if (dutyAbs > 100U) {
        dutyAbs = 100U;
    }

    load = DL_Timer_getLoadValue(DC_Motor_PWM_INST);
    compare = ((load + 1U) * dutyAbs) / 100U;
    if (compare > load) {
        compare = load;
    }

    DL_Timer_setCaptureCompareValue(DC_Motor_PWM_INST, compare, ccIndex);
}

static void DCMotor_SetLeftPwmAbs(uint16_t dutyAbs)
{
    DCMotor_SetPwmAbs(GPIO_DC_Motor_PWM_C0_IDX, dutyAbs);
}

static void DCMotor_SetRightPwmAbs(uint16_t dutyAbs)
{
    DCMotor_SetPwmAbs(GPIO_DC_Motor_PWM_C1_IDX, dutyAbs);
}

void DCMotor_Init(void)
{
    /* 由 SysConfig 生成的电机 PWM 初始化 */
    SYSCFG_DL_DC_Motor_PWM_init();

    /* 确保计数器运行，PWM输出生效 */
    DL_Timer_startCounter(DC_Motor_PWM_INST);

    s_motor.enabled = 1U;
    DCMotor_SetDuty(0, 0);
}

void DCMotor_Enable(uint8_t enable)
{
    s_motor.enabled = (enable != 0U) ? 1U : 0U;

    if (!s_motor.enabled)
    {
        DCMotor_SetDuty(0, 0);
        DL_Timer_stopCounter(DC_Motor_PWM_INST);
    }
    else
    {
        DL_Timer_startCounter(DC_Motor_PWM_INST);
    }
}

void DCMotor_SetDuty(int16_t left_duty_percent, int16_t right_duty_percent)
{
    left_duty_percent = DCMotor_ClampDuty(left_duty_percent);
    right_duty_percent = DCMotor_ClampDuty(right_duty_percent);

    s_motor.left_duty_percent = left_duty_percent;
    s_motor.right_duty_percent = right_duty_percent;

    if (!s_motor.enabled)
    {
        s_motor.left_dir = DCMOTOR_DIR_STOP;
        s_motor.right_dir = DCMOTOR_DIR_STOP;
        DCMotor_SetLeftDirection(DCMOTOR_DIR_STOP);
        DCMotor_SetRightDirection(DCMOTOR_DIR_STOP);
        DCMotor_SetLeftPwmAbs(0);
        DCMotor_SetRightPwmAbs(0);
        return;
    }

    if (left_duty_percent > 0)
    {
        s_motor.left_dir = DCMOTOR_DIR_FORWARD;
        DCMotor_SetLeftDirection(DCMOTOR_DIR_FORWARD);
        DCMotor_SetLeftPwmAbs((uint16_t)left_duty_percent);
    }
    else if (left_duty_percent < 0)
    {
        s_motor.left_dir = DCMOTOR_DIR_BACKWARD;
        DCMotor_SetLeftDirection(DCMOTOR_DIR_BACKWARD);
        DCMotor_SetLeftPwmAbs((uint16_t)(-left_duty_percent));
    }
    else
    {
        s_motor.left_dir = DCMOTOR_DIR_STOP;
        DCMotor_SetLeftDirection(DCMOTOR_DIR_STOP);
        DCMotor_SetLeftPwmAbs(0);
    }

    if (right_duty_percent > 0)
    {
        s_motor.right_dir = DCMOTOR_DIR_FORWARD;
        DCMotor_SetRightDirection(DCMOTOR_DIR_FORWARD);
        DCMotor_SetRightPwmAbs((uint16_t)right_duty_percent);
    }
    else if (right_duty_percent < 0)
    {
        s_motor.right_dir = DCMOTOR_DIR_BACKWARD;
        DCMotor_SetRightDirection(DCMOTOR_DIR_BACKWARD);
        DCMotor_SetRightPwmAbs((uint16_t)(-right_duty_percent));
    }
    else
    {
        s_motor.right_dir = DCMOTOR_DIR_STOP;
        DCMotor_SetRightDirection(DCMOTOR_DIR_STOP);
        DCMotor_SetRightPwmAbs(0);
    }
}

void DCMotor_Stop(void)
{
    DCMotor_SetDuty(0, 0);
}

void DCMotor_GetStatus(DCMotor_Status_t *status)
{
    if (status == 0) {
        return;
    }
    *status = s_motor;
}

const char *DCMotor_DirectionString(DCMotor_Direction_t dir)
{
    switch (dir)
    {
        case DCMOTOR_DIR_FORWARD:  return "FWD";
        case DCMOTOR_DIR_BACKWARD: return "BWD";
        case DCMOTOR_DIR_STOP:
        default:                   return "STP";
    }
}
