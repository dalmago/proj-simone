/*
 * logger.h
 *
 *  Created on: 15/04/2015
 *      Author: Universidade Federal
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#ifndef _WIN32
#include "time_lib.h"
#include "printf_lib.h"
#define puts(x)		printf_lib(x)
#else
#include "time.h"
#endif

#ifndef NULL
#define NULL  (void*)0
#endif

#if _WIN32
#include <dirent.h>
#define LOG_FILETYPE                  FILE*
#define log_openread(filename,file)   ((*(file) = fopen((filename),"rb+")) != NULL)
#define log_openwrite(filename,file)  ((*(file) = fopen((filename),"wb")) != NULL)
#define log_openappend(filename,file)  ((*(file) = fopen((filename),"ab+")) != NULL)
#define log_close(file)               (fclose(*(file)) == 0)
#define log_read(buffer,size,file)    (fgets((char*)(buffer),(size),*(file)) != NULL)
#define log_write(buffer,file)        (fputs((char*)(buffer),*(file)) >= 0)
#define log_rename(source,dest)       (rename((source), (dest)) == 0)
#define log_remove(filename)          (remove(filename) == 0)

#define LOG_FILEPOS                   fpos_t
#define log_tell(file,pos)            ((*(pos) = ftell(*(file))) != (-1L)) //(fgetpos(*(file), (pos)) == 0)
#define log_seek(file,pos)            (fseek(*(file), *(pos), SEEK_SET) == 0) // (fsetpos(*(file), (pos)) == 0)

#define LOG_DIRTYPE                   DIR*
#define LOG_DIRINFO 				  struct dirent *
#define LOG_FILEINFO 				  struct stat
#define log_stat(filename, fileinfo)  (stat((filename), (fileinfo)) == -1)
#define log_opendir(dirname,dir)	  (((dir) = opendir(dirname)) != NULL)
#define log_closedir(dir)			  closedir(dir)
#define log_readdir(dirinfo,dir)  	  (((dirinfo) = readdir(dir)) != NULL)
#define log_chdir(dirname)			  chdir(dirname)
#define log_mkdir(dirname)			  _mkdir(dirname)
#endif

#if _WIN32
#define FATFS_ENABLE 0
#else
#define FATFS_ENABLE 1
#endif

#if FATFS_ENABLE
#define LOG_BUFFERSIZE  256       /* maximum line length, maximum path length */

/* You must set _USE_STRFUNC to 1 or 2 in the include file ff.h (or tff.h)
 * to enable the "string functions" fgets() and fputs().
 */
#include "ff.h"                   /* include tff.h for FatFs */

#define LOG_FILETYPE    			   FIL
#define log_openread(filename,file)   (f_open((file), (filename), FA_READ+FA_OPEN_EXISTING) == FR_OK)
#define log_openwrite(filename,file)  (f_open((file), (filename), FA_WRITE+FA_CREATE_ALWAYS) == FR_OK)
#define log_openappend(filename,file) (f_open((file), (filename), FA_WRITE) == FR_OK)
#define log_close(file)               (f_close(file) == FR_OK)
#define log_read(buffer,size,file)    f_gets((buffer), (size),(file))
#define log_write(buffer,file)        f_puts((buffer), (file))
#define log_remove(filename)          (f_unlink(filename) == FR_OK)

#define LOG_FILEPOS                   DWORD
#define log_tell(file,pos)            (*(pos) = f_tell((file)))
#define log_seek(file,pos)            (f_lseek((file), *(pos)) == FR_OK)

#include "string.h"
static int log_rename(TCHAR *source, const TCHAR *dest)
{
  /* Function f_rename() does not allow drive letters in the destination file */
  const char *drive = strchr(dest, ':');
  drive = (drive == NULL) ? dest : drive + 1;
  return (f_rename(source, drive) == FR_OK);
}

#define LOG_DIRTYPE                   DIR
#define LOG_DIRINFO 				  FILINFO
#define LOG_FILEINFO 				  FILINFO
#define log_stat(filename, fileinfo)  (f_stat((filename), (fileinfo)) == FR_OK)
#define log_opendir(dirname,dir)	  (f_opendir(&(dir),dirname) == FR_OK)
#define log_closedir(dir)			  f_closedir(&(dir))
#define log_readdir(dirinfo,dir)  	  (f_readdir(&(dir), &(dirinfo)) == FR_OK)
#define log_chdir(dirname)			  f_chdir(dirname)
#define log_mkdir(dirname)			  f_mkdir(dirname)

#endif



#ifndef NULL
#define NULL  (void*)0
#endif

void	print_erro(char *format, ...);

#define LOG_HEADER_LEN		 50
#define LOG_MAX_ENTRY_SIZE   256
#define FILENAME_MAX_LENGTH  13
#define LOG_FILENAME_START   "99123123.txt"
#define LOG_METAFILE   		 ".0.txt"
#define MAX_NUM_OF_ENTRIES   ((uint32_t)(-1))
#define MAX_NUM_OF_LOGGERS	 2
#define NUM_OF_FIELDS        4
#define MAX_HOSTNAME_LEN	 (32+1)
#define MAX_APIKEY_LEN	 	 (32+1)

//#define LOG_DIR_NAME 		 "./logs"
//#define TIME_INTERVAL 		 5
//#define ENTRY_SIZE_B  		 12

/* type verification code */
static union
{
    char            int8_t_incorrect[sizeof( int8_t ) == 1];
    char            uint8_t_incorrect[sizeof( uint8_t ) == 1];
    char            int16_t_incorrect[sizeof( int16_t ) == 2];
    char            uint16_t_incorrect[sizeof( uint16_t ) == 2];
    char            int32_t_incorrect[sizeof( int32_t ) == 4];
    char            uint32_t_incorrect[sizeof( uint32_t ) == 4];
}u;

typedef struct
{
	uint16_t year;	/* Years since 0000 */
	uint8_t mon;	/* Months *since* january: 0-11 */
	uint8_t	mday;	/* Day of the month: 1-31 */
	uint8_t	hour;	/* Hours since midnight: 0-23 */
	uint8_t	min;	/* Minutes: 0-59 */
	uint8_t	sec;	/* Seconds: 0-59 */
}timestamp_t;

typedef struct
{
	time_t ts; /* entry timestamp - unix time */
	uint8_t size; /* size of entry in number of bytes, must be not greater than LOG_MAX_ENTRY_SIZE */
	uint8_t *values; /* pointer to 8-bit entry values */
}log_entry_t;

typedef struct
{
	uint8_t version;
	uint8_t mon_id;
	uint16_t entry_size;
	uint16_t time_interv;
}log_headerl1_t;

typedef struct
{
	uint16_t year;	/* Years since 0000 */
	uint8_t mon;	/* Months *since* january: 0-11 */
	uint8_t	mday;	/* Day of the month: 1-31 */
	uint8_t	hour;	/* Hours since midnight: 0-23 */
	uint8_t	min;	/* Minutes: 0-59 */
	uint8_t	sec;	/* Seconds: 0-59 */
	uint8_t synched; /* synch flag: (1) TRUE (0) FALSE */
}log_headerl2_t;

typedef struct
{
	log_headerl1_t h1; /* version, id, entry size and time interval */
	log_headerl2_t h2; /* timestamp and synch flag */
	uint16_t last_idx;	/* index of last sent line */
	uint16_t count;		/* entries count */
}log_header_t;

typedef enum{
	NOT_SYNC = 0,
	SYNC=1
} log_sync_t;

typedef struct
{
	log_sync_t sync_state;
	char  log_name_writing[FILENAME_MAX_LENGTH];
	char  log_name_reading[FILENAME_MAX_LENGTH];
	char  log_dir_name[FILENAME_MAX_LENGTH];
	log_headerl1_t config_h;

}log_state_t;

typedef union
{
	uint8_t byte;
	struct{
		uint8_t num_mon_ok	:1;	/* num. de monitores ok */
		uint8_t server_ok	:1;	/* server ok */
		uint8_t	key_ok		:1;	/* Api Key ok */
		uint8_t	gprs_server_ok:1;	/* GPRS server ok */
		uint8_t	unused1:1 ;	/* Unused */
		uint8_t	unused2:1 ;	/* Unused*/
		uint8_t unused3:1 ; /* Unused */
	}bit;
}log_config_ok_t;

void byte2hex(char *ret, uint8_t c);
void int2hex(char *ret, uint16_t c);
void test_logger(void);

uint8_t log_init(uint8_t logger_num);
void log_sync(char*);

void log_makeheader(char log_header[], log_header_t * h);
void log_setheader(char* filename, log_header_t * h);
void log_getheader(char* filename, log_header_t * h);
void log_newheader(char* filename, uint8_t monitor_id, uint16_t interval, uint16_t entry_size);

void log_createentry(char* string, uint16_t *dados, uint16_t len);
uint16_t log_writeentry(char* filename, char* entry);
uint32_t log_readentry(uint8_t logger_num, char* filename, log_entry_t* entry);

void log_gettimestamp(struct tm * ts, uint32_t time_elapsed_s);
void log_settimestamp(uint8_t logger_num, char* filename);

char* log_getfilename_to_write(uint8_t logger_num);
char* log_getfilename_to_read(uint8_t logger_num);

#endif /* LOGGER_H_ */
