# ISM330DHCX
STM32 SPI DMA-based driver for ISM330DHCX

# CONFIGURATION
Configure spi to :
  DataSize = SPI_DATASIZE_8BIT;
  CLKPolarity = SPI_POLARITY_HIGH;
  CLKPhase = SPI_PHASE_2EDGE;
  keep you clk under 10MHz
  <img width="3263" height="1555" alt="image" src="https://github.com/user-attachments/assets/89e07bc1-83ff-49b0-8536-8bd7bd74d9c6" />

for some rason i had to configure my SPI_CLK pin to Pull-up, without it, ISM330DHCX did not wantd to talk to me. 

<img width="1144" height="245" alt="image" src="https://github.com/user-attachments/assets/c35992e6-159a-4375-8ea5-9aeddabc3e79" />

Add DMA :
<img width="1093" height="1066" alt="image" src="https://github.com/user-attachments/assets/c2d33556-b3f7-4b6f-bf79-d88a79a7b06a" />
<img width="1093" height="1066" alt="image" src="https://github.com/user-attachments/assets/f67e07fb-c5fd-4d18-8a82-490a3386022c" />

Configureate other GPIO, my driver do not use INT2 pin. Remember to enable irq in NVIC : 
<img width="1835" height="300" alt="image" src="https://github.com/user-attachments/assets/869232a7-69c8-46dd-b5e2-d23802d27a7f" />

Also IF you have SBS, remember to this, I had no idea of it and was struglig for a bit : 
<img width="1649" height="1594" alt="image" src="https://github.com/user-attachments/assets/95b96055-07fa-4606-ac25-aaeb0d3e1a51" />

# USAGE

to configure ISM330DHCX, add or change your configuration in ISM330DHCX_Init();. It is important to do it inbetwen of : 
ISM330DHCX_ConfigurationMode(1);
/*	Configuration start */ 

// you config

/*	Configuration end	*/
ISM330DHCX_ConfigurationMode(0);

then call ISM330DHCX_Init(); before main loop.

To get it to work you only need to : 

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  switch (GPIO_Pin) {
        // Called when data from ISM330DHCX is ready.
        // Trigered when data is ready on the ISM330DHCX sensor, currently set to trigger on INT1 pin, refer to ISM330DHCX_Init() for details on INT1 IRQ configuration
        case ISM330DHCX_INT1_Pin:
        	ISM330DHCX_GET_GYRO_AND_ACC_DMA(EVENT_START_CYCLE);
            break;

        case ISM330DHCX_INT2_Pin:
        	// Not supported
            break;
  }
}


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    switch ((uint32_t)hspi->Instance) {
        case (uint32_t)SPI6:
            ISM330DHCX_GET_GYRO_AND_ACC_DMA(EVENT_SPI_TX_RX_DONE);
        	ISM330DHCX_GET_Scaled_GYRO_AND_ACC(&temp_data);
            break;
    }
}

full example is in main.c

# GETTING DATA

Driver is designed to run in "background", so it is doing it things and when you need some data from it, you call anywhere you want : 

ISM330DHCX_GET_Scaled_GYRO_AND_ACC(&temp_data);

where ISM330DHCX_AXIS_SCALED_DATA temp_data;


