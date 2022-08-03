#include "mq3.h"
#include <rtdevice.h>
#include <stdint.h>
#include <rtthread.h>
#include "drivers/adc.h"
#include "ssd1306_12864_hw_i2c_example.h"

#define     THREAD_PRIORITY_MQ3         13
#define     THREAD_STACK_SIZE_MQ3       512
#define     THREAD_TIMESLICE_MQ3         50


#define          ADC_DEV_NAME                  "adc1"  /* ADC 设备名称 */
#define          ADC_DEV_CHANNEL             2        /* ADC 通道 */

#define          WATER_ADC_DEV_NAME      "adc2"  /* ADC 设备名称 */
#define          WATER_ADC_DEV_CHANNEL    3    //雨滴传感器ADC通道

#define          REFER_VOLTAGE                       330         /* 参考电压 3.3V,数据精度乘以100保留2位小数*/
#define          CONVERT_BITS                      (1 << 12)   /* 转换位数为12位 */

//初始化邮箱
rt_mailbox_t  mq3_mb = RT_NULL;

//发送邮件函数
static void mq3_mb_send(rt_uint32_t  data)
{
    rt_mb_send (mq3_mb, data);
}

//接收邮件函数
void mq3_mb_recv(rt_ubase_t *value,rt_uint32_t timeout)
{
     rt_mb_recv (mq3_mb, value, timeout);
}


static rt_adc_device_t   adc_dev;
static rt_adc_device_t   water_adc_dev;


//MQ135数据采集线程
static void mq3_thread_entry(void *parameter)
{
    rt_uint32_t value, vol;

    char   *air="bad";
    char   *rain="raining";
    while(1)
    {
        room_envent_take(RT_WAITING_FOREVER);  //等待采集事件的发生
        //使能ADC通道
        rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);
        rt_adc_enable(water_adc_dev, WATER_ADC_DEV_CHANNEL);
        rt_thread_mdelay(100);
        //采集和处理MQ135的数据
        value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
        vol = value * REFER_VOLTAGE / CONVERT_BITS;

        if(vol<300) {
            rt_kprintf("bad air quality!\r\n");
            mq3_mb_send((rt_uint32_t)&air);
        }
        else {
            rt_kprintf("good air quality!\r\n");
        }
        rt_thread_mdelay(50);
        //采集和处理雨滴传感器的数据
        value = rt_adc_read(water_adc_dev, WATER_ADC_DEV_CHANNEL);
        vol = value * REFER_VOLTAGE / CONVERT_BITS;
        if(vol>300) {
            rt_kprintf("today is raining!\r\n");
            mq3_mb_send((rt_uint32_t)&rain);
        }
        else {
            rt_kprintf("today is not raining!\r\n");
        }
        rt_thread_mdelay(50);

        rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);
        rt_adc_disable(water_adc_dev, WATER_ADC_DEV_CHANNEL);
        rt_thread_mdelay(1000);
    }
}


static rt_thread_t mq3_thread = RT_NULL;
//MQ135线程初始化函数
int mq3_config(void)
{ 
    rt_err_t ret = RT_EOK;
    rt_err_t ret1 = RT_EOK;

    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
        return RT_ERROR;
    }
      /* 使能设备 */
    ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);
    
    //雨滴传感器使能
    water_adc_dev = (rt_adc_device_t)rt_device_find(WATER_ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", WATER_ADC_DEV_NAME);
        return RT_ERROR;
    }
      /* 使能设备 */
    ret1 = rt_adc_enable(water_adc_dev, WATER_ADC_DEV_CHANNEL);


    mq3_mb = rt_mb_create("mq3_mb", 32, RT_IPC_FLAG_FIFO);
    RT_ASSERT(mq3_mb != RT_NULL);

    mq3_thread = rt_thread_create("mq3_thread",
                            mq3_thread_entry, RT_NULL,
                            THREAD_STACK_SIZE_MQ3 ,
                            THREAD_PRIORITY_MQ3 , THREAD_TIMESLICE_MQ3 );
    if (mq3_thread != RT_NULL)
    rt_thread_startup(mq3_thread);
    return ret;
}
INIT_APP_EXPORT(mq3_config);
