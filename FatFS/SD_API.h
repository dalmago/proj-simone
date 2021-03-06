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

#ifndef SD_API_H_
#define SD_API_H_

#include "BRTOS.h"
#include "SD.h"
#include "ff.h"
#include "AppConfig.h"


/* Defines */
#define SD_FAT_MUTEX_EN		1		// Enable SD/FAT file system mutex
#define SD_BMP				0		// Enable SD/BMP file read
#define SD_GLCD_CALIB		0
#define SD_WAVE				0		// Enable SD/WAVE file read

/// SD defines
#define API_COMMAND_FAIL     (INT8U)0x80
#define API_COMMAND_OK       (INT8U)0x81
#define API_FILENAME_ERROR   (INT8U)0x82

#define NO_CAPS              (INT8U)0x83
#define CAPS_1               (INT8U)0x84
#define CAPS_2               (INT8U)0x85
#define CAPS_12              (INT8U)0x86

#define WRITE_BUFFER_SIZE    512


INT8U SDCard_Init(INT8U verbose);
INT8U SDCard_SafeRemove(INT8U verbose);

void ListFiles(CHAR8 *pname1);
INT8U ReadFile(CHAR8 *FileName, INT8U verbose);
INT8U RenameFile(CHAR8 *OldFileName,CHAR8 *NewFileName, INT8U verbose);
INT8U CreateFile(CHAR8 *FileName, INT8U verbose);
INT8U CreateDir(CHAR8 *FileName, INT8U verbose);
INT8U DeleteFile(CHAR8 *FileName, INT8U verbose);
INT8U file_name_verify(CHAR8 *pname1,CHAR8 *pname2, INT8U *pfile, INT8U num);
INT8U ChangeDir(CHAR8 *FileName, INT8U verbose);
INT8U CopyFile(CHAR8 *SrcFileName,CHAR8 *DstFileName, INT8U verbose);
INT8U WriteUptimeLog(INT8U verbose);
BRTOS_Mutex * SDCard_ResourceInit(INT8U priority);
INT8U GetLastCreatedFileName(char fileName[]);
INT8U WriteFile(FIL* fp,  const char* filename, INT8U *ptr_data, INT8U length);

FRESULT open_append (FIL* fp, const char* path );    /* [OUT] File object to create , [IN]  File name to be opened */
FRESULT empty_directory (char* path);      /* Working buffer filled with start directory */

void CSVListFiles(char **files);

void SDCard_PrintStatus(INT8U verbose, INT8U status);

#if 0
INT8U WriteFile(INT8U *FileName, void (*fill_buffer)(void),INT8U verbose);
INT8U FormatDisk(INT8U verbose);
#endif

#if 0
extern FIL      file_obj;
#endif

//Mensagens padr�o da API do SD
#if (COLDUINO || ARDUINO) && 0
extern CONST CHAR8 SD_API_CARD[];
extern CONST CHAR8 SD_API_FILE_NOT_FOUND[];
extern CONST CHAR8 SD_API_FILE_INVALID[];
extern CONST CHAR8 SD_API_CARD_BUSY[];
extern CONST CHAR8 SD_API_CARD_NOT_PRESENT[];
extern CONST CHAR8 SD_API_FILE_REMOVED[];
extern CONST CHAR8 SD_API_CARD_ERROR[];
#endif


/******** COMANDS USED IN THE APPLICATION *************/
enum  SD_STATE
{
 SD_FILE_RENAMED,
 SD_FILE_DELETED, 
 SD_DELETE_FILE_DENIED, 
 SD_FILE_READ,
 SD_FILE_COPIED,
 SD_COPY_FILE_FAILURE,
 SD_FILE_FOUND,
 SD_CREATE_FILE_FAILURE,
 SD_CREATE_FILE_OK,
 SD_CREATE_DIR_OK,
 SD_CREATE_DIR_FAILURE, 
 SD_OPEN_DIR_OK,
 SD_OPEN_DIR_FAILURE,  
 SD_FILE_WRITE_FAILURE,
 SD_FILE_WRITTEN,
 SD_FILE_SUPPORTED,
 SD_FILE_NOT_SUPPORTED,
 SD_FILE_NOT_FOUND,
 SD_FAT_OK,
 SD_FAT_ERROR,
 VERBOSE_ON,
 VERBOSE_OFF
};

enum
{
	SD_CARD_STATUS = 0,
	FILE_NOT_FOUND,
	FILE_INVALID,
	SD_CARD_NOT_PRESENT,
	FILE_REMOVED,
	SD_CARD_ERROR,
	SD_CARD_MOUNTED,
	SD_CARD_DETECTED,
	SD_CARD_MOUNT_FAILURE,
	SD_CARD_INIT_FAILURE
};


enum   
{
    NOME,
    EXTENSAO,
    FIM,
};

#endif


