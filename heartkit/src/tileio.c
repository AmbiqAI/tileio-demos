#include "arm_math.h"
#include "utils.h"
#include "ns_ambiqsuite_harness.h"
#include "FreeRTOS.h"
#include "task.h"
#include "arm_math.h"

#include "tileio.h"
#include "ringbuffer.h"

#if(TIO_BLE_ENABLED)
#include "ns_ble.h"
#endif

#if(TIO_USB_ENABLED)
#include "ns_usb.h"
#include "vendor_device.h"
#include "usb_descriptors.h"
#endif

#define TIO_USB_VENDOR_ID 0xCAFE
#define TIO_USB_PRODUCT_ID 0x0001
#define TIO_USB_PACKET_LEN 256
#define TIO_USB_START_IDX 0
#define TIO_USB_START_VAL 0x55
#define TIO_USB_SLOT_IDX 1
#define TIO_USB_TYPE_IDX 2
#define TIO_USB_DLEN_IDX 3
#define TIO_USB_DLEN_LEN 2
#define TIO_USB_DATA_IDX 5
#define TIO_USB_DATA_LEN 248
#define TIO_USB_CRC_IDX 253
#define TIO_USB_CRC_LEN 2
#define TIO_USB_STOP_IDX 255
#define TIO_USB_STOP_VAL 0xAA
#define TIO_USB_UIO_BUF_LEN (8)

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

#define TIO_USB_RX_BUFSIZE (4096)
#define TIO_USB_TX_BUFSIZE (4096)

////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////

static uint8_t tioDeviceId[6];
static char tioSerialId[13];
static tio_context_t *gTioCtx = NULL;

////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////

static uint16_t
tio_compute_crc16(const uint8_t *data, uint32_t length)
{
    uint32_t m_crcStart = 0xEF4A;
    uint32_t crc = m_crcStart;
    uint32_t j;
    uint32_t i;
    uint32_t byte;
    uint32_t temp;
    const uint32_t andValue = 0x8000U;
    const uint32_t xorValue = 0x1021U;

    for (j = 0; j < length; ++j)
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

static void
tio_get_device_id(uint8_t *deviceId)
{

    am_hal_mcuctrl_device_t device;
    am_hal_mcuctrl_info_get(AM_HAL_MCUCTRL_INFO_DEVICEID, &device);
    // DeviceID formed by ChipID1 (32 bits) and ChipID0 (8-23 bits).
    memcpy(deviceId, &device.ui32ChipID1, sizeof(device.ui32ChipID1));
    // ui32ChipID0 bit 8-31 is test time during chip manufacturing
    deviceId[4] = (device.ui32ChipID0 >> 8) & 0xFF;
    deviceId[5] = (device.ui32ChipID0 >> 16) & 0xFF;
}

static void
tio_device_id_to_serial_id(uint8_t *deviceId, char *serialId, size_t len)
{
    size_t i;
    size_t deviceIdLen = 6;

    for (i = 0; i < deviceIdLen; i++)
    {
        uint32_t lsb = deviceId[i] & 0x0F;
        uint32_t msb = (deviceId[i] >> 4) & 0x0F;
        serialId[i * 2 + 1] = lsb < 10 ? '0' + lsb : 'A' + lsb - 10;
        serialId[i * 2] = msb < 10 ? '0' + msb : 'A' + msb - 10;
    }
    // Fill the rest of the serialId with 0
    for (i = deviceIdLen * 2; i < len; i++)
    {
        serialId[i] = 0;
    }
}

////////////////////////////////////////////////////////////////
// USB
////////////////////////////////////////////////////////////////

#if(TIO_USB_ENABLED)

static uint8_t tioRxBuffer[TIO_USB_RX_BUFSIZE] = {0};
static uint8_t tioTxBuffer[TIO_USB_TX_BUFSIZE] = {0};

static uint8_t tioRxRingBufferData[TIO_USB_RX_BUFSIZE];
static rb_config_t tioRxRingBuffer = {
    .buffer = (void *)tioRxRingBufferData,
    .dlen = sizeof(uint8_t),
    .size = TIO_USB_RX_BUFSIZE,
    .head = 0,
    .tail = 0,
};

static usb_handle_t tioUsbHandle = NULL;
static ns_usb_config_t tioWebUsbConfig = {
    .api = &ns_usb_V1_0_0,
    .deviceType = NS_USB_VENDOR_DEVICE,
    .rx_buffer = tioRxBuffer,
    .rx_bufferLength = TIO_USB_RX_BUFSIZE,
    .tx_buffer = tioTxBuffer,
    .tx_bufferLength = TIO_USB_TX_BUFSIZE,
    .rx_cb = NULL,
    .tx_cb = NULL,
    .service_cb = NULL};

static uint32_t
tio_usb_validate_packet(const uint8_t *buffer, uint32_t length)
{
    // Decode the packet
    if (length != TIO_USB_PACKET_LEN)
    {
        ns_lp_printf("Invalid packet length\n");
        return 1;
    }
    uint8_t start = buffer[TIO_USB_START_IDX];
    uint8_t slot = buffer[TIO_USB_SLOT_IDX];
    uint8_t slotType = buffer[TIO_USB_TYPE_IDX];

    uint16_t dlen = (buffer[TIO_USB_DLEN_IDX + 1] << 8) | buffer[TIO_USB_DLEN_IDX];
    uint16_t crc = (buffer[TIO_USB_CRC_IDX + 1] << 8) | buffer[TIO_USB_CRC_IDX];

    uint16_t stop = buffer[TIO_USB_STOP_IDX];
    uint16_t computedCrc = tio_compute_crc16(buffer + TIO_USB_DLEN_IDX, dlen + TIO_USB_DLEN_LEN);
            // Received packet: 85 0 2 8 35902 170
    ns_lp_printf("Received packet: %d %d %d %d %d %d\n", start, slot, slotType, dlen, crc, stop);
    if (start != TIO_USB_START_VAL || stop != TIO_USB_STOP_VAL)
    {
        ns_lp_printf("Invalid start/stop byte %lu %lu\n", start, stop);
        return 1;
    }
    if (crc != computedCrc)
    {
        ns_lp_printf("Invalid CRC %x %x\n", crc, computedCrc);
        return 1;
    }
    if (slotType <= 1 && dlen > TIO_USB_DATA_LEN)
    {
        ns_lp_printf("Invalid data length for slot type %lu\n", slotType);
        return 1;
    }
    if (slotType == 2 && dlen != TIO_USB_UIO_BUF_LEN)
    {
        ns_lp_printf("Invalid data length for UIO\n");
        return 1;
    }
    return 0;
}

static void
tio_usb_receive_handler(const uint8_t *buffer, uint32_t length, void *args)
{
    tio_context_t *ctx = (tio_context_t *)args;
    ringbuffer_push(&tioRxRingBuffer, (void *)buffer, length);
    uint8_t slotFrame[TIO_USB_PACKET_LEN];
    uint32_t skip = 0;
    while (ringbuffer_len(&tioRxRingBuffer) >= TIO_USB_PACKET_LEN)
    {
        ringbuffer_peek(&tioRxRingBuffer, slotFrame, TIO_USB_PACKET_LEN);
        skip = tio_usb_validate_packet(slotFrame, TIO_USB_PACKET_LEN);
        if (skip)
        {
            ringbuffer_seek(&tioRxRingBuffer, 1);
            continue;
        }
        // If valid, parse the slot frame and send it to the appropriate slot
        uint8_t slot = slotFrame[TIO_USB_SLOT_IDX];
        uint8_t slotType = slotFrame[TIO_USB_TYPE_IDX];
        uint16_t length = (slotFrame[TIO_USB_DLEN_IDX + 1] << 8) | slotFrame[TIO_USB_DLEN_IDX];
        // Slot signal or metrics
        if (slotType <= 1 && ctx->slot_update_cb != NULL)
        {
            ctx->slot_update_cb(slot, slotType, slotFrame + TIO_USB_DATA_IDX, length);
            // Slot UIO
        }
        else if (slotType == 2 && ctx->uio_update_cb != NULL)
        {
            ctx->uio_update_cb(slotFrame + TIO_USB_DATA_IDX, length);
        }
        ringbuffer_seek(&tioRxRingBuffer, TIO_USB_PACKET_LEN);
    }
    ns_lp_printf("Received %d bytes\n", length);
}

/**
 * @brief Send slot data over USB
 * A USB slot frame is 256 bytes long w/ fields:
 *   START: 1 byte      [0x55]
 *    SLOT: 1 byte      [0 - ch0, 1 - ch1, 2 - ch2, 3 - ch3]
 *   STYPE: 1 byte      [0 - signal, 1 - metric, 2 - uio]
 *  LENGTH: 2 bytes     [0 - 248]
 *    DATA: 248 bytes   [...]
 *     CRC: 2 bytes     [CRC16]
 *    STOP: 1 byte      [0xAA]
 *
 * @param slot Slot number (0-3)
 * @param slot_type Slot type (0 - signal, 1 - metric)
 * @param data Slot data (max 240 bytes)
 * @param length Data length
 * @return uint32_t
 */
static uint32_t
tio_usb_send_slot_data(uint8_t slot, uint8_t slot_type, const uint8_t *data, uint32_t length)
{
    if (length > TIO_USB_DATA_LEN)
    {
        ns_lp_printf("Data length exceeds limit\n");
        return 1;
    }
    if (!tud_vendor_mounted()) {
        ns_lp_printf("USB not mounted\n");
        return 1;
    }
    // while (tud_vendor_write_available() < TIO_USB_PACKET_LEN) {
    //     ns_delay_us(200);
    //     ns_lp_printf(".");
    // }
    // Send the signal data for given slot
    uint8_t buffer[TIO_USB_PACKET_LEN] = {0};
    buffer[TIO_USB_START_IDX] = TIO_USB_START_VAL;
    buffer[TIO_USB_SLOT_IDX] = slot;
    buffer[TIO_USB_TYPE_IDX] = slot_type;
    buffer[TIO_USB_DLEN_IDX] = length & 0xFF;
    buffer[TIO_USB_DLEN_IDX + 1] = (length >> 8) & 0xFF;
    memcpy(buffer + TIO_USB_DATA_IDX, data, length);
    // CRC on data length and data
    uint16_t crc = tio_compute_crc16(buffer + TIO_USB_DLEN_IDX, length + TIO_USB_DLEN_LEN);
    buffer[TIO_USB_CRC_IDX] = crc & 0xFF;
    buffer[TIO_USB_CRC_IDX + 1] = (crc >> 8) & 0xFF;
    buffer[TIO_USB_STOP_IDX] = TIO_USB_STOP_VAL;
    webusb_send_data(buffer, 256);
    return 0;
}

static uint32_t
tio_usb_send_uio_data(const uint8_t *data, uint32_t length)
{
    return tio_usb_send_slot_data(0, 2, data, length);
}

static uint32_t
tio_usb_init(tio_context_t *ctx)
{
    webusb_register_raw_cb(tio_usb_receive_handler, ctx);

    tio_get_device_id(tioDeviceId);
    tio_device_id_to_serial_id(tioDeviceId, tioSerialId, 13);

    usb_string_desc_arr[USB_DESCRIPTOR_MANUFACTURER] = "Ambiq";
    usb_string_desc_arr[USB_DESCRIPTOR_PRODUCT] = "Tileio";
    usb_string_desc_arr[USB_DESCRIPTOR_SERIAL] = tioSerialId;

    ringbuffer_flush(&tioRxRingBuffer);

    // Initialize USB
    if (ns_usb_init(&tioWebUsbConfig, &tioUsbHandle))
    {
        return 1;
    }
    return 0;
}

#endif

#if(TIO_BLE_ENABLED)

////////////////////////////////////////////////////////////////
// BLE
////////////////////////////////////////////////////////////////

typedef struct
{
    ns_ble_pool_config_t *pool;
    ns_ble_service_t *service;

    ns_ble_characteristic_t *slot0SigChar;
    ns_ble_characteristic_t *slot1SigChar;
    ns_ble_characteristic_t *slot2SigChar;
    ns_ble_characteristic_t *slot3SigChar;

    ns_ble_characteristic_t *slot0MetChar;
    ns_ble_characteristic_t *slot1MetChar;
    ns_ble_characteristic_t *slot2MetChar;
    ns_ble_characteristic_t *slot3MetChar;

    ns_ble_characteristic_t *uioChar;

    void *slot0SigBuffer;
    void *slot1SigBuffer;
    void *slot2SigBuffer;
    void *slot3SigBuffer;

    void *slot0MetBuffer;
    void *slot1MetBuffer;
    void *slot2MetBuffer;
    void *slot3MetBuffer;

    uint8_t *uioBuffer;

} tio_ble_context_t;

// WSF buffer pools are a bit of black magic. More development needed.
#define WEBBLE_WSF_BUFFER_POOLS 4
#define WEBBLE_WSF_BUFFER_SIZE \
    (WEBBLE_WSF_BUFFER_POOLS * 16 + 16 * 8 + 32 * 4 + 64 * 6 + 280 * 14) / sizeof(uint32_t)

static uint32_t webbleWSFBufferPool[WEBBLE_WSF_BUFFER_SIZE];
static wsfBufPoolDesc_t webbleBufferDescriptors[WEBBLE_WSF_BUFFER_POOLS] = {
    {16, 8}, // 16 bytes, 8 buffers
    {32, 4},
    {64, 6},
    {512, 14}};

static ns_ble_pool_config_t bleWsfBuffers = {
    .pool = webbleWSFBufferPool,
    .poolSize = sizeof(webbleWSFBufferPool),
    .desc = webbleBufferDescriptors,
    .descNum = WEBBLE_WSF_BUFFER_POOLS};

static uint8_t bleSlot0SigBuffer[TIO_BLE_SLOT_SIG_BUF_LEN] = {0};
static uint8_t bleSlot1SigBuffer[TIO_BLE_SLOT_SIG_BUF_LEN] = {0};
static uint8_t bleSlot2SigBuffer[TIO_BLE_SLOT_SIG_BUF_LEN] = {0};
static uint8_t bleSlot3SigBuffer[TIO_BLE_SLOT_SIG_BUF_LEN] = {0};
static uint8_t *bleSlotBuffers[4] = {
    bleSlot0SigBuffer,
    bleSlot1SigBuffer,
    bleSlot2SigBuffer,
    bleSlot3SigBuffer};
static uint8_t bleSlot0MetBuffer[TIO_BLE_SLOT_MET_BUF_LEN] = {0};
static uint8_t bleSlot1MetBuffer[TIO_BLE_SLOT_MET_BUF_LEN] = {0};
static uint8_t bleSlot2MetBuffer[TIO_BLE_SLOT_MET_BUF_LEN] = {0};
static uint8_t bleSlot3MetBuffer[TIO_BLE_SLOT_MET_BUF_LEN] = {0};
static uint8_t *bleSlotMetBuffers[4] = {
    bleSlot0MetBuffer,
    bleSlot1MetBuffer,
    bleSlot2MetBuffer,
    bleSlot3MetBuffer};
static uint8_t bleUioBuffer[TIO_BLE_UIO_BUF_LEN] = {0};

static ns_ble_service_t bleService;
static ns_ble_characteristic_t bleSlot0SigChar;
static ns_ble_characteristic_t bleSlot1SigChar;
static ns_ble_characteristic_t bleSlot2SigChar;
static ns_ble_characteristic_t bleSlot3SigChar;
static ns_ble_characteristic_t bleSlot0MetChar;
static ns_ble_characteristic_t bleSlot1MetChar;
static ns_ble_characteristic_t bleSlot2MetChar;
static ns_ble_characteristic_t bleSlot3MetChar;
static ns_ble_characteristic_t *bleSlotSigChars[4] = {
    &bleSlot0SigChar,
    &bleSlot1SigChar,
    &bleSlot2SigChar,
    &bleSlot3SigChar};
static ns_ble_characteristic_t *bleSlotMetChars[4] = {
    &bleSlot0MetChar,
    &bleSlot1MetChar,
    &bleSlot2MetChar,
    &bleSlot3MetChar};

static ns_ble_characteristic_t bleUioChar;

static tio_ble_context_t tioBleCtx = {
    .pool = &bleWsfBuffers,
    .service = &bleService,
    .slot0SigChar = &bleSlot0SigChar,
    .slot1SigChar = &bleSlot1SigChar,
    .slot2SigChar = &bleSlot2SigChar,
    .slot3SigChar = &bleSlot3SigChar,
    .slot0MetChar = &bleSlot0MetChar,
    .slot1MetChar = &bleSlot1MetChar,
    .slot2MetChar = &bleSlot2MetChar,
    .slot3MetChar = &bleSlot3MetChar,
    .uioChar = &bleUioChar,
    .slot0SigBuffer = bleSlot0SigBuffer,
    .slot1SigBuffer = bleSlot1SigBuffer,
    .slot2SigBuffer = bleSlot2SigBuffer,
    .slot3SigBuffer = bleSlot3SigBuffer,
    .slot0MetBuffer = bleSlot0MetBuffer,
    .slot1MetBuffer = bleSlot1MetBuffer,
    .slot2MetBuffer = bleSlot2MetBuffer,
    .slot3MetBuffer = bleSlot3MetBuffer,
    .uioBuffer = bleUioBuffer};

void webbleHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    ns_lp_printf("webbleHandler\n");
}
void webbleHandlerInit(wsfHandlerId_t handlerId)
{
    ns_lp_printf("webbleHandlerInit\n");
}

int tio_ble_notify_sig_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c)
{
    return NS_STATUS_SUCCESS;
}

int tio_ble_notify_met_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c)
{
    return NS_STATUS_SUCCESS;
}

int tio_ble_notify_uio_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c)
{
    return NS_STATUS_SUCCESS;
}

int tio_ble_uio_read_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c, void *dest)
{
    memcpy(dest, c->applicationValue, c->valueLen);
    return NS_STATUS_SUCCESS;
}

int tio_ble_uio_write_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c, void *src)
{
    memcpy(c->applicationValue, src, c->valueLen);
    if (c == tioBleCtx.uioChar)
    {
        if (gTioCtx->uio_update_cb != NULL)
        {
            gTioCtx->uio_update_cb(src, c->valueLen);
        }
    }
    return NS_STATUS_SUCCESS;
}

static void
tio_ble_send_slot_data(uint8_t slot, uint8_t slot_type, const uint8_t *data, uint32_t length)
{
    uint8_t *buffer = NULL;
    ns_ble_characteristic_t *bleChar = NULL;
    if (length > 240)
    {
        ns_lp_printf("Data length exceeds 240 bytes\n");
        return;
    }
    if (slot >= 4)
    {
        ns_lp_printf("Invalid slot number\n");
        return;
    }
    if (slot_type >= 2)
    {
        ns_lp_printf("Invalid slot type\n");
        return;
    }
    if (slot_type == 0)
    {
        buffer = bleSlotBuffers[slot];
        bleChar = bleSlotSigChars[slot];
    }
    else
    {
        buffer = bleSlotMetBuffers[slot];
        bleChar = bleSlotMetChars[slot];
    }
    buffer[0] = length & 0xFF;
    buffer[1] = (length >> 8) & 0xFF;
    memset(buffer + 2, 0, 240);
    memcpy(buffer + 2, data, length);
    ns_ble_send_value(bleChar, NULL);
}

static void
tio_ble_send_uio(const uint8_t *data, uint32_t length)
{
    if (length != 8)
    {
        ns_lp_printf("Invalid UIO data length\n");
        return;
    }
    memcpy(tioBleCtx.uioBuffer, data, length);
    ns_ble_send_value(tioBleCtx.uioChar, NULL);
}

static int
tio_ble_service_init(void)
{
    // Initialize BLE service
    char bleName[] = "Tileio"; // TODO: Get custom name
    NS_TRY(ns_ble_char2uuid(TIO_SLOT_SVC_UUID, &(tioBleCtx.service->uuid128)), "Failed to convert UUID\n");
    memcpy(tioBleCtx.service->name, bleName, sizeof(bleName));
    tioBleCtx.service->nameLen = strlen(bleName);
    tioBleCtx.service->baseHandle = 0x0800;
    tioBleCtx.service->poolConfig = tioBleCtx.pool;
    tioBleCtx.service->numAttributes = 0;

    // Create all slots
    ns_ble_create_characteristic(
        tioBleCtx.slot0SigChar, TIO_SLOT0_SIG_CHAR_UUID, tioBleCtx.slot0SigBuffer, TIO_BLE_SLOT_SIG_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, &tio_ble_notify_sig_handler,
        1000, true, &(tioBleCtx.service->numAttributes));
    ns_ble_create_characteristic(
        tioBleCtx.slot0MetChar, TIO_SLOT0_MET_CHAR_UUID, tioBleCtx.slot0MetBuffer, TIO_BLE_SLOT_MET_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, &tio_ble_notify_met_handler,
        1000, true, &(tioBleCtx.service->numAttributes));

    ns_ble_create_characteristic(
        tioBleCtx.slot1SigChar, TIO_SLOT1_SIG_CHAR_UUID, tioBleCtx.slot1SigBuffer, TIO_BLE_SLOT_SIG_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, &tio_ble_notify_sig_handler,
        1000, true, &(tioBleCtx.service->numAttributes));
    ns_ble_create_characteristic(
        tioBleCtx.slot1MetChar, TIO_SLOT1_MET_CHAR_UUID, tioBleCtx.slot1MetBuffer, TIO_BLE_SLOT_MET_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, &tio_ble_notify_met_handler,
        1000, true, &(tioBleCtx.service->numAttributes));

    ns_ble_create_characteristic(
        tioBleCtx.slot2SigChar, TIO_SLOT2_SIG_CHAR_UUID, tioBleCtx.slot2SigBuffer, TIO_BLE_SLOT_SIG_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, &tio_ble_notify_sig_handler,
        1000, true, &(tioBleCtx.service->numAttributes));
    ns_ble_create_characteristic(
        tioBleCtx.slot2MetChar, TIO_SLOT2_MET_CHAR_UUID, tioBleCtx.slot2MetBuffer, TIO_BLE_SLOT_MET_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, &tio_ble_notify_met_handler,
        1000, true, &(tioBleCtx.service->numAttributes));

    ns_ble_create_characteristic(
        tioBleCtx.slot3SigChar, TIO_SLOT3_SIG_CHAR_UUID, tioBleCtx.slot3SigBuffer, TIO_BLE_SLOT_SIG_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, &tio_ble_notify_sig_handler,
        1000, true, &(tioBleCtx.service->numAttributes));
    ns_ble_create_characteristic(
        tioBleCtx.slot3MetChar, TIO_SLOT3_MET_CHAR_UUID, tioBleCtx.slot3MetBuffer, TIO_BLE_SLOT_MET_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, tio_ble_notify_met_handler,
        1000, true, &(tioBleCtx.service->numAttributes));

    // UIO
    ns_ble_create_characteristic(
        tioBleCtx.uioChar, TIO_UIO_CHAR_UUID, tioBleCtx.uioBuffer, TIO_BLE_UIO_BUF_LEN,
        NS_BLE_READ | NS_BLE_WRITE | NS_BLE_NOTIFY,
        &tio_ble_uio_read_handler, &tio_ble_uio_write_handler, &tio_ble_notify_uio_handler,
        1000, true, &(tioBleCtx.service->numAttributes));

    tioBleCtx.service->numCharacteristics = 9;
    ns_ble_create_service(tioBleCtx.service);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.slot0SigChar);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.slot0MetChar);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.slot1SigChar);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.slot1MetChar);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.slot2SigChar);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.slot2MetChar);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.slot3SigChar);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.slot3MetChar);
    ns_ble_add_characteristic(tioBleCtx.service, tioBleCtx.uioChar);
    // Initialize BLE, create structs, start service
    ns_ble_start_service(tioBleCtx.service);
    return NS_STATUS_SUCCESS;
}

#endif

void
TioTask(void *pvParameters)
{
#if(TIO_BLE_ENABLED)
    NS_TRY(tio_ble_service_init(), "BLE init failed.\n");
    while (1)
    {
        wsfOsDispatcher();
    }
#endif
    while (1)
    {
        vTaskDelay(1000);
    }
}

void tio_send_slot_data(uint8_t slot, uint8_t slot_type, const uint8_t *data, uint32_t length)
{
#if(TIO_BLE_ENABLED)
    tio_ble_send_slot_data(slot, slot_type, data, length);
#endif
#if(TIO_USB_ENABLED)
    tio_usb_send_slot_data(slot, slot_type, data, length);
#endif
}

void tio_send_uio_state(const uint8_t *data, uint32_t length)
{
#if(TIO_BLE_ENABLED)
    tio_ble_send_uio(data, length);
#endif
#if(TIO_USB_ENABLED)
    tio_usb_send_uio_data(data, length);
#endif
}

void tio_start()
{
#if(TIO_BLE_ENABLED)
    ns_ble_pre_init();
#endif
}

uint32_t tio_init(tio_context_t *ctx)
{
    // Pass it name, manufacturer, and serial number
    // Provide callbacks (e.g. UIO is written to, signal data is read from, etc.)
    // Initialize BLE and USB
    uint32_t status = NS_STATUS_SUCCESS;
#if(TIO_USB_ENABLED)
    status |= tio_usb_init(ctx);
#endif
    gTioCtx = ctx;
    return status;
}
