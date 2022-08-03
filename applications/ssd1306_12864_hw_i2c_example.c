#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <u8g2_port.h>
#include <ntp.h>
#include   <stdio.h>
#include <device_aht10.h>
#include "http_client.h"
#include "http_util.h"
#include "mq3.h"
#include "ssd1306_12864_hw_i2c_example.h"

#define EVENT_FLAG3 (1 << 3)    //�¼�3�����ڿ��Ʒ���Ĵ���������
#define EVENT_FLAG5 (1 << 5)    //�¼�5���˳���ȡ����Ĵ���������

#define  PIN_KEY0  20   //�����ж�����ΪA4
#define  STACK_SIZE          4000
#define  THREAD_PRIORITY     25
#define  TIMESLICE           5

ALIGN(RT_NAME_MAX )

u8g2_t u8g2;
//����һ��������ƿ飬���ڰ����жϺ���ʾ�̵߳�ͨ��
static    rt_mailbox_t  status1=RT_NULL;

rt_sem_t  dynamic_sem = RT_NULL;   //����������Ϣ��ȡ�ź���
/* �¼����ƿ� */
static struct rt_event event;
rt_sem_t  weather_sem = RT_NULL;   //����������ȡ�ź���

//�����ĺ궨��
#define      SUN                       0
#define     SUN_CLOUD         1
#define     CLOUD                  2
#define     RAIN                      3
#define     THUNDER             4

// �����ʹ��:
// u8g2_font_open_iconic_embedded_6x_t
// u8g2_font_open_iconic_weather_6x_t
//ʹ�õ� encoding values, ��: https://github.com/olikraus/u8g2/wiki/fntgrpiconic

//������ʾ����
static void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{
      switch(symbol)
      {
        case SUN:
          u8g2_ClearBuffer(&u8g2);
          u8g2_SetFont(&u8g2, u8g2_font_open_iconic_weather_4x_t);
          u8g2_DrawGlyph(&u8g2, x, y, 69 );
          u8g2_SendBuffer(&u8g2);
          break;
        case SUN_CLOUD:
         u8g2_ClearBuffer(&u8g2);
         u8g2_SetFont(&u8g2, u8g2_font_open_iconic_weather_4x_t);
         u8g2_DrawGlyph(&u8g2, x, y, 65 );
         u8g2_SendBuffer(&u8g2);
          break;
        case CLOUD:
            u8g2_ClearBuffer(&u8g2);
            u8g2_SetFont(&u8g2, u8g2_font_open_iconic_weather_4x_t);
            u8g2_DrawGlyph(&u8g2, x, y, 64 );
            u8g2_SendBuffer(&u8g2);
          break;
        case RAIN:
            u8g2_ClearBuffer(&u8g2);
            u8g2_SetFont(&u8g2, u8g2_font_open_iconic_weather_4x_t);
            u8g2_DrawGlyph(&u8g2, x, y, 67 );
            u8g2_SendBuffer(&u8g2);
          break;
        case THUNDER:
          u8g2_ClearBuffer(&u8g2);
          u8g2_SetFont(&u8g2,u8g2_font_open_iconic_other_4x_t);
          u8g2_DrawGlyph(&u8g2, x, y, 64 );
          u8g2_SendBuffer(&u8g2);
          break;
      }
}

static void display_clock(void)
{
    time_t now;
    int min = 0, hour = 0;
    struct tm *p;
    char mstr[3];
    char hstr[3];
    now = time(RT_NULL); //�õ���ǰ��ʱ��
    p=localtime((const time_t*) &now);  //�õ����ص�ʱ��
    hour = p->tm_hour;
    min = p->tm_min;
    rt_sprintf(mstr, "%02d", min);
    rt_sprintf(hstr, "%02d", hour);

    //������ʾ��ͼ��
    //LOG����ʾ
    u8g2_ClearBuffer(&u8g2);  //�����ʾ
    u8g2_SetFont(&u8g2,u8g2_font_open_iconic_www_1x_t);
    u8g2_DrawGlyph(&u8g2, 120, 8, 81 );
    u8g2_SetFont(&u8g2,u8g2_font_open_iconic_app_2x_t);
    u8g2_DrawGlyph(&u8g2, 0, 16, 69 );
    //ʱ�����ʾ
    u8g2_SetFont(&u8g2,  u8g2_font_logisoso20_tr );
    u8g2_DrawStr(&u8g2, 35, 40,hstr );
    u8g2_DrawStr(&u8g2, 60, 40,":" );
    u8g2_DrawStr(&u8g2, 66, 40,mstr);
    u8g2_SendBuffer(&u8g2);  //��ʾͼ��
}


static void display_room_state(void)
{
    char temperature_str[3];
    char humidity_str[3];
    char *rev_str;
    char   *air="bad";
    char   *rain="raining";

    mq3_mb_recv((rt_ubase_t *)rev_str,RT_WAITING_NO); //���շ��͹������ʼ�

    //������ת��Ϊchar���͵�
    rt_sprintf(temperature_str, "%02d",  (int)temperature);
    rt_sprintf(humidity_str, "%02d",  (int)humidity);
    //��ʾ������ͼ��
    u8g2_ClearBuffer(&u8g2);  //��ʾ
    u8g2_SetFont(&u8g2,u8g2_font_open_iconic_www_1x_t);
    u8g2_DrawGlyph(&u8g2, 120, 8, 81 );

    u8g2_SetFont(&u8g2,u8g2_font_streamline_weather_t);
    u8g2_DrawGlyph(&u8g2, 0,64 ,54 ); //��ʾ�¶ȼ�ͼ�깦��

    u8g2_SetFont(&u8g2,  u8g2_font_t0_18_tf );
    u8g2_DrawStr(&u8g2, 24, 50,temperature_str);
    u8g2_DrawGlyph(&u8g2, 39, 50, 176 );
    u8g2_DrawStr(&u8g2, 24, 64,humidity_str);

    u8g2_SetFont(&u8g2,  u8g2_font_t0_13_tf );
    u8g2_DrawStr(&u8g2, 0, 23,"air quality:");
    if (rt_strcmp(rev_str, air)==0) {
        u8g2_DrawStr(&u8g2, 90, 23,"bad");
    }
    else {
        u8g2_DrawStr(&u8g2, 90, 23,"good");
    }

    u8g2_SendBuffer(&u8g2);  //��ʾͼ��
}



static void display_weather(void)
{
    rt_err_t weather_status = RT_EOK;
    char *weather_str;
    char *weather_str1= "Cloudy";
    char *weather_str2= "Overcast";
    char *weather_str5= "Overcast34";
    char *weather_str3= "Sun";
    char *weather_str4= "Rain";
    char *weather_str6="Light rain";
    char *weather_str7="Moderate rain";

    //��ʾ������ͼ��
//    u8g2_ClearBuffer(&u8g2);  //��ʾ
//    u8g2_SetFont(&u8g2,u8g2_font_open_iconic_www_1x_t);
//    u8g2_DrawGlyph(&u8g2, 120, 8, 81 );

    weather_status = rt_mb_recv(&weather_mb,           // ���������
                      (rt_ubase_t *)&weather_str,                    // ����������Ϣ
                      RT_WAITING_NO );                // �ȴ�����������Ϣ
    if(weather_status == RT_EOK)
    {
        if (rt_strcmp(weather_str,weather_str1)==0) {
                                rt_kprintf("today is %s !\r\n",weather_str1);
                                drawWeatherSymbol(55,50,SUN_CLOUD);
            }
        else  if ((rt_strcmp(weather_str,weather_str2)==0)||(rt_strcmp(weather_str,weather_str5)==0)) {
                                rt_kprintf("today is %s !\r\n",weather_str2);
                                drawWeatherSymbol(55,50,CLOUD );
            }
        else  if (rt_strcmp(weather_str,weather_str3)==0) {
                                rt_kprintf("today is %s !\r\n",weather_str3);
                                drawWeatherSymbol(55,50, SUN);
            }
        else  if ((rt_strcmp(weather_str,weather_str4)==0)||(rt_strcmp(weather_str,weather_str6)==0)||(rt_strcmp(weather_str,weather_str7)==0)) {
                                rt_kprintf("today is %s !\r\n",weather_str4);
                                drawWeatherSymbol(55,50, RAIN);
            }
    }

}

/* OLED��ʾ���߳���ں��� */
static void oled_thread_entry(void *parameter)
{
    rt_err_t uwRet = RT_EOK;
    int status=0; //��ʾ��ҳ���־λ
    char *r_str;
    //������ʾ������
    //���Ѿ�������WIFI��ͼ��
    while (1)
    {
        // �ȴ�����������Ϣ
        uwRet = rt_mb_recv(status1,                          // ���������
                          (rt_ubase_t *)&r_str,                    // ����������Ϣ
                          RT_WAITING_NO );                // �ȴ�����������Ϣ
        if(uwRet == RT_EOK)
        {
            rt_kprintf("KEY0 pressed%s\r\n", r_str);
            status++;
            if (status==1) {
                rt_event_send(&event, EVENT_FLAG3);   //�����¼�3
            }
            if (status==2) {
                rt_event_send(&event, EVENT_FLAG5);   //�����¼�5
                rt_sem_release(weather_sem);  //�ͷ�������ȡ�������ź���
            }
            if (status==3) {
                status=0;
            }
        }

        switch (status) {
            case 0:
                display_clock() ;                            //��ʾʱ�ӹ���
//                rt_kprintf("%d\r\n",status);
                break;
            case 1:
                display_room_state() ;
                break;
            case 2:
                display_weather();
                break;
            default:
                break;
        }
     rt_thread_mdelay(500);
    }
}
ALIGN(RT_ALIGN_SIZE)


int  app_oled_init(void)
{
    // ��ʼ������
    u8g2_Setup_ssd1306_i2c_128x64_noname_f( &u8g2, U8G2_R0, u8x8_byte_rtthread_hw_i2c, u8x8_gpio_and_delay_rtthread);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

   //��������
    status1=rt_mb_create("oleddisplay", 4, RT_IPC_FLAG_PRIO);
    rt_thread_t oled_thread=RT_NULL;

    /* �����߳� 1�������� thread1������� thread1_entry*/
    oled_thread = rt_thread_create("oled_thread",
                             oled_thread_entry, RT_NULL,
                            1024,
                            16, 50);

    /* �������߳̿��ƿ飬��������߳� */
    if (oled_thread != RT_NULL)
        rt_thread_startup(oled_thread);

    return 0;
}
//INIT_DEVICE_EXPORT(app_oled_init);  //OLED�豸��ʼ��



/* �жϻص����� ---------------------------------------------------------------------------- */
static void hdr_callback(void *args)
{
    while(!(rt_pin_read(PIN_KEY0)));
    char *str = args;
//    rt_kprintf("KEY0 pressed%s\n", str);
    rt_mb_send(status1,(rt_ubase_t)str);
}

/* irq�߳���ں��� -------------------------------------------------------------------------*/
static void irq_thread_entry(void *parameter)
{
    /* ����KEY0����Ϊ�������� */
    rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT_PULLDOWN);
    /* ���жϻص��������½���ģʽ���ص���������Ϊ�ַ���"--By ZhengNian" */
    rt_pin_attach_irq(PIN_KEY0, PIN_IRQ_MODE_FALLING, hdr_callback, (void*)"--By yijianpeng");
    /* ʹ�������ж� */
    rt_pin_irq_enable(PIN_KEY0, PIN_IRQ_ENABLE);
}

//�ź����Ļ�ȡ����
void room_envent_take(rt_int32_t time)
{
      rt_uint32_t e;
     /* ��һ�ν����¼����¼� 3 ���Դ����߳� */
     if (rt_event_recv(&event, (EVENT_FLAG3),
                         RT_EVENT_FLAG_AND,
                         time, &e) == RT_EOK)
     {
         rt_kprintf("recv event \n");
     }

     /* ��һ�ν����¼����¼� 3 ���Դ����߳� */
     if (rt_event_recv(&event, (EVENT_FLAG3 | EVENT_FLAG5),
                         RT_EVENT_FLAG_AND| RT_EVENT_FLAG_CLEAR,
                         RT_WAITING_NO, &e) == RT_EOK)
     {
         rt_kprintf("clear event !\n", e);
     }

}

//�ź����Ļ�ȡ����
void weather_sem_take(rt_int32_t time)
{
    static rt_err_t result;
    /* ���÷�ʽ�ȴ��ź�������ȡ���ź�������ִ�� number �ԼӵĲ��� */
     result = rt_sem_take(weather_sem, time);
     if (result != RT_EOK)
     {
         rt_kprintf("take a weather semaphore, failed.\n");
     }
}

/* �����жϺ���*/
int app_irq(void)
{
     rt_err_t result;
    /* ����һ����̬�ź�������ʼֵ�� 0 */
    weather_sem = rt_sem_create("wsem", 0, RT_IPC_FLAG_PRIO);
    if (weather_sem == RT_NULL)
    {
        rt_kprintf("create weather semaphore failed.\n");
        return -1;
    }

    /* ��ʼ���¼����� */
    result = rt_event_init(&event, "event", RT_IPC_FLAG_PRIO);
    if (result != RT_EOK)
    {
        rt_kprintf("init event failed.\n");
        return -1;
    }
    /* �����߳̾�� */
    rt_thread_t tid;
    /* ������̬pin�߳� �����ȼ� 25 ��ʱ��Ƭ 5��ϵͳ�δ��߳�ջ512�ֽ� */
    tid = rt_thread_create("irq_thread",
                  irq_thread_entry,
                  RT_NULL,
                  STACK_SIZE,
                  THREAD_PRIORITY,
                  TIMESLICE);
    /* �����ɹ���������̬�߳� */
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    return 0;
}
INIT_APP_EXPORT(app_irq);

