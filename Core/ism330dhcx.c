#include "ism330dhcx_defines.h"
#include "ism330dhcx.h"
#include "main.h"
#include <stdint.h>
#include <string.h>
#include "spi.h"


/**
 * ===============================================================================
 * ISM330DHCX 6-Axis IMU Driver
 *
 * This driver provides a complete interface for the ST ISM330DHCX
 * Inertial Measurement Unit (gyroscope + accelerometer) sensor.
 *
 * Key Features:
 * - SPI communication (blocking and DMA modes)
 * - Independent gyro/accel configuration
 * - Raw and scaled data readout
 * - Interrupt configuration support
 * - Sensor calibration support
 *
 * Data Format: Little-endian byte ordering
 * SPI Protocol: Standard SPI mode with 8-bit transfers
 * ===============================================================================
 */

// Only for calibration
// #define ISM330DHCX_CALIBRATION 0
#define ISM330DHCX_SI_UNITS


// Global variables
SPI_HandleTypeDef *ISM330DHCX_hspi;
static ISM330DHCX_State ISM330DHCX_Sensor_State;
static ISM330DHCX_AXIS_SCALED_DATA ISM330DHCX_ScaledData;

/**
 * @brief Read a single register from the ISM330DHCX sensor using SPI blocking mode.
 * @details Uses SPI with MSB set to 1 (0x80) to indicate a read operation.
 *          Performs a 2-byte transaction: register address (TX) and data (RX).
 *          The received data is in the second byte of the response.
 * @param reg Register address to read (7-bit address, will be ORed with read flag)
 * @param data Pointer to destination byte where register value will be stored
 * @return ISM330DHCX_OK on success, ISM330DHCX_SPI_ERROR on SPI failure
 * @note This function uses blocking SPI with HAL_MAX_DELAY timeout
 * @warning Ensure SPI and CS line are properly initialized before calling
 */
ISM330DHCX_STATUS ISM330DHCX_ReadReg(uint8_t reg, uint8_t *data){
    uint8_t tx_buff[2] = {0};
    uint8_t rx_buff[2] = {0};

    tx_buff[0] = reg | ISM330DHCX_READ_FROM_REG; // Set MSB for read operation

    ISM330DHCX_CS_0;
    if (HAL_SPI_TransmitReceive(ISM330DHCX_hspi, tx_buff, rx_buff, sizeof(tx_buff), HAL_MAX_DELAY) != HAL_OK) {
    	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_SPI_ERROR;
    }
    ISM330DHCX_CS_1;

    data[0] = rx_buff[1];

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK;
}

/**
 * @brief Write a single register on the ISM330DHCX sensor using SPI blocking mode.
 * @details Performs a 2-byte SPI transaction: register address (with write flag) and data.
 * @param reg Register address to write (7-bit address, MSB will be 0 for write)
 * @param data Value to write into the register
 * @return ISM330DHCX_OK on success, ISM330DHCX_SPI_ERROR on SPI failure
 * @note This function uses blocking SPI with HAL_MAX_DELAY timeout
 * @warning Ensure SPI and CS line are properly initialized before calling
 */
ISM330DHCX_STATUS ISM330DHCX_WriteReg(uint8_t reg, uint8_t data){
    uint8_t tx_buff[2];

    tx_buff[0] = reg;
    tx_buff[1] = data;

    ISM330DHCX_CS_0;
    if (HAL_SPI_Transmit(ISM330DHCX_hspi, tx_buff, sizeof(tx_buff), HAL_MAX_DELAY) != HAL_OK) {
    	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_SPI_ERROR;
    }
    ISM330DHCX_CS_1;

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK;
}

/**
 * @brief Read multiple consecutive bytes from the ISM330DHCX sensor using SPI blocking mode.
 * @details The ISM330DHCX supports auto-increment addressing for burst reads.
 *          Performs (length + 1)-byte SPI transaction: register address and data.
 * @param reg_start First register address to read from (will be ORed with read flag)
 * @param data Destination buffer for the read bytes
 * @param length Number of bytes to read; buffer must be at least this size
 * @return ISM330DHCX_OK on success, ISM330DHCX_SPI_ERROR on SPI failure
 * @note Uses VLA (Variable Length Array) for buffer - ensure stack is sufficient
 * @warning Ensure SPI is initialized and data buffer is valid
 */
ISM330DHCX_STATUS ISM330DHCX_ReadMultiple(uint8_t reg_start, uint8_t* data, uint16_t length){
    uint8_t tx_buff[length + 1];
    uint8_t rx_buff[length + 1];

    memset(&tx_buff[1], 0, sizeof(tx_buff) - 1);
    memset(rx_buff, 0, sizeof(rx_buff));

    tx_buff[0] = reg_start | ISM330DHCX_READ_FROM_REG;


    ISM330DHCX_CS_0;
    if (HAL_SPI_TransmitReceive(ISM330DHCX_hspi, tx_buff, rx_buff, sizeof(tx_buff), HAL_MAX_DELAY) != HAL_OK) {
    	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_SPI_ERROR;
    }
    ISM330DHCX_CS_1;

    memcpy(data, &rx_buff[1], length);

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Return 0 for success
}

/**
 * @brief Start a non-blocking DMA read of multiple consecutive bytes from the ISM330DHCX sensor.
 * @param reg_start First register address to read from.
 * @param data Destination buffer for the DMA transfer.
 * @param length Number of bytes to read.
 * @return 0 on success, non-zero on error.
 */
ISM330DHCX_STATUS ISM330DHCX_Read13_Start_DMA(uint8_t reg_start, uint8_t *data){
    NC_RAM static uint8_t tx_buffer[ISM330DHCX_GYRO_AND_ACCEL_READ_SIZE] = {0}; 				// Buffer for transmitting register address, should be at least 1 byte for register address + length of data to read
    tx_buffer[0] = reg_start | ISM330DHCX_READ_FROM_REG;    // Set MSB for read operation

    ISM330DHCX_CS_0;
    if (HAL_SPI_TransmitReceive_DMA(ISM330DHCX_hspi, tx_buffer, data, sizeof(tx_buffer)) != HAL_OK){
    	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_SPI_ERROR;
    }

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Return 0 for success
}

/**
 * @brief Stop a non-blocking DMA SPI read and release the sensor CS line.
 * @return 0 on success, non-zero on error.
 */
ISM330DHCX_STATUS ISM330DHCX_ReadMultiple_Stop_DMA(void){
    ISM330DHCX_CS_1;

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Return 0 for success
}

/**
 * @brief Handle INT1 interrupt from the ISM330DHCX sensor.
 * @return 0 on success, non-zero on error.
 */
ISM330DHCX_STATUS ISM330DHCX_INT1_IRQHandler(void){
    // TODO change to reading data in non-blocking using DMA 
    // ISM330DHCX_GET_GYRO_AND_ACC_DMA(EVENT_START_CYCLE);

    return ISM330DHCX_GET_GYRO_AND_ACC();
}

/**
 * @brief Perform a soft reset of the ISM330DHCX sensor.\n * @details Initiates a soft reset by setting the SWRESET bit in CTRL3_C register,
 *          then waits for the bit to auto-clear (indicates reset complete).
 * @return ISM330DHCX_OK on success
 * @note All registers are reset to default values
n * @note Delays 100ms after reset for sensor stabilization
 */
ISM330DHCX_STATUS ISM330DHCX_Reset(void) {
	uint8_t temp = ISM330DHCX_CTRL3_C_SWRESET;

	ISM330DHCX_WriteReg(ISM330DHCX_REG_CTRL3_C, temp);

	while (temp == ISM330DHCX_CTRL3_C_SWRESET) {
		ISM330DHCX_ReadReg(ISM330DHCX_REG_CTRL3_C, &temp);
	}

	HAL_Delay(100);

	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK;
}

/**
 * @brief Enter or exit sensor configuration mode.
 * @details In configuration mode, the sensor is disabled to allow register updates.
 *          Must enter configuration mode before changing sensor settings.
 * @param enable Non-zero to enter config mode, zero to exit
 * @return ISM330DHCX_OK on success
 * @note While in config mode, sensor readings are not updated
 */
ISM330DHCX_STATUS ISM330DHCX_ConfigurationMode(uint8_t enable) {
	uint8_t temp = 0;
	if (enable) {
		temp = 0x02;
	}
	ISM330DHCX_WriteReg(ISM330DHCX_REG_CTRL9_XL, temp);

	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK;
}

/**
 * @brief Initialize the ISM330DHCX sensor and configure its SPI interface.
 * @param hspi Pointer to the HAL SPI handle to use for communication.
 * @return 0 on success, 1 for communication failure, 2 if the device ID does not match.
 */
ISM330DHCX_STATUS ISM330DHCX_Init(SPI_HandleTypeDef *hspi){
    uint8_t who_am_i = 0;
    // SETUP SPI handle
    ISM330DHCX_hspi = hspi;

    ISM330DHCX_Reset();
  
    /* Check device ID */
    if (ISM330DHCX_ReadReg(ISM330DHCX_REG_WHO_AM_I, &who_am_i) != ISM330DHCX_OK) {
        return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR; // Communication error
    }
    
    if (who_am_i != ISM330DHCX_WHO_AM_I_VALUE) {
        return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_ERROR; // Device not found
    }
    
    ISM330DHCX_ConfigurationMode(1);
    /*	Configuration start */

    ISM330DHCX_WriteReg(ISM330DHCX_REG_CTRL3_C, 0x44);
    /*  
    * 1. Enables gyroscope digital LPF1 
    * 2. DRDY_MASK, so irq will fire eaven if data was not read.
    */
    ISM330DHCX_WriteReg(ISM330DHCX_REG_CTRL4_C, 0x0A); 

    ISM330DHCX_GYRO_CONFIG(ODR_6_66kHz, FS_2000dps, 0, 0);
    ISM330DHCX_ACCEL_CONFIG(ODR_6_66kHz, FS_16g, 0);
#ifdef ISM330DHCX_CALIBRATION
    ISM330DHCX_AXIS_SCALED_DATA offset_data;

    ISM330DHCX_GetGyroOffset(&offset_data);
#endif

    ISM330DHCX_IRQ_CONFIG(0x02, 0x00);

    /*	Configuration end	*/
    ISM330DHCX_ConfigurationMode(0);

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Success
}


/**
 * @brief Configure the gyroscope output data rate and full-scale range for the ISM330DHCX.
 * @details Sets gyro ODR (output data rate) and full-scale (sensitivity) range.
 *          Two alternative full-scale options available:
 *          - Standard: 125/250/500/1000/2000 dps (via fs parameter)
 *          - Alternative: 125 dps (fs_125=1) or 4000 dps (fs_4000=1) override
 * @param odr Output data rate selection (see ISM330DHCX_ODR enum)
 * @param fs Full-scale range selection in standard mode (see ISM330DHCX_GYRO_FS enum)
 * @param fs_125 Flag to override full-scale to 125 dps (overrides fs parameter if set)
 * @param fs_4000 Flag to override full-scale to 4000 dps (overrides fs parameter if set)
 * @return ISM330DHCX_OK on success
 *         ISM330DHCX_REG_ACCESS_ERROR if register read/write fails or verification fails
 *         ISM330DHCX_ERROR for invalid full-scale setting
 * @note Updates global ISM330DHCX_Sensor_State with ODR, FS, and Sensitivity values
 * @note Function verifies that written value matches register read-back
 */
ISM330DHCX_STATUS ISM330DHCX_GYRO_CONFIG(ISM330DHCX_ODR odr, ISM330DHCX_GYRO_FS fs, uint8_t fs_125, uint8_t fs_4000) {
    if (fs_4000 && fs_125) {
        return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_ERROR; // Invalid gyro full-scale setting
    }
    if (fs_125 || fs_4000) {
        fs = 0;
    }

    // Write configuration to sensor
    uint8_t reg_value = (odr << 4) | (fs << 2) | (fs_125 << 1) | fs_4000;
    uint8_t reg_r_value = 0;

    ISM330DHCX_WriteReg(ISM330DHCX_REG_CTRL2_G, reg_value);

    HAL_Delay(10);

    ISM330DHCX_ReadReg(ISM330DHCX_REG_CTRL2_G, &reg_r_value);

    if (reg_r_value != reg_value) {
    	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR;
    }

    // Update global config struct
    ISM330DHCX_Sensor_State.GYRO_Config_Data.odr = odr;
    ISM330DHCX_Sensor_State.GYRO_Config_Data.fs = fs;
    if (fs_125) {
        ISM330DHCX_Sensor_State.GYRO_Config_Data.fs = FS_125dps; // Override fs to 125 dps if fs_125 flag is set
    } else if (fs_4000) {
        ISM330DHCX_Sensor_State.GYRO_Config_Data.fs = FS_4000dps; // Override fs to 4000 dps if fs_4000 flag is set
    }

    // Set sensitivity divisor based on selected full-scale range
    // Sensitivity = full-scale range value (used in scaling calculations)
    switch (ISM330DHCX_Sensor_State.GYRO_Config_Data.fs) {
        case FS_125dps:
            ISM330DHCX_Sensor_State.GYRO_Config_Data.Sensitivity = 125.0f;
            break;
        case FS_250dps:
            ISM330DHCX_Sensor_State.GYRO_Config_Data.Sensitivity = 250.0f;
            break;
        case FS_500dps:
            ISM330DHCX_Sensor_State.GYRO_Config_Data.Sensitivity = 500.0f;
            break;
        case FS_1000dps:
            ISM330DHCX_Sensor_State.GYRO_Config_Data.Sensitivity = 1000.0f;
            break;
        case FS_2000dps:
            ISM330DHCX_Sensor_State.GYRO_Config_Data.Sensitivity = 2000.0f;
            break;
        case FS_4000dps:
            ISM330DHCX_Sensor_State.GYRO_Config_Data.Sensitivity = 4000.0f;
            break;
        default:
            return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_ERROR; // Invalid gyro full-scale setting
    }

    ISM330DHCX_Sensor_State.GYRO_Config_Data.Scale = ISM330DHCX_Sensor_State.GYRO_Config_Data.Sensitivity / 32768.0f;

    /* for converting to SI uints */
    ISM330DHCX_Sensor_State.GYRO_Config_Data.SIScale = ISM330DHCX_Sensor_State.GYRO_Config_Data.Scale * __PI / 180.0f;

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK;
}


/**
 * @brief Configure the accelerometer output data rate and full-scale range for the ISM330DHCX.
 * @details Sets accel ODR (output data rate) and full-scale (sensitivity) range.
 *          Supports enabling digital low-pass filter (LPF) for noise reduction.
 * @param odr Output data rate selection (see ISM330DHCX_ODR enum)
 * @param fs Full-scale range selection (see ISM330DHCX_ACCEL_FS enum)
 * @param lpf2_xl_en Enable digital low-pass filter (1=enabled, 0=disabled)
 * @return ISM330DHCX_OK on success
 *         ISM330DHCX_REG_ACCESS_ERROR if register read/write fails or verification fails
 *         ISM330DHCX_ERROR for invalid full-scale setting
 * @note Updates global ISM330DHCX_Sensor_State with ODR, FS, and Sensitivity values
 * @note Function verifies that written value matches register read-back
 */
ISM330DHCX_STATUS ISM330DHCX_ACCEL_CONFIG(ISM330DHCX_ODR odr, ISM330DHCX_ACCEL_FS fs, uint8_t lpf2_xl_en) {
    // Write configuration to sensor
    uint8_t reg_value = (odr << 4) | (fs << 2) | (lpf2_xl_en << 1);
    uint8_t reg_r_value = 0;

    ISM330DHCX_WriteReg(ISM330DHCX_REG_CTRL1_XL, reg_value);

    HAL_Delay(10);

    ISM330DHCX_ReadReg(ISM330DHCX_REG_CTRL1_XL, &reg_r_value);

    if (reg_r_value != reg_value) {
    	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR;
    }

    // Update global config struct
    ISM330DHCX_Sensor_State.ACCEL_Config_Data.odr = odr;
    ISM330DHCX_Sensor_State.ACCEL_Config_Data.fs = fs;

    // Set sensitivity divisor based on selected full-scale range
    // Sensitivity = full-scale range value (used in scaling calculations)
    switch (ISM330DHCX_Sensor_State.ACCEL_Config_Data.fs) {
        case FS_2g:
            ISM330DHCX_Sensor_State.ACCEL_Config_Data.Sensitivity = 2.0f;
            break;

        case FS_4g:
            ISM330DHCX_Sensor_State.ACCEL_Config_Data.Sensitivity = 4.0f;
            break;

        case FS_8g:
            ISM330DHCX_Sensor_State.ACCEL_Config_Data.Sensitivity = 8.0f;
            break;

        case FS_16g:
            ISM330DHCX_Sensor_State.ACCEL_Config_Data.Sensitivity = 16.0f;
            break;

        default:
            return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_ERROR; // Invalid accel full-scale setting
    }

    ISM330DHCX_Sensor_State.ACCEL_Config_Data.Scale = ISM330DHCX_Sensor_State.ACCEL_Config_Data.Sensitivity / 32768.0f;
    /* for converting to SI uints */
    ISM330DHCX_Sensor_State.ACCEL_Config_Data.SIScale = ISM330DHCX_Sensor_State.ACCEL_Config_Data.Scale * __G;

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK;
}


ISM330DHCX_STATUS ISM330DHCX_IRQ_CONFIG(uint8_t int1, uint8_t int2) {
	uint8_t r_reg = 0;

	__disable_irq();

	if (ISM330DHCX_WriteReg(ISM330DHCX_REG_INT1_CTRL, int1) != ISM330DHCX_OK) {
        return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR; // Communication error
    }
	ISM330DHCX_ReadReg(ISM330DHCX_REG_INT1_CTRL, &r_reg);
	if (r_reg != int1) {
		return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR;
	}


	if (ISM330DHCX_WriteReg(ISM330DHCX_REG_INT2_CTRL, int2) != ISM330DHCX_OK) {
        return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR; // Communication error
    }
	ISM330DHCX_ReadReg(ISM330DHCX_REG_INT2_CTRL, &r_reg);
	if (r_reg != int2) {
		return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR;
	}

	__enable_irq();

	return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK;
}

/**
 * @brief Read the status register from the ISM330DHCX sensor.
 * @details Reads the STATUS_REG register to check data availability and sensor state.
 *          Status bit indicators:
 *          - Bit 0: XLDA - Accelerometer data available
 *          - Bit 1: GDA  - Gyroscope data available
 * @param status Pointer to receive the status register value
 * @return ISM330DHCX_OK on success, ISM330DHCX_REG_ACCESS_ERROR on SPI failure
 */
ISM330DHCX_STATUS ISM330DHCX_GET_SENSOR_STATUS(uint8_t* status){
    if (ISM330DHCX_ReadReg(ISM330DHCX_REG_STATUS_REG, status) != ISM330DHCX_OK) {
        return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR; // Communication error
    }
    
    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Success
}

/**
 * @brief Combine raw sensor bytes into signed 16-bit gyro and accelerometer values.
 * @param data_in Input buffer containing raw register bytes.
 * @param data_out Output structure to receive combined axis values.
 * @return 0 on success.
 */
ISM330DHCX_STATUS ISM330DHCX_COMBINE_RAW_DATA(uint8_t* data_in, ISM330DHCX_AXIS_RAW_DATA *data_out){
    // TODO ISM330DHCX_AXIS_RAW_DATA to array, posible speed up
    // Combine high and low bytes for gyro and accel data
    data_out->Gyro_X  =  ((int16_t)(data_in[1] << 8)  | data_in[0]);
    data_out->Gyro_Y  =  ((int16_t)(data_in[3] << 8)  | data_in[2]);
    data_out->Gyro_Z  =  ((int16_t)(data_in[5] << 8)  | data_in[4]);
    data_out->Accel_X =  ((int16_t)(data_in[7] << 8)  | data_in[6]);
    data_out->Accel_Y =  ((int16_t)(data_in[9] << 8)  | data_in[8]);
    data_out->Accel_Z =  ((int16_t)(data_in[11] << 8) | data_in[10]);

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Success
}


/*
*
*
*/
ISM330DHCX_STATUS ISM330DHCX_ScaleRawData (void) {
    ISM330DHCX_ScaledData.Gyro_X = ((float)ISM330DHCX_Sensor_State.Raw_Data.Gyro_X * (float)ISM330DHCX_Sensor_State.GYRO_Config_Data.Scale);
    ISM330DHCX_ScaledData.Gyro_Y = ((float)ISM330DHCX_Sensor_State.Raw_Data.Gyro_Y * (float)ISM330DHCX_Sensor_State.GYRO_Config_Data.Scale);
    ISM330DHCX_ScaledData.Gyro_Z = ((float)ISM330DHCX_Sensor_State.Raw_Data.Gyro_Z * (float)ISM330DHCX_Sensor_State.GYRO_Config_Data.Scale);

    // Convert raw data to physical [g]
    ISM330DHCX_ScaledData.Accel_X = ((float)ISM330DHCX_Sensor_State.Raw_Data.Accel_X * (float)ISM330DHCX_Sensor_State.ACCEL_Config_Data.Scale);
    ISM330DHCX_ScaledData.Accel_Y = ((float)ISM330DHCX_Sensor_State.Raw_Data.Accel_Y * (float)ISM330DHCX_Sensor_State.ACCEL_Config_Data.Scale);
    ISM330DHCX_ScaledData.Accel_Z = ((float)ISM330DHCX_Sensor_State.Raw_Data.Accel_Z * (float)ISM330DHCX_Sensor_State.ACCEL_Config_Data.Scale);

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Success
}

ISM330DHCX_STATUS ISM330DHCX_ScaleRawDataToSI (void) {
    ISM330DHCX_ScaledData.Gyro_X = ((float)ISM330DHCX_Sensor_State.Raw_Data.Gyro_X * (float)ISM330DHCX_Sensor_State.GYRO_Config_Data.SIScale);
    ISM330DHCX_ScaledData.Gyro_Y = ((float)ISM330DHCX_Sensor_State.Raw_Data.Gyro_Y * (float)ISM330DHCX_Sensor_State.GYRO_Config_Data.SIScale);
    ISM330DHCX_ScaledData.Gyro_Z = ((float)ISM330DHCX_Sensor_State.Raw_Data.Gyro_Z * (float)ISM330DHCX_Sensor_State.GYRO_Config_Data.SIScale);

    // Convert raw data to physical [g]
    ISM330DHCX_ScaledData.Accel_X = ((float)ISM330DHCX_Sensor_State.Raw_Data.Accel_X * (float)ISM330DHCX_Sensor_State.ACCEL_Config_Data.SIScale);
    ISM330DHCX_ScaledData.Accel_Y = ((float)ISM330DHCX_Sensor_State.Raw_Data.Accel_Y * (float)ISM330DHCX_Sensor_State.ACCEL_Config_Data.SIScale);
    ISM330DHCX_ScaledData.Accel_Z = ((float)ISM330DHCX_Sensor_State.Raw_Data.Accel_Z * (float)ISM330DHCX_Sensor_State.ACCEL_Config_Data.SIScale);

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Success
}

/**
 * @brief Read raw gyro and accelerometer data from the ISM330DHCX sensor using blocking SPI.
 * @details Waits for new data availability status, then reads all 12 bytes (6 gyro + 6 accel)
 *          in a single SPI transaction with auto-increment addressing.
 *          Data is combined into signed 16-bit values and stored in sensor state.
 * @return ISM330DHCX_OK on success, ISM330DHCX_REG_ACCESS_ERROR on communication failure
 * @note Updates the global ISM330DHCX_Sensor_State.Raw_Data structure
 * @warning Can block indefinitely if sensor stops responding - implementation recommended timeout
 */
ISM330DHCX_STATUS ISM330DHCX_GET_GYRO_AND_ACC(void){
    uint8_t data[ISM330DHCX_GYRO_AND_ACCEL_READ_SIZE]; // 6 bytes for gyro + 6 bytes for accel
     uint8_t status = 0;

     // wait for new data to be available
     while(status == 0){ // TODO add timeout to avoid infinite loop in case of communication error
         if (ISM330DHCX_GET_SENSOR_STATUS(&status) != ISM330DHCX_OK){
             return ISM330DHCX_REG_ACCESS_ERROR; // Communication error
         }
     }

    /* Read gyro and accel data */
    if (ISM330DHCX_ReadMultiple(ISM330DHCX_REG_OUTX_L_G, data, ISM330DHCX_GYRO_AND_ACCEL_READ_SIZE) != ISM330DHCX_OK) {
        return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_REG_ACCESS_ERROR; // Communication error
    }
  
    ISM330DHCX_COMBINE_RAW_DATA(data, &ISM330DHCX_Sensor_State.Raw_Data);
    
    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Success
}

/**
 * @brief A non-blocking DMA read for gyro and accelerometer data.
 * @param event Describes type of event which called this function.
 * @return 0 on success, non-zero on error.
 */
ISM330DHCX_STATUS ISM330DHCX_GET_GYRO_AND_ACC_DMA(Event_t event){
    static ISM330DHCX_STAGES_t Stage = ISM_STAGE_IDLE;
    static ISM330DHCX_STAGES_t Last_Stage = ISM_STAGE_SPI_ONGOING;
    NC_RAM static uint8_t SPI_RX_DMA_BUFFER[ISM330DHCX_GYRO_AND_ACCEL_READ_SIZE];

    /* Simple "time" out */
    static uint8_t ct = 0;
    if (Last_Stage == Stage) {
        ct++;
        if (ct > 5) {
            ISM330DHCX_ReadMultiple_Stop_DMA();
            ct = 0;
            Stage = ISM_STAGE_IDLE;

            return ISM330DHCX_ERROR;
        }
    }
    Last_Stage = Stage;


    switch (Stage) {
        case ISM_STAGE_IDLE:
            if (event == EVENT_START_CYCLE) {
                ISM330DHCX_Read13_Start_DMA(ISM330DHCX_REG_OUTX_L_G, SPI_RX_DMA_BUFFER);

                Stage = ISM_STAGE_SPI_ONGOING;

                return TS_IN_PROGRESS;
            }
            break;

        case ISM_STAGE_SPI_ONGOING:
            if (event == EVENT_SPI_TX_RX_DONE) {
                uint8_t temp_buff[ISM330DHCX_GYRO_AND_ACCEL_READ_SIZE];

                ISM330DHCX_ReadMultiple_Stop_DMA(); 

                for (int i = 0; i < 12; i++) {
                    temp_buff[i] = SPI_RX_DMA_BUFFER[i + 1]; 
                }

                ISM330DHCX_COMBINE_RAW_DATA(temp_buff, &ISM330DHCX_Sensor_State.Raw_Data);

#ifdef ISM330DHCX_SI_UNITS
                ISM330DHCX_ScaleRawDataToSI();
#else
                ISM330DHCX_ScaleRawData();
#endif

                Stage = ISM_STAGE_IDLE;

                return TS_END_WITH_SUCCESS;
            }
            break;
    }

    // Should never reach here
    return ISM330DHCX_OK;
}

/**
 * @brief Convert raw gyro and accelerometer readings into physical units.
 * @param scaled_data Output structure to receive scaled axis values.
 * @return 0 on success.
 */
ISM330DHCX_STATUS ISM330DHCX_GET_Scaled_GYRO_AND_ACC(ISM330DHCX_AXIS_SCALED_DATA *scaled_data) {
    // Convert raw data to physical [deg/s]
    scaled_data->Gyro_X = ISM330DHCX_ScaledData.Gyro_X;
    scaled_data->Gyro_Y = ISM330DHCX_ScaledData.Gyro_Y;
    scaled_data->Gyro_Z = ISM330DHCX_ScaledData.Gyro_Z;

    // Convert raw data to physical [g]
    scaled_data->Accel_X = ISM330DHCX_ScaledData.Accel_X;
    scaled_data->Accel_Y = ISM330DHCX_ScaledData.Accel_Y;
    scaled_data->Accel_Z = ISM330DHCX_ScaledData.Accel_Z;

    return ISM330DHCX_Sensor_State.Sensor_Status = ISM330DHCX_OK; // Success
}

/**
 * @brief Perform measurment of gyroscope and acc offset
 * @details Collects a large number of samples in stationary position and computes
 *          average values (biases) for calibration. Gyro should not move during this.
 *          This is slower but more accurate for initial system calibration. Use with
 *          debuger to see calculated values.
 * @param offset_data Output pointer to structure that will receive calculated offsets
 *                    in deg/s for gyro and g for accel
 * @return void
 * @note This function BLOCKS until all samples are collected (takes several seconds),
 * @warning Sensor must remain STATIONARY during calibration
 * @warning For Z-axis accel, expect value close to ±1g depending on sensor orientation
 */
void ISM330DHCX_GetGyroOffset(ISM330DHCX_AXIS_SCALED_DATA *offset_data){
    ISM330DHCX_AXIS_SCALED_DATA temp = {0};
    double gx = 0;
    double gy = 0;
    double gz = 0;

    double ax = 0;
    double ay = 0;
    double az = 0;

    for (uint32_t i = 0; i < 5000000; i++) {
        ISM330DHCX_GET_GYRO_AND_ACC();
        ISM330DHCX_GET_Scaled_GYRO_AND_ACC(&temp);

        gx += temp.Gyro_X;
        gy += temp.Gyro_Y;
        gz += temp.Gyro_Z;

        ax += temp.Accel_X;
        ay += temp.Accel_Y;
        az += temp.Accel_Z;
    }

    double gX = gx/(double)5000000;
    double gY = gy/(double)5000000;
    double gZ = gz/(double)5000000;

    double aX = ax/(double)5000000;
    double aY = ay/(double)5000000;
    double aZ = az/(double)5000000;

    /* Compiler happy */
    gX = gY + gZ + aX + aY + aZ;
    aZ = gX;
}
