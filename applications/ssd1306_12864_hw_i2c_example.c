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

#define EVENT_FLAG3 (1 << 3)    //事假3，用于控制房间的传感器数据
#define EVENT_FLAG5 (1 << 5)    //事假5，退出读取房间的传感器数据

#define  PIN_KEY0  20   //定义中断引脚为A4
#define  STACK_SIZE          4000
#define  THREAD_PRIORITY     25
#define  TIMESLICE           5

ALIGN(RT_NAME_MAX )

u8g2_t u8g2;
//定义一个邮箱控制块，用于按键中断和显示线程的通信
static    rt_mailbox_t  status1=RT_NULL;

rt_sem_t  dynamic_sem = RT_NULL;   //创建房间信息获取信号量
/* 事件控制块 */
static struct rt_event event;
rt_sem_t  weather_sem = RT_NULL;   //创建天气获取信号量

//天气的宏定义
#define      SUN                       0
#define     SUN_CLOUD         1
#define     CLOUD                  2
#define     RAIN                      3
#define     THUNDER             4

// 字体的使用:
// u8g2_font_open_iconic_embedded_6x_t
// u8g2_font_open_iconic_weather_6x_t
//使用的 encoding values, 见: https://github.com/olikraus/u8g2/wiki/fntgrpiconic

//天气显示部分
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
    now = time(RT_NULL); //得到当前的时间
    p=localtime((const time_t*) &now);  //得到本地的时间
    hour = p->tm_hour;
    min = p->tm_min;
    rt_sprintf(mstr, "%02d", min);
    rt_sprintf(hstr, "%02d", hour);

    //绘制显示的图像
    //LOG的显示
    u8g2_ClearBuffer(&u8g2);  //清空显示
    u8g2_SetFont(&u8g2,u8g2_font_open_iconic_www_1x_t);
    u8g2_DrawGlyph(&u8g2, 120, 8, 81 );
    u8g2_SetFont(&u8g2,u8g2_font_open_iconic_app_2x_t);
    u8g2_DrawGlyph(&u8g2, 0, 16, 69 );
    //时间的显示
    u8g2_SetFont(&u8g2,  u8g2_font_logisoso20_tr );
    u8g2_DrawStr(&u8g2, 35, 40,hstr );
    u8g2_DrawStr(&u8g2, 60, 40,":" );
    u8g2_DrawStr(&u8g2, 66, 40,mstr);
    u8g2_SendBuffer(&u8g2);  //显示图案
}


static void display_room_state(void)
{
    char temperature_str[3];
    char humidity_str[3];
    char *rev_str;
    char   *air="bad";
    char   *rain="raining";

    mq3_mb_recv((rt_ubase_t *)rev_str,RT_WAITING_NO); //接收发送过来的邮件

    //将数据转化为char类型的
    rt_sprintf(temperature_str, "%02d",  (int)temperature);
    rt_sprintf(humidity_str, "%02d",  (int)humidity);
    //显示基础的图标
    u8g2_ClearBuffer(&u8g2);  //显示
    u8g2_SetFont(&u8g2,u8g2_font_open_iconic_www_1x_t);
    u8g2_DrawGlyph(&u8g2, 120, 8, 81 );

    u8g2_SetFont(&u8g2,u8g2_font_streamline_weather_t);
    u8g2_DrawGlyph(&u8g2, 0,64 ,54 ); //显示温度计图标功能

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

    u8g2_SendBuffer(&u8g2);  //显示图案
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

    //显示基础的图标
//    u8g2_ClearBuffer(&u8g2);  //显示
//    u8g2_SetFont(&u8g2,u8g2_font_open_iconic_www_1x_t);
//    u8g2_DrawGlyph(&u8g2, 120, 8, 81 );

    weather_status = rt_mb_recv(&weather_mb,           // 邮箱对象句柄
                      (rt_ubase_t *)&weather_str,                    // 接收邮箱信息
                      RT_WAITING_NO );                // 等待接收邮箱信息
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

/* OLED显示屏线程入口函数 */
static void oled_thread_entry(void *parameter)
{
    rt_err_t uwRet = RT_EOK;
    int status=0; //显示的页面标志位
    char *r_str;
    //初步显示的内容
    //画已经连接上WIFI的图案
    while (1)
    {
        // 等待接收邮箱信息
        uwRet = rt_mb_recv(status1,                          // 邮箱对象句柄
                          (rt_ubase_t *)&r_str,                    // 接收邮箱信息
                          RT_WAITING_NO );                // 等待接收邮箱信息
        if(uwRet == RT_EOK)
        {
            rt_kprintf("KEY0 pressed%s\r\n", r_str);
            status++;
            if (status==1) {
                rt_event_send(&event, EVENT_FLAG3);   //发送事件3
            }
            if (status==2) {
                rt_event_send(&event, EVENT_FLAG5);   //发送事件5
                rt_sem_release(weather_sem);  //释放天气获取函数的信号量
            }
            if (status==3) {
                status=0;
            }
        }

        switch (status) {
            case 0:
                display_clock() ;                            //显示时钟功能
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
    // 初始化函数
    u8g2_Setup_ssd1306_i2c_128x64_noname_f( &u8g2, U8G2_R0, u8x8_byte_rtthread_hw_i2c, u8x8_gpio_and_delay_rtthread);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

   //创建邮箱
    status1=rt_mb_create("oleddisplay", 4, RT_IPC_FLAG_PRIO);
    rt_thread_t oled_thread=RT_NULL;

    /* 创建线程 1，名称是 thread1，入口是 thread1_entry*/
    oled_thread = rt_thread_create("oled_thread",
                             oled_thread_entry, RT_NULL,
                            1024,
                            16, 50);

    /* 如果获得线程控制块，启动这个线程 */
    if (oled_thread != RT_NULL)
        rt_thread_startup(oled_thread);

    return 0;
}
//INIT_DEVICE_EXPORT(app_oled_init);  //OLED设备初始化



/* 中断回调函数 ---------------------------------------------------------------------------- */
static void hdr_callback(void *args)
{
    while(!(rt_pin_read(PIN_KEY0)));
    char *str = args;
//    rt_kprintf("KEY0 pressed%s\n", str);
    rt_mb_send(status1,(rt_ubase_t)str);
}

/* irq线程入口函数 -------------------------------------------------------------------------*/
static void irq_thread_entry(void *parameter)
{
    /* 配置KEY0引脚为下拉输入 */
    rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT_PULLDOWN);
    /* 绑定中断回调函数，下降沿模式，回调函数参数为字符串"--By ZhengNian" */
    rt_pin_attach_irq(PIN_KEY0, PIN_IRQ_MODE_FALLING, hdr_callback, (void*)"--By yijianpeng");
    /* 使能引脚中断 */
    rt_pin_irq_enable(PIN_KEY0, PIN_IRQ_ENABLE);
}

//信号量的获取函数
void room_envent_take(rt_int32_t time)
{
      rt_uint32_t e;
     /* 第一次接收事件，事件 3 可以触发线程 */
     if (rt_event_recv(&event, (EVENT_FLAG3),
                         RT_EVENT_FLAG_AND,
                         time, &e) == RT_EOK)
     {
         rt_kprintf("recv event \n");
     }

     /* 第一次接收事件，事件 3 可以触发线程 */
     if (rt_event_recv(&event, (EVENT_FLAG3 | EVENT_FLAG5),
                         RT_EVENT_FLAG_AND| RT_EVENT_FLAG_CLEAR,
                         RT_WAITING_NO, &e) == RT_EOK)
     {
         rt_kprintf("clear event !\n", e);
     }

}

//信号量的获取函数
void weather_sem_take(rt_int32_t time)
{
    static rt_err_t result;
    /* 永久方式等待信号量，获取到信号量，则执行 number 自加的操作 */
     result = rt_sem_take(weather_sem, time);
     if (result != RT_EOK)
     {
         rt_kprintf("take a weather semaphore, failed.\n");
     }
}

/* 创建中断函数*/
int app_irq(void)
{
     rt_err_t result;
    /* 创建一个动态信号量，初始值是 0 */
    weather_sem = rt_sem_create("wsem", 0, RT_IPC_FLAG_PRIO);
    if (weather_sem == RT_NULL)
    {
        rt_kprintf("create weather semaphore failed.\n");
        return -1;
    }

    /* 初始化事件对象 */
    result = rt_event_init(&event, "event", RT_IPC_FLAG_PRIO);
    if (result != RT_EOK)
    {
        rt_kprintf("init event failed.\n");
        return -1;
    }
    /* 定义线程句柄 */
    rt_thread_t tid;
    /* 创建动态pin线程 ：优先级 25 ，时间片 5个系统滴答，线程栈512字节 */
    tid = rt_thread_create("irq_thread",
                  irq_thread_entry,
                  RT_NULL,
                  STACK_SIZE,
                  THREAD_PRIORITY,
                  TIMESLICE);
    /* 创建成功则启动动态线程 */
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    return 0;
}
INIT_APP_EXPORT(app_irq);

