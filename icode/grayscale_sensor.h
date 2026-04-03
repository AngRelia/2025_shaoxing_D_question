/**
  ******************************************************************************
  * @file    grayscale_sensor.h
  * @brief   双 8 路灰度传感器模块头文件 (适用 TI MSPM0G 系列)
  * @author  
  * @version V2.0
  * @date    2026-04-03
  ******************************************************************************
  * @attention
  * 
  * 硬件连接：
  * - I2C 控制器：根据 ti_msp_dl_config.h 中的 I2C_GRAYSCALE_INST 设定
  * - 传感器 1 (左侧) 地址: 0x4C (7位地址)
  * - 传感器 2 (右侧) 地址: 0x4E (7位地址)
  ******************************************************************************
  */

#ifndef ICODE_GRAYSCALE_SENSOR_H_
#define ICODE_GRAYSCALE_SENSOR_H_

#include <stdint.h>
#include "ti_msp_dl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================= 寄存器及命令宏定义 ================= */
#define GW_GRAY_PING                  0xAA
#define GW_GRAY_PING_OK               0x66
#define GW_GRAY_DIGITAL_MODE          0xDD
#define GW_GRAY_ANALOG_BASE_          0xB0
#define GW_GRAY_ANALOG_SINGLE_BASE    0xB1
#define GW_GRAY_CHANNEL_ENABLE        0xCE
#define GW_GRAY_NORMALIZE_ENABLE      0xCF
#define GW_GRAY_HYSTERESIS_GRAYB      0xD0
#define GW_GRAY_HYSTERESIS_GRAYW      0xD1
#define GW_GRAY_SOFTWARE_ADDRESS      0xAD
#define GW_GRAY_ERROR_INFO            0xDE
#define GW_GRAY_SOFTWARE_RESET        0xC0
#define GW_GRAY_FIRMWARE_VERSION      0xC1

/* ================= 传感器 I2C 地址 ================= */
#define GW_GRAY_ADDR_SENSOR1          0x4C   // 左侧灰度地址
#define GW_GRAY_ADDR_SENSOR2          0x4E   // 右侧灰度地址

/* ================= 运行状态宏 ================= */
#define GW_GRAY_OK                    0x00
#define GW_GRAY_ERROR                 0x01
#define GW_GRAY_PING_FAIL             0x02
#define GW_GRAY_I2C_ERROR             0x03

/* 全局数组：存储所有感知的状态，前8位左，后8位右 */
extern uint8_t sensor[16];

/* ================== 用户 API 声明 ================== */
unsigned char Ping(uint8_t addr);
unsigned char IIC_Get_Digtal(uint8_t addr);
unsigned char IIC_Get_Digtal_Ex(uint8_t addr, uint8_t *digitalValue);
unsigned char IIC_Get_Anolog(uint8_t addr, unsigned char *Result, unsigned char len);
unsigned char IIC_Get_Single_Anolog(uint8_t addr, unsigned char Channel);
unsigned char IIC_Anolog_Normalize(uint8_t addr, uint8_t Normalize_channel);
unsigned short IIC_Get_Offset(void);
unsigned char IIC_Set_Channel_Enable(uint8_t addr, uint8_t ChannelEnable);
unsigned char IIC_Get_Error_Info(uint8_t addr);
unsigned char IIC_Software_Reset(uint8_t addr);
unsigned char IIC_Get_Firmware_Version(uint8_t addr);

void grayscale_byte_to_sensor_array(uint8_t data, uint8_t start_index);
unsigned char IIC_Probe(uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* ICODE_GRAYSCALE_SENSOR_H_ */