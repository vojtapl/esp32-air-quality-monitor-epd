// GPIO ping definitions
#pragma once
#include <PDLS_EXT3_Basic_Global.h>

// UART
#define UART_TX 37
#define UART_RX 36

// I2C
#define I2C_SDA 1
#define I2C_SCL 2

// SD card
#define D0 3
#define D1 4
#define D2 5
#define D3 6
#define CMD 7
#define CLK 8
#define SD_DET 9

// EPD + SPI
#define NSPI_EDP_CS 10
#define SPI_SCLK 11
#define SPI_MOSI 12
#define EPD_BUSY 39
#define EPD_RES 40
#define EPD_D_C 41
#define EPD_POWER 46

// rotary encoder
#define ROTARY_ENCODER_A 13
#define ROTARY_ENCODER_B 14
#define ROTARY_ENCODER_BTN 47

// other
#define ALARM_LOW_BAT 48

pins_t epd_pins = {
    .panelBusy = EPD_BUSY,
    .panelDC = EPD_D_C,
    .panelReset = EPD_RES,
    .flashCS = NOT_CONNECTED,
    .panelCS = NSPI_EDP_CS,
    .panelCSS = NOT_CONNECTED,
    .flashCSS = NOT_CONNECTED,
    .panelPower = EPD_POWER,
};
