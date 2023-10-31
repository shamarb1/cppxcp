/*
 * Copyright (C) 2019 - Volvo Car Corporation
 *
 * All Rights Reserved
 *
 * LEGAL NOTICE:  All information (including intellectual and technical
 * concepts) contained herein is, and remains the property of or licensed to
 * Volvo Car Corporation. This information is proprietary to Volvo Car
 * Corporation and may be covered by patents or patent applications. This
 * information is protected by trade secret or copyright law. Dissemination of
 * this information or reproduction of this material is strictly forbidden
 * unless prior written permission is obtained from Volvo Car Corporation.
 */

/**********************************************************************************************************************/
/* This is an example app that will show how the XCP server can be used to measure data and how to calibrate parameters
   in an app running in the HPA.

   When this app is run togheter with an ASAM tool (like Etas INCA or Vector CANape among others) it is possible to
   measure up to 180 signals of different type distributed over 3 time rasters (10ms, 100ms and 1000ms). The total
   amount of bytes allowed in each DAQ list is 240. If all 10ms signals are measured in the 10 ms raster, all 100 ms
   signals are measured in the 100 ms raster and all 100ms signals are measured in the 1000 ms raster, there will be
   230 bytes measured in each raster.

   The update of the signals can be turned on and off with the help of a calibration parameters:
    - The signals updated in the  100 ms raster will be updated when parameter myCalibrationConstant2 is set to "2"
    - The signals updated in the 1000 ms raster will be updated when parameter myCalibrationConstant3 is set to "3"
    - The signals updated in the   10 ms raster will be updated when: myCalibrationConstant1 is set to "1" AND
      myCalibrationConstant4 is set to "4" AND myCalConst[0] is set to "0.0"

   myCalConst is a 5000 element long array of double => 40 000 bytes which is present only to demonstrate the ability
   of the XCP server to handle big calibration pages (the limit is 65535 bytes and the app handles a 40012 bytes long
   calibration page). */

#include "business_logic.hpp"

#ifdef QNX_SPECIFIC_CODE
#include <sys/neutrino.h>  // Only builds for target and only of interest for measuring of XcpEvent duration
#endif

#include <inttypes.h>

#include <cstdint>

#define RASTER_10MS 1U
#define RASTER_100MS 2U
#define RASTER_1000MS 3U

#define NO_OF_10MS_TICKS_PER_100MS 10U
#define NO_OF_100MS_TICKS_PER_1000MS 10U

uint16_t centMsCounter = 0U;
uint16_t tenMsCounter = 0U;

//@a2l on
//@a2l-type characteristic
//@a2l-min 1
//@a2l-max 5
uint8_t Uint8CalVar_0 __attribute__((section("caldata"))) = 5U;
//@a2l on
//@a2l-type characteristic
//@a2l-min 1
//@a2l-max 5
uint8_t Uint8CalVar_1 __attribute__((section("caldata"))) = 5U;
//@a2l on
//@a2l-type characteristic
//@a2l-min 1
//@a2l-max 5
uint8_t Uint8CalVar_2 __attribute__((section("caldata"))) = 5U;
//@a2l on
//@a2l-type characteristic
//@a2l-min 1
//@a2l-max 5
uint8_t Uint8CalVar_3 __attribute__((section("caldata"))) = 5U;
//@a2l on
//@a2l-type characteristic
//@a2l-min 1
//@a2l-max 5
uint8_t Uint8CalVar_4 __attribute__((section("caldata"))) = 5U;

//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_0 = 4U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_1 = 14U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_2 = 24U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_3 = 34U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_4 = 44U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_5 = 54U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_6 = 64U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_7 = 74U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_8 = 84U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_9 = 94U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_10 = 104U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_11 = 114U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_12 = 124U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_13 = 134U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_14 = 144U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_15 = 154U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_16 = 164U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_17 = 174U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_18 = 184U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10ms_19 = 194U;

//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_0 = 5U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_1 = 15U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_2 = 25U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_3 = 35U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_4 = 45U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_5 = 55U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_6 = 65U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_7 = 75U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_8 = 85U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_9 = 95U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_10 = 105U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_11 = 115U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_12 = 125U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_13 = 135U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_14 = 145U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_15 = 155U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_16 = 165U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_17 = 175U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_18 = 185U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas100ms_19 = 195U;

//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_0 = 6U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_1 = 16U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_2 = 26U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_3 = 36U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_4 = 46U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_5 = 56U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_6 = 66U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_7 = 76U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_8 = 86U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_9 = 96U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_10 = 106U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas10000ms_11 = 116U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_12 = 126U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_13 = 136U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_14 = 146U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_15 = 156U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_16 = 166U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_17 = 176U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_18 = 186U;
//@a2l on
//@a2l-type measurement
uint8_t Uint8Meas1000ms_19 = 196U;

//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_0 = 5000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_1 = 10000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_2 = 15000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_3 = 20000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_4 = 25000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_5 = 30000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_6 = 35000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_7 = 40000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_8 = 45000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_9 = 50000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_10 = 55000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_11 = 50000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_12 = 45000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_13 = 40000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_14 = 35000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_15 = 30000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_16 = 25000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_17 = 20000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_18 = 15000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas10ms_19 = 5000U;

//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_0 = 5000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_1 = 10000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_2 = 15000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_3 = 20000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_4 = 25000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_5 = 30000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_6 = 35000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_7 = 40000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_8 = 45000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_9 = 50000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_10 = 55000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_11 = 50000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_12 = 45000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_13 = 40000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_14 = 35000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_15 = 30000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_16 = 25000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_17 = 20000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_18 = 15000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas100ms_19 = 10000U;

//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_0 = 5000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_1 = 10000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_2 = 15000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_3 = 20000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_4 = 25000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_5 = 30000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_6 = 35000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_7 = 40000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_8 = 45000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_9 = 50000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_10 = 55000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_11 = 50000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_12 = 45000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_13 = 40000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_14 = 35000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_15 = 30000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_16 = 25000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_17 = 20000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_18 = 15000U;
//@a2l on
//@a2l-type measurement
uint16_t Uint16Meas1000ms_19 = 10000U;

//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_0 = 100000000U;  // 0x05F5E100
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_1 = 500000000U;  // 0x1DCD6500
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_2 = 900000000U;  // 0x35A4E900
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_3 = 1300000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_4 = 1700000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_5 = 2100000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_6 = 2500000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_7 = 2900000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_8 = 3300000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_9 = 3700000000U;  // 0xDC898500
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_10 = 410000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_11 = 450000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_12 = 490000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_13 = 530000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_14 = 570000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_15 = 610000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_16 = 650000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_17 = 690000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_18 = 730000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas10ms_19 = 770000000U;

//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_0 = 100000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_1 = 500000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_2 = 900000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_3 = 1300000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_4 = 1700000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_5 = 2100000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_6 = 2500000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_7 = 2900000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_8 = 3300000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_9 = 3700000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_10 = 4100000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_11 = 450000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_12 = 490000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_13 = 530000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_14 = 570000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_15 = 610000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_16 = 650000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_17 = 690000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_18 = 730000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas100ms_19 = 770000000U;

//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_0 = 100000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_1 = 500000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_2 = 900000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_3 = 1300000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_4 = 1700000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_5 = 2100000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_6 = 2500000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_7 = 2900000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_8 = 3300000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_9 = 3700000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_10 = 4100000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_11 = 400000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_12 = 490000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_13 = 530000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_14 = 570000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_15 = 610000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_16 = 650000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_17 = 690000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_18 = 730000000U;
//@a2l on
//@a2l-type measurement
uint32_t Uint32Meas1000ms_19 = 770000000U;

//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_0 = 10000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_1 = 50000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_2 = 90000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_3 = 130000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_4 = 170000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_5 = 210000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_6 = 250000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_7 = 290000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_8 = 330000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_9 = 370000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_10 = 410000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_11 = 450000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_12 = 490000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_13 = 530000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_14 = 570000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_15 = 610000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_16 = 650000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_17 = 690000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_18 = 730000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas10ms_19 = 770000000;

//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_0 = 10000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_1 = 50000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_2 = 90000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_3 = 130000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_4 = 170000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_5 = 210000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_6 = 250000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_7 = 290000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_8 = 330000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_9 = 370000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_10 = 410000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_11 = 450000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_12 = 490000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_13 = 530000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_14 = 570000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_15 = 610000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_16 = 650000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_17 = 690000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_18 = 730000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas100ms_19 = 770000000;

//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_0 = 10000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_1 = 50000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_2 = 90000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_3 = 130000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_4 = 170000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_5 = 210000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_6 = 250000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_7 = 290000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_8 = 330000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_9 = 370000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_10 = 410000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_11 = 450000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_12 = 490000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_13 = 530000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_14 = 570000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_15 = 610000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_16 = 650000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_17 = 690000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_18 = 730000000;
//@a2l on
//@a2l-type measurement
int32_t Int32Meas1000ms_19 = 770000000;

//@a2l on
//@a2l-type measurement
float FloatMeas10ms_0 = 0.0000F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_1 = 0.0011F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_2 = 0.0022F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_3 = 0.0033F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_4 = 0.0044F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_5 = 0.0055F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_6 = 0.0066F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_7 = 0.0077F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_8 = 0.0088F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_9 = 0.0099F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_10 = 0.0000F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_11 = 0.0111F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_12 = 0.0222F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_13 = 0.0333F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_14 = 0.0444F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_15 = 0.0555F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_16 = 0.0666F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_17 = 0.0777F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_18 = 0.0888F;
//@a2l on
//@a2l-type measurement
float FloatMeas10ms_19 = 0.0999F;

//@a2l on
//@a2l-type measurement
float FloatMeas100ms_0 = 0.0000F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_1 = 0.0011F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_2 = 0.0022F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_3 = 0.0033F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_4 = 0.0044F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_5 = 0.0055F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_6 = 0.0066F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_7 = 0.0077F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_8 = 0.0088F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_9 = 0.0099F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_10 = 0.0000F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_11 = 0.0111F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_12 = 0.0222F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_13 = 0.0333F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_14 = 0.0444F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_15 = 0.0555F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_16 = 0.0666F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_17 = 0.0777F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_18 = 0.0888F;
//@a2l on
//@a2l-type measurement
float FloatMeas100ms_19 = 0.0999F;

//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_0 = 0.0000F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_1 = 0.0011F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_2 = 0.0022F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_3 = 0.0033F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_4 = 0.0044F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_5 = 0.0055F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_6 = 0.0066F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_7 = 0.0077F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_8 = 0.0088F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_9 = 0.0099F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_10 = 0.0000F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_11 = 0.0111F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_12 = 0.0222F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_13 = 0.0333F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_14 = 0.0444F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_15 = 0.0555F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_16 = 0.0666F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_17 = 0.0777F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_18 = 0.0888F;
//@a2l on
//@a2l-type measurement
float FloatMeas1000ms_19 = 0.0999F;

//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_0 = 0.000;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_1 = 0.001;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_2 = 0.002;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_3 = 0.003;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_4 = 0.004;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_5 = 0.005;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_6 = 0.006;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_7 = 0.007;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_8 = 0.008;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_9 = 0.009;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_10 = 0.0000;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_11 = 0.0011;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_12 = 0.0022;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_13 = 0.0033;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_14 = 0.0044;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_15 = 0.0055;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_16 = 0.0066;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_17 = 0.0077;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_18 = 0.0088;
//@a2l on
//@a2l-type measurement
double DoubleMeas10ms_19 = 0.0099;

//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_0 = 0.000;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_1 = 0.011;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_2 = 0.022;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_3 = 0.033;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_4 = 0.044;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_5 = 0.055;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_6 = 0.066;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_7 = 0.077;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_8 = 0.088;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_9 = 0.099;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_10 = 0.000;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_11 = 0.0111;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_12 = 0.0222;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_13 = 0.0333;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_14 = 0.0444;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_15 = 0.0555;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_16 = 0.0666;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_17 = 0.0777;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_18 = 0.0888;
//@a2l on
//@a2l-type measurement
double DoubleMeas100ms_19 = 0.0999;

//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_0 = 0.00;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_1 = 0.11;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_2 = 0.22;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_3 = 0.33;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_4 = 0.44;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_5 = 0.55;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_6 = 0.66;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_7 = 0.77;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_8 = 0.88;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_9 = 0.99;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_10 = 0.00;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_11 = 0.111;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_12 = 0.222;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_13 = 0.333;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_14 = 0.444;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_15 = 0.555;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_16 = 0.666;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_17 = 0.777;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_18 = 0.888;
//@a2l on
//@a2l-type measurement
double DoubleMeas1000ms_19 = 0.999;

//@a2l on
//@a2l-type characteristic
volatile uint16_t myCalibrationConstant3 __attribute__((section("caldata"))) = 3U;  // Start/Stop 1000 ms data
//@a2l on
//@a2l-type characteristic
volatile uint16_t myCalibrationConstant2 __attribute__((section("caldata"))) = 2U;  // Start/Stop 1000 ms data
//@a2l on
//@a2l-type characteristic
volatile uint16_t myCalibrationConstant4 __attribute__((section("caldata"))) = 4U;  // Start/Stop 1000 ms data
//@a2l on
//@a2l-type characteristic
volatile uint16_t myCalibrationConstant1 __attribute__((section("caldata"))) = 1U;  // Start/Stop 1000 ms data
//@a2l on
//@a2l-type characteristic
volatile double myCalConst[5000] __attribute__((section("caldata")));  // A big array just to get a big cal page

// Here follows some basic examples of how to declare objects for which A2l entries shall be created. Full
// documentation in VccA2lTools/README.md. Please read this for details on specifiying properties per type
// or per instance, precedence rules etc etc.

// @a2l on
// @a2l-type measurement
struct AStruct {
    // @a2l-min 13
    int x;
    int y;
};

AStruct aStructInstance;

class ABaseClass {
    int x;
    int y;
};

class ADerivedClass : public ABaseClass {
    int ax;
    // @a2l-min 14
    int ay;
};

// @a2l on
// @a2l-type measurement
ADerivedClass aDerivedClassInstance;

// @a2l on
// @a2l-type measurement
enum AnEnum { ONE, TWO, THREE };

// @a2l on
// @a2l-type measurement
typedef double MyType;

MyType myTypeInstance;

AnEnum anEnumInstance;

uint64_t TimeStamp10msBefore = 0U;
uint64_t TimeStamp10msAfter = 0LU;
uint64_t XcpEvent10msDuration = 0LU;
uint64_t TimeStamp100msBefore = 0LU;
uint64_t TimeStamp100msAfter = 0LU;
uint64_t XcpEvent10m0sDuration = 0LU;
uint64_t TimeStamp1000msBefore = 0LU;
uint64_t TimeStamp1000msAfter = 0LU;
uint64_t XcpEvent1000msDuration = 0LU;

//@a2l on
//@a2l-type measurement
uint32_t XcpEvent10msDurationLSB = 0U;
//@a2l on
//@a2l-type measurement
uint32_t XcpEvent10msDurationMSB = 0U;
//@a2l on
//@a2l-type measurement
uint32_t TimeStamp10msBeforeLSB = 0U;
//@a2l on
//@a2l-type measurement
uint32_t TimeStamp10msBeforeMSB = 0U;
//@a2l on
//@a2l-type measurement
uint32_t TimeStamp10msAfterLSB = 0U;
//@a2l on
//@a2l-type measurement
uint32_t TimeStamp10msAfterMSB = 0U;
void business_logic() {
    // myCalConst and myCalibrationConstant4 used here just to exemplify that the order variables are sorted
    // in the map file can be based on order of occurance in the code rather than definition order
    if ((0.0 == myCalConst[0]) && (1U == myCalibrationConstant1) && (4U == myCalibrationConstant4)) {
        // Perid time 2.56 seconds
        Uint8Meas10ms_0 += 1U;
        Uint8Meas10ms_1 += 1U;
        Uint8Meas10ms_2 += 1U;
        Uint8Meas10ms_3 += 1U;
        Uint8Meas10ms_4 += 1U;
        Uint8Meas10ms_5 += 1U;
        Uint8Meas10ms_6 += 1U;
        Uint8Meas10ms_7 += 1U;
        Uint8Meas10ms_8 += 1U;
        Uint8Meas10ms_9 += 1U;

        // Period time 6.556 seconds
        Uint16Meas10ms_0 += 100U;
        Uint16Meas10ms_1 += 100U;
        Uint16Meas10ms_2 += 100U;
        Uint16Meas10ms_3 += 100U;
        Uint16Meas10ms_4 += 100U;
        Uint16Meas10ms_5 += 100U;
        Uint16Meas10ms_6 += 100U;
        Uint16Meas10ms_7 += 100U;
        Uint16Meas10ms_8 += 100U;
        Uint16Meas10ms_9 += 100U;

        // Period time 77 seconds
        Uint32Meas10ms_0 += 555555U;
        Uint32Meas10ms_1 += 555555U;
        Uint32Meas10ms_2 += 555555U;
        Uint32Meas10ms_3 += 555555U;
        Uint32Meas10ms_4 += 555555U;
        Uint32Meas10ms_5 += 555555U;
        Uint32Meas10ms_6 += 555555U;
        Uint32Meas10ms_7 += 555555U;
        Uint32Meas10ms_8 += 555555U;
        Uint32Meas10ms_9 += 555555U;

        // Period time 77 seconds
        Int32Meas10ms_0 -= 555555;
        Int32Meas10ms_1 -= 555555;
        Int32Meas10ms_2 -= 555555;
        Int32Meas10ms_3 -= 555555;
        Int32Meas10ms_4 -= 555555;
        Int32Meas10ms_5 -= 555555;
        Int32Meas10ms_6 -= 555555;
        Int32Meas10ms_7 -= 555555;
        Int32Meas10ms_8 -= 555555;
        Int32Meas10ms_9 -= 555555;

        FloatMeas10ms_0 += 1.0F;
        FloatMeas10ms_1 += 1.0F;
        FloatMeas10ms_2 += 1.0F;
        FloatMeas10ms_3 += 1.0F;
        FloatMeas10ms_4 += 1.0F;
        FloatMeas10ms_5 += 1.0F;
        FloatMeas10ms_6 += 1.0F;
        FloatMeas10ms_7 += 1.0F;
        FloatMeas10ms_8 += 1.0F;
        FloatMeas10ms_9 += 1.0F;

        DoubleMeas10ms_0 += 1.0;
        DoubleMeas10ms_1 += 1.0;
        DoubleMeas10ms_2 += 1.0;
        DoubleMeas10ms_3 += 1.0;
        DoubleMeas10ms_4 += 1.0;
        DoubleMeas10ms_5 += 1.0;
        DoubleMeas10ms_6 += 1.0;
        DoubleMeas10ms_7 += 1.0;
        DoubleMeas10ms_8 += 1.0;
        DoubleMeas10ms_9 += 1.0;
    }
// This part is for measurement of XcpEvent duration on target
#ifdef QNX_SPECIFIC_CODE
    TimeStamp10msBefore = ClockCycles();
    TimeStamp10msBeforeLSB = (uint32_t)TimeStamp10msBefore;
    TimeStamp10msBeforeMSB = (uint32_t)(TimeStamp10msBefore >> 32);
#endif

    XcpEvent(RASTER_10MS);  // Call 10 ms XCP event

// This part is for measurement of XcpEvent duration on target
#ifdef QNX_SPECIFIC_CODE
    TimeStamp10msAfter = ClockCycles();
    TimeStamp10msAfterLSB = (uint32_t)TimeStamp10msAfter;
    TimeStamp10msAfterMSB = (uint32_t)(TimeStamp10msAfter >> 32);
    if (TimeStamp10msAfter > TimeStamp10msBefore) {
        XcpEvent10msDuration = TimeStamp10msAfter - TimeStamp10msBefore;
        XcpEvent10msDurationLSB = (uint32_t)XcpEvent10msDuration;
        XcpEvent10msDurationMSB = (uint32_t)(XcpEvent10msDuration >> 32);
    } else {
        XcpEvent10msDurationMSB = 11U;
        XcpEvent10msDurationLSB = 11U;
    }
#endif

    tenMsCounter++;
    if (tenMsCounter >= NO_OF_10MS_TICKS_PER_100MS) {
        if (2U == myCalibrationConstant2) {
            // Perid time 12.8 seconds
            Uint8Meas100ms_0 += 2U;
            Uint8Meas100ms_1 += 2U;
            Uint8Meas100ms_2 += 2U;
            Uint8Meas100ms_3 += 2U;
            Uint8Meas100ms_4 += 2U;
            Uint8Meas100ms_5 += 2U;
            Uint8Meas100ms_6 += 2U;
            Uint8Meas100ms_7 += 2U;
            Uint8Meas100ms_8 += 2U;
            Uint8Meas100ms_9 += 2U;

            // Period time 32.7 seconds
            Uint16Meas100ms_0 += 200U;
            Uint16Meas100ms_1 += 200U;
            Uint16Meas100ms_2 += 200U;
            Uint16Meas100ms_3 += 200U;
            Uint16Meas100ms_4 += 200U;
            Uint16Meas100ms_5 += 200U;
            Uint16Meas100ms_6 += 200U;
            Uint16Meas100ms_7 += 200U;
            Uint16Meas100ms_8 += 200U;
            Uint16Meas100ms_9 += 200U;

            // Period time 55 seconds
            Uint32Meas100ms_0 += 7777777U;
            Uint32Meas100ms_1 += 7777777U;
            Uint32Meas100ms_2 += 7777777U;
            Uint32Meas100ms_3 += 7777777U;
            Uint32Meas100ms_4 += 7777777U;
            Uint32Meas100ms_5 += 7777777U;
            Uint32Meas100ms_6 += 7777777U;
            Uint32Meas100ms_7 += 7777777U;
            Uint32Meas100ms_8 += 7777777U;
            Uint32Meas100ms_9 += 7777777U;

            // Period time 55 seconds
            Int32Meas100ms_0 -= 7777777;
            Int32Meas100ms_1 -= 7777777;
            Int32Meas100ms_2 -= 7777777;
            Int32Meas100ms_3 -= 7777777;
            Int32Meas100ms_4 -= 7777777;
            Int32Meas100ms_5 -= 7777777;
            Int32Meas100ms_6 -= 7777777;
            Int32Meas100ms_7 -= 7777777;
            Int32Meas100ms_8 -= 7777777;
            Int32Meas100ms_9 -= 7777777;

            FloatMeas100ms_0 += 1.0F;
            FloatMeas100ms_1 += 1.0F;
            FloatMeas100ms_2 += 1.0F;
            FloatMeas100ms_3 += 1.0F;
            FloatMeas100ms_4 += 1.0F;
            FloatMeas100ms_5 += 1.0F;
            FloatMeas100ms_6 += 1.0F;
            FloatMeas100ms_7 += 1.0F;
            FloatMeas100ms_8 += 1.0F;
            FloatMeas100ms_9 += 1.0F;

            DoubleMeas100ms_0 += 1.0;
            DoubleMeas100ms_1 += 1.0;
            DoubleMeas100ms_2 += 1.0;
            DoubleMeas100ms_3 += 1.0;
            DoubleMeas100ms_4 += 1.0;
            DoubleMeas100ms_5 += 1.0;
            DoubleMeas100ms_6 += 1.0;
            DoubleMeas100ms_7 += 1.0;
            DoubleMeas100ms_8 += 1.0;
            DoubleMeas100ms_9 += 1.0;
        }
        XcpEvent(RASTER_100MS);  // Call 100 ms XCP event
        tenMsCounter = 0U;
        centMsCounter++;
    }
    if (centMsCounter >= NO_OF_100MS_TICKS_PER_1000MS) {
        if (3U == myCalibrationConstant3) {
            // Perid time 85 seconds
            Uint8Meas1000ms_0 += 2U;
            Uint8Meas1000ms_1 += 2U;
            Uint8Meas1000ms_2 += 2U;
            Uint8Meas1000ms_3 += 2U;
            Uint8Meas1000ms_4 += 2U;
            Uint8Meas1000ms_5 += 2U;
            Uint8Meas1000ms_6 += 2U;
            Uint8Meas1000ms_7 += 2U;
            Uint8Meas1000ms_8 += 2U;
            Uint8Meas1000ms_9 += 2U;

            // Period time 218 seconds
            Uint16Meas1000ms_0 += 300U;
            Uint16Meas1000ms_1 += 300U;
            Uint16Meas1000ms_2 += 300U;
            Uint16Meas1000ms_3 += 300U;
            Uint16Meas1000ms_4 += 300U;
            Uint16Meas1000ms_5 += 300U;
            Uint16Meas1000ms_6 += 300U;
            Uint16Meas1000ms_7 += 300U;
            Uint16Meas1000ms_8 += 300U;
            Uint16Meas1000ms_9 += 300U;

            // Period time 429 seconds
            Uint32Meas1000ms_0 += 99999999U;
            Uint32Meas1000ms_1 += 99999999U;
            Uint32Meas1000ms_2 += 99999999U;
            Uint32Meas1000ms_3 += 99999999U;
            Uint32Meas1000ms_4 += 99999999U;
            Uint32Meas1000ms_5 += 99999999U;
            Uint32Meas1000ms_6 += 99999999U;
            Uint32Meas1000ms_7 += 99999999U;
            Uint32Meas1000ms_8 += 99999999U;
            Uint32Meas1000ms_9 += 99999999U;

            // Period time 429 seconds
            Int32Meas1000ms_0 -= 9999999;
            Int32Meas1000ms_1 -= 9999999;
            Int32Meas1000ms_2 -= 9999999;
            Int32Meas1000ms_3 -= 9999999;
            Int32Meas1000ms_4 -= 9999999;
            Int32Meas1000ms_5 -= 9999999;
            Int32Meas1000ms_6 -= 9999999;
            Int32Meas1000ms_7 -= 9999999;
            Int32Meas1000ms_8 -= 9999999;
            Int32Meas1000ms_9 -= 9999999;

            FloatMeas1000ms_0 += 1.0F;
            FloatMeas1000ms_1 += 1.0F;
            FloatMeas1000ms_2 += 1.0F;
            FloatMeas1000ms_3 += 1.0F;
            FloatMeas1000ms_4 += 1.0F;
            FloatMeas1000ms_5 += 1.0F;
            FloatMeas1000ms_6 += 1.0F;
            FloatMeas1000ms_7 += 1.0F;
            FloatMeas1000ms_8 += 1.0F;
            FloatMeas1000ms_9 += 1.0F;

            DoubleMeas1000ms_0 += 1.0;
            DoubleMeas1000ms_1 += 1.0;
            DoubleMeas1000ms_2 += 1.0;
            DoubleMeas1000ms_3 += 1.0;
            DoubleMeas1000ms_4 += 1.0;
            DoubleMeas1000ms_5 += 1.0;
            DoubleMeas1000ms_6 += 1.0;
            DoubleMeas1000ms_7 += 1.0;
            DoubleMeas1000ms_8 += 1.0;
            DoubleMeas1000ms_9 += 1.0;
        }
        XcpEvent(RASTER_1000MS);  // Call 1000 ms XCP event
        centMsCounter = 0U;
    }
}
