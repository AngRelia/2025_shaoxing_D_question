#ifndef ICODE_DELAY_H_
#define ICODE_DELAY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 微秒级阻塞延时
 * @param us 延时时间（单位：us）
 */
void delay_us(uint32_t us);

/**
 * @brief 毫秒级阻塞延时
 * @param ms 延时时间（单位：ms）
 */
void delay_ms(uint32_t ms);

/**
 * @brief 秒级阻塞延时
 * @param s 延时时间（单位：s）
 */
void delay_s(uint32_t s);

#ifdef __cplusplus
}
#endif

#endif /* ICODE_DELAY_H_ */
