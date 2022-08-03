/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-26     ASUS       the first version
 */
#ifndef APPLICATIONS_DEVICE_AHT10_DEVICE_AHT10_H_
#define APPLICATIONS_DEVICE_AHT10_DEVICE_AHT10_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "aht10.h"
#include "sensor_asair_aht10.h"

#define   AHT10_I2C_BUS    "i2c1"

 extern float humidity;
 extern float  temperature;
 int app_aht10_init(void);


#endif /* APPLICATIONS_DEVICE_AHT10_DEVICE_AHT10_H_ */
