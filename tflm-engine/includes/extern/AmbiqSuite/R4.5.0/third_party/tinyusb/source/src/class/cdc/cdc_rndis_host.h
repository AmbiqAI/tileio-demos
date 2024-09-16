<<<<<<<< HEAD:ei-ecg-foundation/src/ambiq-copy-me-into-porting-dir/debug_log.cpp
/* Edge Impulse inferencing library
 * Copyright (c) 2021 EdgeImpulse Inc.
========
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
>>>>>>>> 9de8e658cc867fef843e161d691ac37f214f0eb3:tflm-engine/includes/extern/AmbiqSuite/R4.5.0/third_party/tinyusb/source/src/class/cdc/cdc_rndis_host.h
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

<<<<<<<< HEAD:ei-ecg-foundation/src/ambiq-copy-me-into-porting-dir/debug_log.cpp
#include "../ei_classifier_porting.h"


#if EI_PORTING_AMBIQ == 1

#include "ns_ambiqsuite_harness.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/debug_log.h"
#include <stdio.h>
#include <stdarg.h>

// On mbed platforms, we set up a serial port and write to it for debug logging.
#if defined(__cplusplus) && EI_C_LINKAGE == 1
extern "C"
#endif // defined(__cplusplus) && EI_C_LINKAGE == 1
void DebugLog(const char* s) {
    ns_lp_printf("%s", s);
}

#endif // EI_PORTING_AMBIQ == 1
========
/** \ingroup CDC_RNDIS
 * \defgroup CDC_RNSID_Host Host
 *  @{ */

#ifndef _TUSB_CDC_RNDIS_HOST_H_
#define _TUSB_CDC_RNDIS_HOST_H_

#include "common/tusb_common.h"
#include "host/usbh.h"
#include "cdc_rndis.h"

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------+
// INTERNAL RNDIS-CDC Driver API
//--------------------------------------------------------------------+
typedef struct {
  OSAL_SEM_DEF(semaphore_notification);
  osal_semaphore_handle_t sem_notification_hdl;  // used to wait on notification pipe
  uint32_t max_xfer_size; // got from device's msg initialize complete
  uint8_t mac_address[6];
}rndish_data_t;

void rndish_init(void);
bool rndish_open_subtask(uint8_t dev_addr, cdch_data_t *p_cdc);
void rndish_xfer_isr(cdch_data_t *p_cdc, pipe_handle_t pipe_hdl, xfer_result_t event, uint32_t xferred_bytes);
void rndish_close(uint8_t dev_addr);

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CDC_RNDIS_HOST_H_ */

/** @} */
>>>>>>>> 9de8e658cc867fef843e161d691ac37f214f0eb3:tflm-engine/includes/extern/AmbiqSuite/R4.5.0/third_party/tinyusb/source/src/class/cdc/cdc_rndis_host.h
