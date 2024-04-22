/**
 * @file ledstick.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Control SparkFun Qwiic LED Stick (https://www.sparkfun.com/products/18354)
 * @version 0.1
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __LED_STICK_H
#define __LED_STICK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LED stick
 *
 * @param cfg I2C configuration
 * @param devAddr I2C device address
 * @return uint32_t
 */
uint32_t
ledstick_init(ns_i2c_config_t *cfg, uint32_t devAddr);

/**
 * @brief Set the color of a single LED
 *
 * @param cfg I2C configuration
 * @param devAddr I2C device address
 * @param number LED number
 * @param red Red value
 * @param green Green value
 * @param blue Blue value
 * @return uint32_t
 */
uint32_t
ledstick_set_color(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t number, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Set the color of all LEDs
 *
 * @param cfg I2C configuration
 * @param devAddr I2C device address
 * @param red Red value
 * @param green Green value
 * @param blue Blue value
 * @return uint32_t
 */
uint32_t
ledstick_set_all_colors(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Turn off all LEDs
 *
 * @param cfg I2C configuration
 * @param devAddr I2C device address
 * @return uint32_t
 */
uint32_t
ledstick_set_all_off(ns_i2c_config_t *cfg, uint32_t devAddr);

/**
 * @brief Change the I2C address of the LED stick
 *
 * @param cfg I2C configuration
 * @param devAddr I2C device address
 * @param newAddress New I2C address
 * @return uint32_t
 */
uint32_t
ledstick_change_address(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t newAddress);

/**
 * @brief Change the number of LEDs
 *
 * @param cfg I2C configuration
 * @param devAddr I2C device address
 * @param newLength New number of LEDs
 * @return uint32_t
 */
uint32_t
ledstick_set_led_length(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t newLength);

/**
 * @brief Set the brightness of all LEDs
 *
 * @param cfg I2C configuration
 * @param devAddr I2C device address
 * @param brightness Brightness value (0-31)
 * @return uint32_t
 */
uint32_t
ledstick_set_all_brightness(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t brightness);

/**
 * @brief Set the brightness of a single LED
 *
 * @param cfg I2C configuration
 * @param devAddr I2C device address
 * @param number LED number
 * @param brightness Brightness value (0-31)
 * @return uint32_t
 */
uint32_t
ledstick_set_brightness(ns_i2c_config_t *cfg, uint32_t devAddr, uint8_t number, uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif // __LED_STICK_H
