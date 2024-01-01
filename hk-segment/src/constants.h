/**
 * @file constants.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Store global app constants
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __PK_CONSTANTS_H
#define __PK_CONSTANTS_H

///////////////////////////////////////////////////////////////////////////////
// EVB Configuration
///////////////////////////////////////////////////////////////////////////////

#define I2C_IOM (1)
#define I2C_SPEED_HZ (400000)
#define MAX86150_ADDR (0x5E)
#define LEDSTICK_ADDR (0x23)

///////////////////////////////////////////////////////////////////////////////
// Sensor Configuration
///////////////////////////////////////////////////////////////////////////////

#define SENSOR_RATE (200)
#define SAMPLE_RATE (100)
#define DOWNSAMPLE_RATE (SENSOR_RATE / SAMPLE_RATE)
#define SENSOR_PPG1_SLOT (0)
#define SENSOR_PPG2_SLOT (1)
#define SENSOR_ECG_SLOT (2)
#define SENSOR_BUF_LEN (2 * 32) // Double FIFO depth


///////////////////////////////////////////////////////////////////////////////
// Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////

#define ECG_SOS_LEN (3)
#define PPG_SOS_LEN (3)
#define NORM_STD_EPS (0.1)

#define SEG_MODEL_SIZE_KB (60)
#define SEG_THRESHOLD (0.75)
#define SEG_NUM_CLASS (2)
#define SEG_WINDOW_LEN (250)
#define SEG_PAD_LEN (25)
#define SEG_VALID_LEN (SEG_WINDOW_LEN - 2 * SEG_PAD_LEN)
#define SEG_BUF_LEN (2 * SEG_WINDOW_LEN)

// ECG Segmentation Mask
// [1-0] : 2-bit segmentation (0:none, 1:p-wave, 2:qrs, 3:t-wave)
// [2-3] : 2-bit fiducial (0:none, 1:p-peak, 2:qrs, 3:t-peak)
// [4-5] : 2-bit QoS (0:bad, 1:poor, 2:fair, 3:good)
// [6-7] : 2-bit beat type (0:none, 1:nsr, 2:pac/pvc, 3:noise)

// PPG Segmentation Mask
// [1-0] : 2-bit segmentation (0:none, 1:systolic, 2:diastolic)
// [2-3] : 2-bit fiducial (0:none, 1:s-peak, 2:d-peak, 3:d-notch)
// [4-5] : 2-bit QoS (0:bad, 1:poor, 2:fair, 3:good)
// [6-7] : 2-bit beat type (0:none, 1:nsr, 2:pac/pvc, 3:noise)

#define SIG_MASK_SEG_OFFSET (0)
#define SIG_MASK_FID_OFFSET (2)
#define SIG_MASK_QOS_OFFSET (4)
#define SIG_MASK_BEAT_OFFSET (6)
#define SIG_MASK_SEG_MASK (0x3)
#define SIG_MASK_FID_MASK (0x3)
#define SIG_MASK_QOS_MASK (0x3)
#define SIG_MASK_BEAT_MASK (0x3)

// ECG Segmentation Classes
#define ECG_SEG_NONE (0)
#define ECG_SEG_PWAVE (3)
#define ECG_SEG_QRS (1)
#define ECG_SEG_TWAVE (3)

// ECG Fiducial Classes
#define ECG_FID_NONE (0)
#define ECG_FID_PPEAK (3)
#define ECG_FID_QRS (1)
#define ECG_FID_TPEAK (3)

// SIG QoS Classes
#define SIG_QOS_BAD (0)
#define SIG_QOS_POOR (1)
#define SIG_QOS_FAIR (2)
#define SIG_QOS_GOOD (3)

// ECG Beat Classes
#define ECG_BEAT_NONE (0)
#define ECG_BEAT_NSR (1)
#define ECG_BEAT_PNC (2)
#define ECG_BEAT_NOISE (3)

// PPG Segmentation Classes
#define PPG_SEG_NONE (0)
#define PPG_SEG_SYS (1)
#define PPG_SEG_DIA (2)

// PPG Fiducial Classes
#define PPG_FID_NONE (0)
#define PPG_FID_SPEAK (1)
#define PPG_FID_DNOTCH (2)
#define PPG_FID_DPEAK (3)

// PPG Beat Classes
#define PPG_BEAT_NONE (0)
#define PPG_BEAT_NSR (1)
#define PPG_BEAT_PNC (2)
#define PPG_BEAT_NOISE (3)

///////////////////////////////////////////////////////////////////////////////
// Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

#define MIN_RR_SEC (0.3)
#define MAX_RR_SEC (2.0)
#define MIN_RR_DELTA (0.3)
#define MET_CAPTURE_SEC (5)
#define MAX_RR_PEAKS (100 * MET_CAPTURE_SEC)
#define MET_WINDOW_LEN (MET_CAPTURE_SEC * SAMPLE_RATE)
#define MET_PAD_LEN (50)
#define MET_VALID_LEN (MET_WINDOW_LEN - 2 * MET_PAD_LEN)
// #define MET_VALID_LEN (250)
#define MET_BUF_LEN (2 * MET_WINDOW_LEN)
#define MET_PPG_MIN_VAL (80000)

///////////////////////////////////////////////////////////////////////////////
// BLE Configuration
///////////////////////////////////////////////////////////////////////////////

#define BLE_NUM_CHARS (3)
#define BLE_SIG_OBJ_BYTE_LEN (9)
#define BLE_SIG_NUM_OBJ (25)
#define BLE_SIG_ECG_OFFSET (0)
#define BLE_SIG_PPG1_OFFSET (2)
#define BLE_SIG_PPG2_OFFSET (4)
#define BLE_ECG_SCALE (1000)
#define BLE_PPG_SCALE (1000)
#define BLE_SIG_ECG_MASK_OFFSET (6)
#define BLE_SIG_PPG1_MASK_OFFSET (7)
#define BLE_SIG_PPG2_MASK_OFFSET (8)

#define BLE_SIG_BUF_LEN (BLE_SIG_NUM_OBJ * BLE_SIG_OBJ_BYTE_LEN)
#define BLE_SIG_SAMPLE_RATE (SAMPLE_RATE / BLE_SIG_NUM_OBJ)
#define PK_DEV_SVC_UUID "f2a8eb8944724ca899033f3238955bf5"
#define PK_DEV_NAME_CHAR_UUID "d1ba1b6144704e4b8ac767dce77c760a"
#define PK_DEV_LOC_CHAR_UUID "523133b72bdb49a0b82d884742d79641"
#define PK_SIG_SVC_UUID "eecb7db88b2d402cb995825538b49328"
#define PK_SIG_CHAR_UUID "3f2c190835ba49e6bdf1d8c73625dead"
#define PK_MET_SVC_UUID "995b00cb47674ed59184f3f93929e31f"
#define PK_MET_CHAR_UUID "529cb6e342d74ad691387da2e614721e"
#define PK_UIO_CHAR_UUID "b9488d48069b47f794f0387f7fbfd1fa"

#define BLE_BUF_LEN (MET_BUF_LEN)

// WSF buffer pools are a bit of black magic. More development needed.
#define WEBBLE_WSF_BUFFER_POOLS 4
#define WEBBLE_WSF_BUFFER_SIZE \
    (WEBBLE_WSF_BUFFER_POOLS * 16 + 16 * 8 + 32 * 4 + 64 * 6 + 280 * 14) / sizeof(uint32_t)

///////////////////////////////////////////////////////////////////////////////
// APP Configuration
///////////////////////////////////////////////////////////////////////////////

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) (MAX(MAX(a, b), c))
#define MAX4(a, b, c, d) (MAX(MAX(a, b), MAX(c, d)))
#define CLIP(a, min, max) (MAX(MIN(a, max), min))

#endif // __PK_CONSTANTS_H
