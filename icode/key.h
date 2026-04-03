#ifndef ICODE_KEY_H_
#define ICODE_KEY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 周期调用的按键扫描任务（当前工程在1ms中断中调用，内部20ms消抖）
 */
void Key_Tick(void);

/**
 * @brief 获取一次按键值（1~4），无新按键返回0
 */
uint8_t Key_GetNum(void);

#ifdef __cplusplus
}
#endif

#endif /* ICODE_KEY_H_ */
