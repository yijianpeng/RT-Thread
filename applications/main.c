#include <stdint.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <device_aht10.h>
#include <ntp.h>
#include "aht10.h"
#include "sensor_asair_aht10.h"
#include <netdev_ipaddr.h>
#include <netdev.h>            /* 包含全部的 netdev 相关操作接口函数 */
#include <mq3.h>
#include "http_client.h"
#include "ssd1306_12864_hw_i2c_example.h"

/* defined the LED1 pin: PB5 */
#define LED1_PIN    57


int rt_hw_aht10_port(void)
{
    struct rt_sensor_config cfg;
    cfg.intf.dev_name  = AHT10_I2C_BUS;
    cfg.intf.user_data = (void *)AHT10_I2C_ADDR;
    rt_hw_aht10_init("aht10", &cfg);
    return RT_EOK;
}
INIT_ENV_EXPORT(rt_hw_aht10_port);


int main(void)
{

    uint32_t Speed = 200;
    /* set LED1 pin mode to output */
    rt_pin_mode(LED1_PIN, PIN_MODE_OUTPUT);
    //获取网卡对象
   struct netdev* net = netdev_get_by_name("esp0");
   //阻塞判断当前网络是否正常连接
   while(netdev_is_internet_up(net) != 1)
   {
      rt_thread_mdelay(10);  //10ms判断1次
   }
   //提示当前网络已就绪
   rt_kprintf("network is ok!\n");
   //NTP自动对时
   time_t cur_time;
   cur_time = ntp_sync_to_rtc(NULL);//设置网络时间
   if (cur_time)
   {
       rt_kprintf("Cur Time: %s", ctime((const time_t*) &cur_time));
   }
   else
   {
       rt_kprintf("NTP sync fail.\n");
   }
   int t;
  int k;
   k=app_oled_init();
   t=app_aht10_init();
   t= http_client_weather();
    while (1)
    {
        rt_pin_write(LED1_PIN, PIN_LOW);
        rt_thread_mdelay(Speed);
        rt_pin_write(LED1_PIN, PIN_HIGH);
        rt_thread_mdelay(Speed);
    }
}

