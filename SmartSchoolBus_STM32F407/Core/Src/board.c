#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
SPI_HandleTypeDef spi;
I2C_HandleTypeDef i2c;
SD_HandleTypeDef sd;
void xPortSysTickHandler(void);
void SysTick_Handler(void){
    HAL_IncTick();
    if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED) xPortSysTickHandler();
}
static void check(HAL_StatusTypeDef r){if(r!=HAL_OK) for(;;){}}
static void pins(GPIO_TypeDef *port,unsigned mask,unsigned mode,unsigned pull,unsigned af){
    GPIO_InitTypeDef p={.Pin=mask,.Mode=mode,.Pull=pull,.Speed=GPIO_SPEED_FREQ_VERY_HIGH,.Alternate=af};
    HAL_GPIO_Init(port,&p);
}
void buzz(bool on){
    HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12,(on != BUZZER_ACTIVE_LOW)?GPIO_PIN_SET:GPIO_PIN_RESET);
}
void board_init(void){
    HAL_Init();
    // Internal 16 MHz HSI -> 96 MHz SYSCLK and 48 MHz SDIO.
    RCC_OscInitTypeDef osc={.OscillatorType=RCC_OSCILLATORTYPE_HSI,.HSIState=RCC_HSI_ON,
        .HSICalibrationValue=RCC_HSICALIBRATION_DEFAULT,
        .PLL={.PLLState=RCC_PLL_ON,.PLLSource=RCC_PLLSOURCE_HSI,.PLLM=16,.PLLN=192,.PLLP=RCC_PLLP_DIV2,.PLLQ=4}};
    check(HAL_RCC_OscConfig(&osc));
    RCC_ClkInitTypeDef clk={.ClockType=15,.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK,
        .AHBCLKDivider=RCC_SYSCLK_DIV1,.APB1CLKDivider=RCC_HCLK_DIV4,.APB2CLKDivider=RCC_HCLK_DIV2};
    check(HAL_RCC_ClockConfig(&clk,FLASH_LATENCY_3));
    __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE(); __HAL_RCC_I2C1_CLK_ENABLE(); __HAL_RCC_SDIO_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_SET); buzz(false);
    pins(GPIOB,GPIO_PIN_12,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,0);
    pins(GPIOD,GPIO_PIN_8|GPIO_PIN_12,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,0);
    pins(GPIOD,GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11,GPIO_MODE_INPUT,GPIO_PULLUP,0);
    pins(GPIOB,GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15,GPIO_MODE_AF_PP,GPIO_NOPULL,GPIO_AF5_SPI2);
    pins(GPIOB,GPIO_PIN_6|GPIO_PIN_7,GPIO_MODE_AF_OD,GPIO_PULLUP,GPIO_AF4_I2C1);
    pins(GPIOC,GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11,GPIO_MODE_AF_PP,GPIO_PULLUP,GPIO_AF12_SDIO);
    pins(GPIOC,GPIO_PIN_12,GPIO_MODE_AF_PP,GPIO_NOPULL,GPIO_AF12_SDIO);
    pins(GPIOD,GPIO_PIN_2,GPIO_MODE_AF_PP,GPIO_PULLUP,GPIO_AF12_SDIO);
    spi.Instance=SPI2;
    spi.Init=(SPI_InitTypeDef){.Mode=SPI_MODE_MASTER,.Direction=SPI_DIRECTION_2LINES,
        .DataSize=SPI_DATASIZE_8BIT,.NSS=SPI_NSS_SOFT,.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_32};
    check(HAL_SPI_Init(&spi));
    i2c.Instance=I2C1;
    i2c.Init=(I2C_InitTypeDef){.ClockSpeed=100000,.DutyCycle=I2C_DUTYCYCLE_2,.AddressingMode=I2C_ADDRESSINGMODE_7BIT};
    check(HAL_I2C_Init(&i2c));
    sd.Instance=SDIO;
    sd.Init=(SD_InitTypeDef){.ClockEdge=SDIO_CLOCK_EDGE_RISING,.BusWide=SDIO_BUS_WIDE_1B,.ClockDiv=118};
}
