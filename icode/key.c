#include "key.h"
#include "ti_msp_dl_config.h"

static uint8_t Key_Num = 0U; /* 保存最终按键值 */

#ifndef GPIO_PIN_RESET
#define GPIO_PIN_RESET  (0U)
#endif

#ifndef GPIO_PIN_SET
#define GPIO_PIN_SET    (1U)
#endif


static uint8_t Key_GetState(void)
{
    if (DL_GPIO_readPins(KEY_PORT, KEY_KEY_1_PIN) == GPIO_PIN_RESET) return 1U;
    if (DL_GPIO_readPins(KEY_PORT, KEY_KEY_2_PIN) == GPIO_PIN_RESET) return 2U;
    if (DL_GPIO_readPins(KEY_PORT, KEY_KEY_3_PIN) == GPIO_PIN_RESET) return 3U;
    if (DL_GPIO_readPins(KEY_PORT, KEY_KEY_4_PIN) == GPIO_PIN_RESET) return 4U;
    return 0U;
}

uint8_t Key_GetNum(void)
{
    uint8_t temp = Key_Num;
    Key_Num = 0U; /* 取走后清零，保证一次只响应一次 */
    return temp;
}

void Key_Tick(void)
{

    static uint8_t count = 0U;
    static uint8_t currState = 0U;
    static uint8_t prevState = 0U;

    count++;
    if (count >= 20U) {
        count = 0U;
        prevState = currState;
        currState = Key_GetState();

        /* 按下沿触发（0 -> 键值） */
        if ((currState != 0U) && (prevState == 0U)) {
            Key_Num = currState;
        }
    }
}
