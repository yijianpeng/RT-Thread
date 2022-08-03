/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-01     ASUS       the first version
 */
#ifndef APPLICATIONS_SSD1306_12864_HW_I2C_EXAMPLE_H_
#define APPLICATIONS_SSD1306_12864_HW_I2C_EXAMPLE_H_


int  app_oled_init(void);
void room_envent_take(rt_int32_t time);
void weather_sem_take(rt_int32_t time);

#endif /* APPLICATIONS_SSD1306_12864_HW_I2C_EXAMPLE_H_ */
