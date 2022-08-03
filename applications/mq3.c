#include "mq3.h"
#include <rtdevice.h>
#include <stdint.h>
#include <rtthread.h>
#include "drivers/adc.h"
#include "ssd1306_12864_hw_i2c_example.h"

#define     THREAD_PRIORITY_MQ3         13
#define     THREAD_STACK_SIZE_MQ3       512
#define     THREAD_TIMESLICE_MQ3         50


#define          ADC_DEV_NAME                  "adc1"  /* ADC �豸���� */
#define          ADC_DEV_CHANNEL             2        /* ADC ͨ�� */

#define          WATER_ADC_DEV_NAME      "adc2"  /* ADC �豸���� */
#define          WATER_ADC_DEV_CHANNEL    3    //��δ�����ADCͨ��

#define          REFER_VOLTAGE                       330         /* �ο���ѹ 3.3V,���ݾ��ȳ���100����2λС��*/
#define          CONVERT_BITS                      (1 << 12)   /* ת��λ��Ϊ12λ */

//��ʼ������
rt_mailbox_t  mq3_mb = RT_NULL;

//�����ʼ�����
static void mq3_mb_send(rt_uint32_t  data)
{
    rt_mb_send (mq3_mb, data);
}

//�����ʼ�����
void mq3_mb_recv(rt_ubase_t *value,rt_uint32_t timeout)
{
     rt_mb_recv (mq3_mb, value, timeout);
}


static rt_adc_device_t   adc_dev;
static rt_adc_device_t   water_adc_dev;


//MQ135���ݲɼ��߳�
static void mq3_thread_entry(void *parameter)
{
    rt_uint32_t value, vol;

    char   *air="bad";
    char   *rain="raining";
    while(1)
    {
        room_envent_take(RT_WAITING_FOREVER);  //�ȴ��ɼ��¼��ķ���
        //ʹ��ADCͨ��
        rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);
        rt_adc_enable(water_adc_dev, WATER_ADC_DEV_CHANNEL);
        rt_thread_mdelay(100);
        //�ɼ��ʹ���MQ135������
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
        //�ɼ��ʹ�����δ�����������
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
//MQ135�̳߳�ʼ������
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
      /* ʹ���豸 */
    ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);
    
    //��δ�����ʹ��
    water_adc_dev = (rt_adc_device_t)rt_device_find(WATER_ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", WATER_ADC_DEV_NAME);
        return RT_ERROR;
    }
      /* ʹ���豸 */
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
