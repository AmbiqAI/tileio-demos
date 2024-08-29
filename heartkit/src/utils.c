/**
 * @file utils.c
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Utility functions
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "arm_math.h"
#include "ns_ambiqsuite_harness.h"
#include "utils.h"

void
print_array_f32(float32_t *arr, size_t len, char *name) {
    ns_lp_printf("%s = np.array([", name);
    for (size_t i = 0; i < len; i++) {
        ns_lp_printf("%f, ", arr[i]);
    }
    ns_lp_printf("])\n");
}

void
print_array_u32(uint32_t *arr, size_t len, char *name) {
    ns_lp_printf("%s = np.array([", name);
    for (size_t i = 0; i < len; i++) {
        ns_lp_printf("%lu, ", arr[i]);
    }
    ns_lp_printf("])\n");
}


uint16_t computeCRC16(const uint8_t *data, uint32_t lengthInBytes)
{
    uint32_t m_crcStart = 0xEF4A;
    uint32_t crc = m_crcStart;
    uint32_t j;
    uint32_t i;
    uint32_t byte;
    uint32_t temp;
    const uint32_t andValue = 0x8000U;
    const uint32_t xorValue = 0x1021U;

    for (j = 0; j < lengthInBytes; ++j)
    {
        byte = data[j];
        crc ^= byte << 8;
        for (i = 0; i < 8U; ++i)
        {
            temp = crc << 1;
            if (0UL != (crc & andValue))
            {
                temp ^= xorValue;
            }
            crc = temp;
        }
    }

    return (uint16_t)crc;
}


void getDeviceId(uint8_t *deviceId) {

    // uint8_t deviceId[6] = {0};
    am_hal_mcuctrl_device_t device;


    am_hal_mcuctrl_info_get(AM_HAL_MCUCTRL_INFO_DEVICEID, &device);

    // DeviceID formed by ChipID1 (32 bits) and ChipID0 (8-23 bits).
    memcpy(deviceId, &device.ui32ChipID1, sizeof(device.ui32ChipID1));
    // ui32ChipID0 bit 8-31 is test time during chip manufacturing
    deviceId[4] = (device.ui32ChipID0 >> 8) & 0xFF;
    deviceId[5] = (device.ui32ChipID0 >> 16) & 0xFF;
}

void deviceId2SerialId(uint8_t *deviceId, char *serialId, size_t len) {
    size_t i;
    size_t deviceIdLen = 6;

    for (i = 0; i < deviceIdLen; i++) {
        uint32_t lsb = deviceId[i] & 0x0F;
        uint32_t msb = (deviceId[i] >> 4) & 0x0F;
        serialId[i * 2 + 1] = lsb < 10 ? '0' + lsb : 'A' + lsb - 10;
        serialId[i * 2] = msb < 10 ? '0' + msb : 'A' + msb - 10;
    }
    // Fill the rest of the serialId with 0
    for (i = deviceIdLen * 2; i < len; i++) {
        serialId[i] = 0;
    }
}
