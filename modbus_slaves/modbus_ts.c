
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
 * 
 * Brief: MODBUS Slave Treetech TS
 * 
 * Slave Address 1 
 * Serial comm: 8-None-2 @ 19200 baud
 * 
 * Taxa de transmiss�o: 9600, 19200 ou 38400 bps
 * Bits de dados: 8
 * Bits de parada: 2
 * Paridade: Nenhuma
 * 
 * 
 * */
#ifndef _WIN32
#include "BRTOS.h"
#else
#define CONST const
#endif

#include "AppConfig.h"
#include "modbus_ts.h"


#define Parametro_RL1 	(1<<0)
#define Parametro_RL2 	(1<<1)
#define Parametro_RL3 	(1<<2)
#define Parametro_RL4 	(1<<3)

#if COLDUINO
/* Holding registers */
CONST uint16_t Parametro_ALMO = 1;  /* alarme por temperatura do �leo */
CONST uint16_t Parametro_DSLO = 2;  /* desligamento por temperatura do �leo */
CONST uint16_t Parametro_RDSO = 3;  /* retardo para o desligamento por temperatura do �leo */
CONST uint16_t Parametro_ALME = 4;  /* alarme por temperatura do enrolamento */
CONST uint16_t Parametro_DSLE = 5;  /* desligamento por temperatura do enrolamento */
CONST uint16_t Parametro_RDSE = 6;  /* retardo para o desligamento por temperatura do enrolamento*/
CONST uint16_t Parametro_IDI 	= 7;  /* sele��o do idioma para as legendas */
CONST uint16_t Parametro_RTDS = 8;  /* seleciona quantos sensores de temperatura Pt100 */
CONST uint16_t Parametro_DISP = 9;  /* seleciona o modo de exibi��o do display */
CONST uint16_t Parametro_ALO 	= 10; /* seleciona um rel� para atuar na ocorr�ncia de alarme por temperatura do �leo */
CONST uint16_t Parametro_DSO 	= 11; /* seleciona um rel� para atuar na ocorr�ncia de desligamento por temperatura do �leo */
CONST uint16_t Parametro_ALE	= 12; /* seleciona um rel� para atuar na ocorr�ncia de alarme por temperatura do enrolamento */
CONST uint16_t Parametro_DSE 	= 13; /* seleciona um rel� para atuar na ocorr�ncia de desligamento por temperatura do enrolamento */
CONST uint16_t Parametro_RL 	= 14; /* seleciona o modo de funcionamento para os Rel�s */
CONST uint16_t Parametro_VSAN = 15; /* seleciona a medi��o de temperatura */
CONST uint16_t Parametro_FSAN = 16; /* seleciona a faixa da sa�da de corrente */
CONST uint16_t Parametro_FESA = 17; /* ajusta o valor de temperatura para o fim de escala da sa�da de corrente. */
CONST uint16_t Parametro_IESA = 18; /* ajusta o valor de temperatura para o in�cio de escala da sa�da de corrente */
CONST uint16_t Parametro_GEO  = 19; /* ajuste do gradiente de temperatura enrolamento m�dio topo do �leo  */
CONST uint16_t Parametro_TE 	= 20; /* ajuste da CONSTante de tempo do enrolamento */
CONST uint16_t Parametro_HS_MAIS = 21; /* ajuste do fator de hot-spot do enrolamento - ABNT NBR 5416 e IEEE Std C57.91-1995.*/
CONST uint16_t Parametro_HS_AST  = 22; /* ajuste do fator de hot-spot do enrolamento - IEC354 */
CONST uint16_t Parametro_2M  = 23; /* ajuste da exponencial de eleva��o de temperatura do enrolamento */
CONST uint16_t Parametro_CNT = 24; /* ajuste da corrente nominal do transformador */
CONST uint16_t Parametro_CNS = 25; /* ajuste da corrente nominal do transformador referida ao secund�rio */
CONST uint16_t Parametro_RF1 = 26; /* temperatura para acionamento do 1� grupo de refrigera��o for�ada */
CONST uint16_t Parametro_RF2 = 27; /* temperatura para acionamento do 2� grupo de refrigera��o for�ada */
CONST uint16_t Parametro_HIS = 28; /* temperatura para desacionamento dos grupos de refrigera��o for�ada */
CONST uint16_t Parametro_ALT = 29; /* altern�ncia autom�tica entre os grupos de ventiladores */
CONST uint16_t Parametro_CV1 = 30; /* percentual de carga do transformador para atua��o do 1� grupo de refrigera��o for�ada */
CONST uint16_t Parametro_CV2 = 31; /* percentual de carga do transformador para atua��o do 2� grupo de refrigera��o for�ada */
CONST uint16_t Parametro_HIC = 32; /* Histerese para parada dos grupos de resfriamento por redu��o da carga do transformador */
CONST uint16_t Parametro_EVH = 33; /* ajuste da hora em que dever�o ser acionados os grupos de refrigera��o for�ada */
CONST uint16_t Parametro_EVM  = 34; /* ajuste do minuto em que dever�o ser acionados os grupos de refrigera��o for�ada */
CONST uint16_t Parametro_TEV  = 35; /* ajuste da tempo que os grupos de refrigera��o for�ada dever�o permanecer acionados */
CONST uint16_t Parametro_HLOG = 36; /* ajusta o valor de varia��o de temperatura acima da qual o TS far� uma grava��o na mem�ria de massa. */
CONST uint16_t Parametro_TLOG = 37; /* ajusta os intervalos de tempo em que o TS far� as grava��es na mem�ria de massa */
CONST uint16_t Parametro_RLOG = 38; /* Reset do LOG. Apaga todos os dados na mem�ria de massa do TS. */
CONST uint16_t Parametro_MES  = 39; /* ajuste do m�s do calend�rio */
CONST uint16_t Parametro_DIA  = 40; /* ajuste do dia do calend�rio */
CONST uint16_t Parametro_ANO  = 41; /* ajuste do ano do calend�rio */
CONST uint16_t Parametro_HORA = 42; /* ajuste da hora do rel�gio */
CONST uint16_t Parametro_MIN  = 43; /* ajuste dos minutos do rel�gio */
CONST uint16_t Modo_RF1  		= 44;
CONST uint16_t Modo_RF2  		= 45;

/* Input registers */
CONST uint16_t Temperatura_oleo 		 	= 1001;
CONST uint16_t Temperatura_enrolamento 		= 1002;
CONST uint16_t Temperatura_RTD2 		 	= 1003;
CONST uint16_t Temperatura_RTD3	 	 		= 1004;
CONST uint16_t Temperatura_maxima_oleo 		= 1005;
CONST uint16_t Temperatura_maxima_enrolamento = 1006;
CONST uint16_t Temperatura_maxima_RTD2 		= 1007;
CONST uint16_t Temperatura_maxima_RTD3 		= 1008;
CONST uint16_t Gradiente_Final_Temperatura 	= 1009;
CONST uint16_t Percentual_carga 				= 1010;
CONST uint16_t Corrente_secundario_TC 		= 1011;
CONST uint16_t Corrente_transformador 		= 1012;
CONST uint16_t Estado_Reles 					= 1013;
CONST uint16_t Variavel_erros 				= 1014;
CONST uint16_t Opcionais 						= 1015;
CONST uint16_t Reles 							= 1016;
CONST uint16_t Ponteiro_LOG 					= 1017;

#endif

/* 
 * Supported MODBUS Commands
Command Description
0x03 Read holding registers
0x04 Read input registers
 * 
 * */


/* ----------------------- Modbus includes ----------------------------------*/
#if COLDUINO
#if !__GNUC__
#pragma warn_implicitconv off
#endif
#endif

#include "modbus.h"
#include "modbus_slaves.h"
#include "modbus_ts.h" /* TS device */
#include "string.h"
#include "time_lib.h"
#include "utils.h"

/* ----------------------- Constants ------------------------------------------*/

#if 0
CONST char TS_ID_string[] = "TS";
CONST uint16_t SIZEOF_TS_ID_STRING = sizeof(TS_ID_string);
#endif
		
/* static IR and HR list for TS device */
modbus_ts_input_register_list  	  TS_IRList;
//static modbus_ts_holding_register_list    TS_HRList;

#define MODBUS_SLAVE_TS_SIMULATION 	0
#if MODBUS_SLAVE_TS_SIMULATION
#include "random_lib.h"
#endif

uint8_t ts_read_data(uint8_t slave_addr, uint8_t* buf, uint8_t max_len);

uint8_t ts_read_data(uint8_t slave_addr, uint8_t* buf, uint8_t max_len)
{
	
		uint8_t nregs = 0;
		uint8_t retries = TS_REG_INPUT_NREGS*2;	
		
		/* limit number of registers to the max. available */
		if(max_len > sizeof(modbus_ts_input_register_list))
		{
			max_len = sizeof(modbus_ts_input_register_list);
		}
		
#if !MODBUS_SLAVE_TS_SIMULATION			
		uint16_t start_addr = TS_REG_INPUT_START;
#endif			
		
		memset(TS_IRList.Regs8,0x00,sizeof(TS_IRList.Regs8));

		for(nregs = 0; nregs < TS_REG_INPUT_NREGS;)
		{
			
	#if MODBUS_SLAVE_TS_SIMULATION			
			/* return random data */
			TS_IRList.Regs16[nregs + TS_REG_OFFSET] = random_get();
			nregs++;
			DelayTask(7000/1920); /* delay of communication @19200 = 7Bx1000/1920 B*/
	#else				
			
			if(Modbus_GetData(slave_addr, FC_READ_INPUT_REGISTERS, (uint8_t*)&TS_IRList.Regs16[nregs + TS_REG_OFFSET],
			start_addr, 1) == MODBUS_OK)
			{
				start_addr++;
				nregs++;
			}
			else
			{
				
				if(--retries == 0)
				{
					break;
				}
			}
					
			
	#endif					
		}		
		SetModbusHeader(slave_addr, TS_IRList.Regs8);
		memcpy(buf,TS_IRList.Regs8,max_len);		
		return (max_len);
			
}

CONST modbus_slave_t slave_TS =
{
		MS_TS,
		"TS",
		ts_read_data,
};
