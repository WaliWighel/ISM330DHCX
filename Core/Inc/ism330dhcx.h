#ifndef ISM330DHCX_H
#define ISM330DHCX_H

#include "main.h"
#include "ism330dhcx_defines.h"
#include <stdint.h>
/* Data structures for sensor readings */
typedef struct {
    int16_t Gyro_X;
    int16_t Gyro_Y;
    int16_t Gyro_Z;
    int16_t Accel_X;
    int16_t Accel_Y;
    int16_t Accel_Z;
} ISM330DHCX_AXIS_RAW_DATA;

typedef struct {
    float Gyro_X;
    float Gyro_Y;
    float Gyro_Z;
    float Accel_X;
    float Accel_Y;
    float Accel_Z;
} ISM330DHCX_AXIS_SCALED_DATA;

typedef enum{
    ISM330DHCX_OK = 0x01,
    ISM330DHCX_SPI_ERROR = 0x02,
    ISM330DHCX_ERROR = 0x03,
	ISM330DHCX_REG_ACCESS_ERROR = 0x04,
} ISM330DHCX_STATUS;

typedef struct {
    ISM330DHCX_AXIS_RAW_DATA Raw_Data;
    ISM330DHCX_G_Config GYRO_Config_Data;
    ISM330DHCX_XL_Config ACCEL_Config_Data;
    ISM330DHCX_STATUS Sensor_Status;
} ISM330DHCX_State;

/* Register I/O operations */
ISM330DHCX_STATUS ISM330DHCX_ReadReg(uint8_t reg, uint8_t* data);
ISM330DHCX_STATUS ISM330DHCX_WriteReg(uint8_t reg, uint8_t data);
ISM330DHCX_STATUS ISM330DHCX_ReadMultiple(uint8_t reg_start, uint8_t* data, uint16_t length);
ISM330DHCX_STATUS ISM330DHCX_WriteMultiple(uint8_t reg_start, uint8_t* data, uint16_t length);
ISM330DHCX_STATUS ISM330DHCX_Read13_Start_DMA(uint8_t reg_start, uint8_t *data);
ISM330DHCX_STATUS ISM330DHCX_ReadMultiple_Stop_DMA(void);

/* Sensor initialization and data acquisition */
ISM330DHCX_STATUS ISM330DHCX_Init(SPI_HandleTypeDef *hspi);
ISM330DHCX_STATUS ISM330DHCX_GET_GYRO_AND_ACC(void);
ISM330DHCX_STATUS ISM330DHCX_GET_Scaled_GYRO_AND_ACC(ISM330DHCX_AXIS_SCALED_DATA *scaled_data);

// /* DMA-based data acquisition */
ISM330DHCX_STATUS ISM330DHCX_GET_GYRO_AND_ACC_DMA(Event_t event);
// uint8_t ISM330DHCX_GET_GYRO_AND_ACC_DMA_Start(uint8_t *raw_data);
// uint8_t ISM330DHCX_GET_GYRO_AND_ACC_DMA_Stop(void);

/* Sensor status and configuration */
ISM330DHCX_STATUS ISM330DHCX_GET_SENSOR_STATUS(uint8_t* status);
ISM330DHCX_STATUS ISM330DHCX_GYRO_CONFIG(ISM330DHCX_ODR odr, ISM330DHCX_GYRO_FS fs, uint8_t fs_125, uint8_t fs_4000);
ISM330DHCX_STATUS ISM330DHCX_ACCEL_CONFIG(ISM330DHCX_ODR odr, ISM330DHCX_ACCEL_FS fs, uint8_t lpf2_xl_en);
ISM330DHCX_STATUS ISM330DHCX_IRQ_CONFIG(uint8_t int1, uint8_t int2);



void ISM330DHCX_GetGyroOffset(ISM330DHCX_AXIS_SCALED_DATA *offset_data);

#endif /* ISM330DHCX_H */
