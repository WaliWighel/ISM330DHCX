# ISM330DHCX STM32 Driver

A high-performance, DMA-based SPI driver for the ST ISM330DHCX 6-axis Inertial Measurement Unit (IMU) on STM32 microcontrollers.

## Features

- **SPI Communication**: DMA and blocking modes for flexible integration
- **Dual-Sensor Support**: Independent gyroscope (±125/250/500/1000/2000/4000 dps) and accelerometer (±2/4/8/16 g) configuration
- **Configurable Output Data Rate**: ODR from 12.5 Hz to 6.66 kHz
- **Real-time Data Access**: Background operation with on-demand data retrieval
- **Interrupt Support**: INT1 interrupt-driven data acquisition
- **Calibration Support**: Built-in gyro/accel offset measurement
- **Scaled Output**: Automatic raw-to-physical unit conversion (deg/s, g)

## Hardware Setup

### SPI Configuration

Configure your STM32CubeMX SPI peripheral with these settings:

```
Mode:              Full-Duplex Master
Data Size:         8-bit
Clock Polarity:    High (CPOL=1)
Clock Phase:       2 Edge (CPHA=1)
Clock Speed:       ≤10 MHz (required by ISM330DHCX)
NSS:               Software (manual CS control)
```

**Critical**: The SPI clock must remain below 10 MHz for reliable operation.

### GPIO Configuration

| Pin | Purpose | Configuration | Notes |
|-----|---------|---|---|
| CS (NSS) | Chip Select | Output, Push-Pull | Manually controlled in driver |
| CLK | Clock | Alternate Function | **Must have Pull-Up enabled** |
| MOSI | Master Out | Alternate Function | |
| MISO | Master In | Alternate Function | |
| INT1 | Data Ready Interrupt | Input, External Interrupt | Recommended for DMA operation |
| INT2 | Interrupt 2 | Input | Not currently supported by driver |

⚠️ **Important**: The SPI_CLK pin **requires Pull-Up configuration**. Without it, the sensor will not respond to communication attempts.

### DMA Configuration

Enable DMA for SPI6 (or your SPI instance):

<img width="1087" height="1025" alt="image" src="https://github.com/user-attachments/assets/63e80ae7-cd9c-4756-ad41-29559e03fa1f" />
<img width="1087" height="1025" alt="image" src="https://github.com/user-attachments/assets/95ccb4cc-9e78-4b53-b4df-6abc46570759" />



### NVIC Configuration

Enable interrupts in NVIC:
- **EXTI** (for INT1 pin): Enabled with appropriate priority

### Optional: (SBS)

If your STM32 includes SBS  functionality, ensure it is properly configured in STM32CubeMX.

## Software Installation

1. **Header File**: Copy `Core/Inc/ism330dhcx.h` and `Core/Inc/ism330dhcx_defines.h` to your project
2. **Source File**: Copy `Core/Src/ism330dhcx.c` to your project
3. **Integration**: Include `ism330dhcx.h` in your main application

## Usage Guide

### Initialization

```c
#include "ism330dhcx.h"

int main(void) {
    // ... STM32 initialization code ...
    
    // Initialize the ISM330DHCX on SPI6
    ISM330DHCX_Init(&hspi6);
    
    // Main loop
    while(1) {
        // Your application code
    }
}
```

The `ISM330DHCX_Init()` function performs:
- SPI device verification (WHO_AM_I check)
- Sensor soft reset
- Default configuration (6.66 kHz ODR, ±1000 dps gyro, ±4g accel)
- INT1 interrupt configuration for data-ready events
- Optional calibration (if `ISM330DHCX_CALIBRATION` is enabled)

### Custom Configuration

To modify sensor settings, edit the configuration within `ISM330DHCX_ConfigurationMode()`:

```c
// In ISM330DHCX_Init() or your code:
ISM330DHCX_ConfigurationMode(1);  // Enter configuration mode

/* Configuration start */
ISM330DHCX_GYRO_CONFIG(ODR_6_66kHz, FS_1000dps, 0, 0);
ISM330DHCX_ACCEL_CONFIG(ODR_6_66kHz, FS_4g, 0);
ISM330DHCX_IRQ_CONFIG(0x03, 0x00);  // INT1 on data-ready
/* Configuration end */

ISM330DHCX_ConfigurationMode(0);  // Exit configuration mode
```

### Data Acquisition with DMA (Recommended)

Implement two HAL callbacks for interrupt-driven, non-blocking data acquisition:

```c
/**
 * GPIO External Interrupt Callback
 * Triggered when ISM330DHCX data is ready (INT1 pin)
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == ISM330DHCX_INT1_Pin) {
        // Start DMA read cycle
        ISM330DHCX_GET_GYRO_AND_ACC_DMA(EVENT_START_CYCLE);
    }
}

/**
 * SPI DMA Transfer Complete Callback
 * Called when DMA transfer finishes
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI6) {
        // Complete DMA read cycle
        ISM330DHCX_GET_GYRO_AND_ACC_DMA(EVENT_SPI_TX_RX_DONE);
        
        // Data is now ready - fetch scaled values
        ISM330DHCX_AXIS_SCALED_DATA sensor_data;
        ISM330DHCX_GET_Scaled_GYRO_AND_ACC(&sensor_data);
        
        // Use sensor_data.Gyro_X, sensor_data.Gyro_Y, etc. (in deg/s)
        // Use sensor_data.Accel_X, sensor_data.Accel_Y, etc. (in g)
    }
}
```

### Data Retrieval

The driver operates in the background, continuously updating sensor data. Retrieve scaled data anywhere in your application:

```c
ISM330DHCX_AXIS_SCALED_DATA sensor_reading;
ISM330DHCX_GET_Scaled_GYRO_AND_ACC(&sensor_reading);

// Access data in physical units
float gyro_x_dps = sensor_reading.Gyro_X;   // deg/s
float accel_z_g = sensor_reading.Accel_Z;   // g
```

**Data Structures**:

```c
// Raw 16-bit sensor values
typedef struct {
    int16_t Gyro_X, Gyro_Y, Gyro_Z;
    int16_t Accel_X, Accel_Y, Accel_Z;
} ISM330DHCX_AXIS_RAW_DATA;

// Scaled physical units
typedef struct {
    float Gyro_X, Gyro_Y, Gyro_Z;    // deg/s
    float Accel_X, Accel_Y, Accel_Z; // g
} ISM330DHCX_AXIS_SCALED_DATA;
```

## Status Codes

| Code | Value | Meaning |
|------|-------|---------|
| `ISM330DHCX_OK` | 0x01 | Operation successful |
| `ISM330DHCX_SPI_ERROR` | 0x02 | SPI communication failed |
| `ISM330DHCX_ERROR` | 0x03 | General sensor error |
| `ISM330DHCX_REG_ACCESS_ERROR` | 0x04 | Register read/write verification failed |

## API Reference

### Initialization & Configuration

- `ISM330DHCX_Init(SPI_HandleTypeDef *hspi)` - Initialize sensor
- `ISM330DHCX_Reset()` - Perform soft reset
- `ISM330DHCX_ConfigurationMode(uint8_t enable)` - Enter/exit config mode
- `ISM330DHCX_GYRO_CONFIG(odr, fs, fs_125, fs_4000)` - Configure gyroscope
- `ISM330DHCX_ACCEL_CONFIG(odr, fs, lpf2_xl_en)` - Configure accelerometer
- `ISM330DHCX_IRQ_CONFIG(int1, int2)` - Configure interrupts

### Data Acquisition

- `ISM330DHCX_GET_GYRO_AND_ACC()` - Blocking read (not recommended)
- `ISM330DHCX_GET_GYRO_AND_ACC_DMA(event)` - Non-blocking DMA read
- `ISM330DHCX_GET_Scaled_GYRO_AND_ACC(data)` - Retrieve scaled data

### Register I/O

- `ISM330DHCX_ReadReg(reg, data)` - Read single register
- `ISM330DHCX_WriteReg(reg, data)` - Write single register
- `ISM330DHCX_ReadMultiple(reg_start, data, length)` - Read multiple registers
- `ISM330DHCX_GET_SENSOR_STATUS(status)` - Read status register

### Calibration

- `ISM330DHCX_GetGyroOffset(offset_data)` - Measure gyro/accel bias

## Troubleshooting

### Sensor Not Responding
- **Verify SPI clock is ≤10 MHz** (most common issue)
- Confirm CS line is properly controlled (manual GPIO toggle)
- Check SPI_CLK pin has Pull-Up enabled
- Verify WHO_AM_I register reads 0x6C

### Data Appears Invalid
- Ensure configuration mode is properly entered/exited
- Verify ODR and full-scale settings match your expectations
- Check that sensor remains powered and properly grounded

### DMA Transfers Failing
- Enable SPI6 DMA in NVIC
- Verify DMA channel is correctly configured
- Check that data buffers are placed in valid RAM region

### Intermittent Communication
- Reduce SPI clock speed (try 5 MHz)
- Add small delays after register writes
- Ensure adequate power supply current

## Memory Placement

Large DMA buffers are placed in RAM1 using the `RAM1` attribute. Ensure your linker script allocates sufficient RAM1 space for sensor data buffers.

## Calibration

Enable calibration by uncommenting `#define ISM330DHCX_CALIBRATION` in `ism330dhcx.c`. The calibration function collects 5 million samples while the sensor is stationary to compute gyro/accel biases. Use with a debugger to observe calculated offset values.

## License

This driver is provided under the Apache License 2.0. See LICENSE file for details.

## Additional Resources

- ISM330DHCX Datasheet: ST Microelectronics
- STM32CubeMX Configuration: See included `.ioc` file for example project setup

---

**Last Updated**: 2026-05-17  
**Tested On**: STM32H7RS series
