//*****************************************************************************
//
//  am_mcu_apollo4l_info0.h
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2023, Ambiq Micro, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision release_sdk_4_4_1-7498c7b770 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#ifndef AM_REG_INFO0_H
#define AM_REG_INFO0_H

#define AM_REG_INFO0_BASEADDR 0x42000000
#define AM_REG_INFO0n(n) 0x42000000

#define AM_REG_INFO0_SIGNATURE0_O 0x00000000
#define AM_REG_INFO0_SIGNATURE0_ADDR 0x42000000
#define AM_REG_INFO0_SIGNATURE1_O 0x00000004
#define AM_REG_INFO0_SIGNATURE1_ADDR 0x42000004
#define AM_REG_INFO0_SIGNATURE2_O 0x00000008
#define AM_REG_INFO0_SIGNATURE2_ADDR 0x42000008
#define AM_REG_INFO0_SIGNATURE3_O 0x0000000c
#define AM_REG_INFO0_SIGNATURE3_ADDR 0x4200000c
#define AM_REG_INFO0_CUSTOMER_TRIM_O 0x00000014
#define AM_REG_INFO0_CUSTOMER_TRIM_ADDR 0x42000014
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_O 0x00000028
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_ADDR 0x42000028
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_O 0x0000002c
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_ADDR 0x4200002c
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG2_O 0x00000030
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG2_ADDR 0x42000030
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG3_O 0x00000034
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG3_ADDR 0x42000034
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG4_O 0x00000038
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG4_ADDR 0x42000038
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG5_O 0x0000003c
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG5_ADDR 0x4200003c
#define AM_REG_INFO0_SECURITY_VERSION_O 0x00000040
#define AM_REG_INFO0_SECURITY_VERSION_ADDR 0x42000040
#define AM_REG_INFO0_SECURITY_SRAM_RESV_O 0x00000044
#define AM_REG_INFO0_SECURITY_SRAM_RESV_ADDR 0x42000044
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_O 0x00000048
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_ADDR 0x42000048
#define AM_REG_INFO0_WIRED_TIMEOUT_O 0x00000054
#define AM_REG_INFO0_WIRED_TIMEOUT_ADDR 0x42000054
#define AM_REG_INFO0_SBR_SDCERT_ADDR_O 0x00000058
#define AM_REG_INFO0_SBR_SDCERT_ADDR_ADDR 0x42000058
#define AM_REG_INFO0_MAINPTR_O 0x00000060
#define AM_REG_INFO0_MAINPTR_ADDR 0x42000060
#define AM_REG_INFO0_CERTCHAINPTR_O 0x00000064
#define AM_REG_INFO0_CERTCHAINPTR_ADDR 0x42000064

// SIGNATURE0 - Word 0 (low word, bits 31:0) of the 128-bit INFO0 signature.
#define AM_REG_INFO0_SIGNATURE0_SIG0_S 0
#define AM_REG_INFO0_SIGNATURE0_SIG0_M 0xFFFFFFFF
#define AM_REG_INFO0_SIGNATURE0_SIG0(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SIGNATURE0_SIG0_Pos 0
#define AM_REG_INFO0_SIGNATURE0_SIG0_Msk 0xFFFFFFFF

// SIGNATURE1 - Word 1 (bits 63:32) of the 128-bit INFO0 signature.
#define AM_REG_INFO0_SIGNATURE1_SIG1_S 0
#define AM_REG_INFO0_SIGNATURE1_SIG1_M 0xFFFFFFFF
#define AM_REG_INFO0_SIGNATURE1_SIG1(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SIGNATURE1_SIG1_Pos 0
#define AM_REG_INFO0_SIGNATURE1_SIG1_Msk 0xFFFFFFFF

// SIGNATURE2 - Word 2 (bits 95:64) of the 128-bit INFO0 signature.
#define AM_REG_INFO0_SIGNATURE2_SIG2_S 0
#define AM_REG_INFO0_SIGNATURE2_SIG2_M 0xFFFFFFFF
#define AM_REG_INFO0_SIGNATURE2_SIG2(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SIGNATURE2_SIG2_Pos 0
#define AM_REG_INFO0_SIGNATURE2_SIG2_Msk 0xFFFFFFFF

// SIGNATURE3 - Word 3 (high word, bits 127:96) of the 128-bit INFO0 signature.
#define AM_REG_INFO0_SIGNATURE3_SIG3_S 0
#define AM_REG_INFO0_SIGNATURE3_SIG3_M 0xFFFFFFFF
#define AM_REG_INFO0_SIGNATURE3_SIG3(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SIGNATURE3_SIG3_Pos 0
#define AM_REG_INFO0_SIGNATURE3_SIG3_Msk 0xFFFFFFFF

// CUSTOMER_TRIM - Customer Programmable trim overrides. Bits in this register are loaded into hardware registers at reset.
#define AM_REG_INFO0_CUSTOMER_TRIM_ERR001_MUSTBE_0_S 0
#define AM_REG_INFO0_CUSTOMER_TRIM_ERR001_MUSTBE_0_M 0x00000001
#define AM_REG_INFO0_CUSTOMER_TRIM_ERR001_MUSTBE_0(n) (((uint32_t)(n) << 0) & 0x00000001)
#define AM_REG_INFO0_CUSTOMER_TRIM_ERR001_MUSTBE_0_Pos 0
#define AM_REG_INFO0_CUSTOMER_TRIM_ERR001_MUSTBE_0_Msk 0x00000001

// SECURITY_WIRED_IFC_CFG0 - This 32-bit word contains the interface configuration word0 for the UART wired update. This feature is not applicable to Apollo4 revA without pre-installed bootloader.
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD30_S 30
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD30_M 0xC0000000
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD30(n) (((uint32_t)(n) << 30) & 0xC0000000)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD30_Pos 30
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD30_Msk 0xC0000000
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_BAUDRATE_S 8
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_BAUDRATE_M 0x3FFFFF00
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_BAUDRATE(n) (((uint32_t)(n) << 8) & 0x3FFFFF00)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_BAUDRATE_Pos 8
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_BAUDRATE_Msk 0x3FFFFF00
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_DATALEN_S 6
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_DATALEN_M 0x000000C0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_DATALEN(n) (((uint32_t)(n) << 6) & 0x000000C0)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_DATALEN_Pos 6
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_DATALEN_Msk 0x000000C0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_2STOP_S 5
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_2STOP_M 0x00000020
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_2STOP(n) (((uint32_t)(n) << 5) & 0x00000020)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_2STOP_Pos 5
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_2STOP_Msk 0x00000020
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_EVEN_S 4
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_EVEN_M 0x00000010
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_EVEN(n) (((uint32_t)(n) << 4) & 0x00000010)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_EVEN_Pos 4
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_EVEN_Msk 0x00000010
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_PAR_S 3
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_PAR_M 0x00000008
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_PAR(n) (((uint32_t)(n) << 3) & 0x00000008)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_PAR_Pos 3
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_PAR_Msk 0x00000008
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_CTS_S 2
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_CTS_M 0x00000004
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_CTS(n) (((uint32_t)(n) << 2) & 0x00000004)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_CTS_Pos 2
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_CTS_Msk 0x00000004
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RTS_S 1
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RTS_M 0x00000002
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RTS(n) (((uint32_t)(n) << 1) & 0x00000002)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RTS_Pos 1
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RTS_Msk 0x00000002
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD0_S 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD0_M 0x00000001
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD0(n) (((uint32_t)(n) << 0) & 0x00000001)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD0_Pos 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG0_RSVD0_Msk 0x00000001

// SECURITY_WIRED_IFC_CFG1 - This 32-bit word contains the interface configuration word1 for the UART wired update. This feature is not applicable to Apollo4 revA without pre-installed bootloader.
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN3_S 24
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN3_M 0xFF000000
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN3(n) (((uint32_t)(n) << 24) & 0xFF000000)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN3_Pos 24
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN3_Msk 0xFF000000
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN2_S 16
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN2_M 0x00FF0000
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN2(n) (((uint32_t)(n) << 16) & 0x00FF0000)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN2_Pos 16
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN2_Msk 0x00FF0000
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN1_S 8
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN1_M 0x0000FF00
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN1(n) (((uint32_t)(n) << 8) & 0x0000FF00)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN1_Pos 8
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN1_Msk 0x0000FF00
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN0_S 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN0_M 0x000000FF
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN0(n) (((uint32_t)(n) << 0) & 0x000000FF)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN0_Pos 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG1_PIN0_Msk 0x000000FF

// SECURITY_WIRED_IFC_CFG2 - This 32-bit word contains the raw Pin configuration for the UART wired interface pin 0. This feature is not applicable to Apollo4 revA without pre-installed bootloader.
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG2_PINCFG_S 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG2_PINCFG_M 0xFFFFFFFF
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG2_PINCFG(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG2_PINCFG_Pos 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG2_PINCFG_Msk 0xFFFFFFFF

// SECURITY_WIRED_IFC_CFG3 - This 32-bit word contains the raw Pin configuration for the UART wired interface pin 1. This feature is not applicable to Apollo4 revA without pre-installed bootloader.
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG3_PINCFG_S 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG3_PINCFG_M 0xFFFFFFFF
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG3_PINCFG(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG3_PINCFG_Pos 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG3_PINCFG_Msk 0xFFFFFFFF

// SECURITY_WIRED_IFC_CFG4 - This 32-bit word contains the raw Pin configuration for the UART wired interface pin 2. This feature is not applicable to Apollo4 revA without pre-installed bootloader.
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG4_PINCFG_S 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG4_PINCFG_M 0xFFFFFFFF
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG4_PINCFG(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG4_PINCFG_Pos 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG4_PINCFG_Msk 0xFFFFFFFF

// SECURITY_WIRED_IFC_CFG5 - This 32-bit word contains the raw Pin configuration for the UART wired interface pin 3. This feature is not applicable to Apollo4 revA without pre-installed bootloader.
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG5_PINCFG_S 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG5_PINCFG_M 0xFFFFFFFF
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG5_PINCFG(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG5_PINCFG_Pos 0
#define AM_REG_INFO0_SECURITY_WIRED_IFC_CFG5_PINCFG_Msk 0xFFFFFFFF

// SECURITY_VERSION - This 32-bit word contains the version ID used for revision control
#define AM_REG_INFO0_SECURITY_VERSION_VERSION_S 0
#define AM_REG_INFO0_SECURITY_VERSION_VERSION_M 0xFFFFFFFF
#define AM_REG_INFO0_SECURITY_VERSION_VERSION(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SECURITY_VERSION_VERSION_Pos 0
#define AM_REG_INFO0_SECURITY_VERSION_VERSION_Msk 0xFFFFFFFF

// SECURITY_SRAM_RESV - This 32-bit word indicates the amount of DTCM to keep reserved for application scratch space. This reserves the specified memory at the top end of DTCM memory address range. This memory is not disturbed by the Secure Boot Loader. This feature is not applicable to Apollo4 revA without pre-installed bootloader.
#define AM_REG_INFO0_SECURITY_SRAM_RESV_SRAM_RESV_S 0
#define AM_REG_INFO0_SECURITY_SRAM_RESV_SRAM_RESV_M 0x000FFFFF
#define AM_REG_INFO0_SECURITY_SRAM_RESV_SRAM_RESV(n) (((uint32_t)(n) << 0) & 0x000FFFFF)
#define AM_REG_INFO0_SECURITY_SRAM_RESV_SRAM_RESV_Pos 0
#define AM_REG_INFO0_SECURITY_SRAM_RESV_SRAM_RESV_Msk 0x000FFFFF

// SECURITY_RMAOVERRIDE - Enables Ambiq to have the ability to download Ambiq RMA.
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_RSVD_ZERO_S 3
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_RSVD_ZERO_M 0xFFFFFFF8
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_RSVD_ZERO(n) (((uint32_t)(n) << 3) & 0xFFFFFFF8)
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_RSVD_ZERO_Pos 3
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_RSVD_ZERO_Msk 0xFFFFFFF8
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_OVERRIDE_S 0
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_OVERRIDE_M 0x00000007
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_OVERRIDE(n) (((uint32_t)(n) << 0) & 0x00000007)
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_OVERRIDE_Pos 0
#define AM_REG_INFO0_SECURITY_RMAOVERRIDE_OVERRIDE_Msk 0x00000007

// WIRED_TIMEOUT - Holds the timeout value for wired transfers (in milliseconds).
#define AM_REG_INFO0_WIRED_TIMEOUT_TIMEOUT_S 0
#define AM_REG_INFO0_WIRED_TIMEOUT_TIMEOUT_M 0x0000FFFF
#define AM_REG_INFO0_WIRED_TIMEOUT_TIMEOUT(n) (((uint32_t)(n) << 0) & 0x0000FFFF)
#define AM_REG_INFO0_WIRED_TIMEOUT_TIMEOUT_Pos 0
#define AM_REG_INFO0_WIRED_TIMEOUT_TIMEOUT_Msk 0x0000FFFF

// SBR_SDCERT_ADDR - Location where bootloader will find SD certificates. Customer configures this based on the memory layout.
#define AM_REG_INFO0_SBR_SDCERT_ADDR_ICV_S 0
#define AM_REG_INFO0_SBR_SDCERT_ADDR_ICV_M 0xFFFFFFFF
#define AM_REG_INFO0_SBR_SDCERT_ADDR_ICV(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_SBR_SDCERT_ADDR_ICV_Pos 0
#define AM_REG_INFO0_SBR_SDCERT_ADDR_ICV_Msk 0xFFFFFFFF

// MAINPTR - Pointer to the main OEM image when Secure Boot is disabled.
#define AM_REG_INFO0_MAINPTR_ADDRESS_S 0
#define AM_REG_INFO0_MAINPTR_ADDRESS_M 0xFFFFFFFF
#define AM_REG_INFO0_MAINPTR_ADDRESS(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_MAINPTR_ADDRESS_Pos 0
#define AM_REG_INFO0_MAINPTR_ADDRESS_Msk 0xFFFFFFFF

// CERTCHAINPTR - Pointer to OEM certificate chain when Secure Boot is enabled.
#define AM_REG_INFO0_CERTCHAINPTR_ADDRESS_S 0
#define AM_REG_INFO0_CERTCHAINPTR_ADDRESS_M 0xFFFFFFFF
#define AM_REG_INFO0_CERTCHAINPTR_ADDRESS(n) (((uint32_t)(n) << 0) & 0xFFFFFFFF)
#define AM_REG_INFO0_CERTCHAINPTR_ADDRESS_Pos 0
#define AM_REG_INFO0_CERTCHAINPTR_ADDRESS_Msk 0xFFFFFFFF

#endif
