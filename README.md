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
- **Register Verification**: Write operations verified via read-back to ensure reliability

## Hardware Setup

### SPI Configuration

Configure your STM32CubeMX SPI peripheral with these settings:

```
Mode:              Full-Duplex Master
Data Size:         8-bit
Clock Polarity:    Low (CPOL=0)
Clock Phase:       1 Edge (CPHA=0)
Clock Speed:       ≤10 MHz (required by ISM330DHCX)
NSS:               Software (manual CS control)
```

**Critical**: The SPI clock must remain below 10 MHz for reliable operation.

### GPIO Configuration

| Pin | Purpose | Configuration | Notes |
|-----|---------|---|---|
| CS (NSS) | Chip Select | Output, Push-Pull | Manually controlled in driver |
| CLK | Clock | Alternate Function | |
| MOSI | Master Out | Alternate Function | |
| MISO | Master In | Alternate Function | |
| INT1 | Data Ready Interrupt | Input, External Interrupt | Recommended for DMA operation |
| INT2 | Interrupt 2 | Input | Not currently supported by driver |


### DMA Configuration

Enable DMA for SPI6 (or your SPI instance):

<img width="1087" height="1025" alt="image" src="https://github.com/user-attachments/assets/63e80ae7-cd9c-4756-ad41-29559e03fa1f" />
<img width="1087" height="1025" alt="image" src="https://github.com/user-attachments/assets/95ccb4cc-9e78-4b53-b4df-6abc46570759" />

### NVIC Configuration

Enable interrupts in NVIC:
- **EXTI** (for INT1 pin): Enabled with appropriate priority

### Optional: SBS Configuration

If your STM32 includes SBS functionality, ensure it is properly configured in STM32CubeMX.

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
- SPI handle configuration
- Sensor soft reset
- WHO_AM_I verification (must read 0x6C)
- Default configuration (6.66 kHz ODR, ±1000 dps gyro, ±4g accel)
- INT1 interrupt configuration for data-ready events (INT1_CTRL = 0x02)
- Optional calibration (if `ISM330DHCX_CALIBRATION` is enabled)

### Custom Configuration

To modify sensor settings, edit the configuration before calling `ISM330DHCX_ConfigurationMode(0)`:

```c
// Enter configuration mode (disables sensor)
ISM330DHCX_ConfigurationMode(1);

/* Configuration start */
ISM330DHCX_GYRO_CONFIG(ODR_6_66kHz, FS_1000dps, 0, 0);
ISM330DHCX_ACCEL_CONFIG(ODR_6_66kHz, FS_4g, 0);
ISM330DHCX_IRQ_CONFIG(0x02, 0x00);  // INT1 on data-ready
/* Configuration end */

// Exit configuration mode (enables sensor)
ISM330DHCX_ConfigurationMode(0);
```

**Configuration Details**:
- `ISM330DHCX_GYRO_CONFIG(odr, fs, fs_125, fs_4000)`: Sets gyroscope output data rate and full-scale range
  - `fs_125`: Set to 1 to override full-scale to 125 dps
  - `fs_4000`: Set to 1 to override full-scale to 4000 dps
  - All writes are verified via read-back
  
- `ISM330DHCX_ACCEL_CONFIG(odr, fs, lpf2_xl_en)`: Sets accelerometer ODR, full-scale, and optional low-pass filter
  - `lpf2_xl_en`: Set to 1 to enable digital low-pass filter

- `ISM330DHCX_IRQ_CONFIG(int1, int2)`: Configures interrupt routing
  - Briefly disables global interrupts to prevent race conditions
  - Verifies both INT1_CTRL and INT2_CTRL registers via read-back

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

**DMA Read Operation Flow**:
1. `EVENT_START_CYCLE`: Initiates DMA read from gyro/accel output registers
2. `EVENT_SPI_TX_RX_DONE`: Completes DMA transfer and processes data
3. State machine automatically returns to idle after processing

### Data Retrieval

The driver operates in the background when using DMA, continuously updating sensor data. Retrieve scaled data anywhere in your application:

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
| `ISM330DHCX_ERROR` | 0x03 | General sensor error (e.g., invalid WHO_AM_I) |
| `ISM330DHCX_REG_ACCESS_ERROR` | 0x04 | Register read/write verification failed |

**DMA-specific Return Values**:
- `TS_IN_PROGRESS`: DMA operation initiated, waiting for completion
- `TS_END_WITH_SUCCESS`: DMA operation completed successfully

## API Reference

### Initialization & Configuration

- `ISM330DHCX_Init(SPI_HandleTypeDef *hspi)` - Initialize sensor with default configuration
- `ISM330DHCX_Reset()` - Perform soft reset (clears all registers to defaults, waits 100ms)
- `ISM330DHCX_ConfigurationMode(uint8_t enable)` - Enter/exit configuration mode (disables sensor output during configuration)
- `ISM330DHCX_GYRO_CONFIG(odr, fs, fs_125, fs_4000)` - Configure gyroscope with register verification
- `ISM330DHCX_ACCEL_CONFIG(odr, fs, lpf2_xl_en)` - Configure accelerometer with register verification
- `ISM330DHCX_IRQ_CONFIG(int1, int2)` - Configure interrupt routing with atomic register updates

### Data Acquisition

- `ISM330DHCX_GET_GYRO_AND_ACC()` - Blocking read with status polling (not recommended for real-time applications)
- `ISM330DHCX_GET_GYRO_AND_ACC_DMA(event)` - Non-blocking DMA-based read with state machine
- `ISM330DHCX_GET_Scaled_GYRO_AND_ACC(data)` - Retrieve most recent scaled data

### Register I/O

- `ISM330DHCX_ReadReg(reg, data)` - Read single register with SPI blocking mode
- `ISM330DHCX_WriteReg(reg, data)` - Write single register with SPI blocking mode
- `ISM330DHCX_ReadMultiple(reg_start, data, length)` - Read multiple consecutive registers with auto-increment
- `ISM330DHCX_Read13_Start_DMA(reg_start, data)` - Start non-blocking DMA read
- `ISM330DHCX_ReadMultiple_Stop_DMA()` - Stop DMA read and release CS line
- `ISM330DHCX_GET_SENSOR_STATUS(status)` - Read status register (bits 0-1 indicate data availability)

### Calibration

- `ISM330DHCX_GetGyroOffset(offset_data)` - Measure gyro/accel bias (collects 5 million samples, **blocks indefinitely**)

### Internal Helper Functions

- `ISM330DHCX_COMBINE_RAW_DATA(data_in, data_out)` - Combine raw register bytes into signed 16-bit values
- `ISM330DHCX_ScaleRawData()` - Convert raw values to physical units using stored scaling factors

## Troubleshooting

### Sensor Not Responding
- **Verify SPI clock is ≤10 MHz** 
- Confirm CS line is properly controlled (manual GPIO toggle via `ISM330DHCX_CS_0` / `ISM330DHCX_CS_1` macros)
- Verify WHO_AM_I register reads 0x6C (`ISM330DHCX_WHO_AM_I_VALUE`)
- Check that sensor is powered and properly grounded

### Data Appears Invalid
- Ensure configuration mode is properly entered/exited
- Verify ODR and full-scale settings match your expectations
- Check that sensor remains powered during initialization
- Confirm register write verification succeeds (all `_CONFIG()` functions verify writes)

### DMA Transfers Failing
- Verify DMA channels are correctly configured for the SPI instance
- **Check that DMA buffers are placed in DMA-accessible memory (RAM1)** - use the `RAM1` attribute
- Verify DMA interrupt callbacks (`HAL_SPI_TxRxCpltCallback`) are properly implemented
- Ensure external interrupt callback (`HAL_GPIO_EXTI_Callback`) is correctly triggered by INT1 pin

### Intermittent Communication
- Reduce SPI clock speed (try 5 MHz)
- Add delays between successive SPI transactions
- Verify adequate power supply current (≥50mA recommended during operation)
- Check signal integrity on SPI lines (CS, CLK, MOSI, MISO)

### Register Verification Failures
- Register write failures return `ISM330DHCX_REG_ACCESS_ERROR`
- Verify sensor is not in a hung state - try reset
- Check for electrical noise or signal integrity issues on SPI bus

## Memory Placement

DMA buffers are placed in RAM1 using the `RAM1` attribute to ensure DMA accessibility. Ensure your linker script allocates sufficient RAM1 space:

```c
RAM1 static uint8_t SPI_RX_DMA_BUFFER[ISM330DHCX_GYRO_AND_ACCEL_READ_SIZE];
```

## Calibration

Enable calibration by uncommenting `#define ISM330DHCX_CALIBRATION` in `ism330dhcx.c`. The calibration function collects 5 million samples while the sensor is stationary to compute gyro/accel biases.

**WARNING**: The current implementation:
- **Blocks indefinitely** during calibration collection
- **Does NOT write computed offsets** back to the output structure (TODO item)
- Requires the sensor to remain completely **stationary**
- Will take several minutes on a typical STM32 at 6.66 kHz ODR

**Recommended Improvements**:
- Reduce sample count (5M is excessive for production)
- Implement timeout mechanism
- Write computed averages to output structure
- Add progress callback or non-blocking alternative

## Interrupt Handling Notes

- `ISM330DHCX_IRQ_CONFIG()` briefly disables global interrupts (`__disable_irq()`/`__enable_irq()`) to ensure atomic register updates
- INT1 is configured for data-ready events (all 3 axes ready)
- INT2 routing is configurable but not actively used in the driver
- For real-time applications, use DMA-based acquisition with interrupt callbacks

## Platform Support

- **Tested on**: STM32H7RS series
- **Requires**: HAL SPI implementation with DMA support
- **Language**: C99 with some C11 features (VLA in `ISM330DHCX_ReadMultiple`)
- **License**: Apache License 2.0

## Known Limitations & TODOs

1. **Blocking operations**: `ISM330DHCX_GET_GYRO_AND_ACC()` has no timeout - can hang indefinitely
2. **Calibration**: `ISM330DHCX_GetGyroOffset()` blocks for ~30+ seconds, doesn't populate output structure
3. **D-Cache**: Cache invalidation for DMA buffers is not implemented (TODO in code)
4. **INT2 support**: Only INT1 is actively used; INT2 is configured but not integrated
5. **Status polling**: Blocking read relies on status register polling rather than interrupt-driven approach

## Additional Resources

- ISM330DHCX Datasheet: ST Microelectronics
- STM32CubeMX Configuration: See included `.ioc` file for example project setup
- HAL SPI Documentation: STM32 HAL User Manual

---

**Last Updated**: 2026-05-23  
**Tested On**: STM32H7RS series  
**License**: Apache License 2.0
