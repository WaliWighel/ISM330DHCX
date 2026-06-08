#ifndef ISM330DHCX_DEFINES_H
#define ISM330DHCX_DEFINES_H


#include "main.h"

#define __PI 3.141592654f
#define __G  9.8105f

typedef enum {
    ODR_OFF = 0x00,
    ODR_12_5Hz = 0x01,
    ODR_26Hz = 0x02,
    ODR_52Hz = 0x03,
    ODR_104Hz = 0x04,
    ODR_208Hz = 0x05,
    ODR_416Hz = 0x06,
    ODR_833Hz = 0x07,
    ODR_1_66kHz = 0x08,
    ODR_3_33kHz = 0x09,
    ODR_6_66kHz = 0x0A
}ISM330DHCX_ODR;

typedef enum  {
    FS_2g = 0x00,
    FS_4g = 0x02,
    FS_8g = 0x03,
    FS_16g = 0x01
}ISM330DHCX_ACCEL_FS;

typedef enum  {
    FS_125dps = 0x04,
    FS_250dps = 0x00,
    FS_500dps = 0x01,
    FS_1000dps = 0x02,
    FS_2000dps = 0x03,
    FS_4000dps = 0x05
}ISM330DHCX_GYRO_FS;

typedef struct {
	ISM330DHCX_ODR odr;
	ISM330DHCX_ACCEL_FS fs;
    float Sensitivity;
    float Scale;
    /* [m/s^2] */
    float SIScale;
} ISM330DHCX_XL_Config;

typedef struct {
	ISM330DHCX_ODR odr;
	ISM330DHCX_GYRO_FS fs;
	float Sensitivity;
	float Scale;
    /* [rad/s] */
    float SIScale;
} ISM330DHCX_G_Config;


typedef enum{
    ISM_STAGE_IDLE = 0x00,
    ISM_STAGE_SPI_ONGOING = 0x01,
} ISM330DHCX_STAGES_t;

#define ISM330DHCX_CS_0 HAL_GPIO_WritePin(ISM330DHCX_CS_GPIO_Port, ISM330DHCX_CS_Pin, GPIO_PIN_RESET) // Set CS low
#define ISM330DHCX_CS_1 HAL_GPIO_WritePin(ISM330DHCX_CS_GPIO_Port, ISM330DHCX_CS_Pin, GPIO_PIN_SET)   // Set CS high

#define ISM330DHCX_GYRO_AND_ACCEL_READ_SIZE (13U) // 6 bytes for gyro + 6 bytes for accel + 1 dummy byte
#define ISM330DHCX_REG_SIZE (1U) // 1 byte for register address
#define ISM330DHCX_READ_FROM_REG (0x80U) // MSB set for read operations

#define ISM330DHCX_WHO_AM_I_VALUE           (0x6BU)

/* ISM330DHCX Register Map */
#define ISM330DHCX_REG_FUNC_CFG_ACCESS      (0x01U)
#define ISM330DHCX_REG_PIN_CTRL             (0x02U)
#define ISM330DHCX_REG_FIFO_CTRL1           (0x07U)
#define ISM330DHCX_REG_FIFO_CTRL2           (0x08U)
#define ISM330DHCX_REG_FIFO_CTRL3           (0x09U)
#define ISM330DHCX_REG_FIFO_CTRL4           (0x0AU)
#define ISM330DHCX_REG_COUNTER_BDR_REG1     (0x0BU)
#define ISM330DHCX_REG_COUNTER_BDR_REG2     (0x0CU)
#define ISM330DHCX_REG_INT1_CTRL            (0x0DU)
#define ISM330DHCX_REG_INT2_CTRL            (0x0EU)   
#define ISM330DHCX_REG_WHO_AM_I             (0x0FU)
#define ISM330DHCX_REG_CTRL1_XL             (0x10U)
#define ISM330DHCX_REG_CTRL2_G              (0x11U)
#define ISM330DHCX_REG_CTRL3_C              (0x12U)
#define ISM330DHCX_REG_CTRL4_C              (0x13U)
#define ISM330DHCX_REG_CTRL5_C              (0x14U)
#define ISM330DHCX_REG_CTRL6_C              (0x15U)
#define ISM330DHCX_REG_CTRL7_G              (0x16U)
#define ISM330DHCX_REG_CTRL8_XL             (0x17U)
#define ISM330DHCX_REG_CTRL9_XL             (0x18U)
#define ISM330DHCX_REG_CTRL10_C             (0x19U)
#define ISM330DHCX_REG_ALL_INT_SRC          (0x1AU)
#define ISM330DHCX_REG_WAKE_UP_SRC          (0x1BU)
#define ISM330DHCX_REG_TAP_SRC              (0x1CU)
#define ISM330DHCX_REG_D6D_SRC              (0x1DU)
#define ISM330DHCX_REG_STATUS_REG           (0x1EU)
#define ISM330DHCX_REG_OUT_TEMP_L           (0x20U)
#define ISM330DHCX_REG_OUT_TEMP_H           (0x21U)
#define ISM330DHCX_REG_OUTX_L_G             (0x22U)
#define ISM330DHCX_REG_OUTX_H_G             (0x23U)
#define ISM330DHCX_REG_OUTY_L_G             (0x24U)
#define ISM330DHCX_REG_OUTY_H_G             (0x25U)
#define ISM330DHCX_REG_OUTZ_L_G             (0x26U)
#define ISM330DHCX_REG_OUTZ_H_G             (0x27U)
#define ISM330DHCX_REG_OUTX_L_A             (0x28U)
#define ISM330DHCX_REG_OUTX_H_A             (0x29U)
#define ISM330DHCX_REG_OUTY_L_A             (0x2AU)
#define ISM330DHCX_REG_OUTY_H_A             (0x2BU)
#define ISM330DHCX_REG_OUTZ_L_A             (0x2CU)
#define ISM330DHCX_REG_OUTZ_H_A             (0x2DU)
#define ISM330DHCX_REG_FIFO_STATUS1         (0x3AU)
#define ISM330DHCX_REG_FIFO_STATUS2         (0x3BU)
#define ISM330DHCX_REG_TAP_CFG2				(0x58U)
#define ISM330DHCX_REG_MD1_CFG				(0x5EU)
#define ISM330DHCX_REG_FIFO_DATA_OUT_TAG    (0x78U)
#define ISM330DHCX_REG_FIFO_DATA_OUT_X_L    (0x79U)
#define ISM330DHCX_REG_FIFO_DATA_OUT_X_H    (0x7AU)
#define ISM330DHCX_REG_FIFO_DATA_OUT_Y_L    (0x7BU)
#define ISM330DHCX_REG_FIFO_DATA_OUT_Y_H    (0x7CU)
#define ISM330DHCX_REG_FIFO_DATA_OUT_Z_L    (0x7DU)
#define ISM330DHCX_REG_FIFO_DATA_OUT_Z_H    (0x7EU)
#define ISM330DHCX_REG_EMB_FUNC_INT1		(0x0AU)


#define ISM330DHCX_CTRL3_C_SWRESET			(0x01U)

#endif /* ISM330DHCX_DEFINES_H */

