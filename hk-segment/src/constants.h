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
#define SENSOR_PPG1_SLOT (0)
#define SENSOR_PPG2_SLOT (1)
#define SENSOR_ECG_SLOT (2)
#define SENSOR_BUF_LEN (2 * 32) // Double FIFO depth
#define SENSOR_NOM_REFRESH_LEN (5)

///////////////////////////////////////////////////////////////////////////////
// Preprocess Configuration
///////////////////////////////////////////////////////////////////////////////

#define NORM_STD_EPS (0.1)

#define ECG_SOS_LEN (3)
#define ECG_SAMPLE_RATE (100)
#define ECG_DS_RATE (SENSOR_RATE / ECG_SAMPLE_RATE)

#define PPG_SOS_LEN (2)
#define PPG_SAMPLE_RATE (50)
#define PPG_DS_RATE (SENSOR_RATE / PPG_SAMPLE_RATE)


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
#define ECG_SEG_PWAVE (1) // 3
#define ECG_SEG_QRS (2) // 1
#define ECG_SEG_TWAVE (3)  // 3

///////////////////////////////////////////////////////////////////////////////
// PPG Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////

#define PPG_SEG_MODEL_SIZE_KB (0)
#define PPG_SEG_THRESHOLD (0.5)
#define PPG_SEG_NUM_CLASS (2)
#define PPG_SEG_WINDOW_LEN (125)
#define PPG_SEG_PAD_LEN (12)
#define PPG_SEG_VALID_LEN (PPG_SEG_WINDOW_LEN - 2 * PPG_SEG_PAD_LEN)
#define PPG_SEG_BUF_LEN (2 * PPG_SEG_WINDOW_LEN)
#define PPG_SEG_FFT_WINDOW_LEN (256)
#define PPG_SEG_FFT_MIN_IDX (6) // ceil(FREQ/(FS/FFT_LEN)) where FREQ=0.5, FS=50, FFT_LEN=256 -> ceil(0.5/(50/256))
#define PPG_SEG_FFT_MAX_IDX (11) // ceil(FREQ/(FS/FFT_LEN)) where FREQ=3.0, FS=50, FFT_LEN=256 -> ceil(2.0/(50/256))


// PPG Segmentation Classes
#define PPG_SEG_NONE (0)
#define PPG_SEG_SYS (1)
#define PPG_SEG_DIA (2)


///////////////////////////////////////////////////////////////////////////////
// PK BLE Mask Format
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
#define ECG_QOS_BAD_AVG_THRESH (0.65)

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
// TIO BLE SLOT0 (ECG) Mask Format
///////////////////////////////////////////////////////////////////////////////

// PPG Mask
// [5-0] : 6-bit segmentation (0:none, 1:systolic, 2:diastolic)
// [7-6] : 2-bit QoS (0:bad, 1:poor, 2:fair, 3:good)
// [9-8] : 2-bit fiducial (0:none, 1:s-peak, 2:d-peak, 3:d-notch)
// [15-10] : 6-bit beat type (0:none, 1:nsr, 2:pac/pvc, 3:noise)

#define PPG_MASK_SEG_OFFSET (0)
#define PPG_MASK_SEG_MASK (0x3F)
#define PPG_MASK_QOS_OFFSET (6)
#define PPG_MASK_QOS_MASK (0x3)

#define PPG_MASK_FID_PEAK_OFFSET (8)
#define PPG_MASK_FID_PEAK_MASK (0x3)
#define PPG_MASK_FID_BEAT_OFFSET (10)
#define PPG_MASK_FID_BEAT_MASK (0x3F)

// PPG Fiducial Peak Classes
#define PPG_FID_PEAK_NONE (0)
#define PPG_FID_PEAK_SPEAK (1)
#define PPG_FID_PEAK_DNOTCH (2)
#define PPG_FID_PEAK_DPEAK (3)

// PPG Fiducial Beat Classes
#define PPG_FID_BEAT_NONE (0)
#define PPG_FID_BEAT_NSR (1)
#define PPG_FID_BEAT_PNC (2)
#define PPG_FID_BEAT_NOISE (3)


///////////////////////////////////////////////////////////////////////////////
// Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

#define MIN_RR_SEC (0.3)
#define MAX_RR_SEC (2.0)
#define MIN_RR_DELTA (0.3)
#define MET_CAPTURE_SEC (10)
#define MAX_RR_PEAKS (100 * MET_CAPTURE_SEC)

#define ECG_MET_WINDOW_LEN (MET_CAPTURE_SEC * ECG_SAMPLE_RATE)
#define ECG_MET_PAD_LEN (50)
#define ECG_MET_VALID_LEN (ECG_MET_WINDOW_LEN - 2 * ECG_MET_PAD_LEN)
#define ECG_MET_BUF_LEN (2 * ECG_MET_WINDOW_LEN)

#define PPG_MET_WINDOW_LEN (MET_CAPTURE_SEC * PPG_SAMPLE_RATE)
#define PPG_MET_PAD_LEN (25)
#define PPG_MET_VALID_LEN (PPG_MET_WINDOW_LEN - 2 * PPG_MET_PAD_LEN)
#define PPG_MET_BUF_LEN (2 * PPG_MET_WINDOW_LEN)
#define PPG_MET_FFT_WINDOW_LEN (512)
#define PPG_FFT_MIN_IDX (6) // ceil(FREQ/(FS/FFT_LEN)) where FREQ=0.5, FS=50, FFT_LEN=256 -> ceil(0.5/(50/256)) = 6
#define PPG_FFT_MAX_IDX (31) // ceil(FREQ/(FS/FFT_LEN)) where FREQ=3.0, FS=50, FFT_LEN=256 -> ceil(3.0/(50/256)) = 31
#define PPG_MET_MIN_VAL (80000)

#define ECG_TX_BUF_LEN ECG_SEG_BUF_LEN
#define PPG_TX_BUF_LEN PPG_SEG_BUF_LEN

///////////////////////////////////////////////////////////////////////////////
// BLE Configuration
///////////////////////////////////////////////////////////////////////////////

#define TIO_BLE_SLOT_SIG_BUF_LEN (242)
#define TIO_BLE_SLOT_MET_BUF_LEN (242)
#define TIO_BLE_UIO_BUF_LEN (8)

#define TIO_SLOT_SVC_UUID "eecb7db88b2d402cb995825538b49328"
#define TIO_SLOT0_SIG_CHAR_UUID "5bca2754ac7e4a27a1270f328791057a"
#define TIO_SLOT1_SIG_CHAR_UUID "45415793a0e94740bca4ce90bd61839f"
#define TIO_SLOT2_SIG_CHAR_UUID "dd19792c63f1420f920cc58bada8efb9"
#define TIO_SLOT3_SIG_CHAR_UUID "f1f691580bd64cab90a8528baf74cc74"

#define TIO_SLOT0_MET_CHAR_UUID "44a3a7b8d7c849329a10d99dd63775ae"
#define TIO_SLOT1_MET_CHAR_UUID "e64fa683462848c5bede824aaa7c3f5b"
#define TIO_SLOT2_MET_CHAR_UUID "b9d28f5365f04392afbcc602f9dc3c8b"
#define TIO_SLOT3_MET_CHAR_UUID "917c9eb43dbc4cb3bba2ec4e288083f4"

#define TIO_UIO_CHAR_UUID "b9488d48069b47f794f0387f7fbfd1fa"

#define BLE_NUM_CHARS (2+2+1)

#define BLE_SLOT0_NUM_CH (1)
#define BLE_SLOT0_SIG_NUM_VALS (50)
#define BLE_SLOT0_FS (ECG_SAMPLE_RATE / BLE_SLOT0_SIG_NUM_VALS)
#define BLE_SLOT0_SCALE (1000)

#define BLE_SLOT1_NUM_CH (2)
#define BLE_SLOT1_SIG_NUM_VALS (25)
#define BLE_SLOT1_FS (PPG_SAMPLE_RATE / BLE_SLOT1_SIG_NUM_VALS)
#define BLE_SLOT1_SCALE (1000)


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
