/*
 * simon-api.h
 *
 *  Created on: Jul 22, 2015
 *      Author: Universidade Federal
 */

#ifndef SIMON_API_H_
#define SIMON_API_H_

#ifndef _WIN32
#include "BRTOS.h"
#endif

#include "time_lib.h"
#include "stdint.h"

#define API_KEY  "90a004390f3530d0ba10199ac2b1ac3d"
#define SERVER_NAME "emon-gpsnetcms.rhcloud.com"

#ifndef OK
#define OK 		(1)
#endif

#define MAX_HOSTNAME_LEN	 (32+1)
#define MAX_APIKEY_LEN	 	 (32+1)

typedef uint8_t (*input)(uint8_t *, uint16_t *);
typedef uint8_t (*output)(uint8_t *, uint16_t);
typedef uint8_t (*set_host)(char *);

uint8_t simon_init(input _in, output _out, set_host sethost);
uint8_t simon_get_time(struct tm * t);
uint8_t simon_send_data(uint8_t *buf, uint16_t len);
char* simon_get_apikey(void);
char* simon_get_hostname(void);
void simon_set_apikey(const char*);
void simon_set_hostname(const char*);

#endif /* SIMON_API_H_ */