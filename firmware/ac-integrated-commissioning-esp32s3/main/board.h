#pragma once

#if defined(AC_BOARD_H2) == defined(AC_BOARD_R9)
#error "Exactly one physical board profile is required"
#endif

#ifdef AC_BOARD_H2
#define AC_BOARD_NAME "H2"
#define AC_ADS_ADDRESS 0x49
#define AC_HAS_DISPLAY 1
#else
#define AC_BOARD_NAME "R9"
#define AC_ADS_ADDRESS 0x48
#define AC_HAS_DISPLAY 0
#endif

enum {
    AC_PRESS_H_ON = 4, AC_PRESS_L_ON = 5, AC_BOOST_EN = 6, AC_ADC_RDY = 7,
    AC_SDA = 15, AC_SCL = 16, AC_MUX = 17, AC_SD_CD = 18,
    AC_SD_CS = 9, AC_MOSI = 10, AC_SCK = 11, AC_MISO = 12,
    AC_PRESS_H_FLT = 21, AC_PRESS_L_FLT = 47,
    AC_MENU = 8, AC_UP = 48, AC_DOWN = 2, AC_SELECT = 1,
    AC_LCD_CS = 13, AC_LCD_DC = 14, AC_LCD_RESET = 38, AC_LCD_BL = 39,
    AC_TOUCH_IRQ = 40, AC_RTD_H_CS = 41, AC_RTD_L_CS = 42,
    AC_GAUGE_ADDRESS = 0x36, AC_TOUCH_ADDRESS = 0x48,
};
