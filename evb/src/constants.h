/**
 * @file constants.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Store global app constants
 * @version 1.0
 * @date 2023-03-27
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __PK_CONSTANTS_H
#define __PK_CONSTANTS_H

#define SENSOR_RATE (400)
#define SAMPLE_RATE (250)
#define MAX86150_ADDR (0x5E)

#define GPIO_TRIGGER 22

#define DISPLAY_LEN_USEC (2000000)

#define PK_SENSOR_LEN (5 * SENSOR_RATE)
#define PK_DATA_LEN (5 * SAMPLE_RATE)
#define PK_PEAK_LEN (120)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif // __PK_CONSTANTS_H
