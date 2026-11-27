#pragma once
#include "stm32f4xx_hal.h"
#include <stdbool.h>
extern SPI_HandleTypeDef spi;
extern I2C_HandleTypeDef i2c;
extern SD_HandleTypeDef sd;
#define BUZZER_ACTIVE_LOW 1
void board_init(void);
void buzz(bool on);
bool reader_init(void);
// 1 = UID, 0 = empty field, -1 = protocol or bus error
int read_uid(char out[9]);
bool oled_init(void);
void oled_line(unsigned page,const char *s);
void oled_title(const char *s);

