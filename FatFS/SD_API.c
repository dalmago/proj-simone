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
#include "BRTOS.h"
#include "debug_stack.h"
#include "SD_API.h"
#include "printf_lib.h"
#include "drivers.h"
#include "utils.h"
#include "string.h"
#include "terminal_io.h"

#define SD_PRINT 1

#if SD_PRINT
#if ARDUINO
#define PRINT(a,...) if(a) { printf_lib(__VA_ARGS__);}
//#define PRINT(a,...) if(a) { extern char BufferText[]; snprintf_lib(BufferText,SIZEARRAY(BufferText), __VA_ARGS__);  printf_terminal(BufferText);}
#else
#define PRINT(a,...) if(a) { printf_lib(__VA_ARGS__);}
#endif
#else
#define PRINT(a,...)
#endif

#if COLDUINO || ARDUINO
#if __GNUC__
#define nop() 		__asm__ volatile ("nop");
#else
#define nop()		asm{NOP}
#endif
#elif ARDUINO	
#define nop()		asm("NOP");
#endif	

#pragma warn_implicitconv off


#if (SD_FAT_MUTEX_EN == 1)
  BRTOS_Mutex *SDCardResource = NULL;
#endif
  
SD_STATE  sd_command = SD_INACTIVE;    // Variable to indicate commands for SD

static FATFS FATFS_Obj;

// File object
FIL      file_obj;
// File object 2
//FIL      file_obj2;

// File Info object
FILINFO Finfo;
// Directory object
DIR     Dir;
// Read/Write Buffer
INT8U   Buff[512];

#if _USE_LFN
//char Lfname[512];
CHAR8 Lfname[256];
#endif


//Mensagens padr�o da API do SD
#define SD_API_FILE_NOT_FOUND_DEF		"File or directory not found.\n\r"
#define SD_API_FILE_INVALID_DEF			"Invalid file or directory name.\n\r"
#define SD_API_CARD_DEF					"\r\nSD card "
#define SD_API_CARD_BUSY_DEF			"busy!\n\r"
#define SD_API_CARD_NOT_PRESENT_DEF		"not present or not initialized !\n\r"
#define SD_API_FILE_REMOVED_DEF			"All contents removed."
#define SD_API_CARD_ERROR_DEF			"Function failed."
#define SD_API_CARD_MOUNTED_DEF			"mounted!\n\r"
#define SD_API_CARD_DETECTED_DEF		"detected!\n\r"
#define SD_API_CARD_MOUNT_FAILURE_DEF   " failed to mount!\n\r"
#define SD_API_CARD_INIT_FAILURE_DEF    " failed to initialize!\n\r"

#if COLDUINO
#define PROGMEM
#define PGM_P const char*
#endif

CONST CHAR8 SD_API_CARD[] PROGMEM =SD_API_CARD_DEF;
CONST CHAR8 SD_API_FILE_NOT_FOUND[] PROGMEM =SD_API_FILE_NOT_FOUND_DEF;
CONST CHAR8 SD_API_FILE_INVALID[] PROGMEM =SD_API_FILE_INVALID_DEF;
CONST CHAR8 SD_API_CARD_NOT_PRESENT[] PROGMEM =SD_API_CARD_NOT_PRESENT_DEF;
CONST CHAR8 SD_API_FILE_REMOVED[] PROGMEM=SD_API_FILE_REMOVED_DEF;
CONST CHAR8 SD_API_CARD_ERROR[] PROGMEM=SD_API_CARD_ERROR_DEF;
CONST CHAR8 SD_API_CARD_MOUNTED[] PROGMEM =SD_API_CARD_MOUNTED_DEF;
CONST CHAR8 SD_API_CARD_DETECTED[] PROGMEM =SD_API_CARD_DETECTED_DEF;
CONST CHAR8 SD_API_CARD_MOUNT_FAILURE[] PROGMEM =SD_API_CARD_MOUNT_FAILURE_DEF;
CONST CHAR8 SD_API_CARD_INIT_FAILURE[] PROGMEM =SD_API_CARD_INIT_FAILURE_DEF;

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

PGM_P CONST SDCard_StringTable[] PROGMEM =
{
	SD_API_CARD,
	SD_API_FILE_NOT_FOUND,
	SD_API_FILE_INVALID,
	SD_API_CARD_NOT_PRESENT,
	SD_API_FILE_REMOVED,
	SD_API_CARD_ERROR,
	SD_API_CARD_MOUNTED,
	SD_API_CARD_DETECTED,
	SD_API_CARD_MOUNT_FAILURE,
	SD_API_CARD_INIT_FAILURE
};


BRTOS_Mutex * SDCard_ResourceInit(INT8U priority)
{
  // Cria um mutex informando que o recurso est� dispon�vel
  // Prioridade m�xima a acessar o recurso = priority
#if (SD_FAT_MUTEX_EN == 1)
  OSMutexCreate(&SDCardResource,priority);
#else
  (void)priority;
#endif  
  return SDCardResource;
}

#include "terminal.h"
void SDCard_PrintStatus(INT8U verbose, INT8U status)
{
	#if COLDUINO
		PRINT(verbose==VERBOSE_ON, SDCard_StringTable[0]);	
		PRINT(verbose==VERBOSE_ON, SDCard_StringTable[status]);	
	#else
	if(verbose==VERBOSE_ON)
	{
		printf_terminal_P((PGM_P)pgm_read_word(&(SDCard_StringTable[0])));	
		printf_terminal_P((PGM_P)pgm_read_word(&(SDCard_StringTable[status])));		
	}
	#endif
}

INT8U SDCard_Init(INT8U verbose)
{ 
  FRESULT f_res;
  
#if SD_CARD_PORT && SD_CARD_DETECTION

	#if __GNUC__
	  SD_AUSENT_DIR_OUT();
	  SD_AUSENT_PUP();
	#else
	  _SD_AUSENT = _IN;
	  SD_AUSENT_PULLUP = 1; 
	#endif
	#ifdef SOCKWP
		#if __GNUC__
			SD_WP_DIR_IN();
			SD_WP_PUP();
		#else
			_SD_WP = _IN;
			SD_WP_PULLUP = 1;
		#endif
	#endif
#endif  
  
  nop();nop();nop();  
  if (GetCardInit())
  {    	
	SDCard_PrintStatus(verbose,SD_CARD_MOUNTED);	
    return SD_OK;
  }
  else
  {	
    // Check the status of the SD card and sends messages
    if (SD_AUSENT)
    {	  
      SDCard_PrintStatus(verbose, SD_CARD_NOT_PRESENT);
      return NO_SD_CARD;
    }   
    else
    {         		      
		  // Initialize SD card
		  if (disk_initialize(0) == OK)
		  {
			   SDCard_PrintStatus(verbose, SD_CARD_DETECTED); 
			  
				// Mount FAT File System
				//f_res = f_mount(0, &FATFS_Obj);
    			f_res = f_mount(&FATFS_Obj, "", 0);
        
				if (f_res != FR_OK)
				{		  
				  SDCard_PrintStatus(verbose, SD_CARD_MOUNT_FAILURE);    
				  return MOUNT_SD_FAILS;
				}else
				{     
				  #if 1	  
				  switch(GetCardType())
				  {
					case CT_MMC:
                		PRINT((verbose == VERBOSE_ON)," MMC v3");
					  break;

					case CT_SD1:
                		PRINT((verbose == VERBOSE_ON), " SD v1");
					  break;
                  
					case CT_SD2:
                		PRINT((verbose == VERBOSE_ON)," SD v2");
					  break;
                  
					case CT_SDC:
                		PRINT((verbose == VERBOSE_ON)," SDHC");
					  break;
                  
					default:
                		PRINT((verbose == VERBOSE_ON)," Unknown");
					  break;
				  }
				  #endif
				  SDCard_PrintStatus(verbose, SD_CARD_MOUNTED);	
			  return SD_OK;
			}
		}else
		{
			SDCard_PrintStatus(verbose, SD_CARD_INIT_FAILURE);
			return INIT_SD_FAILS;
		}
    }
  }
}


INT8U SDCard_SafeRemove(INT8U verbose)
{        
  if (GetCardInit())
  {    
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif    
      // Umount File System
      //f_mount(0,NULL);
      f_mount(NULL,"", 0);
      SetCardStat(STA_NOINIT | GetCardStat());
      
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif    
	    
      PRINT((verbose == VERBOSE_ON), " is safe to remove!\n\r");
      return SD_OK;
  }
  else
  {	  
	  SDCard_PrintStatus(verbose, SD_CARD_NOT_PRESENT);
      return SD_FAT_ERROR;
  }    
}


void ListFiles(INT8U *pname1)
{
	 FRESULT f_res;
	 INT32U  p1, s1, s2;
	 CHAR8   *ptr;
	 FATFS   *fs;				// Pointer to file system object*/
    
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif  
	  
    if (GetCardInit())
    {      
			// list files
			PRINT(TRUE, "\n\r");
      		if (pname1 == NULL)
      		{
      		  ptr = ".";
      		}else
      		{
      		  ptr = pname1;
      		}
      		p1 = s1 = s2 = 0;
			f_res = f_opendir(&Dir, ptr);
				
			if (!f_res)
			{
  					for(;;)
  					{
				#if _USE_LFN
  						Finfo.lfname = Lfname;
  						Finfo.lfsize = sizeof(Lfname);
				#endif            
  						f_res = f_readdir(&Dir, &Finfo);
  						if ((f_res != FR_OK) || !Finfo.fname[0]) break;
  						if (Finfo.fattrib & AM_DIR)
  						{
  							s2++;
  						} else
  						{
  							s1++;
  							p1 += Finfo.fsize;
  						}
#if 0
  					 
#else
					PRINT(TRUE,"%c%c%c%c%c %u/%02u/%02u %02u:%02u %9l  %s",
					(Finfo.fattrib & AM_DIR) ? 'D' : '-',
					(Finfo.fattrib & AM_RDO) ? 'R' : '-',
					(Finfo.fattrib & AM_HID) ? 'H' : '-',
					(Finfo.fattrib & AM_SYS) ? 'S' : '-',
					(Finfo.fattrib & AM_ARC) ? 'A' : '-',
					(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
					(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
					Finfo.fsize, &(Finfo.fname[0]));
#endif						 
                Finfo.fname[0] = 0;
				#if _USE_LFN
  					PRINT(TRUE,"  %s", Lfname);					
				#endif
  					PRINT(TRUE, "\n\r");
				
  				}
			}

			PRINT(TRUE,"%4l File(s), %l bytes total \n\r%4l Dir(s)", s1, p1, s2);

			if (f_getfree(ptr, (DWORD*)&p1, &fs) == FR_OK)
			{
				PRINT(TRUE,", %l bytes free \n\r", (uint32_t)(p1 * fs->csize * 512));
			}     
	}
	else
	{
		SDCard_PrintStatus(TRUE, SD_CARD_NOT_PRESENT);
	}

    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexRelease(SDCardResource);
    #endif
}



#define CSV_NAME_SIZE  13
void CSVListFiles(char **files)
{
	 FRESULT f_res;
	 INT32U  p1, s1, s2;
	 INT8U   i;
	 INT8U j;
	 CHAR8   *ptr;
	 char	 *file;  
	 
    
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif    
    if (GetCardInit())
    {      
        // list files
      	ptr = ".";
      	
      	p1 = s1 = s2 = 0;
		f_res = f_opendir(&Dir, ptr);
		
		file = (char*)files;				
		if (!f_res) 
		{
			i = 0;
			for(;;)
  			{            
  				f_res = f_readdir(&Dir, &Finfo);
  				if ((f_res != FR_OK) || !Finfo.fname[0]) break;
  				if (Finfo.fattrib & AM_DIR)
  				{
  					s2++;
  				} else
  				{
  					s1++;
  					p1 += Finfo.fsize;
  				}
  				
			    /* CSV is found ? */
  				for(j = 0; j<CSV_NAME_SIZE; j++)
  				{
  					if (Finfo.fname[j] == '.')
  					{
  						if ((Finfo.fname[j+1] == 'C') || (Finfo.fname[j+1] == 'c'))
  						{
  							if ((Finfo.fname[j+2] == 'S') || (Finfo.fname[j+2] == 's'))
							{
  								if ((Finfo.fname[j+3] == 'V') || (Finfo.fname[j+3] == 'v'))
  								{
  					  				for(j = 0; j<CSV_NAME_SIZE; j++)
  					  				{
  					  					
  					  					/* yes. it is a csv file */
  					  					if (j<(CSV_NAME_SIZE-1))
  					  					{
  					  						*file++ = Finfo.fname[j];
  					  					}else
  					  					{
  					  						*file++ = 0;
  					  					}
  					  				}
  					  				i++;  								
  								}
							}
  						}
  					}
  				}  				
  				
  				if (i>=CSV_NAME_SIZE) break; 
				
  			}
		}
	}

    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexRelease(SDCardResource);
    #endif
}

  
#define ReadDataHandle(SerPort, data) putcharSer(SerPort, data)


INT8U ReadFile(CHAR8 *FileName, INT8U verbose)
{
	INT32U  p1, p2, s2;
	INT16U  cnt = 0;
	INT16U  i = 0;
    INT8U   sd_status = 0;
  
  if (GetCardInit())
  {

		#if (SD_FAT_MUTEX_EN == 1)
		OSMutexAcquire(SDCardResource);
		#endif
      
		  if (f_open(&file_obj, (CHAR8*)FileName, 	FA_READ) == FR_OK)
		  {  
			PRINT((verbose == VERBOSE_ON),"\n\r");
        
			p2 = 0;  			
			SetFatTimer((INT32U)0);     
			p1 = f_size(&file_obj);
        
			while (p1) 
			{
				if (p1 >= sizeof(Buff))	
				{ 
				  cnt = sizeof(Buff);
				  p1 -= sizeof(Buff);
				}
				else 			
				{
				  cnt = (INT16U)p1;
				  p1 = 0;
				}
				if (f_read(&file_obj, (CHAR8*)Buff, cnt, (UINT*)&s2) != FR_OK)
				{
				  break;
				}else
				{
					p2 += s2;
					if (cnt != s2) break;  					                					
			
					// Envia os dados para a porta serial
					for(i=0;i<cnt;i++)
					{
					  ReadDataHandle(USE_USB, Buff[i]);
					}
				}
			} // end while
				
			GetFatTimer(&s2);
			f_close(&file_obj);
        
			PRINT(TRUE,"\n\r%u bytes read with %u bytes/sec.\n\r", p2, s2 ? (p2 * 100 / s2) : 0);        
			PRINT(TRUE, "\n\r");
        
			#if (SD_FAT_MUTEX_EN == 1)
			  OSMutexRelease(SDCardResource);
			#endif        
        
			return SD_FILE_READ;
		} 
		else
		{       
        
			PRINT((verbose == VERBOSE_ON), "\n\r");
			PRINT((verbose == VERBOSE_ON), (CHAR8*)FileName);
			PRINT((verbose == VERBOSE_ON), " not found.\n\r");
			
			#if (SD_FAT_MUTEX_EN == 1)
			OSMutexRelease(SDCardResource);
			#endif
			
			return SD_FILE_NOT_FOUND;
		}
	}
	else
	{
		PRINT((verbose == VERBOSE_ON), SD_API_CARD);
		PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_NOT_PRESENT);
		return SD_FAT_ERROR;
	}
}


INT8U ChangeDir(CHAR8 *FileName, INT8U verbose)
{
  INT8U   sd_status = 0;
  
  if (GetCardInit())
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif 
      
      if (f_chdir(FileName) == FR_OK)
      {  
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\r");        
        return SD_OPEN_DIR_OK;
      }
      else
      {
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\rDirectory ");
        PRINT((verbose == VERBOSE_ON), (CHAR8*)FileName);
        PRINT((verbose == VERBOSE_ON), (CHAR8*)" does not exist !\n\r");
        
        return SD_OPEN_DIR_FAILURE;
      }
  }
  else
  {
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_NOT_PRESENT);
      return SD_FAT_ERROR;
  }
}

INT8U CreateFile(CHAR8 *FileName, INT8U verbose)
{
  INT8U   sd_status = 0;
  
  if (GetCardInit())
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif       
    
    if (sd_command == SD_INACTIVE)
    {    
      sd_command = SD_FILE_WRITING;
      
      if (f_open(&file_obj, FileName, 	FA_CREATE_NEW) == FR_OK)
      {  
        f_close(&file_obj);
        
        //Sets these variables to inactive states
        sd_command = SD_INACTIVE;        
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\r");
        PRINT((verbose == VERBOSE_ON), (CHAR8*)FileName);
        PRINT((verbose == VERBOSE_ON), (CHAR8*)" was created successfully.\n\r");
        
        return SD_CREATE_FILE_OK;
      }
      else
      {
        sd_command = SD_INACTIVE;
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\r");
        PRINT((verbose == VERBOSE_ON), (CHAR8*)FileName);
        PRINT((verbose == VERBOSE_ON), (CHAR8*)" was not created.\n\r");        
        return SD_CREATE_FILE_FAILURE;
      }
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
	  
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      return SD_BUSY;
    }
  }
  else
  {
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_NOT_PRESENT);
      return SD_FAT_ERROR;
  }
}


INT8U CreateDir(CHAR8 *FileName, INT8U verbose)
{
  INT8U   sd_status = 0;
  
  if (GetCardInit())
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif    
        
    if (sd_command == SD_INACTIVE)
    {    
      sd_command = SD_DIR_CREATING;
      
      if (f_mkdir( FileName) == FR_OK)
      {  
        sd_command = SD_INACTIVE;        
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\rDirectory ");
        PRINT((verbose == VERBOSE_ON), (CHAR8*)FileName);
        PRINT((verbose == VERBOSE_ON), (CHAR8*)" was created successfully.\n\r");
        
        return SD_CREATE_DIR_OK;
      }
      else
      {
        sd_command = SD_INACTIVE;
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\rDirectory ");
        PRINT((verbose == VERBOSE_ON), (CHAR8*)FileName);
        PRINT((verbose == VERBOSE_ON), (CHAR8*)" was not created.\n\r");
        
        return SD_CREATE_DIR_FAILURE;
      }
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_BUSY);
      
      return SD_BUSY;
    }
  }
  else
  {
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_NOT_PRESENT);
      return SD_FAT_ERROR;
  }
}


INT8U DeleteFile(CHAR8 *FileName, INT8U verbose)
{
  INT8U sd_status = 0;
  
  if (GetCardInit())
  {  
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif    
    
    if (sd_command == SD_INACTIVE)
    {
      // Indicates that the file will be deleted
      sd_command = SD_FILE_DELETING;
      
      sd_status = f_unlink(FileName);
      
      sd_command = SD_INACTIVE;      
      
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
      
      if (sd_status == FR_OK)
      {

        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\r");                       
        PRINT((verbose == VERBOSE_ON), (CHAR8*)FileName);
        PRINT((verbose == VERBOSE_ON), (CHAR8*)" deleted! \n\r");
      
        return SD_FILE_DELETED;
      }
      else
      {
        if (sd_status == FR_DENIED)
        {
           PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\rDelete file or directory denied.\n\r");
           PRINT((verbose == VERBOSE_ON), (CHAR8*)"Directory is not empty or file is write-protected.\n\r");        
           return SD_DELETE_FILE_DENIED;
        }
        else
        {
          PRINT((verbose == VERBOSE_ON), SD_API_FILE_NOT_FOUND);
          return SD_FILE_NOT_FOUND;
        }
      }
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
		PRINT((verbose == VERBOSE_ON), SD_API_CARD);
       PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_BUSY);

      return SD_BUSY;
    }
  }
  else
  {
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_NOT_PRESENT);
      return SD_FAT_ERROR;
  }
}


INT8U RenameFile(CHAR8 *OldFileName,CHAR8 *NewFileName, INT8U verbose)
{
  INT8U sd_status = 0;
  
  if (GetCardInit())
  {  
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif    
    
    if (sd_command == SD_INACTIVE)
    {
      // Indicates that the file will be renamed
      sd_command = SD_FILE_RENAMING;
      
      // Passa para a fun��o os nomes dos arquivos
      sd_status = f_rename(OldFileName, NewFileName);

      sd_command = SD_INACTIVE;
      
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
      
      if(sd_status == FR_OK)
      {
        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\rFile found and renamed !\n\r");      
        
        return SD_FILE_RENAMED;
      }
      else
      {
           
        PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_FILE_NOT_FOUND);       
        
        return SD_FILE_NOT_FOUND;
      }
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
	  
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_BUSY);
      
      return SD_BUSY;
    }
  }
  else
  {
      PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_NOT_PRESENT);
      
      return SD_FAT_ERROR;
  }
}

#if 0	
INT8U CopyFile(CHAR8 *SrcFileName,CHAR8 *DstFileName, INT8U verbose)
{
	
  INT32U  p1, p2, s1, s2;
  CHAR8   *NewDstName, *CopyName;
  INT8U   f_res = 0;
  
  if (GetCardInit())
  {  
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif

    if (sd_command == SD_INACTIVE)
    {
      // Indicates that the file will be renamed
      sd_command = SD_FILE_COPYING;
      
      // Passa para a fun��o os nomes dos arquivos
      if (f_open(&file_obj, SrcFileName, FA_OPEN_EXISTING | FA_READ) != FR_OK)
      {        
        sd_command = SD_INACTIVE;
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\rSource file does not exist !\n\r");      
        
        return SD_COPY_FILE_FAILURE;      
      }
      
      if (f_open(&file_obj2, DstFileName, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
      {
        // Complementa nome do arquivo
        CopyName = SrcFileName;
        NewDstName = DstFileName;
        
        while(*NewDstName)
        {
          NewDstName++;
        }
        
        NewDstName--;
        if (*NewDstName != '/')
        {
          NewDstName++;
          *NewDstName = '/';
          NewDstName++;
        }else
        {
          NewDstName++;
        }
        
        strcpy(NewDstName,CopyName);
        
        if (f_open(&file_obj2, DstFileName, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
        {                
          sd_command = SD_INACTIVE;
          
          #if (SD_FAT_MUTEX_EN == 1)
            OSMutexRelease(SDCardResource);
          #endif          
          
          PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\rDestination file could not be created or already exist !\n\r");       
          
            
          f_close(&file_obj);
          return SD_COPY_FILE_FAILURE;            
        }
      }

      PRINT((verbose == VERBOSE_ON), (CHAR8*)"\n\rCopying file\n\r");
     
      
	  SetFatTimer((INT32U)0);  
	  p1 = 0;
		
	  for (;;) 
	  {
		f_res = f_read(&file_obj, Buff, sizeof(Buff), (UINT*)&s1);
		if (f_res || s1 == 0) break;   /* error or eof */
		f_res = f_write(&file_obj2, Buff, s1, (UINT*)&s2);
		p1 += s2;
		if (f_res || s2 < s1) break;   /* error or disk full */											
	  }			
		
	  f_close(&file_obj);
	  f_close(&file_obj2);
      
            
       GetFatTimer(&p2);                                   
       PRINT((verbose == VERBOSE_ON),"\n\r%u bytes copied with %u bytes/sec.\n\r", p1, p2 ? (p1 * 100 / p2) : 0);    
 
      
      sd_command = SD_INACTIVE;
      
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
      
      return SD_FILE_COPIED;
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
	  
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON),(CHAR8*)SD_API_CARD_BUSY);
      return SD_BUSY;
    }
  }
  else
  {
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
	  PRINT((verbose == VERBOSE_ON),(CHAR8*)SD_API_CARD_NOT_PRESENT);
      return SD_FAT_ERROR;
  }  
}
#endif



INT8U WriteUptimeLog(INT8U verbose)
{
  INT8U   sd_status = 0;
  OSTime Uptime;
  OSDate UpDate;   
  
  if (GetCardInit())
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif

    if (sd_command == SD_INACTIVE)
    {    
        sd_command = SD_FILE_WRITING;
        
        if (f_open(&file_obj, "uptime.txt", 	FA_WRITE) == FR_NO_FILE)
        {     
          f_open(&file_obj, "uptime.txt", 	FA_CREATE_NEW); 
          f_open(&file_obj, "uptime.txt", 	FA_WRITE);
        }
        f_lseek(&file_obj,f_size(&file_obj));
        
        UserEnterCritical();
        Uptime = OSUptime();
        UpDate = OSUpDate();
        UserExitCritical();
        
        f_printf(&file_obj, "Uptime: %d Days, %d Hours, %d minutes and %d seconds \n\r", UpDate.RTC_Day, Uptime.RTC_Hour, Uptime.RTC_Minute,Uptime.RTC_Second);  
        f_close(&file_obj);
        
        sd_command = SD_INACTIVE;
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        PRINT((verbose == VERBOSE_ON),(CHAR8*)"\n\rUptime written !\n\r");  
        
        return SD_FILE_WRITTEN;
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      
	  
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
      PRINT((verbose == VERBOSE_ON), (CHAR8*)SD_API_CARD_BUSY);

      return SD_BUSY;
    }
  }
  else
  {
	  PRINT((verbose == VERBOSE_ON), SD_API_CARD);
	  PRINT((verbose == VERBOSE_ON),(CHAR8*)SD_API_CARD_NOT_PRESENT);
      return SD_FAT_ERROR;
  }
}

#include <ctype.h>

INT8U file_name_verify(CHAR8 *pname1,CHAR8 *pname2, INT8U *pfile, INT8U num)
{
   INT8U i=0;
   INT8U j=0;
   INT8U number = num;
   INT8U test_caps = 0;
   INT8U estado = NOME;   

   while (num) 
   {   
     j=0;
     i=0;
     if(*pfile!=0x20) return API_FILENAME_ERROR; //verifica se foi digitado espa�o entre o comando e o nome do arquivo  
     pfile++;         //incrementa endere�o do vetor de dados para iniciar a leitura do nome do arquivo que foi digitado          
     
     estado = NOME;   
     while(estado!=FIM) //faz a leitura at� que chegue ao final do arquivo
        {
          switch(estado)
            {
              case NOME:   //estado inicial, come�a pelo nome do arquivo
                if(*pfile!='.')//faz a leitura do nome at� encontar o ponto ou o n�mero m�ximo de caracteres estourar
                {
                   
                   if (!isalnum(*pfile))
                   {
					  return API_FILENAME_ERROR;
                   }    
                   
                   if (num == 1)*pname1=*pfile; //faz a leitura letra por letra
                   if (num == 2)*pname2=*pfile; //se existirem dois nomes de arquivo (caso rename) le o segundo arquivo ap�s ler o primeiro 
                   pfile++;
                   if (num == 1) pname1++;
                   if (num == 2) pname2++;
                   i++;                   
                   
                   if(i>=60)//se o nome exceder o n�mero m�ximo de caracteres
                   {
                      return API_FILENAME_ERROR; 
                   }
                   
                   if(*pfile==0x20)
                   {
					   if ((number > 1) && (num == 2))
					   {                      
						   estado = FIM;
					   }
					   else
					   {                      
						  return API_FILENAME_ERROR; 
					   }
                   }
                                      
                   if(*pfile==0)
                   {
                      *pname1=*pfile;
                      *pname2=*pfile;
                      estado = FIM;
                   }                          
                }
                else //quando o ponto for encontrado e o nome estiver correto muda estado para ler a extens�o do arquivo
                {
                     if (num == 1)*pname1=*pfile;
                     if (num == 2)*pname2=*pfile;
                     estado = EXTENSAO;
                     pfile++;
                     if (num == 1) pname1++;
                     if (num == 2) pname2++;
                     i++;                   
                }
                
              break;
                
                case EXTENSAO:
                  if((*pfile!=0x20)&&(*pfile!=0))//verifica se n�o existe espa�os ou caracteres incorretos
                  {
					if (!isalnum(*pfile))
					{
					  return API_FILENAME_ERROR;
					}             
                    
                    if (num == 1)*pname1=*pfile;
                    if (num == 2)*pname2=*pfile;
                    pfile++;
                    if (num == 1) pname1++;
                    if (num == 2) pname2++;
                    j++;
                    
                    // fim de string
                    if (j == 3)
                    {                    	
                        if (num == 1)*pname1=0;
                        if (num == 2)*pname2=0;
                    }else
                    {
						if(j>=4) //se a extens�o for maior do que tr�s caracteres retorna erro
						{
						  return API_FILENAME_ERROR; 
						}
                    }
                  }
                  else
                  {
                    // Um arquivo pode n�o ter extens�o ou ter extens�o menor do que 3 caracteres
                    if(j)
                    {                      
                      if (number == 1)
                      {
                        if(*pfile==0x20)
                        {
                          return API_FILENAME_ERROR; 
                        }
                      }
                      estado = FIM; //vai para estado final
                    }
                    else
                    {
                      return API_FILENAME_ERROR; 
                    }
                     
                  }
                break;
                
                default:
                return API_FILENAME_ERROR;
                break;
            }        
        }
        num--;// decrementa num e le o pr�ximo arquivo, se existir
   }   
   
   return API_COMMAND_OK; //retorna leitura de aquivos correta

}

/*------------------------------------------------------------/
/ Open or create a file in append mode
/------------------------------------------------------------*/

FRESULT open_append (
    FIL* fp,            /* [OUT] File object to create */
    const char* path    /* [IN]  File name to be opened */
)
{
    FRESULT fr;
    
#if (SD_FAT_MUTEX_EN == 1)
  OSMutexAcquire(SDCardResource);
#endif 
  
    /* Opens an existing file. If not exist, creates a new file. */
    fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
        /* Seek to end of the file to append data */
        fr = f_lseek(fp, f_size(fp));
        if (fr != FR_OK)
            f_close(fp);
    }
    
#if (SD_FAT_MUTEX_EN == 1)
  OSMutexRelease(SDCardResource);
#endif     
  
    return fr;
}

/*------------------------------------------------------------/
/ Remove all contents of a directory
/ This function works regardless of _FS_RPATH.
/------------------------------------------------------------*/

FRESULT empty_directory (
    char* path      /* Working buffer filled with start directory */
)
{
    UINT i, j;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    
#if (SD_FAT_MUTEX_EN == 1)
  OSMutexAcquire(SDCardResource);
#endif 

#if _USE_LFN
    fno.lfname = 0; /* Disable LFN output */
#endif
    fr = f_opendir(&dir, path);
    if (fr == FR_OK) {
        for (i = 0; path[i]; i++) ;
        path[i++] = '/';
        for (;;) {
            fr = f_readdir(&dir, &fno);
            if (fr != FR_OK || !fno.fname[0]) break;
            if (_FS_RPATH && fno.fname[0] == '.') continue;
            j = 0;
            do
                path[i+j] = fno.fname[j];
            while (fno.fname[j++]);
            if (fno.fattrib & AM_DIR) {
                fr = empty_directory(path);
                if (fr != FR_OK) break;
            }
            fr = f_unlink(path);
            if (fr != FR_OK) break;
        }
        path[--i] = '\0';
        f_closedir(&dir);
    }
    
#if (SD_FAT_MUTEX_EN == 1)
  OSMutexRelease(SDCardResource);
#endif     

    return fr;
}

#define NIB_LOW(c)	((c) & 0x0F)
#define NIB_HIGH(c) ((c >> 4) & 0x0F)
		
static void bin2char(CHAR s[], INT8U c)
{
	s[1] = ((NIB_LOW(c)) < 10) ? (NIB_LOW(c)) + '0': (NIB_LOW(c)) + '7'; 
	s[0] = ((NIB_HIGH(c)) < 10) ? (NIB_HIGH(c)) + '0': (NIB_HIGH(c)) + '7'; 
}
		
INT8U WriteFile(FIL* fp,  const char* filename, INT8U *ptr_data, INT8U length)
{

  if (ptr_data == NULL) {  return SD_FAT_ERROR; }
  
  if (GetCardInit())
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif

    if (sd_command == SD_INACTIVE)
    {    
        sd_command = SD_FILE_WRITING;
        
        if (f_open(fp, filename, FA_WRITE) == FR_NO_FILE)
        {     
        	f_open(fp, filename, FA_CREATE_NEW | FA_WRITE); 
        }
        
        /* Move o cursor para o fim do arquivo */
        f_lseek(fp,f_size(fp));
				
		INT8U c;
		CHAR s[2];		
		
		f_putc('W',fp);
		
		while(length > 0)
		{
			c = *ptr_data;			
			ptr_data++;
			length--;
			bin2char(s,c);
			
			f_putc(s[1],fp);
			f_putc(s[0],fp);
			//f_putc(',',fp);
		}
		
		f_printf(fp, "\n\r");
		
        f_close(fp);
        
        sd_command = SD_INACTIVE;
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif       
        
        return SD_FILE_WRITTEN;
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif 
      return SD_BUSY;
    }
  }
  else
  {
      return SD_FAT_ERROR;
  }
}

/* Pega ultimo arquivo criado. A fun��o lista os arquivos por data
 * de cria��o. Desta forma, o �ltmo arquivo salvo � o �litmo listado.
 *  */
INT8U GetLastCreatedFileName(char fileName[])
{

	FRESULT f_res;
	CHAR8 *ptr = ".";
	INT8U ret = FALSE;

#if (SD_FAT_MUTEX_EN == 1)
	OSMutexAcquire(SDCardResource);
#endif    
	if (GetCardInit())
	{
		if (sd_command == SD_INACTIVE)
		{
			sd_command = SD_FILE_LISTING;

			f_res = f_opendir(&Dir, ptr);
			
			/* Percorre o diret�rio.*/
			if (!f_res)
			{
				for (;;)
				{
#if _USE_LFN
					Finfo.lfname = Lfname;
					Finfo.lfsize = sizeof(Lfname);
#endif
					f_res = f_readdir(&Dir, &Finfo);

					if ((f_res != FR_OK) || !Finfo.fname[0])
					{
						break;						
					}
					else
					{
						/* guarda nome do ultimo arquivo salvo */
						strcpy(fileName, Finfo.fname);
						ret = TRUE;
					}
					
					Finfo.fname[0] = 0;
				}
			}

			sd_command = SD_INACTIVE;			
		}
	}

#if (SD_FAT_MUTEX_EN == 1)
	OSMutexRelease(SDCardResource);
#endif
	
	return ret;
}



#if (SD_BMP == 1)
INT8U OpenBmp(D4D_POINT* ppt, const D4D_BMP* pBmp, D4D_BOOL greyScale)
{
  INT32U           p1, p2, s2;
  INT16U           cnt = 0;
  INT16U           i = 0;
  INT8U            sd_status = 0;
  D4D_BMP_states   bmp_state = BMP_HEADER;  
  CHAR8            *FileName;
  
  FileName = (CHAR8*)pBmp->pParam;
  
  if (GetCardInit())
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif
    
    if (sd_command == SD_INACTIVE)
    {    
      sd_command = SD_FILE_READING;
      
      if (f_open(&file_obj, FileName, 	FA_READ) == FR_OK)
      {  
				p2 = 0;
				p1 = f_size(&file_obj);
        
				while (p1) 
				{
					if (p1 >= sizeof(Buff))	
					{
					  cnt = sizeof(Buff);
					  p1 -= sizeof(Buff);
					}
					else 			
					{
					  cnt = (INT16U)p1;
					  p1 = 0;
					}
					if (f_read(&file_obj, Buff, cnt, (UINT*)&s2) != FR_OK)
					{
					  break;
					}else
					{
						p2 += s2;
						if (cnt != s2) break;
  					                					
						// Align problem on kinetis MCU						
						BmpFile = *(BMP_FILE*)Buff;
							
						#if PSP_ENDIAN == BRTOS_LITTLE_ENDIAN
              // The values are in little endian
						#else
							BmpFile.bfsize          = LWordSwap(BmpFile.bfsize);
							BmpFile.bfoffsetbits    = LWordSwap(BmpFile.bfoffsetbits);
							BmpFile.size            = LWordSwap(BmpFile.size);
							BmpFile.width           = LWordSwap(BmpFile.width);
							BmpFile.height          = LWordSwap(BmpFile.height);
							BmpFile.planes          = ByteSwap(BmpFile.planes);
							BmpFile.bitcount        = ByteSwap(BmpFile.bitcount);
							BmpFile.compression     = LWordSwap(BmpFile.compression);
							BmpFile.sizeimage       = LWordSwap(BmpFile.sizeimage);
							BmpFile.colorsused      = LWordSwap(BmpFile.colorsused);
							BmpFile.colorsimportant = LWordSwap(BmpFile.colorsimportant);						
						#endif
											
						break;
					}
				}
				
				f_close(&file_obj);
        
				D4D_DrawBmpFromFileSystem(ppt, pBmp, greyScale, &BmpFile, Buff);
        
        //Sets these variables to inactive states
        sd_command = SD_INACTIVE;                
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        return SD_FILE_READ;
      } 
      else
      {               
        sd_command = SD_INACTIVE;
      
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        return SD_FILE_NOT_FOUND;
      }
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      

      return SD_BUSY;
    }
  }
  else
  {
      return SD_FAT_ERROR;
  }
}

CopyCalibValue(INT16U *calib, INT16U *index, INT8U value)
{
	INT16U i = 0;
	switch(*index)
	{
		case 0:
			if ((value >= 48) && (value <= 57))
			{
				i = 10000 * (value-48);
			}
			break;			
		case 1:
			if ((value >= 48) && (value <= 57))
			{			
				i = 1000 * (value-48);
			}
			break;
		case 2:
			if ((value >= 48) && (value <= 57))
			{			
				i = 100 * (value-48);
			}
			break;
		case 3:
			if ((value >= 48) && (value <= 57))
			{			
				i = 10 * (value-48);
			}
			break;
			
		case 4:
			if ((value >= 48) && (value <= 57))
			{			
				i = (value-48);
			}
			break;			
	}
	
	*calib += i;
}


INT8U ReadCalibrationFile(CHAR8 *FileName, D4D_TOUCHSCREEN_CALIB *d4d_tch)
{
	INT32U  p1, p2, s2;
	INT16U  cnt = 0;
	INT16U  i = 0;
	INT16U  j = 0;
	INT16U  k = 0;
	INT16U  value = 0;
    INT8U   sd_status = 0;
  
  if (GetCardInit())
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif
    
    if (sd_command == SD_INACTIVE)
    {    
      sd_command = SD_FILE_READING;
      
      if (f_open(&file_obj, (CHAR8*)FileName, 	FA_READ) == FR_OK)
      {          
		p2 = 0;  			     
		p1 = f_size(&file_obj);
        
		while (p1) 
		{
			if (p1 >= sizeof(Buff))	
			{ 
			  cnt = sizeof(Buff);
			  p1 -= sizeof(Buff);
			}
			else 			
			{
			  cnt = (INT16U)p1;
			  p1 = 0;
			}
			if (f_read(&file_obj, (CHAR8*)Buff, cnt, (UINT*)&s2) != FR_OK)
			{
			  break;
			}else
			{
			p2 += s2;
			if (cnt != s2) break;			
								
			for(i=0;i<cnt;i++)
			{
				if (Buff[i] == ',')
				{
					j++;
					k = 0;
				}else
				{
					switch(j)
					{
						case 0:
							CopyCalibValue(&d4d_tch->TouchScreenXoffset, &k, Buff[i]);
							k++;
							break;
							
						case 1:
							CopyCalibValue(&d4d_tch->TouchScreenYoffset, &k, Buff[i]);
							k++;
							break;
							
						case 2:
							CopyCalibValue(&d4d_tch->TouchScreenXBitsPerPixelx16, &k, Buff[i]);
							k++;
							break;						
						
						case 3:
							CopyCalibValue(&d4d_tch->TouchScreenYBitsPerPixelx16, &k, Buff[i]);
							k++;
							break;
							
						default:
							break;						
					}
				}
			}
		 }
		}
		
        f_close(&file_obj);
        
        //Sets these variables to inactive states
        sd_command = SD_INACTIVE;                       
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        if (k <= 5)
        {
          return SD_FILE_READ;
        }else
        {
          return SD_FILE_NOT_FOUND;	
        }
      } 
      else
      {               
        sd_command = SD_INACTIVE;
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif
        
        return SD_FILE_NOT_FOUND;
      }
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif                        
      
      return SD_BUSY;      
    }
  }
  else
  {
      return SD_FAT_ERROR;
  }
}


INT8U WriteCalibrationFile(D4D_TOUCHSCREEN_CALIB *d4d_tch)
{
  INT8U  sd_status = 0;   
  
  if (GetCardInit())
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif

    if (sd_command == SD_INACTIVE)
    {    
        sd_command = SD_FILE_WRITING;
        
        if (f_open(&file_obj, "calib.txt", 	FA_WRITE) == FR_NO_FILE)
        {     
          f_open(&file_obj, "calib.txt", 	FA_CREATE_NEW); 
          f_open(&file_obj, "calib.txt", 	FA_WRITE);
        }
        f_lseek(&file_obj,f_size(&file_obj));        
        
        f_printf(&file_obj, "%05u,%05u,%05u,%05u", d4d_tch->TouchScreenXoffset, d4d_tch->TouchScreenYoffset, d4d_tch->TouchScreenXBitsPerPixelx16,d4d_tch->TouchScreenYBitsPerPixelx16);  
        f_close(&file_obj);
        
        sd_command = SD_INACTIVE;
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif                
        
        return SD_FILE_WRITTEN;
    }
    else
    {
      #if (SD_FAT_MUTEX_EN == 1)
        OSMutexRelease(SDCardResource);
      #endif      

      return SD_BUSY;
    }
  }
  else
  {
      return SD_FAT_ERROR;
  }
}


#endif




/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*	 																	*/
/*	 					Wave API functions								*/
/*	 																	*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
#if (SD_WAVE == 1)


/*----------------------------------------------------------------------*/
/*  Null Buffer Error function											*/
/*----------------------------------------------------------------------*/
static
void Wave_NullBufferError(void)
{
	WaveBuff.Buff 		= NULL;
	WaveBuff.BitRate	= 0;
	WaveBuff.Size		= 0;
	(void)OSMboxPost(SendWaveBuffer,(void *)&WaveBuff); //Mail Box	
}

/*----------------------------------------------------------------------*/
/*  Test File Support function											*/
/*----------------------------------------------------------------------*/
static
INT8U Wave_TestFileSupport(void)
{
	//"RIFF" in little-endian form	
	if(WaveFile.ChunkID	!= 0x46464952) //Or 1179011410, but tested with hex in order to avoid big-endian files
	{return SD_FILE_NOT_SUPPORTED;}

	//"WAVE"
	else if(WaveFile.Format	!= 1163280727)
	{return SD_FILE_NOT_SUPPORTED;}

	//"fmt"
	else if(WaveFile.Subchunk1ID	!= 544501094)
	{return SD_FILE_NOT_SUPPORTED;}

	//1 = Linear quantization; Others = compression
	else if(WaveFile.AudioFormat != 1)
	{return SD_FILE_NOT_SUPPORTED;}

	//1=mono, 2=Stereo, ...	
	else if(WaveFile.NumChannels >= 3)
	{return SD_FILE_NOT_SUPPORTED;}

	//8.000, 11.025, 22.050 or 44.100kHz
	else if((WaveFile.SampleRate == 8000) || (WaveFile.SampleRate == 11025) || (WaveFile.SampleRate	== 22050)  || (WaveFile.SampleRate	== 44100)/* || (WaveFile.SampleRate	== 48000)*/)
	{}else
	{return SD_FILE_NOT_SUPPORTED;}

	//8 or 16
	if((WaveFile.BitsPerSamples == 8) || (WaveFile.BitsPerSamples == 16) )
	{}else
	{return SD_FILE_NOT_SUPPORTED;}

	//"data"
	if(WaveFile.Subchunk2ID	!= 1635017060)
	{return SD_FILE_NOT_SUPPORTED;}

	//All tests Ok
	else
	{return SD_FILE_SUPPORTED;}
	
}


/*----------------------------------------------------------------------*/
/* Open/Read wave file function											*/
/*----------------------------------------------------------------------*/
extern INT8U		PlayState;
INT8U Wave_ReadFile(CHAR8 *FileName, INT8U* Buffer0, INT8U* Buffer1)
{
  INT32U           	p1, p2, s2;
  INT16U           	cnt = 0;
  INT16U           	i = 0;
  INT16U			j = 0;
  INT16U			ReadBytes;
  INT8U				*pData;
  INT8U            	sd_status = 0;
  INT8U				SelectBuffer = 0;	//Select Buffer 0 or 1
  INT8U				playstate = 0;

  
  if ((GetCardStat() & STA_NOINIT) != STA_NOINIT)
  {
    #if (SD_FAT_MUTEX_EN == 1)
      OSMutexAcquire(SDCardResource);
    #endif
    
    if (sd_command == SD_INACTIVE)
    {    
      sd_command = SD_FILE_READING;
      
      if (f_open(&file_obj, FileName, 	FA_READ) == FR_OK)
      {  
		p2 = 0;
		p1 = f_size(&file_obj);

		while (p1)	//Read file
		{
			if (p1 >= sizeof(Buff))		//SD card buffer	
			{
			  cnt = sizeof(Buff);		//SD card buffer
			  p1 -= sizeof(Buff);		//SD card buffer
			}
			else 			
			{
			  cnt = (INT16U)p1;
			  p1 = 0;
			}

			if (SelectBuffer == 0) //Seleciona buffer de armazenamento
			{
				pData = Buffer0;
			}
			else
			{
				pData = Buffer1;
			}
			
			//L� arquivo:
			if (f_read(&file_obj, pData, cnt, (UINT*)&s2) != FR_OK)	//Error test
			{
			  break;
			}
			else
			{
				p2 += s2;
				if (cnt != s2) break;	//Error test
												
				if (j == 0)	//1�bloco = header
				{
					// Align problem on kinetis MCU						
					WaveFile = *(WAVE_FILE*)pData;
					
					#if PSP_ENDIAN != BRTOS_LITTLE_ENDIAN					
					   WaveFile.ChunkID         = LWordSwap(WaveFile.ChunkID);
					   WaveFile.ChunkSize       = LWordSwap(WaveFile.ChunkSize);
					   WaveFile.Format			= LWordSwap(WaveFile.Format);
					   WaveFile.Subchunk1ID		= LWordSwap(WaveFile.Subchunk1ID);
					   WaveFile.Subchunk1Size	= LWordSwap(WaveFile.Subchunk1Size);
					   WaveFile.AudioFormat		= ByteSwap(WaveFile.AudioFormat);
					   WaveFile.NumChannels		= ByteSwap(WaveFile.NumChannels);
					   WaveFile.SampleRate		= LWordSwap(WaveFile.SampleRate);
					   WaveFile.ByteRate		= LWordSwap(WaveFile.ByteRate);
					   WaveFile.BlockAlign		= ByteSwap(WaveFile.BlockAlign);
					   WaveFile.BitsPerSamples	= ByteSwap(WaveFile.BitsPerSamples);
					   WaveFile.Subchunk2ID		= LWordSwap(WaveFile.Subchunk2ID);
					   WaveFile.Subchunk2Size	= LWordSwap(WaveFile.Subchunk2Size);
					#endif
	
					pData += 44;			//Header wave size
					ReadBytes = (s2 - 44);	//Armazena do buffer passado para tarefa de Play

					
					// Test File support
				#if (TestWaveFileSuport == 1) 
					if(Wave_TestFileSupport() != SD_FILE_SUPPORTED)
					{
						(void)Wave_NullBufferError(); //EOF - end of file: Return Error (NULL) Buffer
						
						//File Close
						f_close(&file_obj);

				        //Sets these variables to inactive states
				        sd_command = SD_INACTIVE;				        
				        
				        #if (SD_FAT_MUTEX_EN == 1)
				          OSMutexRelease(SDCardResource);
				        #endif
						return SD_FILE_NOT_SUPPORTED;
					}
				#endif
					
					//Preencher dados:
					WaveBuff.SampleRate	= WaveFile.SampleRate;
					WaveBuff.BitRate	= WaveFile.BitsPerSamples;
					WaveBuff.NumChannels= WaveFile.NumChannels;
				
				}
				else
				{
					ReadBytes = s2;			//Armazena do buffer passado para tarefa de Play		
				}
				
				if (WaveFile.BitsPerSamples == 16) //
				{
					ReadBytes = ReadBytes >>1;
				}
			}

			//L� dados do wave
			
			//Troca de buffer:
			if(SelectBuffer)
			{
				SelectBuffer = 0;
			}
			else
			{
				SelectBuffer = 1;
			}
			
			// Se o 2 buffer estao cheios, espera o "play" de um deles:
			if (j)
			{
				OSSemPend(SyncWaveBuffer,0);
			}
			
			UserEnterCritical();
			playstate = PlayState;
			UserExitCritical();
			// Se o estado � de reproduzindo um arquivo de som
			if (playstate == PLAYING)
			{
				//Acorda tarefa PlayWaveSound:					
				WaveBuff.Buff 		= pData;
				WaveBuff.Size		= ReadBytes;
				(void)OSMboxPost(SendWaveBuffer,(void *)&WaveBuff); //Mail Box
				j++;	//N�mero de buffers cheios
			}
			else
			{
				// Sen�o finaliza a leitura do arquivo e consequentemente a sua reprodu��o
				break;
			}
		}

		if (playstate == PLAYING)
		{
			OSSemPend(SyncWaveBuffer,0);
		}
		
		(void)Wave_NullBufferError(); //EOF - end of file: Return Error (NULL) Buffer
		
		//File Close
		f_close(&file_obj);

        //Sets these variables to inactive states
        sd_command = SD_INACTIVE;                
        
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        
        
        return SD_FILE_READ;
      } 
      else
      {               
        sd_command = SD_INACTIVE;
      
        #if (SD_FAT_MUTEX_EN == 1)
          OSMutexRelease(SDCardResource);
        #endif        

        (void)Wave_NullBufferError(); //No sound in buffer to play: Return Error (NULL) Buffer
          
        return SD_FILE_NOT_FOUND;
      }
    }
    else
    {
		#if (SD_FAT_MUTEX_EN == 1)
			OSMutexRelease(SDCardResource);
		#endif      
		
        (void)Wave_NullBufferError(); //No sound in buffer to play: Return Error (NULL) Buffer

        return SD_BUSY;
    }
  }
  else
  {
	  (void)Wave_NullBufferError(); //EOF - end of file: Return Error (NULL) Buffer
	  return SD_FAT_ERROR;
  }
}



/*----------------------------------------------------------------------*/
/* Install Play Wave file Components  function							*/
/*----------------------------------------------------------------------*/
void InstallPlayComponents(void)
{
	if (OSSemCreate(0,&SyncWaveBuffer) != ALLOC_EVENT_OK)
	{
		while(1)
		{
			DelayTask(1000);
			//Colocar Block Task:
			//(void)BlockTask(TaskID);
		}
	}
	
	if (OSMboxCreate(&SendWaveBuffer,NULL) != ALLOC_EVENT_OK)
	{
		while(1)
		{
			DelayTask(1000);
			//Colocar Block Task:
			//(void)BlockTask(TaskID);
		}
	}	
}


void Wave_SetSampleRate(INT16U WaveSampleRate)
{

	/*
	 * Timer Clock Source = configCPU_CLOCK_HZ (with USB = 24000000)
	 */

	switch (WaveSampleRate)
	  {
	    case 8000:		// 8kHz:
	    	Timer2ChangeModule(((configCPU_CLOCK_HZ / 8000) - 1));	//Reconfigure timer
	    	break;
	    case 11025:		// 11.025kHz:
	    	Timer2ChangeModule(((configCPU_CLOCK_HZ / 11025) - 1));	//Reconfigure timer
	    	break;
	    case 22050:		// 22.050kHz:
	    	Timer2ChangeModule(((configCPU_CLOCK_HZ / 22050) - 1));	//Reconfigure timer
	    	break;
	    case 44100:		// 44.1kHz:
	    	Timer2ChangeModule(((configCPU_CLOCK_HZ / 44100) - 1));	//Reconfigure timer
	    	break;
	    case 48000:		// 48kHz:
	    	Timer2ChangeModule(((configCPU_CLOCK_HZ / 48000) - 1));	//Reconfigure timer
	    	break;
	    default:
	    	break;    
	  }	
}

	

#endif



