#include "pid.h"
// 如果需要使用电机控制与速度获取，请引入对应头文件
// #include "motor.h"       
// #include "math.h"        

// ================= 全局变量定义 =================
PID_TypeDef PID_Left_Speed;    
PID_TypeDef PID_Right_Speed;   

float left_pwm = 0.0f;
float right_pwm = 0.0f;

// ================= 内部计算函数 =================

/**
 * @brief  PID 单步计算
 * @param  pid: PID结构体指针
 * @param  target: 目标值
 * @param  actual: 实际反馈值
 * @retval 计算后的PWM输出值
 */
float PID_Calculate_Step(PID_TypeDef *pid, float target, float actual)
{
    pid->Error1 = pid->Error0;
    pid->Error0 = target - actual;
    
    // 积分抗饱和逻辑
    if (pid->Ki != 0.0f) {
        pid->ErrorInt += pid->Error0;
        if (pid->ErrorInt > pid->IntegralMax) pid->ErrorInt = pid->IntegralMax;
        if (pid->ErrorInt < pid->IntegralMin) pid->ErrorInt = pid->IntegralMin;
    }
    
    // 定位式 PID 计算：P + I + D
    pid->Output = pid->Kp * pid->Error0 + 
                  pid->Ki * pid->ErrorInt + 
                  pid->Kd * (pid->Error0 - pid->Error1);
    
    // 输出限幅
    if (pid->Output > pid->OutputMax) pid->Output = pid->OutputMax;
    if (pid->Output < pid->OutputMin) pid->Output = pid->OutputMin;
    
    return pid->Output;
}

// ================= 核心控制函数 =================

/**
 * @brief PID 系统初始化 (主要配置Kp, Ki, Kd及限幅)
 */
void PID_Init(void)
{
    // 初始化 PID - 左轮速度环
    PID_Left_Speed.Kp = 0.6f; 
    PID_Left_Speed.Ki = 0.05f; 
    PID_Left_Speed.Kd = 0.0f;    
    
    PID_Left_Speed.OutputMax = 1.0f; 
    PID_Left_Speed.OutputMin = -1.0f;
    PID_Left_Speed.IntegralMax = 0.3f; 
    PID_Left_Speed.IntegralMin = -0.3f;
    
    PID_Left_Speed.Error0 = 0.0f;
    PID_Left_Speed.Error1 = 0.0f;
    PID_Left_Speed.ErrorInt = 0.0f;
    PID_Left_Speed.Output = 0.0f;
    
    PID_Right_Speed.Kp = 0.6f; 
    PID_Right_Speed.Ki = 0.05f; 
    PID_Right_Speed.Kd = 0.0f;    
    
    PID_Right_Speed.OutputMax = 1.0f; 
    PID_Right_Speed.OutputMin = -1.0f;
    PID_Right_Speed.IntegralMax = 0.3f; 
    PID_Right_Speed.IntegralMin = -0.3f;
    
    PID_Right_Speed.Error0 = 0.0f;
    PID_Right_Speed.Error1 = 0.0f;
    PID_Right_Speed.ErrorInt = 0.0f;
    PID_Right_Speed.Output = 0.0f;
}

/**
 * @brief 动态设置指定电机的 PID 参数
 */
void PID_SetParameters(Motor_Select motor, float kp, float ki, float kd)
{
    PID_TypeDef *pid = (motor == MOTOR_LEFT) ? &PID_Left_Speed : &PID_Right_Speed;
    pid->Kp = kp; 
    pid->Ki = ki; 
    pid->Kd = kd;
}

/**
 * @brief 清除指定电机的 PID 历史状态（用于停车或防突变）
 */
void PID_Reset(Motor_Select motor)
{
    PID_TypeDef *pid = (motor == MOTOR_LEFT) ? &PID_Left_Speed : &PID_Right_Speed;
    pid->Error0 = 0.0f; 
    pid->Error1 = 0.0f; 
    pid->ErrorInt = 0.0f; 
    pid->Output = 0.0f;
}

/**
 * @brief 获取当前 PID 的输出值
 */
float PID_GetOutput(Motor_Select motor)
{
    return (motor == MOTOR_LEFT) ? PID_Left_Speed.Output : PID_Right_Speed.Output;
}