#ifndef __PID_H
#define __PID_H

#include <stdint.h>

// 选择控制的电机方向
typedef enum {
    MOTOR_LEFT = 0,
    MOTOR_RIGHT
} Motor_Select;

// PID 参数结构体
typedef struct {
    float Kp;
    float Ki;
    float Kd;
    
    float Error0;       // 当前误差
    float Error1;       // 上次误差
    float ErrorInt;     // 误差积分
    
    float Output;       // PID 输出值
    float OutputMax;    // 输出限幅最大值
    float OutputMin;    // 输出限幅最小值
    float IntegralMax;  // 积分限幅最大值
    float IntegralMin;  // 积分限幅最小值
} PID_TypeDef;

// 外部声明，方便其他文件调用
extern PID_TypeDef PID_Left_Speed;    
extern PID_TypeDef PID_Right_Speed;   

// 函数声明
void PID_Init(void);
float PID_Calculate_Step(PID_TypeDef *pid, float target, float actual);
void PID_SetParameters(Motor_Select motor, float kp, float ki, float kd);
void PID_Reset(Motor_Select motor);
float PID_GetOutput(Motor_Select motor);

#endif // __PID_H