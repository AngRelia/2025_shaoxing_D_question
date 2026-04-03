#include "encoder.h"

#include "ti_msp_dl_config.h"

/* 按你的要求：
 * 左轮编码器: E1A 暂留，E1B 测速
 * 右轮编码器: E2A 测速，E2B 暂留
 */
#define ENCODER_LEFT_RSVD_INST            E1A_INST
#define ENCODER_LEFT_RSVD_INST_INT_IRQN   E1A_INST_INT_IRQN
#define ENCODER_LEFT_MEAS_INST            E1B_INST
#define ENCODER_LEFT_MEAS_INST_INT_IRQN   E1B_INST_INT_IRQN

#define ENCODER_RIGHT_MEAS_INST           E2A_INST
#define ENCODER_RIGHT_MEAS_INST_INT_IRQN  E2A_INST_INT_IRQN
#define ENCODER_RIGHT_RSVD_INST           E2B_INST
#define ENCODER_RIGHT_RSVD_INST_INT_IRQN  E2B_INST_INT_IRQN

#define ENCODER_PI                   (3.1415926f)

static volatile int32_t g_right_count = 0;
static volatile int32_t g_left_count  = 0;

static volatile float g_right_speed_mps = 0.0f;
static volatile float g_left_speed_mps  = 0.0f;

static volatile float g_right_rpm = 0.0f;
static volatile float g_left_rpm  = 0.0f;

static volatile uint8_t g_speed_update_flag = 0;

static float encoder_count_to_rpm(int32_t count)
{
    const float rev_per_sample = ((float) count) /
                                 (ENCODER_PPR * ENCODER_GEAR_RATIO * ENCODER_EDGE_FACTOR);
    return rev_per_sample * (60.0f / ENCODER_SAMPLE_PERIOD_S);
}

static float rpm_to_mps(float rpm)
{
    const float wheel_circ = ENCODER_PI * ENCODER_WHEEL_DIAMETER_M;
    return (rpm / 60.0f) * wheel_circ;
}

void encoder_init(void)
{
    g_right_count = 0;
    g_left_count  = 0;
    g_right_speed_mps = 0.0f;
    g_left_speed_mps  = 0.0f;
    g_right_rpm = 0.0f;
    g_left_rpm  = 0.0f;
    g_speed_update_flag = 0;

    /*
     * E1A / E2B 只作为“暂留相”电平读取使用。
     * 将其切回普通数字输入，确保 DL_GPIO_readPins() 能读到真实高低电平。
     */
    DL_GPIO_initDigitalInputFeatures(GPIO_E1A_C0_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(GPIO_E2B_C0_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);

    NVIC_ClearPendingIRQ(ENCODER_LEFT_RSVD_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(ENCODER_LEFT_MEAS_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(ENCODER_RIGHT_MEAS_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(ENCODER_RIGHT_RSVD_INST_INT_IRQN);

    NVIC_EnableIRQ(ENCODER_LEFT_RSVD_INST_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_LEFT_MEAS_INST_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_RIGHT_MEAS_INST_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_RIGHT_RSVD_INST_INT_IRQN);

    DL_TimerG_startCounter(ENCODER_LEFT_RSVD_INST);
    DL_TimerA_startCounter(ENCODER_LEFT_MEAS_INST);
    DL_TimerG_startCounter(ENCODER_RIGHT_MEAS_INST);
    DL_TimerG_startCounter(ENCODER_RIGHT_RSVD_INST);
}

float encoder_get_left_speed_mps(void)
{
    return g_left_speed_mps;
}

float encoder_get_right_speed_mps(void)
{
    return g_right_speed_mps;
}

float encoder_get_left_rpm(void)
{
    return g_left_rpm;
}

float encoder_get_right_rpm(void)
{
    return g_right_rpm;
}

uint8_t encoder_is_updated(void)
{
    return g_speed_update_flag;
}

void encoder_clear_update_flag(void)
{
    g_speed_update_flag = 0;
}

void E1A_INST_IRQHandler(void)
{
    /* 左轮A相暂留（当前不参与测速） */
    switch (DL_TimerG_getPendingInterrupt(ENCODER_LEFT_RSVD_INST)) {
        case DL_TIMER_IIDX_CC0_DN:
        case DL_TIMER_IIDX_CC0_UP:
            break;

        case DL_TIMER_IIDX_ZERO:
            /* 预留：可用于方向判定或超时处理 */
            break;

        default:
            break;
    }
}

void E1B_INST_IRQHandler(void)
{
    /* 左轮B相测速 */
    switch (DL_TimerA_getPendingInterrupt(ENCODER_LEFT_MEAS_INST)) {
        case DL_TIMER_IIDX_CC0_DN: /* 边沿捕获事件 */
        {
            uint8_t aLevel =
                (DL_GPIO_readPins(GPIO_E1A_C0_PORT, GPIO_E1A_C0_PIN) != 0U) ? 1U : 0U;
            uint8_t bLevel =
                (DL_GPIO_readPins(GPIO_E1B_C0_PORT, GPIO_E1B_C0_PIN) != 0U) ? 1U : 0U;

            /* 正交解码：左轮以 (A != B) 为正向，(A == B) 为反向 */
            if (aLevel != bLevel) {
                g_left_count++;
            } else {
                g_left_count--;
            }
            break;
        }

        case DL_TIMER_IIDX_ZERO:
        {
            g_left_rpm = encoder_count_to_rpm(g_left_count) * ENCODER_LEFT_DIR_SIGN;
            g_left_speed_mps = rpm_to_mps(g_left_rpm);
            g_left_count     = 0;
            g_speed_update_flag = 1;
            break;
        }

        default:
            break;
    }
}

void E2A_INST_IRQHandler(void)
{
    /* 右轮A相测速 */
    switch (DL_TimerG_getPendingInterrupt(ENCODER_RIGHT_MEAS_INST)) {
        case DL_TIMER_IIDX_CC0_DN: /* 边沿捕获事件 */
        {
            uint8_t aLevel =
                (DL_GPIO_readPins(GPIO_E2A_C0_PORT, GPIO_E2A_C0_PIN) != 0U) ? 1U : 0U;
            uint8_t bLevel =
                (DL_GPIO_readPins(GPIO_E2B_C0_PORT, GPIO_E2B_C0_PIN) != 0U) ? 1U : 0U;

            /* 正交解码：右轮物理镜像安装，方向规则与左轮相反 */
            if (aLevel == bLevel) {
                g_right_count++;
            } else {
                g_right_count--;
            }
            break;
        }

        case DL_TIMER_IIDX_ZERO:
        {
            g_right_rpm = encoder_count_to_rpm(g_right_count) * ENCODER_RIGHT_DIR_SIGN;
            g_right_speed_mps = rpm_to_mps(g_right_rpm);
            g_right_count     = 0;
            g_speed_update_flag = 1;
            break;
        }

        default:
            break;
    }
}

void E2B_INST_IRQHandler(void)
{
    /* 右轮B相预留（当前不参与计数） */
    switch (DL_TimerG_getPendingInterrupt(ENCODER_RIGHT_RSVD_INST)) {
        case DL_TIMER_IIDX_CC0_DN:
        case DL_TIMER_IIDX_CC0_UP:
            break;

        case DL_TIMER_IIDX_ZERO:
            /* 预留：可用于方向判定或超时处理 */
        default:
            break;
    }
}
