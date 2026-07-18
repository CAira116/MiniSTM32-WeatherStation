#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"
#include <stdint.h>

int esp_cmd(const char *cmd, const char *keyword, uint32_t timeout_ms);
int esp8266_connect_wifi(void);
int esp8266_tcp_connect(void);
int esp8266_tcp_send(const char*data);
#endif
