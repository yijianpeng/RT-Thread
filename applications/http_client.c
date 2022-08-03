#include <rtthread.h>
#include <webclient.h>
#include <string.h>
#include "cJSON_util.h"
#include <board.h>
#include <arpa/inet.h> 
#include <netdev.h>
#include "ssd1306_12864_hw_i2c_example.h"
#include "http_client.h"
#include "http_util.h"

#define DBG_COLOR
#define DBG_TAG "http_client"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THREAD_PRIORITY          18
#define THREAD_STACK_SIZE      2048
#define THREAD_TIMESLICE        5

#define GET_HEADER_BUFSZ        2048
#define GET_RESP_BUFSZ          2048
#define GET_RESP_CONTENTSZ      4096
#define URL_LEN_MAX             2048


#define GET_LOCAL_URI    "http://api.seniverse.com/v3/weather/now.json?key=SBsiRxuCRBe5xizs8&location=%s&language=en&unit=c"
#define LOCAL_CITY              "maoming"


static rt_thread_t thread_id = RT_NULL;
//struct rt_mailbox weather_mb;
struct rt_mailbox weather_mb;

static char mb_pool[16];

static http_weather_info_t weather_info;

static http_weather_info_t weather_info1;
/*
 strncpy(weather_info[i].humidity, list->valuestring,strlen(list->valuestring))

}
*/

//更改为实时的气候信息
static void weather_data_parse(char* data, unsigned char debug)
{
    cJSON *root = RT_NULL, *arrayItem = RT_NULL,*object = RT_NULL;
    cJSON *daily = RT_NULL, *list = RT_NULL, *item = RT_NULL;
    cJSON *weather_list;
    root = cJSON_Parse((const char *)data);
    if (!root)
    {
        LOG_E("No memory for cJSON root!\n");
        return;
    }

    arrayItem = cJSON_GetObjectItem(root, "results");
    if (arrayItem == RT_NULL)
    {
        LOG_E("cJSON get results failed.");
        goto __EXIT;
    }

    object = cJSON_GetArrayItem(arrayItem, 0);
    if (object == RT_NULL)
    {
        LOG_E("cJSON get object failed.");
        goto __EXIT;
    }

    item = cJSON_GetObjectItem(object, "location");
    if (item == RT_NULL)
    {
        LOG_E("cJSON get location failed.");
        goto __EXIT;
    }

    if ((list = cJSON_GetObjectItem(item, "name")) != NULL)
    {
        if (debug)
        {
            rt_kprintf("city:%s\r\n",list->valuestring);
        }
    }

    daily = cJSON_GetObjectItem(object, "now");
    if (daily == RT_NULL)
    {
        LOG_E("cJSON get dayItem failed.");
        goto __EXIT;
    }

    if ((list = cJSON_GetObjectItem(daily, "text")) != NULL)
    {
        if (debug)
        {
          strcpy(weather_info.weather, "");
          strncpy(weather_info.weather, list->valuestring,strlen(list->valuestring));
          rt_kprintf("weather:%s\r\n",weather_info.weather);
        }
     }
//    if ((list = cJSON_GetObjectItem(daily, "temperature")) != NULL)
//    {
//        if (debug)
//        {
//            strncpy(weather_info.temperature, list->valuestring,strlen(list->valuestring));
//            rt_kprintf("temperature: %s\r\n",weather_info.temperature);
//        }
//     }

__EXIT:
   if (root != RT_NULL)
       cJSON_Delete(root);
}


static int http_weather_request(int argc, char **argv)
{
    struct webclient_session* session = RT_NULL;
    unsigned char *buffer = RT_NULL;
    char *url = RT_NULL;
    int index, ret = 0;
    int bytes_read, resp_status;
    int content_length = -1;
    char *city_name = rt_calloc(1,255);
    unsigned char debug = 0;
    int content_pos = 0;

    if (argc == 1)
    {
        strcpy(city_name, LOCAL_CITY);
        http_urlencode(city_name);
        debug = 1;
    }
    else if (argc == 2)
    {
        strcpy(city_name, argv[1]);
        http_urlencode(city_name);
        debug = 1;
    }
    else if(argc > 2)
    {
        rt_kprintf("wt [CityName]  - http weather request.\n");
        return -1;
    }

    url = rt_calloc(1, URL_LEN_MAX);
    rt_snprintf(url, URL_LEN_MAX, GET_LOCAL_URI, city_name);
    buffer = (unsigned char *) web_malloc(GET_RESP_CONTENTSZ);
    if (buffer == RT_NULL)
    {
        LOG_E("no memory for receive buffer.\n");
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* create webclient session and set header response size */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        ret = -RT_ENOMEM;
        goto __exit;
    }

    if (debug)
    {
        /* send GET request by default header */
        rt_kprintf("%s\n", url);
    }

    if ((resp_status = webclient_get(session, url)) != 200)
    {
        LOG_E("webclient_get failed, response(%d) error.\n", resp_status);
        ret = -RT_ERROR;
        goto __exit;
    }

    content_length = webclient_content_length_get(session);
    if (content_length < 0)
    {
        rt_kprintf("webclient_content_length_get is chunked.\n");
        do
        {
            bytes_read = webclient_read(session, buffer, GET_RESP_BUFSZ);
            if (bytes_read <= 0)
            {
                break;
            }

            for (index = 0; index < bytes_read; index++)
            {
                rt_kprintf("%c", buffer[index]);
            }
        } while (1);
        rt_kprintf("\n");
    }
    else
    {
        do
        {
            bytes_read = webclient_read(session, buffer+content_pos, GET_RESP_BUFSZ);
            if (bytes_read <= 0)
            {
                break;
            }
            content_pos += bytes_read;
        } while (content_pos < content_length);

        weather_data_parse((char*)buffer, debug);
    }

__exit:
    if (session)
    {
        webclient_close(session);
    }

    if (buffer)
    {
        web_free(buffer);
    }

    if (url)
    {
        web_free(url);
    }

    if (city_name)
    {
        rt_free(city_name);
    }

    return ret;
}

//http_weather_info_t *http_weather_info()
//{
//    if (rt_mb_recv(&weather_mb, (rt_ubase_t *)&weather_info, RT_WAITING_NO) == RT_EOK)
//    {
//        return weather_info;
//    }
//    else
//    {
//        return RT_NULL;
//    }
//}

static void http_weather_entry(void *parameter)
{
    while(1)
    {
        weather_sem_take(RT_WAITING_FOREVER);
        http_weather_request(1, 0);
        rt_mb_send(&weather_mb, (rt_ubase_t)weather_info.weather);
        rt_thread_mdelay(10*1000);
    }
}

int http_client_weather(void)
{
    rt_err_t result = RT_EOK;
    result = rt_mb_init(&weather_mb,
                        "http_mbt",
                        &mb_pool[0],
                        sizeof(mb_pool) / 4,
                        RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        LOG_E("rt_mb_init failed.\n");
        return -1;
    }

    thread_id = rt_thread_create("weather",
            http_weather_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (thread_id != RT_NULL)
    {
        rt_thread_startup(thread_id);
    }

    return 0;
}
//INIT_APP_EXPORT(http_client_weather);

#ifdef  __cplusplus
}
#endif
