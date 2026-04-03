#include "delay.h"

#include "ti_msp_dl_config.h"

void delay_us(uint32_t us)
{
    /* 每1us对应的CPU周期数 */
    const uint32_t cycles_per_us = CPUCLK_FREQ / 1000000U;

    while (us--) {
        delay_cycles(cycles_per_us);
    }
}

void delay_ms(uint32_t ms)
{
    while (ms--) {
        delay_us(1000U);
    }
}

void delay_s(uint32_t s)
{
    while (s--) {
        delay_ms(1000U);
    }
}
