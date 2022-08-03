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
static rt_sem_t Sample_sem = RT_NULL;   /* �ź������ƿ�,����������ʾ */


/* ����һ����ʪ�Ȳɼ��߳̾���ṹ��ָ�� */
static rt_thread_t aht10_thread = RT_NULL;
float humidity, temperature;

/* ��ʪ�Ȳɼ��߳���ں���*/
 void aht10_thread_entry(void *parameter)
{
     static aht10_device_t temp_humi_device;
     temp_humi_device=aht10_init(AHT10_I2C_BUS );
    while (1)
    {
        room_envent_take(RT_WAITING_FOREVER);
        /* read humidity �ɼ�ʪ�� */
        humidity = aht10_read_humidity(temp_humi_device);
        rt_kprintf("humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10); /* former is integer and behind is decimal */
        /* read temperature �ɼ��¶� */
        temperature = aht10_read_temperature(temp_humi_device);
        rt_kprintf("temperature: %d.%d \n", (int)temperature, (int)(temperature * 10) % 10); /* former is integer and behind is decimal */
        rt_thread_mdelay(5000);
    }
}
//�̳߳�ʼ������
 int app_aht10_init(void)
{
    rt_err_t rt_err;
    /* ������ʪ�Ȳɼ��߳�*/
    aht10_thread = rt_thread_create("aht10thread",     /* �̵߳����� */
                                    aht10_thread_entry, /* �߳���ں��� */
                                    RT_NULL,            /* �߳���ں����Ĳ���   */
                                    1024,                /* �߳�ջ��С����λ���ֽ�  */
                                    16,                 /* �̵߳����ȼ�����ֵԽС���ȼ�Խ��*/
                                    50);                /* �̵߳�ʱ��Ƭ��С */
    /* �������߳̿��ƿ飬��������߳� */
    if (aht10_thread != RT_NULL)
        rt_err = rt_thread_startup(aht10_thread);
    else
        rt_kprintf("aht10 thread create failure !!! \n");
    /* �ж��߳��Ƿ񴴽��ɹ� */
    if( rt_err != RT_EOK)
        rt_kprintf("aht10 thread startup err. \n");
    return 0;
}
 MSH_CMD_EXPORT(app_aht10_init,aht10_init);






