/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-26     ASUS       the first version
 */
#include "device_aht10.h"
#include "aht10.h"
#include "sensor_asair_aht10.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "ssd1306_12864_hw_i2c_example.h"


static aht10_device_t temp_humi_device;
static rt_sem_t Sample_sem = RT_NULL;   /* 信号量控制块,控制数据显示 */


/* 定义一个温湿度采集线程句柄结构体指针 */
static rt_thread_t aht10_thread = RT_NULL;
float humidity, temperature;

/* 温湿度采集线程入口函数*/
 void aht10_thread_entry(void *parameter)
{
     static aht10_device_t temp_humi_device;
     temp_humi_device=aht10_init(AHT10_I2C_BUS );
    while (1)
    {
        room_envent_take(RT_WAITING_FOREVER);
        /* read humidity 采集湿度 */
        humidity = aht10_read_humidity(temp_humi_device);
        rt_kprintf("humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10); /* former is integer and behind is decimal */
        /* read temperature 采集温度 */
        temperature = aht10_read_temperature(temp_humi_device);
        rt_kprintf("temperature: %d.%d \n", (int)temperature, (int)(temperature * 10) % 10); /* former is integer and behind is decimal */
        rt_thread_mdelay(5000);
    }
}
//线程初始化函数
 int app_aht10_init(void)
{
    rt_err_t rt_err;
    /* 创建温湿度采集线程*/
    aht10_thread = rt_thread_create("aht10thread",     /* 线程的名称 */
                                    aht10_thread_entry, /* 线程入口函数 */
                                    RT_NULL,            /* 线程入口函数的参数   */
                                    1024,                /* 线程栈大小，单位是字节  */
                                    16,                 /* 线程的优先级，数值越小优先级越高*/
                                    50);                /* 线程的时间片大小 */
    /* 如果获得线程控制块，启动这个线程 */
    if (aht10_thread != RT_NULL)
        rt_err = rt_thread_startup(aht10_thread);
    else
        rt_kprintf("aht10 thread create failure !!! \n");
    /* 判断线程是否创建成功 */
    if( rt_err != RT_EOK)
        rt_kprintf("aht10 thread startup err. \n");
    return 0;
}
 MSH_CMD_EXPORT(app_aht10_init,aht10_init);






