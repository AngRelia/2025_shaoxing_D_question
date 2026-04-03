/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * [注释内容略，保留原有版权声明]
 */

#include "ti_msp_dl_config.h"
#include "OLED.h"
#include "Serial.h"
#include "key.h"
#include "dc_motor.h"
#include "encoder.h"
#include "pid.h"
#include "grayscale_sensor.h"
#include <string.h>
#include <stdio.h>

static volatile uint8_t g_oledRefreshFlag = 1U;
static volatile uint8_t g_speedPidUpdateFlag = 0U;
static volatile uint8_t g_speedReportEventFlag = 0U;
static uint8_t g_taskId = 0U;

static int16_t g_leftDutyCmd = 0;
static int16_t g_rightDutyCmd = 0;

// 增加一个全局的目标速度变量，方便你调试时修改（单位：m/s）
static float g_targetSpeedMps = 0.7f; 

static int16_t clamp_duty(int16_t duty)
{
    if (duty > 100) {
        return 100;
    }
    if (duty < -100) {
        return -100;
    }
    return duty;
}

int main(void)
{
    char grayBitsStr[17];
    char speedLine[64];
    char pidRspLine[64];
    char rxLocal[SERIAL0_PACKET_SIZE];
    DCMotor_Status_t motorStatus;
    float leftSpeed;
    float rightSpeed;
    uint8_t grayLeftDigital;
    uint8_t grayRightDigital;
    uint8_t grayLeftRet;
    uint8_t grayRightRet;
    float kpLeftNew;
    float kiLeftNew;
    float kdLeftNew;
    float kpRightNew;
    float kiRightNew;
    float kdRightNew;
    float targetSpeedNew;
    uint8_t bitIdx;
    uint8_t keyNum;

    SYSCFG_DL_init();
    Serial0_Init();
    DCMotor_Init();
    encoder_init();
    
    // ================= 替换旧的 PID 初始化 =================
    PID_Init(); 
    // =======================================================
    
    DCMotor_Enable(1U);
    DCMotor_SetDuty(0, 0);

    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, "Speed PID Ready", OLED_8X16);
    OLED_Update();

    /* 灰度模块初始化：左右都做连通与通道使能 */
    (void)IIC_Probe(GW_GRAY_ADDR_SENSOR1);
    (void)IIC_Set_Channel_Enable(GW_GRAY_ADDR_SENSOR1, 0xFFU);
    (void)IIC_Probe(GW_GRAY_ADDR_SENSOR2);
    (void)IIC_Set_Channel_Enable(GW_GRAY_ADDR_SENSOR2, 0xFFU);

    NVIC_ClearPendingIRQ(TIMER_FOR_1MS_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_FOR_1MS_INST_INT_IRQN);
    DL_TimerA_startCounter(TIMER_FOR_1MS_INST);

    while (1) {
        keyNum = Key_GetNum();
        if (keyNum == 1U) {
            /* 改回任务1：按键启动电机任务，并重置 PID 历史 */
            g_taskId = 1U;
            PID_Reset(MOTOR_LEFT);
            PID_Reset(MOTOR_RIGHT);
        }else if (keyNum == 2U) {
            g_taskId = 2U;
        }else if (keyNum == 3U) {
            g_taskId = 3U;
        }else if (keyNum == 4U) {
            g_taskId = 4U;
        }
        

        // ================= 20ms PID 控制周期 =================
        if (g_speedPidUpdateFlag) {
            __disable_irq();
            g_speedPidUpdateFlag = 0U;
            __enable_irq();

            leftSpeed = encoder_get_left_speed_mps();
            rightSpeed = encoder_get_right_speed_mps();

            if (g_taskId == 1U) {
                // 1. 调用新的 PID_Calculate_Step 计算，得到归一化输出（±1.0）
                float out_l = PID_Calculate_Step(&PID_Left_Speed, g_targetSpeedMps, leftSpeed);
                float out_r = PID_Calculate_Step(&PID_Right_Speed, g_targetSpeedMps, rightSpeed);

                // 2. 仅使用 PID 输出，并放大到 ±100 的占空比范围内
                g_leftDutyCmd = (int16_t)(out_l * 100.0f);
                g_rightDutyCmd = (int16_t)(out_r * 100.0f);
            } else {
                g_leftDutyCmd = 0;
                g_rightDutyCmd = 0;
            }
            
            // 安全钳位并输出PWM
            g_leftDutyCmd = clamp_duty(g_leftDutyCmd);
            g_rightDutyCmd = clamp_duty(g_rightDutyCmd);

            //DCMotor_SetDuty(50,50);
            DCMotor_SetDuty(g_leftDutyCmd, g_rightDutyCmd);
        }

        // ================= 100ms 串口波形数据上报 =================
        if (g_speedReportEventFlag) {
            int speedLen;

            __disable_irq();
            g_speedReportEventFlag = 0U;
            __enable_irq();

            leftSpeed = encoder_get_left_speed_mps();
            rightSpeed = encoder_get_right_speed_mps();

            speedLen = snprintf(speedLine,
                                sizeof(speedLine),
                                "%.2f,%.3f,%.3f\n",
                                (float)g_targetSpeedMps,
                                (float)leftSpeed,
                                (float)rightSpeed);
            if (speedLen > 0) {
                Serial0_SendStringBlocking(speedLine);
            }
        }

        // ================= 串口接收指令解析 =================
        if (Serial0_RxFlag != 0U) {
            int parseCount;

            __disable_irq();
            Serial0_RxFlag = 0U;
            (void)strncpy(rxLocal, Serial0_RxPacket, SERIAL0_PACKET_SIZE - 1U);
            rxLocal[SERIAL0_PACKET_SIZE - 1U] = '\0';
            __enable_irq();

            /*
             * 串口参数格式：
             * @ L_KP L_KI L_KD R_KP R_KI R_KD TARGET_SPEED
             * 例如：@ 0.6 0.05 0.0 0.7 0.04 0.0 0.80
             * 兼容旧格式（不带目标速度）：
             * @ L_KP L_KI L_KD R_KP R_KI R_KD
             * 兼容前缀 '%'（若上位机发 % 作为起始符）
             */
            if ((rxLocal[0] == '@') || (rxLocal[0] == '%')) {
                parseCount = sscanf(&rxLocal[1],
                                    "%f %f %f %f %f %f %f",
                                    &kpLeftNew,
                                    &kiLeftNew,
                                    &kdLeftNew,
                                    &kpRightNew,
                                    &kiRightNew,
                                    &kdRightNew,
                                    &targetSpeedNew);
                if ((parseCount == 6) || (parseCount == 7)) {
                    PID_SetParameters(MOTOR_LEFT, kpLeftNew, kiLeftNew, kdLeftNew);
                    PID_SetParameters(MOTOR_RIGHT, kpRightNew, kiRightNew, kdRightNew);
                    if (parseCount == 7) {
                        g_targetSpeedMps = targetSpeedNew;
                    }
                    (void)snprintf(pidRspLine,
                                   sizeof(pidRspLine),
                                   "@PID:OK L(%.3f,%.3f,%.3f) R(%.3f,%.3f,%.3f) T(%.3f)\r\n",
                                   (float)PID_Left_Speed.Kp,
                                   (float)PID_Left_Speed.Ki,
                                   (float)PID_Left_Speed.Kd,
                                   (float)PID_Right_Speed.Kp,
                                   (float)PID_Right_Speed.Ki,
                                   (float)PID_Right_Speed.Kd,
                                   (float)g_targetSpeedMps);
                    Serial0_SendString(pidRspLine);
                } else {
                    Serial0_SendString("@PID:ERR fmt=@Lkp Lki Lkd Rkp Rki Rkd [Tg]\r\n");
                }
            }
        }

        // ================= OLED 显示刷新 =================
        if (g_oledRefreshFlag) {
            g_oledRefreshFlag = 0U;

            DCMotor_GetStatus(&motorStatus);
            leftSpeed = encoder_get_left_speed_mps();
            rightSpeed = encoder_get_right_speed_mps();

            grayLeftRet = IIC_Get_Digtal_Ex(GW_GRAY_ADDR_SENSOR1, &grayLeftDigital);
            if (grayLeftRet == GW_GRAY_OK) {
                grayscale_byte_to_sensor_array(grayLeftDigital, 0U);
            } else {
                /* 失败后尝试恢复：重新探测并重新使能通道 */
                grayLeftDigital = 0U;
                grayscale_byte_to_sensor_array(0U, 0U);
                (void)IIC_Probe(GW_GRAY_ADDR_SENSOR1);
                (void)IIC_Set_Channel_Enable(GW_GRAY_ADDR_SENSOR1, 0xFFU);
            }

            grayRightRet = IIC_Get_Digtal_Ex(GW_GRAY_ADDR_SENSOR2, &grayRightDigital);
            if (grayRightRet == GW_GRAY_OK) {
                grayscale_byte_to_sensor_array(grayRightDigital, 8U);
            } else {
                grayRightDigital = 0U;
                grayscale_byte_to_sensor_array(0U, 8U);
                (void)IIC_Probe(GW_GRAY_ADDR_SENSOR2);
                (void)IIC_Set_Channel_Enable(GW_GRAY_ADDR_SENSOR2, 0xFFU);
            }

            for (bitIdx = 0U; bitIdx < 16U; bitIdx++) {
                grayBitsStr[bitIdx] = (sensor[bitIdx] != 0U) ? '1' : '0';
            }
            grayBitsStr[16] = '\0';

            OLED_Clear();
            OLED_Printf(0, 0, OLED_6X8,
                        "GL%u GR%u",
                        (unsigned int)((grayLeftRet == GW_GRAY_OK) ? 1U : 0U),
                        (unsigned int)((grayRightRet == GW_GRAY_OK) ? 1U : 0U));
            OLED_Printf(0, 8, OLED_6X8,
                        "LP%.2f I%.2f D%.2f",
                        (float)PID_Left_Speed.Kp,
                        (float)PID_Left_Speed.Ki,
                        (float)PID_Left_Speed.Kd);
            OLED_Printf(0, 16, OLED_6X8,
                        "RP%.2f I%.2f D%.2f",
                        (float)PID_Right_Speed.Kp,
                        (float)PID_Right_Speed.Ki,
                        (float)PID_Right_Speed.Kd);
            OLED_Printf(0, 24, OLED_6X8,
                        "L%4d%% %s",
                        motorStatus.left_duty_percent,
                        DCMotor_DirectionString(motorStatus.left_dir));
            OLED_Printf(0, 32, OLED_6X8,
                        "Lv:%1.3f m/s",
                        (float)leftSpeed);
            OLED_Printf(0, 40, OLED_6X8,
                        "R%4d%% %s",
                        motorStatus.right_duty_percent,
                        DCMotor_DirectionString(motorStatus.right_dir));
            OLED_Printf(0, 48, OLED_6X8,
                        "Rv:%1.3f m/s",
                        (float)rightSpeed);
            OLED_Printf(0, 56, OLED_6X8, "%s", grayBitsStr);
            OLED_Update();
        }
    }
}

void TIMER_FOR_1MS_INST_IRQHandler(void)
{
    DL_TimerA_clearInterruptStatus(TIMER_FOR_1MS_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT);
    static uint16_t tick20ms = 0U;
    static uint16_t tick200ms = 0U;

    Key_Tick();

    tick20ms++;
    tick200ms++;

    if (tick20ms >= 20U) {
        tick20ms = 0U;
        g_speedPidUpdateFlag = 1U;
        g_oledRefreshFlag = 1U;
    }

    if (tick200ms >= 100U) {
        tick200ms = 0U;
        g_speedReportEventFlag = 1U;
    }
}


//明天的任务 编码器电机正负
