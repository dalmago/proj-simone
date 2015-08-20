/*
 * monitor.c
 *
 *  Created on: 15/04/2015
 *      Author: UFSM
 */

#include "monitor.h"
#include "utils.h"
#include "assert.h"
#include "simon-api.h"
#include "printf_lib.h"


#ifdef _WIN32
#include "stdio.h"
#include "http_client.h"
#else
#include "AppConfig.h"
#endif

#define DEBUG_MONITOR 1

#if DEBUG_MONITOR
#ifndef _WIN32
#define PRINTF(...) printf_lib(__VA_ARGS__);
#else
#define PRINTF(...) printf(__VA_ARGS__);
#endif
#else
#define PRINTF(...)
#endif

#define MONITOR_TESTS 0

extern monitor_state_t monitor_state[MAX_NUM_OF_MONITORES];

#define LOG_MAX_DATA_BUF_SIZE  (LOG_MAX_ENTRY_SIZE/2)
uint16_t monitor_data_buffer[LOG_MAX_ENTRY_SIZE];
char monitor_char_buffer[LOG_MAX_ENTRY_SIZE];

#define CONST const

#include "modbus_slaves.h"
extern CONST modbus_slave_t slave_NULL;
extern CONST modbus_slave_t slave_PM210;
extern CONST modbus_slave_t slave_TS;

CONST modbus_slave_t * modbus_slaves_all[NUM_MODBUS_SLAVES] =
{
		&slave_NULL,
		&slave_PM210,
		&slave_TS,	
};

LOG_FILETYPE stderr_f;

#define error_file		"erro.txt"
#include <stdarg.h>
/*-----------------------------------------------------------------------------------*/
void print_erro(const char *format, ...)
{

  va_list argptr;
  va_start(argptr, format);
  VSPRINTF(monitor_char_buffer, format, argptr);  
  va_end(argptr);

  if(!monitor_openappend(error_file,&stderr_f))
  {
	  if(!monitor_openwrite(error_file,&stderr_f))
	  {
		  return;
	  }
  }
  /* log error */
  (void)monitor_seek_end(&stderr_f);
  (void)monitor_write(monitor_char_buffer,&stderr_f);
  (void)monitor_close(&stderr_f);
}


void monitor_createentry(char* buffer, uint16_t *dados, uint8_t len)
{

	uint16_t dado = 0;
	do
	{
		dado=*dados;
		int2hex(buffer,dado);
		buffer+=4;
		dados++;
		len--; 
	}while(len > 0);
	
	*buffer++='\r';
	*buffer++='\n';
	*buffer++='\0';
}

void monitor_makeheader(char monitor_header[], monitor_header_t * h)
{

   monitor_header[0] = 'V'; // vers�o
   byte2hex(&monitor_header[1],h->h1.version);
   monitor_header[3] = 'M'; // monitor ID
   byte2hex(&monitor_header[4],h->h1.mon_id);
   monitor_header[6] = 'B'; // tamanho da entrada de dados
   int2hex(&monitor_header[7],h->h1.entry_size);
   monitor_header[11] = 'I'; // intervalo de amostragem
   int2hex(&monitor_header[12],h->h1.time_interv);
   monitor_header[16] = '\r';
   monitor_header[17] = '\n';
   monitor_header[18] = 'T'; // estampa de tempo
   int2hex(&monitor_header[19],h->h2.year);
   byte2hex(&monitor_header[23],h->h2.mon);
   byte2hex(&monitor_header[25],h->h2.mday);
   byte2hex(&monitor_header[27],h->h2.hour);
   byte2hex(&monitor_header[29],h->h2.min);
   byte2hex(&monitor_header[31],h->h2.sec);
   monitor_header[33] = tohex(h->h2.synched);
   monitor_header[34] = '\r';
   monitor_header[35] = '\n';
   monitor_header[36] = 'P'; // �ndice da �ltima entrada enviada
   int2hex(&monitor_header[37],h->last_idx);
   monitor_header[41] = '\r';
   monitor_header[42] = '\n';
   monitor_header[43] = 'C'; // contador de entradas do log
   int2hex(&monitor_header[44],h->count);
   monitor_header[48] = '\r';
   monitor_header[49] = '\n';
   monitor_header[50] = '\0';
}

char monitor_header[LOG_HEADER_LEN+2];

void monitor_setheader(char* filename, monitor_header_t * h)
{
   LOG_FILETYPE fp;  

   if(monitor_openappend(filename,&fp))
   {
	   monitor_makeheader(monitor_header,h);
	   (void)monitor_write(monitor_header,&fp);
	   (void)monitor_close(&fp);
   }

}

uint8_t monitor_getheader(char* filename, monitor_header_t * h)
{

	   LOG_FILETYPE fp;
	   int idx = 0;
	   char hex1,hex2;
	   uint8_t b1,b2;

#define NEXT_2(res)	do{hex1 = monitor_header[idx++]; hex2 = monitor_header[idx++];} while(0); (res) = hex2byte(hex1,hex2);
#define NEXT_4(res) do{hex1 = monitor_header[idx++]; hex2 = monitor_header[idx++]; b1 = hex2byte(hex1,hex2); hex1 = monitor_header[idx++];hex2 = monitor_header[idx++]; b2 = hex2byte(hex1,hex2);}while(0); (res) = byte2int(b1,b2);
	   	   
	   if(!monitor_openread(filename,&fp))
	   {
		   return 0;
	   }

	   /* process first header line */
	   if(!monitor_read(monitor_header,LOG_HEADER_LEN,&fp))
	   {
		   return 0;
	   }
	   
	   if(monitor_header[idx++] != 'V') return 0;
	   {
		   NEXT_2(h->h1.version);
	   }
	   if(monitor_header[idx++]  != 'M') return 0;
	   {
		   NEXT_2(h->h1.mon_id);
	   }
	   if(monitor_header[idx++] != 'B') return 0;
	   {
		   NEXT_4(h->h1.entry_size);
	   }
	   if(monitor_header[idx++] != 'I') return 0;
	   {
		   NEXT_4(h->h1.time_interv);
	   }
	   assert (idx == 16);

	   /* process second header line */
	   if(!monitor_read(monitor_header,LOG_HEADER_LEN,&fp))
	   {
		   return 0;
	   }			   
	   idx = 0;
	   if(monitor_header[idx++] != 'T') return 0;
	   {
		   NEXT_4(h->h2.year);
		   NEXT_2(h->h2.mon);
		   NEXT_2(h->h2.mday);
		   NEXT_2(h->h2.hour);
		   NEXT_2(h->h2.min);
		   NEXT_2(h->h2.sec);
		   h->h2.synched = hex2val(monitor_header[idx++]);
	   }
	   assert (idx == 16);
	
	   /* process third header line */
	   if(!monitor_read(monitor_header,LOG_HEADER_LEN,&fp))
	   {
		   return 0;
	   }
	   idx = 0;
	   if(monitor_header[idx++] != 'P') return 0;
	   {
		   NEXT_4(h->last_idx);
	   }
	   assert (idx == 5);

	   /* process fourth header line */
	   if(!monitor_read(monitor_header,LOG_HEADER_LEN,&fp))
	   {
		   return 0;
	   }
	   idx = 0;
	   if(monitor_header[idx++] != 'C') return 0;
	   {
		   NEXT_4(h->count);
	   }
	   assert (idx == 5);

	   (void)monitor_close(&fp);
	   
	   return 1;
}

uint8_t monitor_newheader(char* filename, uint8_t monitor_id, uint16_t interval, uint16_t entry_size)
{
	monitor_header_t h = {{0,0,0,0},{0,0,0,0,0,0,0},0,0};
	h.h1.mon_id = monitor_id;
	h.h1.time_interv = interval;
	h.h1.entry_size = entry_size;
	monitor_setheader(filename, &h);
	return monitor_getheader(filename, &h);
}

uint8_t monitor_validateheader(char* filename, uint8_t monitor_id, uint16_t interval, uint16_t entry_size)
{
	monitor_header_t h = {{0,0,0,0},{0,0,0,0,0,0,0},0,0};
	if(monitor_getheader(filename, &h))
	{
		if( h.h1.mon_id == monitor_id && 
			h.h1.time_interv == interval &&
			h.h1.entry_size == entry_size)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}	
	return 0;
}

#ifndef _WIN32
#include "time_lib.h"
#else
#include "http_client.h"
#include "time.h"
#endif

void monitor_gettimestamp(struct tm * timestamp, uint32_t time_elapsed_s)
{

	/* GET time/local */
	time_t time_now = 0;

#define SERVER_TIME 1
#if SERVER_TIME
	struct tm t;
	//http_get_time(&t);
	simon_get_time(&t);
	time_now = mktime(&t);
#else
	time_now = time(NULL);
#endif

	time_now = time_now - time_elapsed_s;
	(*timestamp) = *localtime(&(time_t){time_now});
}

#include "string.h"
void monitor_settimestamp(uint8_t monitor_num, char* filename)
{
	uint32_t time_elapsed_s = 0;
	monitor_header_t h = {{0,0,0,0},{0,0,0,0,0,0,0},0,0};
	struct tm ts;
	char new_filename[8+5]={"00000000.txt"};
	uint8_t ret;

	if(!monitor_getheader(filename, &h)) return;

	/* check if already synched */
	if(h.h2.synched == 0)
	{
		time_elapsed_s = (uint32_t)((h.h1.time_interv)*(h.count));
		monitor_gettimestamp(&ts, time_elapsed_s);

		#if 1
			h.h2.year = (uint16_t)(ts.tm_year + 1900);
			h.h2.mon = (uint8_t)(ts.tm_mon + 1);
			h.h2.mday = (uint8_t)ts.tm_mday;
			h.h2.hour = (uint8_t)ts.tm_hour;
			h.h2.min = (uint8_t)ts.tm_min;
			h.h2.sec = (uint8_t)ts.tm_sec;
		#else
			h.h2.year = (uint16_t)2015;
			h.h2.mon = (uint8_t)04;
			h.h2.mday = (uint8_t)21;
			h.h2.hour = (uint8_t)18;
			h.h2.min = (uint8_t)10;
			h.h2.sec = (uint8_t)33;
			PRINTF("\r\n%d%d%d%d%d%d\r\n", h.h2.year,h.h2.mon,h.h2.mday,h.h2.hour,h.h2.min,h.h2.sec);
		#endif

			h.h2.synched = 1;

#if 1
			do
			{
				char timestamp[20];
				strftime(timestamp,20,"T%Y%m%d%H%M%SS\r\n",&ts);
				PRINTF(timestamp);
			}while(0);
#endif

			monitor_setheader(filename, &h);

			/* rename file */
			strftime(new_filename,sizeof(new_filename),"%y%m%d%H.txt",&ts);

			PRINTF("\r\n %s will be renamed to %s \r\n", filename,new_filename);
			ret = (uint8_t)monitor_rename(filename, new_filename);

			/* if rename failed, try other name */
			while(!ret)
			{
				PRINTF("\r\n rename failed \r\n");
				time_t time_now = mktime(&ts);
				time_now +=3600;
				ts = *localtime(&(time_t){time_now});
				strftime(new_filename,sizeof(new_filename),"%y%m%d%H.txt",&ts);

				PRINTF("\r\n %s will be renamed to %s \r\n", filename,new_filename);
				ret = (uint8_t)monitor_rename(filename, new_filename);

			}

			/* log is reading the same file ? */
			if(strcmp(monitor_state[monitor_num].monitor_name_reading, monitor_state[monitor_num].monitor_name_writing) == 0)
			{
				PRINTF("\r\n reading the same file that we renamed! \r\n");

				strcpy(monitor_state[monitor_num].monitor_name_reading,new_filename);

				/* update meta file */
				LOG_FILETYPE fp;
				if(monitor_openwrite(LOG_METAFILE,&fp))
				{
				   if(monitor_write(monitor_state[monitor_num].monitor_name_reading,&fp)){;}
				   (void)monitor_close(&fp);
				}
			}
			strcpy(monitor_state[monitor_num].monitor_name_writing,new_filename);
	}
}


extern volatile uint8_t monitor_is_connected;

uint16_t monitor_writeentry(char* filename, char* entry, uint8_t monitor_num)
{

	LOG_FILETYPE fp;
	monitor_header_t h = {{0,0,0,0},{0,0,0,0,0,0,0},0,0};

	if(monitor_getheader(filename, &h))
	{
		if(monitor_openappend(filename,&fp))
		{
		   (void)monitor_seek_end(&fp);
		   (void)monitor_write(entry,&fp);
		   (void)monitor_close(&fp);

		   h.count++; // incrementa contador de entradas
		   monitor_setheader(filename, &h);

		}else
		{
			h.count = 0;
		}
	}

	/* if file is not synched, try to synch */
	if(monitor_is_connected == 1 && h.h2.synched == 0)
	{
		monitor_sync(monitor_num, filename);
	}

	return h.count;

}

uint16_t build_entry_to_send(char* ptr_data, uint8_t *data, uint8_t len)
{
	uint16_t offset = 0;
	uint16_t max_len = 4*len;
	char hex1,hex2;
	uint8_t val;
	uint16_t cnt = 0;

	while(len > 0)
	{
		len-=2;
		hex1 = *data++;
		hex2 = *data++;
		val = hex2byte(hex1,hex2);

		offset = SNPRINTF(ptr_data, max_len,"%d,", val);
		if(offset < 0 || offset >= max_len)
		{
			return 0;
		}
		ptr_data+= offset;
		cnt+=offset;
	}
	*(--ptr_data) = '\0'; // null terminate string
	return cnt;
}

char data_vector[256*4];

uint8_t monitor_entry_send(monitor_entry_t* entry, uint8_t len)
{
	
	uint16_t cnt = 0;
	cnt = build_entry_to_send(data_vector,entry->values,len);


	PRINTF("\r\n");
	PRINTF(data_vector);
	PRINTF("\r\n");
	
#ifdef _WIN32	
	fflush(stdout);
#endif	

	if(cnt > 0)
	{
#ifdef _WIN32			
		http_send_data(data_vector,cnt);
#endif			
	}

	return 1;
}

uint32_t monitor_readentry(uint8_t monitor_num, char* filename, monitor_entry_t* entry)
{
	uint16_t entry_size;
	LOG_FILETYPE fp;
	LOG_FILEPOS  pos = LOG_HEADER_LEN;
	struct tm ts;

	monitor_header_t h = {{0,0,0,0},{0,0,0,0,0,0,0},0,0};

	monitor_getheader(filename, &h);

	if (h.h2.synched == 0) return (MAX_NUM_OF_ENTRIES); /* file not synched */

	entry_size = (h.h1.entry_size) + 2; // inclui \r\n

	if(h.last_idx < h.count)
	{
		ts.tm_year = h.h2.year - 1900;
		ts.tm_mon = h.h2.mon - 1;
		ts.tm_mday = h.h2.mday;
		ts.tm_hour = h.h2.hour;
		ts.tm_min = h.h2.min;
		ts.tm_sec = h.h2.sec;
		ts.tm_isdst = -1;

		time_t unix_time = mktime(&ts);

		if(unix_time > 0)
		{
			unix_time += (h.last_idx)*(h.h1.time_interv);
			entry->ts = unix_time;
		}

		/* le a proxima entrada */
		if(monitor_openread(filename,&fp))
		{
			pos = pos + (h.last_idx)*entry_size;

			if(monitor_seek(&fp,&pos))
			{
			   (void)monitor_read(entry->values,entry_size,&fp);
			   (void)monitor_close(&fp);

			   /* try to send */
			   if(monitor_entry_send(entry,entry_size-2) == 1) // ignore \r\n
			   {
				   /* if ok */
				   h.last_idx++; // incrementa indice da �ltima entrada lida
				   monitor_setheader(filename, &h);
			   }

			   return h.last_idx;
			}
		}
	}
	else
	{
		// if(h.last_idx == h.count)
		/* log is not writing in the same reading file ? */
		if(strcmp(monitor_state[monitor_num].monitor_name_reading, monitor_state[monitor_num].monitor_name_writing) != 0)
		{
			strcpy(monitor_state[monitor_num].monitor_name_reading,monitor_state[monitor_num].monitor_name_writing);
			/* update meta file */
			if(monitor_openwrite(LOG_METAFILE,&fp))
			{
			   if(monitor_write(monitor_state[monitor_num].monitor_name_reading,&fp)){;}
			   (void)monitor_close(&fp);
			}
		}
	}
	return 0;
}

void monitor_sync(uint8_t monitor_num, char*monitor_fn)
{
	monitor_settimestamp(monitor_num, (char*)monitor_fn); // sincronizar
}

#if _WIN32
#include<windows.h>
#include <sys/stat.h>
#endif

/* open dir and try to open/create a new file */
uint8_t monitor_init(uint8_t monitor_num)
{

	  LOG_DIRTYPE  d;
	  LOG_FILETYPE fp;
	  LOG_DIRINFO  dir;
	  LOG_FILEINFO fnfo;

	  strcpy(monitor_state[monitor_num].monitor_name_writing, LOG_FILENAME_START);

	  if (!monitor_stat(monitor_state[monitor_num].monitor_dir_name, &fnfo))
	  {
		  monitor_mkdir(monitor_state[monitor_num].monitor_dir_name);
	  }

	  if (monitor_opendir(monitor_state[monitor_num].monitor_dir_name, d))
	  {
	    while (monitor_readdir(dir,d))
	    {
#if _WIN32
	    	PRINTF("%s\r\n", dir->d_name);
#else
	    	PRINTF("%s\r\n", (LOG_DIRINFO*)(&dir)->fname);
#endif
	    }
	    monitor_closedir(d);
	  }else
	  {
		  print_erro_and_exit:
		  print_erro("Log init erro: %d", monitor_num);
		  PRINTF("Log init erro: %d", monitor_num);
		  return !OK;
	  }

	 /* change to log dir */
	 monitor_chdir(monitor_state[monitor_num].monitor_dir_name);

     /* try open log or create it now */
	 if(!monitor_openappend(monitor_state[monitor_num].monitor_name_writing,&fp))
	 {
	   if(monitor_openwrite(monitor_state[monitor_num].monitor_name_writing,&fp))
	   {
		   goto make_new_header;
	   }else
	   {
		   goto print_erro_and_exit;
	   }
	 }
	 /* validate header */
	 if(!monitor_validateheader(monitor_state[monitor_num].monitor_name_writing,
			   monitor_state[monitor_num].config_h.mon_id,
			   monitor_state[monitor_num].config_h.time_interv,
			   monitor_state[monitor_num].config_h.entry_size))
	 {
		 make_new_header:
		 if(monitor_newheader(monitor_state[monitor_num].monitor_name_writing,
		 				   monitor_state[monitor_num].config_h.mon_id,
		 				   monitor_state[monitor_num].config_h.time_interv,
		 				   monitor_state[monitor_num].config_h.entry_size) == 0)
		   {
			 goto print_erro_and_exit;
		   }		 
	 }
	 (void)monitor_close(&fp);

     /* try open metalog or create a new one */
	 if(monitor_openread(LOG_METAFILE,&fp))
	 {
		 if(monitor_read(monitor_state[monitor_num].monitor_name_reading,FILENAME_MAX_LENGTH,&fp))
		 {
			 monitor_state[monitor_num].monitor_name_reading[FILENAME_MAX_LENGTH-1] = '\0'; // null terminate
			 (void)monitor_close(&fp);

			 // check if the file exists
			 if(monitor_openread(monitor_state[monitor_num].monitor_name_reading,&fp))
			 {
				 (void)monitor_close(&fp);
			 }
			 else
			 {
				 (void)monitor_close(&fp);
				 goto fail_to_open_monitor_reader;
			 }
		 }else
		 {
			 (void)monitor_close(&fp);
			 goto fail_to_open_monitor_reader;
		 }

	 }else
	 {
	   fail_to_open_monitor_reader:
	   if(monitor_openwrite(LOG_METAFILE,&fp))
	   {
		   strcpy(monitor_state[monitor_num].monitor_name_reading,monitor_state[monitor_num].monitor_name_writing);
		   if(monitor_write(monitor_state[monitor_num].monitor_name_writing,&fp)){;}
		   (void)monitor_close(&fp);
	   }else
	   {
		   goto print_erro_and_exit;
	   }
	 }

	 /* change to parent dir */
	 monitor_chdir("..");
	 return OK;
}

char* monitor_getfilename_to_write(uint8_t monitor_num)
{
	return monitor_state[monitor_num].monitor_name_writing;
}

char* monitor_getfilename_to_read(uint8_t monitor_num)
{
	return monitor_state[monitor_num].monitor_name_reading;
}

void monitor_writer(uint8_t monitor_num)
{	
	uint16_t cnt = 0;
	
#if MONITOR_TESTS
	uint16_t vetor_dados[3]={0x1111,0x2222,0x3333};	
	monitor_createentry(monitor_char_buffer,vetor_dados,3);

	assert((monitor_char_buffer[0] == '1') && (monitor_char_buffer[1]== '1') && 
			(monitor_char_buffer[2] == '1') && (monitor_char_buffer[3]== '1') &&
			(monitor_char_buffer[0+4] == '2') && (monitor_char_buffer[1+4]== '2') && 
			(monitor_char_buffer[2+4] == '2') && (monitor_char_buffer[3+4]== '2') &&
			(monitor_char_buffer[0+2*4] == '3') && (monitor_char_buffer[1+2*4]== '3') && 
			(monitor_char_buffer[2+2*4] == '3') && (monitor_char_buffer[3+2*4]== '3'));

#else	
	
	/* check buffer size */
	if(monitor_state[monitor_num].config_h.entry_size > LOG_MAX_DATA_BUF_SIZE)
	{
		print_erro("Monitor %d: buffer too small \r\n", monitor_num);
		return;
	}
	
	/* read data */
	if(monitor_state[monitor_num].read_data((uint8_t*)monitor_data_buffer,(uint8_t)monitor_state[monitor_num].config_h.entry_size) == 0)
	{
		print_erro("Monitor %d: read failed\r\n", monitor_num);
		return;
	}
	
	/* convert data to hex char */
	monitor_createentry(monitor_char_buffer,(uint16_t*)monitor_data_buffer,(uint8_t)(monitor_state[monitor_num].config_h.entry_size/2));

#endif		

	/* change to log dir */
	monitor_chdir(monitor_state[monitor_num].monitor_dir_name);
	
	/* write data (hex char) to file  */
	cnt = monitor_writeentry(monitor_getfilename_to_write(monitor_num),monitor_char_buffer, monitor_num);
	
	if(cnt == 0)
	{
		print_erro("Monitor %d: write failed\r\n", (uint8_t)monitor_num);
		PRINTF("Monitor %d: write failed\r\n", monitor_num);
	}else
	{
		PRINTF("Entry written %d\r\n", cnt);
	}

	/* change to parent dir */
	monitor_chdir("..");
	
}

void monitor_reader(uint8_t monitor_num)
{
	monitor_entry_t entry;
	uint32_t nread;
	char buffer[sizeof(long)*8+1];

	char* fname = monitor_getfilename_to_read(monitor_num);
	
	if(*fname == '\0') return;
	
	memset(monitor_char_buffer,0x00,sizeof(monitor_char_buffer));
	entry.values = monitor_char_buffer;

	/* change to log dir */
	monitor_chdir(monitor_state[monitor_num].monitor_dir_name);

	if((nread = monitor_readentry(monitor_num, fname, &entry)) != 0)
	{
		if( nread < MAX_NUM_OF_ENTRIES)
		{
			PRINTF((char*)entry.values);
			PRINTF(ltoa((long)entry.ts,buffer,10));
		}
	}

	/* change to parent dir */
	monitor_chdir("..");

}




