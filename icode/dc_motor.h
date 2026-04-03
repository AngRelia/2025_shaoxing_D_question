#ifndef ICODE_DC_MOTOR_H_
#define ICODE_DC_MOTOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DCMOTOR_DIR_STOP = 0,
    DCMOTOR_DIR_FORWARD,
    DCMOTOR_DIR_BACKWARD
} DCMotor_Direction_t;

typedef struct {
    int16_t left_duty_percent;
    int16_t right_duty_percent;
    DCMotor_Direction_t left_dir;
    DCMotor_Direction_t right_dir;
    uint8_t enabled;
} DCMotor_Status_t;

void DCMotor_Init(void);
void DCMotor_Enable(uint8_t enable);
void DCMotor_SetDuty(int16_t left_duty_percent, int16_t right_duty_percent);
void DCMotor_Stop(void);
void DCMotor_GetStatus(DCMotor_Status_t *status);
const char *DCMotor_DirectionString(DCMotor_Direction_t dir);

#ifdef __cplusplus
}
#endif

#endif /* ICODE_DC_MOTOR_H_ */
