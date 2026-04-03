#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SERIAL0_PACKET_SIZE 128U
#define SERIAL0_TX_BUFFER_SIZE 128U

extern char Serial0_RxPacket[SERIAL0_PACKET_SIZE];
extern uint8_t Serial0_RxFlag;

void Serial0_Init(void);

void Serial0_SendByte(uint8_t byte);
void Serial0_SendArray(uint8_t *array, uint16_t length);
void Serial0_SendString(char *str);
void Serial0_SendStringBlocking(const char *str);
void Serial0_SendNumber(uint32_t num, uint8_t length);
void Serial0_Printf(char *format, ...);

void Serial0_DMA_RxEvent(uint16_t size);

#ifdef __cplusplus
}
#endif

#endif
