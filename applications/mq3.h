#ifndef __MQ3_H__
#define __MQ3_H__
#include <rtthread.h>

int mq3_config(void);
void mq3_mb_recv(rt_ubase_t *value,rt_uint32_t timeout);

extern rt_uint32_t   vol;
#endif
