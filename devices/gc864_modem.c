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
 * gc864_modem.c
 *
 *  Created on: Aug 7, 2015
 *      Author: UFSM
 */

#include "AppConfig.h"
#include "BRTOS.h"
#include "gc864_modem.h"
#include "printf_lib.h"
#include "utils.h"
#include "terminal.h"
#include "terminal_commands.h"
#include "string.h"

#include "at_commands.h"

#include "simon-api.h"
#include "modem.h"
#include "monitor.h"

#ifdef _WIN32
#include <stdio.h>
#endif


static state_t modem_state = SETUP;
static char ip[16];
static char ip_resolved = 0;
static char *hostname = NULL;
static char modem_BufferTxRx[64];

extern uint8_t mon_verbosity;
uint16_t modem_watchdog = 0;

#define MAX_MODEM_ERRORS 0
#define DEBUG_PRINT 1

#if DEBUG_PRINT
/* print outputs */
#undef PRINTS_ENABLED
#define PRINTS_ENABLED  1
#include "prints_def.h"

#define PRINT_INFO()	  PRINTF_P(PSTR("Line %d: "), __LINE__);
#define PRINT_P(...)      printf_terminal_P(__VA_ARGS__);
#define PRINT(s) 		  if(*(s)) { PRINT_INFO();printf_terminal((char*)(s));}
#define PRINT_BUF(s)      if(*(s)) { PRINT_INFO();printf_terminal((char*)(s));}
#define PRINT_REPLY(...)  printf_terminal(__VA_ARGS__);
#else
#define PRINT_P(...)      
#define PRINT(...)
#define PRINT_BUF(...)
#define PRINT_REPLY(...)  
#endif

#define UNUSED(x)   (void)x;

#define DEBUG_PUTCHAR	0

#if DEBUG_PUTCHAR
#define DPUTCHAR(x)		putchar_terminal(x);
#else
#define DPUTCHAR(x)		
#endif

#define CONST 		const
#define MAX_RETRIES  (3)

uint8_t Check_Connect_PPP(uint8_t retries);
uint8_t Check_Connection(uint8_t retries);

/* return modem reply */
uint8_t modem_getchar(void)
{
	uint8_t caracter;
	if(OSQueuePend(MODEM_QUEUE, &caracter, MODEM_UART_TIMEOUT) == TIMEOUT)
	{
		return (uint8_t) (-1);
	}
	return (uint8_t)caracter;
}

static INT8U modem_get_reply(char *buf, uint16_t max_len)
{
	uint8_t c;
	uint8_t len = 0;
	
	if(buf!= NULL)
	{
		memset(buf,0x00,max_len); // clear buffer
	}
	while((c=modem_getchar()) != (uint8_t)-1)
	{
		if(buf!= NULL)
		{
			*buf = c;
			buf++;
		}
		len++;
		if(--max_len <= 1)
		{
			break;
		}
	}
	return len;
}

static void modem_get_reply_print(char *buf, uint16_t max_len)
{
	modem_get_reply(buf, max_len);
	if(mon_verbosity > 4 && is_terminal_idle()) PRINT_BUF(buf);
}

static void wait_modem_get_reply(uint16_t time)
{
	DelayTask(time);
	modem_get_reply(modem_BufferTxRx,(INT16U)sizeof(modem_BufferTxRx)-1);
}

#define MODEM_GET_REPLY(b)			modem_get_reply((b), sizeof(b)-1);
#define MODEM_GET_REPLY_PRINT(b)	modem_get_reply_print((b), (sizeof(b)-1));

INT8U is_modem_ok(void)
{
	INT8U ok = FALSE;
	modem_acquire();	
		if(mon_verbosity > 4 && is_terminal_idle()) PRINTS_PP(modem_init_cmd[AT]);	
		modem_printP(modem_init_cmd[AT]);
		DelayTask(10);
		MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
		if(modem_BufferTxRx[0] != 0)
		{
			if(strstr(modem_BufferTxRx, "OK"))
			{
				ok = TRUE;
			}else
			{
				if(strstr(modem_BufferTxRx, "NO CARRIER"))
				{
					if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
				}				
				//DelayTask(10);
				//OSCleanQueue(MODEM_QUEUE);
			}
		}
	modem_release();
	return ok;
}

uint8_t is_modem_ok_retry(uint8_t retries, uint16_t timeout)
{
	while(is_modem_ok() == FALSE)
	{
		if(--retries == 0)
		{
			return FALSE;
		}
		DelayTask(timeout);
	}
	return TRUE;
}

uint8_t gc864_modem_init(void)
{
	
	PRINTS_P(PSTR("Modem init \r\n"));
	
	if(modem_state == SETUP)
	{
		/* init MODEM_UART */
		#if COLDUINO
		#define BAUD(x)			(x)
		#endif
		uart_init(MODEM_UART,BAUD(MODEM_BAUD),MODEM_UART_BUFSIZE,MODEM_MUTEX,MODEM_MUTEX_PRIO);
	}

	if(is_modem_ok_retry(MAX_RETRIES, 100) == FALSE)
	{
		return MODEM_ERR;
	}

    PRINTS_P(PSTR("Modem setup\r\n"));
	
	modem_acquire();
	
    /* setup */
	
	modem_printPP(PSTR("AT&K=0\r\n")); /* disable flow control */
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
	
	modem_printPP(PSTR("ATS12=2\r\n")); /* prompt time = 2 x 20ms */
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
	
	modem_printP(modem_init_cmd[GPRS0]);
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
	
	modem_printP(modem_init_cmd[CREG]);
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
	
	//modem_printP(modem_init_cmd[CGDCONT]);
	extern char simon_gprs_server[];
	SNPRINTF_P(modem_BufferTxRx,sizeof(modem_BufferTxRx)-1,PSTR("AT+CGDCONT=1,\"IP\",\"%s\"\r\n"), simon_gprs_server);
	PRINTF(modem_BufferTxRx);
	
	modem_printR(modem_BufferTxRx);	
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);

	modem_printP(modem_init_cmd[GPRS]);
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
	
	//modem_printR("AT#SCFG=1,1,300,90,600,50\r\n");
	/* conn ID, PDP ctx, pkt size, max timeout (s), conn. timeout (x100ms), Tx timeout (x100ms) */
	modem_printPP(PSTR("AT#SCFG=1,1,300,100,100,50\r\n"));
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
	
	modem_printPP(PSTR("AT#SCFGEXT2=1,0,1,0,0\r\n"));
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
	modem_release();

	modem_state = INIT;

	return MODEM_OK;
}
	
uint8_t gc864_modem_set_hostname(char *host)
{
	hostname = host;
	return MODEM_OK;
}

char* gc864_modem_get_hostname(void)
{
	return hostname;	
}

uint8_t gc864_modem_resolve_ip(char * host, char *buf_ip)
{
	uint16_t retries = 0;
	uint16_t timeout = 0;
	char *_ip;
	
	if(host == NULL)
	{
		return MODEM_ERR;
	}
	
	if(Check_Connection(MAX_RETRIES) == FALSE)
	{
		return MODEM_ERR;
	}
	
	retries = 0;
	
	try_again:
	
	SNPRINTF_P(modem_BufferTxRx,sizeof(modem_BufferTxRx)-1,PSTR("AT#QDNS=%s\r\n"), host);
	PRINTF(modem_BufferTxRx);
	modem_printR(modem_BufferTxRx);	

	timeout = 0;
	
	do
	{
		
		wait_modem_get_reply(10); // wait 100ms;
		PRINT_BUF(modem_BufferTxRx);
		if(strstr((char *)modem_BufferTxRx,"NOT SOLVED") != NULL)
		{
			if(++retries < 5)
			{
				goto try_again;
			}
			return MODEM_ERR;
		}
		else if((_ip = strstr((char *)modem_BufferTxRx,host)) != NULL)
		{
			PRINTS_P(PSTR("Server IP: "));
			_ip = strchr((char *)_ip,',');
			_ip++; _ip++;
			char* _ip_s = _ip;
			while(*_ip_s != '"')
			{
				++_ip_s;
			}
			*_ip_s = '\0';
			if(_ip != NULL)
			{
				strncpy(buf_ip,_ip, 16);				
				gc864_modem_set_ip(buf_ip);
				PRINTF(buf_ip);	
				PRINTS_P(PSTR("\r\n"));
				ip_resolved = 1;			
				return MODEM_OK;
			}
		}		
	} while (++timeout < 1000);
	
	return MODEM_ERR;
}

char* gc864_modem_get_ip(void)
{
	return (char*)ip;
}

uint8_t gc864_modem_set_ip(char* _ip)
{
	if(_ip == NULL)
	{
		return MODEM_ERR;
	}	
	strncpy(ip,_ip, sizeof(ip)-1);	
	return MODEM_OK;
}

#if OLD_SEND
uint8_t gc864_modem_send(char * dados, uint16_t tam)
{
	uint16_t timeout = 0;
	uint8_t  retries = 0;
	uint8_t  send_ok = 0;
	
	if(dados == NULL) goto exit;
	if(hostname == NULL) goto exit;

	*(dados+tam-1) = '\0'; // null terminate

	if(mon_verbosity > 4) PRINTS_P(PSTR("Modem Send: \r\n"));

	/* Flush queue first */
	OSCleanQueue(MODEM_QUEUE);

	/* check modem setup */
	if(modem_state == SETUP)
	{
		if(gc864_modem_init() == MODEM_ERR)
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem Init fail \r\n"));
			goto exit;
		}else
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem Init OK \r\n"));
		}
	}

	/* check modem init */
	if(modem_state == INIT)
	{
		if(is_modem_ok_retry(10, 200) == FALSE)
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS is busy \r\n"));
			goto exit; /* retry later */
		}else
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS is ready \r\n"));
		}
				
		if(Check_Connect_PPP(MAX_RETRIES) == FALSE)
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS connection fail \r\n"));
			goto exit;
		}else
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS is connected \r\n"));
		}
	}

    if(modem_state == OPEN) 
	{		
		goto send;
	}
	
	
	try_again:

    /* open TCP connection */
	if(ip_resolved == 1)	
	{
		SNPRINTF_P(modem_BufferTxRx,sizeof(modem_BufferTxRx)-1,PSTR("AT#SKTD=0,80,%s,0,0\r\n"), (char*)ip);
	}else
	{
		gc864_modem_resolve_ip();
		SNPRINTF_P(modem_BufferTxRx,sizeof(modem_BufferTxRx)-1,PSTR("AT#SKTD=0,80,%s,0,0\r\n"), hostname);
	}
	
    if(mon_verbosity > 3) PRINT(modem_BufferTxRx);
    modem_printR(modem_BufferTxRx);

    timeout = 0;
    retries = 0;

    do{
    	wait_modem_get_reply(100); // wait 100ms;
    	if(mon_verbosity > 2) PRINT_BUF(modem_BufferTxRx);

    	if(strstr((char *)modem_BufferTxRx,"CONNECT"))
    	{
			modem_state = OPEN;
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS connection OK \r\n"));
    		goto send;
    	}
    	if(strstr((char *)modem_BufferTxRx,"NO CARRIER"))
		{
			modem_state = INIT;
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS connection error \r\n"));
    		if(++retries < MAX_RETRIES)
    		{
    			goto try_again;
    		}
    		break;
		}
    }while(++timeout < 20);

    if(mon_verbosity > 2) PRINTS_P(PSTR("\r\nConnection failed \r\n"));
    goto exit;

    /* connection is open, send data */
    send:

		if(mon_verbosity > 1) PRINT(dados);
		modem_printR(dados);

		timeout = 0;
		send_ok = 0;
		do
		{
			wait_modem_get_reply(10);
			if(mon_verbosity > 2) PRINT_BUF(modem_BufferTxRx);
			if(strstr((char *)modem_BufferTxRx,"OK"))
			{
				send_ok=1;
				DelayTask(200);
				OSCleanQueue(MODEM_QUEUE);
				break;
			}
		}while(++timeout < 200); /* espera ate 2 segundos */

		if(send_ok == 1)
		{
			if(mon_verbosity > 0) PRINTS_P(PSTR("\r\nsend ok\r\n"));
			modem_watchdog = 0;
			return MODEM_OK;
		}else
		{
			if(mon_verbosity > 0) PRINTS_P(PSTR("\r\nsend fail\r\n"));
			modem_state = INIT;
		}

	exit:
	if(++modem_watchdog > MAX_MODEM_ERRORS)
	{
		modem_watchdog = 0;
		gc864_modem_close();	
	}
	return MODEM_ERR;
}
#else

uint8_t Check_Connection(uint8_t retries)
{
	
	do
	{
		/* check GPRS connection */
		modem_printR("AT#SGACT?\r\n");
		wait_modem_get_reply(100); // wait 100ms;
		if(mon_verbosity > 2) PRINT_BUF(modem_BufferTxRx);

		if(strstr((char *)modem_BufferTxRx,"#SGACT: 1,1") != NULL)
		{
			/* connected */
			return TRUE;
		}
		else if(strstr((char *)modem_BufferTxRx,"#SGACT: 1,0") != NULL)
		{
			/* not connected */
			if(--retries == 0) return FALSE;

			modem_printR("AT#SGACT=1,1\r\n");
			wait_modem_get_reply(500); // wait 500ms;
			if(mon_verbosity > 0)  PRINT_BUF(modem_BufferTxRx);
		}else
		{
			/* no reply, buffer may be full */
			if(--retries == 0) return FALSE;
		}
	}while(TRUE);

	return FALSE;
	
}

uint8_t modem_close_tcp(void);

uint8_t modem_close_tcp(void)
{
	char buffer[64];
	uint8_t  retries = 0;
	
	while(retries < 3)
	{
		++retries;
		DelayTask(100);
		modem_printPP(PSTR("+++"));
		DelayTask(100);
		
		DelayTask(10);
		modem_get_reply(buffer,64);
		if(mon_verbosity > 3) PRINT_BUF(buffer);
		if(strstr((char *)buffer,"OK"))		
		{
			if(mon_verbosity > 3) PRINTS_P(PSTR("\r\nTCP connection paused\r\n"));
			goto close_tcp;
			//return FALSE;
		}
	}
	
	goto exit;
	
	close_tcp:
	
	retries = 0;
	while(retries < 3)
	{
		++retries;	
		modem_printPP(PSTR("AT#SH=1\r\n"));	
		DelayTask(10);
		modem_get_reply(buffer,64);
		if(mon_verbosity > 3) PRINT_BUF(buffer);
		if(strstr((char *)buffer,"OK"))
		{
			if(mon_verbosity > 3) PRINTS_P(PSTR("\r\nTCP closed\r\n"));
			return TRUE;
		}
	}

    exit:
	return FALSE;
	
}
uint8_t gc864_modem_send(char * dados, uint16_t tam)
{
	uint16_t timeout = 0;
	uint8_t  retries = 0;
	uint8_t  send_ok = 0;
	
	if(dados == NULL) goto exit;
	if(hostname == NULL) goto exit;

	*(dados+tam-1) = '\0'; // null terminate	
	
	/* check modem setup */
	if(modem_state == SETUP)
	{
		if(gc864_modem_init() == MODEM_ERR)
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem Init fail \r\n"));
			goto exit;
		}else
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem Init OK \r\n"));
		}
	}

	/* check modem init */
	if(modem_state == INIT)
	{
		if(is_modem_ok_retry(MAX_RETRIES, 1000) == FALSE)
		{						
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS is busy \r\n"));
			if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());	
			goto exit; /* retry later */
		}else
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS is ready \r\n"));
			if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
		}
		
		
		/* check modem GPRS connection */
		if(Check_Connection(MAX_RETRIES) == FALSE)
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS connection fail \r\n"));
			if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
			goto exit;
		}else
		{
			if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS is connected \r\n"));
			if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
		}
	}
	
	retries = 0;
	
	try_again:
	
	
	/* open TCP connection */
	if(ip_resolved == 1)
	{
		SNPRINTF_P(modem_BufferTxRx,sizeof(modem_BufferTxRx)-1,PSTR("AT#SD=1,0,80,%s,0,0,0\r\n"), (char*)ip);
	}else
	{
		gc864_modem_resolve_ip(hostname,ip);
		SNPRINTF_P(modem_BufferTxRx,sizeof(modem_BufferTxRx)-1,PSTR("AT#SD=1,0,80,%s,0,0,0\r\n"), hostname);
	}
	modem_printR(modem_BufferTxRx);
	if(mon_verbosity > 2) PRINT(modem_BufferTxRx);
	
	 timeout = 0; 

	 do{
		 wait_modem_get_reply(100); // wait 100ms;
		 if(mon_verbosity > 2) PRINT_BUF(modem_BufferTxRx);

		 if(strstr((char *)modem_BufferTxRx,"CONNECT"))
		 {
			 // modem_state = OPEN;
			 if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS connection OK \r\n"));
			 if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
			 goto send;
		 }
		 if(strstr((char *)modem_BufferTxRx,"NO CARRIER"))
		 {
			 modem_state = INIT;
			 if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS connection error \r\n"));
			 if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
			 if(++retries < MAX_RETRIES)
			 {
				 goto try_again;
			 }
			 break;
		 }
		 if(strstr((char *)modem_BufferTxRx,"ERROR"))
		 {
			 modem_state = INIT;
			 if(mon_verbosity > 2) PRINTS_P(PSTR("Modem GPRS connection error \r\n"));
			 if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
			 if(++retries < MAX_RETRIES)
			 {
				 goto try_again;
			 }
			 break;
		 }
	 }while(++timeout < 200); /* wait up to 20s */

	 //modem_state = INIT;
	 
	 if(mon_verbosity > 2) PRINTS_P(PSTR("\r\nConnection failed \r\n"));
	 if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
	 
	 goto exit;
	 
	 /* connection is open, send data */
	 send:
	 modem_printR(dados);
	 if(mon_verbosity > 2) PRINT(dados);

	 timeout = 0;
	 send_ok = 0;
	 
	do
	{
		wait_modem_get_reply(10);
		if(mon_verbosity > 2) PRINT_BUF(modem_BufferTxRx);
		if(strstr((char *)modem_BufferTxRx,"HTTP/1.1 200 OK"))
		{
			if(mon_verbosity > 3) PRINTF_P(PSTR("OK @time = %lu\r\n"), (uint32_t)clock_time());
			send_ok=1;
			break;
		}
	}while(++timeout < 2000); /* wait up to 20s */	

	if(send_ok == 1)	
	{
		#if 1
			char buffer[64];
			timeout = 0;
			do
			{
				modem_get_reply(buffer,63);
				if(mon_verbosity > 2) PRINT_BUF(buffer);
			
				if(strstr((char *)buffer,"</html>"))
				{
					if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
					break;
				}
				if(strstr((char *)buffer,"true"))
				{
					if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
					break;
				}
			}while(++timeout < 500); /* espera ate 5 segundos */
		#endif
		
	    modem_close_tcp();
			
		if(mon_verbosity > 1) PRINTS_P(PSTR("\r\nsend ok\r\n"));
		if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
		modem_watchdog = 0;
		return MODEM_OK;
	}else
	{
		if(mon_verbosity > 1) PRINTS_P(PSTR("\r\nsend fail\r\n"));
		if(mon_verbosity > 3) PRINTF_P(PSTR("@time = %lu\r\n"), (uint32_t)clock_time());
				
		#if 1
		char buffer[64];
		timeout = 0;
		do
		{
			modem_get_reply(buffer,63);
			if(mon_verbosity > 2) PRINT_BUF(buffer);
			
		}while(++timeout < 1000); /* espera ate 10 segundos */		
		#endif					
		
		modem_close_tcp();
		modem_state = INIT;
	}
		 	
	exit:
	return MODEM_ERR;
}
#endif


uint8_t gc864_modem_receive(char* buff, uint16_t* len)
{

	uint8_t ret = MODEM_ERR;
	uint16_t size =(uint16_t) MIN(*len,SIZEARRAY(modem_BufferTxRx));
	
	*len = 0;

	if(size)
	{				
		    modem_acquire();
		    if(modem_BufferTxRx[0] !='\0')
		    {		
		    	ret = MODEM_OK;
				memcpy(buff,modem_BufferTxRx,size);
				if(size < SIZEARRAY(modem_BufferTxRx))
				{
					memcpy(modem_BufferTxRx,&modem_BufferTxRx[size],SIZEARRAY(modem_BufferTxRx)-size);
				}
				modem_get_reply(&modem_BufferTxRx[SIZEARRAY(modem_BufferTxRx)-size],size);
				if(mon_verbosity > 4) PRINT_BUF(modem_BufferTxRx);
				* len = size;
		    }    
			modem_release();			
	}
	
	return (uint8_t)ret;
}


uint8_t gc864_modem_check_connection(void)
{
	if(!is_modem_ok_retry(MAX_RETRIES, 100))
	{
		return MODEM_ERR;
	}
	return MODEM_OK;
}

const modem_driver_t gc864_modem_driver  =
{
		gc864_modem_init,
		gc864_modem_receive,
		gc864_modem_send,
		gc864_modem_set_hostname,
		gc864_modem_set_ip,
		gc864_modem_check_connection,
		gc864_modem_resolve_ip,
};


uint8_t Check_Connect_PPP(uint8_t retries)
{

	do
	{
		/* check GPRS connection */
		modem_printP(modem_init_cmd[GPRS]);
		wait_modem_get_reply(100); // wait 100ms;
		if(mon_verbosity > 2) PRINT_BUF(modem_BufferTxRx);

		if(strstr((char *)modem_BufferTxRx,"#GPRS: 1") != NULL)
		{
			/* connected */
			return TRUE;
		}
		else if(strstr((char *)modem_BufferTxRx,"#GPRS: 0") != NULL)
		{
			/* not connected */
			if(--retries == 0) return FALSE;

			modem_printP(modem_init_cmd[GPRS1]);
			wait_modem_get_reply(500); // wait 500ms;
			if(mon_verbosity > 0)  PRINT_BUF(modem_BufferTxRx);
		}else
		{
			/* no reply, buffer may be full */
			if(--retries == 0) return FALSE;
		}
	}while(TRUE);

	return FALSE;
}

/* at commands */

modem_ret_t at_modem_init(void)
{	

	return  gc864_modem_init();

}
modem_ret_t at_modem_open(INT8U host, char* dados)
{
	uint8_t res;

	if(host)
	{	
		res = gc864_modem_set_hostname(dados);
		if(mon_verbosity > 2)
		{
			PRINTS_P(PSTR("Host: "));
			PRINT(dados);
		}

	}else
	{
		res = gc864_modem_set_ip((char*)dados);
		if(mon_verbosity > 2)
		{
			PRINTS_P(PSTR("IP: "));
			PRINT(dados);
		}
	}

	res = Check_Connect_PPP(MAX_RETRIES);

	return (res == TRUE ? MODEM_OK:MODEM_ERR);
}
modem_ret_t at_modem_send(char* dados)
{
	return gc864_modem_send(dados, (uint16_t)strlen(dados));
}
modem_ret_t at_modem_receive(char* buff, uint16_t len)
{
	uint16_t size = len;
	if(buff == NULL)	return MODEM_ERR;
	
	/* try to receive */
	if(gc864_modem_receive(buff,&size) == MODEM_OK)
	{
		*(buff+(size)) = '\0'; // null terminate
		PRINT_BUF(buff);
	}
	
	PRINTS_P(PSTR("\r\n"));	
	
	return MODEM_OK;
	
}

uint8_t gc864_modem_close(void)
{
	modem_acquire();

	modem_printP(modem_init_cmd[GPRS0]);
	MODEM_GET_REPLY_PRINT(modem_BufferTxRx);
	
	modem_release();
	
	return TRUE;
	
}
modem_ret_t at_modem_close(void)
{
	PRINT("Close \r\n");
	
	if(modem_state != CLOSE)
	{
		modem_state = CLOSE;
	}
	
	gc864_modem_close();
	
	return MODEM_OK;
	
}

modem_ret_t at_modem_server(void)
{
	PRINT_P(PSTR("NOT IMPLEMENTED \r\n"));
	return MODEM_OK;
}
modem_ret_t at_modem_dns(char* param)
{
	PRINT_P(PSTR("NOT IMPLEMENTED \r\n"));
	return MODEM_OK;
}
modem_ret_t at_modem_time(void)
{
	PRINT_P(PSTR("NOT IMPLEMENTED \r\n"));
	return MODEM_OK;
}



