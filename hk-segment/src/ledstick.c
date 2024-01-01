/**
 * @file ledstick.c
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Control SparkFun Qwiic LED Stick (https://www.sparkfun.com/products/18354)
 * @version 0.1
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <stdint.h>

#include "ns_i2c.h"
#include "ns_i2c_register_driver.h"
#include "ledstick.h"

#define COMMAND_CHANGE_ADDRESS (0xC7)
#define COMMAND_CHANGE_LED_LENGTH (0x70)
#define COMMAND_WRITE_SINGLE_LED_COLOR (0x71)
#define COMMAND_WRITE_ALL_LED_COLOR (0x72)
#define COMMAND_WRITE_RED_ARRAY (0x73)
#define COMMAND_WRITE_GREEN_ARRAY (0x74)
#define COMMAND_WRITE_BLUE_ARRAY (0x75)
#define COMMAND_WRITE_SINGLE_LED_BRIGHTNESS (0x76)
#define COMMAND_WRITE_ALL_LED_BRIGHTNESS (0x77)
#define COMMAND_WRITE_ALL_LED_OFF (0x78)

uint32_t
ledstick_init(ns_i2c_config_t *cfg, uint32_t devAddr) {
  return 0;
}

uint32_t
ledstick_set_color(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t number, uint8_t red, uint8_t green, uint8_t blue) {
  uint8_t buf[4] = { number, red, green, blue};
  return ns_i2c_write_sequential_regs(cfg, devAddr, COMMAND_WRITE_SINGLE_LED_COLOR, buf, 4);
}

uint32_t
ledstick_set_all_colors(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t red, uint8_t green, uint8_t blue) {
  uint8_t buf[3] = { red, green, blue };
  return ns_i2c_write_sequential_regs(cfg, devAddr, COMMAND_WRITE_ALL_LED_COLOR, buf, 3);
}

uint32_t
ledstick_set_all_off(ns_i2c_config_t *cfg, uint32_t devAddr) {
  uint8_t buf[1] = { COMMAND_WRITE_ALL_LED_OFF };
  return ns_i2c_write(cfg, buf, 1, devAddr);
}

uint32_t
ledstick_change_address(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t newAddress) {
  uint8_t buf[1] = { newAddress };
  return ns_i2c_write_sequential_regs(cfg, devAddr, COMMAND_CHANGE_ADDRESS, buf, 1);
}

uint32_t
ledstick_set_led_length(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t newLength) {
  uint8_t buf[1] = { newLength };
  return ns_i2c_write_sequential_regs(cfg, devAddr, COMMAND_CHANGE_LED_LENGTH, buf, 1);
}

uint32_t
ledstick_set_all_brightness(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t brightness) {
  uint8_t buf = brightness & 0x1F;
  return ns_i2c_write_sequential_regs(cfg, devAddr, COMMAND_WRITE_ALL_LED_BRIGHTNESS, &buf, 1);
}

uint32_t
ledstick_set_brightness(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t number, uint8_t brightness) {
  uint8_t buf[2] = { number, brightness };
  buf[1] &= 0x1F;
  return ns_i2c_write_sequential_regs(cfg, devAddr, COMMAND_WRITE_SINGLE_LED_BRIGHTNESS, buf, 2);
}
