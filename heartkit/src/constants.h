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
#ifndef __APP_CONSTANTS_H
#define __APP_CONSTANTS_H

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

#define NUM_INPUT_PTS (5)
#define PTS_ECG_DATA_LEN (2000)
#define SENSOR_RATE (200)
#define SENSOR_ECG_SLOT (0)
#define SENSOR_BUF_LEN (4 * 64) // Double FIFO depth
#define SENSOR_NOM_REFRESH_LEN (20)
#define MAX86150_PART_ID_VAL (0x1E)

///////////////////////////////////////////////////////////////////////////////
// Preprocess Configuration
///////////////////////////////////////////////////////////////////////////////

#define NORM_STD_EPS (0.001)

#define ECG_SOS_LEN (9)
#define ECG_SAMPLE_RATE (100)
#define ECG_DS_RATE (SENSOR_RATE / ECG_SAMPLE_RATE)

///////////////////////////////////////////////////////////////////////////////
// ECG Denoise Configuration
///////////////////////////////////////////////////////////////////////////////

#define ECG_DEN_MODEL_SIZE_KB (80)
#define ECG_DEN_THRESHOLD (0.5) // 0.75
#define ECG_DEN_WINDOW_LEN (250)
#define ECG_DEN_PAD_LEN (25)
#define ECG_DEN_VALID_LEN (ECG_DEN_WINDOW_LEN - 2 * ECG_DEN_PAD_LEN)
#define ECG_DEN_BUF_LEN (2 * ECG_DEN_WINDOW_LEN)

///////////////////////////////////////////////////////////////////////////////
// ECG Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////

#define ECG_SEG_MODEL_SIZE_KB (60)
#define ECG_SEG_THRESHOLD (0.5) // 0.75
#define ECG_SEG_NUM_CLASS (4) // 2
#define ECG_SEG_WINDOW_LEN (250)
#define ECG_SEG_PAD_LEN (25)
#define ECG_SEG_VALID_LEN (ECG_SEG_WINDOW_LEN - 2 * ECG_SEG_PAD_LEN)
#define ECG_SEG_BUF_LEN (2 * ECG_SEG_WINDOW_LEN)

// ECG Segmentation Classes
#define ECG_SEG_NONE (0)
#define ECG_SEG_PWAVE (1)
#define ECG_SEG_QRS (2)
#define ECG_SEG_TWAVE (3)

///////////////////////////////////////////////////////////////////////////////
// ECG Arrhythmia Configuration
///////////////////////////////////////////////////////////////////////////////

#define ECG_ARR_MODEL_SIZE_KB (80)
#define ECG_ARR_THRESHOLD (0.5)
#define ECG_ARR_WINDOW_LEN (500)
#define ECG_ARR_PAD_LEN (0)
#define ECG_ARR_VALID_LEN (ECG_ARR_WINDOW_LEN - 2 * ECG_ARR_PAD_LEN)
#define ECG_ARR_BUF_LEN (2 * ECG_ARR_WINDOW_LEN)

#define ECG_ARR_INCONCLUSIVE (0)
#define ECG_ARR_SR (1)
#define ECG_ARR_SB (2)
#define ECG_ARR_AFIB (3)
#define ECG_ARR_GSVT (4)

///////////////////////////////////////////////////////////////////////////////
// TIO Mask Format
///////////////////////////////////////////////////////////////////////////////

// [5-0] : 6-bit segmentation
// [7-6] : 2-bit QoS (0:bad, 1:poor, 2:fair, 3:good)
// [15-8] : 8-bit Fiducial

#define SIG_MASK_SEG_OFFSET (0)
#define SIG_MASK_SEG_MASK (0x3F)
#define SIG_MASK_QOS_OFFSET (6)
#define SIG_MASK_QOS_MASK (0x3)
#define SIG_MASK_FID_OFFSET (8)
#define SIG_MASK_FID_MASK (0xFF)

// SIG QoS Classes
#define SIG_QOS_BAD (0)
#define SIG_QOS_POOR (1)
#define SIG_QOS_FAIR (2)
#define SIG_QOS_GOOD (3)

///////////////////////////////////////////////////////////////////////////////
// TIO BLE SLOT0 (ECG) Mask Format
///////////////////////////////////////////////////////////////////////////////

// ECG Mask
// [5-0] : 6-bit segmentation (0:none, 1:p-wave, 2:qrs, 3:t-wave)
// [7-6] : 2-bit QoS (0:bad, 1:poor, 2:fair, 3:good)
// [9-8] : 2-bit fiducial (0:none, 1:p-peak, 2:qrs, 3:t-peak)
// [15-10] : 6-bit beat type (0:none, 1:nsr, 2:pac/pvc, 3:noise)

#define ECG_MASK_SEG_OFFSET (0)
#define ECG_MASK_SEG_MASK (0x3F)
#define ECG_MASK_QOS_OFFSET (6)
#define ECG_MASK_QOS_MASK (0x3)

#define ECG_QOS_GOOD_THRESH (0.65)
#define ECG_QOS_FAIR_THRESH (0.60)
#define ECG_QOS_POOR_THRESH (0.55)
#define ECG_QOS_BAD_AVG_THRESH (0.70)

#define ECG_MASK_FID_PEAK_OFFSET (8)
#define ECG_MASK_FID_PEAK_MASK (0x3)
#define ECG_MASK_FID_BEAT_OFFSET (10)
#define ECG_MASK_FID_BEAT_MASK (0x3F)

// ECG Fiducial Peak Classes
#define ECG_FID_PEAK_NONE (0)
#define ECG_FID_PEAK_PPEAK (1)
#define ECG_FID_PEAK_QRS (2)
#define ECG_FID_PEAK_TPEAK (3)

// ECG Fiducial Beat Classes
#define ECG_FID_BEAT_NONE (0)
#define ECG_FID_BEAT_NSR (1)
#define ECG_FID_BEAT_PNC (2)
#define ECG_FID_BEAT_NOISE (3)


///////////////////////////////////////////////////////////////////////////////
// Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

#define MIN_RR_SEC (0.3)
#define MAX_RR_SEC (2.0)
#define MIN_RR_DELTA (0.3)
#define MET_CAPTURE_SEC (10)
#define MAX_RR_PEAKS (100 * MET_CAPTURE_SEC)

#define ECG_MET_WINDOW_LEN (MET_CAPTURE_SEC * ECG_SAMPLE_RATE)
#define ECG_MET_PAD_LEN (8 * ECG_SAMPLE_RATE)
#define ECG_MET_VALID_LEN (ECG_MET_WINDOW_LEN - ECG_MET_PAD_LEN)
#define ECG_MET_BUF_LEN (2 * ECG_MET_WINDOW_LEN)

#define ECG_TX_BUF_LEN ECG_SEG_BUF_LEN

///////////////////////////////////////////////////////////////////////////////
// Tileio Configuration
///////////////////////////////////////////////////////////////////////////////

#define TIO_BLE_ENABLED true // Enable Tileio BLE
#define TIO_USB_ENABLED true // Enable Tileio USB

#define TIO_SLOT0_NUM_CH (2)
#define TIO_SLOT0_SIG_NUM_VALS (10)
#define TIO_SLOT0_FS (ECG_SAMPLE_RATE / TIO_SLOT0_SIG_NUM_VALS)
#define TIO_SLOT0_SCALE (1000)

#define TIO_SLOT1_NUM_CH (1)
#define TIO_SLOT1_SIG_NUM_VALS (50)
#define TIO_SLOT1_FS (ECG_SAMPLE_RATE / TIO_SLOT1_SIG_NUM_VALS)
#define TIO_SLOT1_SCALE (1000)

#define TIO_SLOT2_NUM_CH (1)
#define TIO_SLOT2_SIG_NUM_VALS (50)
#define TIO_SLOT2_FS (ECG_SAMPLE_RATE / TIO_SLOT2_SIG_NUM_VALS)
#define TIO_SLOT2_SCALE (1000)

#define TIO_SLOT3_NUM_CH (1)
#define TIO_SLOT3_SIG_NUM_VALS (50)
#define TIO_SLOT3_FS (ECG_SAMPLE_RATE / TIO_SLOT3_SIG_NUM_VALS)
#define TIO_SLOT3_SCALE (1000)


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

#endif // __APP_CONSTANTS_H
