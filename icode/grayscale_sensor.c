/**
  ******************************************************************************
  * @file    grayscale_sensor.c
  * @brief   双 8 路灰度传感器模块驱动 (适用 TI MSPM0G 系列)
  * @author  
  * @version V2.0
  * @date    2026-04-03
  ******************************************************************************
  */

#include "grayscale_sensor.h"
#include "delay.h"

#define GW_GRAY_I2C_TIMEOUT_CNT    (200000U)

uint8_t sensor[16] = {0};  // 前8位为左传感器数据，后8位为右传感器数据

/* ============== 内部私有底层 I2C 实现 ===================*/

/**
  * @brief  I2C 死锁或异常后软复位控制器
  */
static void grayscale_i2c_recover(void)
{
    DL_I2C_disableController(I2C_GRAYSCALE_INST);
    DL_I2C_resetControllerTransfer(I2C_GRAYSCALE_INST);
    DL_I2C_flushControllerTXFIFO(I2C_GRAYSCALE_INST);
    DL_I2C_flushControllerRXFIFO(I2C_GRAYSCALE_INST);
    SYSCFG_DL_I2C_GRAYSCALE_init();
}

/**
  * @brief  等待 I2C 总线空闲
  */
static unsigned char grayscale_wait_controller_idle(void)
{
    uint32_t timeout = GW_GRAY_I2C_TIMEOUT_CNT;
    while (((DL_I2C_getControllerStatus(I2C_GRAYSCALE_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) != 0U) && (timeout-- > 0U));
    if (timeout > 0U) return GW_GRAY_OK;
    
    grayscale_i2c_recover();
    return GW_GRAY_I2C_ERROR;
}

/**
  * @brief  I2C 底层连续写操作
  */
static unsigned char grayscale_i2c_tx(uint8_t slave_addr, const uint8_t *buf, uint8_t len)
{
    uint32_t timeout, status;
    uint8_t i;

    if ((buf == 0) || (len == 0U)) return GW_GRAY_ERROR;
    if (grayscale_wait_controller_idle() != GW_GRAY_OK) return GW_GRAY_I2C_ERROR;

    DL_I2C_resetControllerTransfer(I2C_GRAYSCALE_INST);
    DL_I2C_flushControllerTXFIFO(I2C_GRAYSCALE_INST);
    DL_I2C_startControllerTransfer(I2C_GRAYSCALE_INST, slave_addr, DL_I2C_CONTROLLER_DIRECTION_TX, len);

    for (i = 0U; i < len; i++) {
        timeout = GW_GRAY_I2C_TIMEOUT_CNT;
        while ((DL_I2C_getControllerTXFIFOCounter(I2C_GRAYSCALE_INST) == 0U) && (timeout-- > 0U)) {
            status = DL_I2C_getControllerStatus(I2C_GRAYSCALE_INST);
            if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
                grayscale_i2c_recover();
                return GW_GRAY_I2C_ERROR;
            }
        }
        if (timeout == 0U) { grayscale_i2c_recover(); return GW_GRAY_I2C_ERROR; }
        
        DL_I2C_transmitControllerData(I2C_GRAYSCALE_INST, buf[i]);
    }

    if (grayscale_wait_controller_idle() != GW_GRAY_OK) return GW_GRAY_I2C_ERROR;
    
    status = DL_I2C_getControllerStatus(I2C_GRAYSCALE_INST);
    if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) { grayscale_i2c_recover(); return GW_GRAY_I2C_ERROR; }
    
    return GW_GRAY_OK;
}

/**
  * @brief  I2C 复合事务（写地址 + 重启读数据）兼容时序
  */
static unsigned char grayscale_i2c_write_then_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    // 先单独写寄存器地址 (带 STOP)
    uint8_t tx_buf[1] = {reg_addr};
    if (grayscale_i2c_tx(slave_addr, tx_buf, 1) != GW_GRAY_OK) {
        return GW_GRAY_I2C_ERROR;
    }
    
    // 给从机一点点反应时间 (有些性能差的从机需要)
    delay_ms(1); 
    
    // 再单独发起读操作
    if (grayscale_wait_controller_idle() != GW_GRAY_OK) return GW_GRAY_I2C_ERROR;
    
    DL_I2C_resetControllerTransfer(I2C_GRAYSCALE_INST);
    DL_I2C_flushControllerRXFIFO(I2C_GRAYSCALE_INST);
    DL_I2C_startControllerTransfer(I2C_GRAYSCALE_INST, slave_addr, DL_I2C_CONTROLLER_DIRECTION_RX, len);
    
    uint32_t timeout;
    for (uint8_t i = 0; i < len; i++) {
        timeout = GW_GRAY_I2C_TIMEOUT_CNT;
        while ((DL_I2C_getControllerRXFIFOCounter(I2C_GRAYSCALE_INST) == 0U) && (timeout-- > 0U));
        if (timeout == 0U) { grayscale_i2c_recover(); return GW_GRAY_I2C_ERROR; }
        data[i] = DL_I2C_receiveControllerData(I2C_GRAYSCALE_INST);
    }
    
    return GW_GRAY_OK;
}

static unsigned char IIC_Read_Byte(unsigned char Salve_Address, unsigned char Reg_Address, unsigned char *data) {
    return grayscale_i2c_write_then_read(Salve_Address, Reg_Address, data, 1U);
}

static unsigned char IIC_Read_Bytes(unsigned char Salve_Address, unsigned char Reg_Address, unsigned char *Result, unsigned char len) {
    return grayscale_i2c_write_then_read(Salve_Address, Reg_Address, Result, len);
}

static unsigned char IIC_Write_Byte(unsigned char Salve_Address, unsigned char Reg_Address, unsigned char data) {
    uint8_t tx_buf[2] = {Reg_Address, data};
    return grayscale_i2c_tx(Salve_Address, tx_buf, 2U);
}


/* ============== 用户逻辑功能接口 ===================*/

unsigned char IIC_Probe(uint8_t addr)
{
    uint8_t cmd = GW_GRAY_DIGITAL_MODE;
    return grayscale_i2c_tx(addr, &cmd, 1U);
}

unsigned char Ping(uint8_t addr)
{
    unsigned char dat = 0U;
    if (IIC_Read_Byte(addr, GW_GRAY_PING, &dat) == GW_GRAY_OK) {
        if (dat == GW_GRAY_PING_OK) return GW_GRAY_OK;
    }
    return GW_GRAY_PING_FAIL;
}

unsigned char IIC_Get_Digtal(uint8_t addr)
{
    unsigned char dat = 0U;
    (void) IIC_Read_Byte(addr, GW_GRAY_DIGITAL_MODE, &dat);
    return dat;
}

unsigned char IIC_Get_Digtal_Ex(uint8_t addr, uint8_t *digitalValue)
{
    unsigned char dat = 0U;
    if (digitalValue == 0) return GW_GRAY_ERROR;
    
    if (IIC_Read_Byte(addr, GW_GRAY_DIGITAL_MODE, &dat) == GW_GRAY_OK) {
        *digitalValue = dat;
        return GW_GRAY_OK;
    }
    *digitalValue = 0U;
    return GW_GRAY_I2C_ERROR;
}

unsigned char IIC_Get_Anolog(uint8_t addr, unsigned char *Result, unsigned char len)
{
    if (IIC_Write_Byte(addr, GW_GRAY_ANALOG_BASE_, 0x00U) != GW_GRAY_OK) return GW_GRAY_ERROR;
    delay_ms(1U);
    return IIC_Read_Bytes(addr, GW_GRAY_ANALOG_BASE_, Result, len);
}

unsigned char IIC_Get_Single_Anolog(uint8_t addr, unsigned char Channel)
{
    unsigned char dat = 0U;
    if ((Channel < 1U) || (Channel > 8U)) return 0U;
    (void) IIC_Read_Byte(addr, (unsigned char)(GW_GRAY_ANALOG_SINGLE_BASE + (Channel - 1U)), &dat);
    return dat;
}

unsigned char IIC_Anolog_Normalize(uint8_t addr, uint8_t Normalize_channel) {
    return IIC_Write_Byte(addr, GW_GRAY_NORMALIZE_ENABLE, Normalize_channel);
}

unsigned char IIC_Set_Channel_Enable(uint8_t addr, uint8_t ChannelEnable) {
    return IIC_Write_Byte(addr, GW_GRAY_CHANNEL_ENABLE, ChannelEnable);
}

unsigned char IIC_Get_Error_Info(uint8_t addr) {
    unsigned char error_info = 0U;
    (void) IIC_Read_Byte(addr, GW_GRAY_ERROR_INFO, &error_info);
    return error_info;
}

unsigned char IIC_Software_Reset(uint8_t addr) {
    return IIC_Write_Byte(addr, GW_GRAY_SOFTWARE_RESET, 0x00U);
}

void grayscale_byte_to_sensor_array(uint8_t data, uint8_t start_index)
{
    uint8_t i;
    for (i = 0U; i < 8U; i++) {
        if ((data & (1U << i)) != 0U) {
            sensor[start_index + i] = 1U;
        } else {
            sensor[start_index + i] = 0U;
        }
    }
}