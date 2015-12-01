/* The License
 * 
 * Copyright (c) 2015 Universidade Federal de Santa Maria
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

*/
/*
 * gc864_modem.h
 *
 *  Created on: Aug 7, 2015
 *      Author: UFSM
 */

#ifndef GC864_MODEM_H_
#define GC864_MODEM_H_

#include "OS_types.h"
#include "UART.h"
#include "terminal_io.h"
#include "stdint.h"
#include "AppConfig.h"

#include "modem.h"

// configuration 
#define MODEM_UART_BUFSIZE		32
#define MODEM_UART_TIMEOUT		10
#define MODEM_BAUD				19200

/* define uart functions*/
#define MODEM_MUTEX				FALSE
#define MODEM_MUTEX_PRIO		UART1_MUTEX_PRIO
#define modem_acquire()			uart1_acquire()
#define modem_release()			uart1_release()

#if ARDUINO
#define modem_printP(x)			printSer_P(USE_UART1,(PGM_P)pgm_read_word(&(x)));
#define modem_printR(x)			printSer(USE_UART1,(x));
#else
#define modem_printP(x)			printSer(USE_UART1,(char*)x);
#define modem_printR(x)			printSer(USE_UART1,(char*)x);
#endif

#define modem_putchar(x)		putcharSer(USE_UART1,x)
extern BRTOS_Queue 				*Serial1;
#define MODEM_QUEUE				Serial1

modem_ret_t at_modem_init(void);
modem_ret_t at_modem_open(INT8U host_or_ip, INT8U* dados);
modem_ret_t at_modem_send(INT8U* dados);
modem_ret_t at_modem_receive(CHAR8* buff, INT8U len);
modem_ret_t at_modem_close(void);
modem_ret_t at_modem_server(void);
modem_ret_t at_modem_dns(char* param);
modem_ret_t at_modem_time(void);


CHAR8 gc864_modem_getchar(void);
uint8_t gc864_modem_init(void);
uint8_t gc864_modem_open(void);
uint8_t gc864_modem_close(void);

uint8_t gc864_modem_get_time(void);
uint8_t gc864_modem_receive(uint8_t* buff, uint16_t* len);
uint8_t gc864_modem_send(uint8_t * dados, uint16_t tam);
uint8_t gc864_modem_set_ip(char* _ip);
char* gc864_modem_get_ip(void);
uint8_t gc864_modem_set_hostname(char *host);
char* gc864_modem_get_hostname(void);
uint8_t gc864_modem_host_ip(void);
uint8_t gc864_modem_check_connection(void);


#endif /* GC864_MODEM_H_ */
