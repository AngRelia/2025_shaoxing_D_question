#include "Serial.h"

#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_uart_main.h"
#include "ti/driverlib/dl_dma.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char Serial0_RxPacket[SERIAL0_PACKET_SIZE];
uint8_t Serial0_RxFlag = 0U;

static uint8_t s_rxDoubleBuf[2][SERIAL0_PACKET_SIZE];
static volatile uint8_t s_writeBufIndex = 0U;
static volatile uint16_t s_writePos = 0U;

static uint8_t s_txDoubleBuf[2][SERIAL0_TX_BUFFER_SIZE];
static volatile uint16_t s_txLen[2] = {0U, 0U};
static volatile uint8_t s_txActiveBuf = 0xFFU;
static volatile uint8_t s_txPendingBuf = 0xFFU;

static uint32_t Serial0_Pow(uint32_t x, uint8_t y)
{
    uint32_t result = 1U;
    while (y--) {
        result *= x;
    }
    return result;
}

static void Serial0_PublishBuffer(uint8_t bufIndex, uint16_t size)
{
    uint16_t copyLen = size;

    if (copyLen >= SERIAL0_PACKET_SIZE) {
        copyLen = SERIAL0_PACKET_SIZE - 1U;
    }

    memcpy(Serial0_RxPacket, s_rxDoubleBuf[bufIndex], copyLen);
    Serial0_RxPacket[copyLen] = '\0';
    Serial0_RxFlag = 1U;
}

static void Serial0_StartTxDMA(uint8_t bufIndex)
{
    DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
    DL_DMA_setSrcAddr(DMA,
                      DMA_CH1_CHAN_ID,
                      (uint32_t)(uintptr_t)&s_txDoubleBuf[bufIndex][0]);
    DL_DMA_setDestAddr(DMA,
                       DMA_CH1_CHAN_ID,
                       (uint32_t)(uintptr_t)&(UART_0_INST->TXDATA));
    DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, s_txLen[bufIndex]);
    DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
}

static void Serial0_QueueTxBufferBlocking(const uint8_t *data, uint16_t length)
{
    while (1) {
        uint8_t active;
        uint8_t pending;

        __disable_irq();
        active = s_txActiveBuf;
        pending = s_txPendingBuf;

        if (active == 0xFFU) {
            memcpy(&s_txDoubleBuf[0][0], data, length);
            s_txLen[0] = length;
            s_txActiveBuf = 0U;
            
            Serial0_StartTxDMA(0U);  // 先启动硬件DMA
            __enable_irq();          // 再退出临界区
            return;
        }

        if (pending == 0xFFU) {
            uint8_t pendingIndex = (uint8_t)(active ^ 1U);
            memcpy(&s_txDoubleBuf[pendingIndex][0], data, length);
            s_txLen[pendingIndex] = length;
            s_txPendingBuf = pendingIndex;
            __enable_irq();
            return;
        }

        __enable_irq();
    }
}

static void Serial0_WaitTxIdle(void)
{
    while (1) {
        uint8_t active;
        uint8_t pending;

        __disable_irq();
        active = s_txActiveBuf;
        pending = s_txPendingBuf;
        __enable_irq();

        if ((active == 0xFFU) && (pending == 0xFFU)) {
            break;
        }
    }
}

void Serial0_Init(void)
{
    memset((void *)s_rxDoubleBuf, 0, sizeof(s_rxDoubleBuf));
    memset(Serial0_RxPacket, 0, sizeof(Serial0_RxPacket));

    s_writeBufIndex = 0U;
    s_writePos = 0U;
    Serial0_RxFlag = 0U;

    memset((void *)s_txDoubleBuf, 0, sizeof(s_txDoubleBuf));
    s_txLen[0] = 0U;
    s_txLen[1] = 0U;
    s_txActiveBuf = 0xFFU;
    s_txPendingBuf = 0xFFU;

    DL_UART_Main_enableInterrupt(
        UART_0_INST, DL_UART_MAIN_INTERRUPT_RX | DL_UART_MAIN_INTERRUPT_DMA_DONE_TX);
    
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
}

void Serial0_SendByte(uint8_t byte)
{
    Serial0_SendArray(&byte, 1U);
}

void Serial0_SendArray(uint8_t *array, uint16_t length)
{
    uint16_t offset = 0U;

    if ((array == NULL) || (length == 0U)) {
        return;
    }

    while (offset < length) {
        uint16_t chunk = (uint16_t)(length - offset);
        if (chunk > SERIAL0_TX_BUFFER_SIZE) {
            chunk = SERIAL0_TX_BUFFER_SIZE;
        }

        Serial0_QueueTxBufferBlocking(&array[offset], chunk);
        offset = (uint16_t)(offset + chunk);
    }
}

void Serial0_SendString(char *str)
{
    if (str == NULL) {
        return;
    }
    Serial0_SendArray((uint8_t *)str, strlen(str));
}

void Serial0_SendStringBlocking(const char *str)
{
    if (str == NULL) {
        return;
    }
    Serial0_SendArray((uint8_t *)str, strlen(str));
    Serial0_WaitTxIdle();
}

void Serial0_SendNumber(uint32_t num, uint8_t length)
{
    uint8_t i;
    for (i = 0U; i < length; i++) {
        uint32_t div = Serial0_Pow(10U, (uint8_t)(length - i - 1U));
        Serial0_SendByte((uint8_t)(num / div % 10U + '0'));
    }
}

void Serial0_Printf(char *format, ...)
{
    char str[128];
    va_list arg;

    va_start(arg, format);
    (void)vsnprintf(str, sizeof(str), format, arg);
    va_end(arg);

    Serial0_SendString(str);
}

void Serial0_DMA_RxEvent(uint16_t size)
{
    uint8_t fullBuf = s_writeBufIndex;

    Serial0_PublishBuffer(fullBuf, size);

    s_writeBufIndex ^= 1U;
    s_writePos = 0U;
    memset((void *)s_rxDoubleBuf[s_writeBufIndex], 0, SERIAL0_PACKET_SIZE);
}

void UART_0_INST_IRQHandler(void)
{
    DL_UART_IIDX iidx = DL_UART_getPendingInterrupt(UART_0_INST);

    if (iidx == DL_UART_IIDX_RX) {
        uint8_t data = DL_UART_Main_receiveData(UART_0_INST);

        if ((data == '\r') || (data == '\n')) {
            if (s_writePos > 0U) {
                Serial0_DMA_RxEvent(s_writePos);
            }
            return;
        }

        {
            s_rxDoubleBuf[s_writeBufIndex][s_writePos++] = data;
            if (s_writePos >= (SERIAL0_PACKET_SIZE - 1U)) {
                Serial0_DMA_RxEvent(s_writePos);
            }
        }
    } else if (iidx == DL_UART_IIDX_DMA_DONE_TX) {
        if (s_txPendingBuf != 0xFFU) {
            uint8_t nextBuf = s_txPendingBuf;
            s_txPendingBuf = 0xFFU;
            s_txActiveBuf = nextBuf;
            Serial0_StartTxDMA(nextBuf);
        } else {
            s_txActiveBuf = 0xFFU;
        }
    }
}
