

/**
 * \file uart.c
 * \brief Driver for uart
 *
 * This file contains the uart microcontroller drivers.
 *
 **/

#include "BRTOS.h"
#include "hardware.h"
#include "uart.h"
#include "utils.h"
#include "terminal_io.h"

#if (ENABLE_UART0 == TRUE)
INT8U receive_byte0;
BRTOS_Mutex *SerialResource0;
BRTOS_Sem *SerialTX0;
OS_QUEUE SerialPortBuffer0;
BRTOS_Queue *Serial0;
#endif

#if (ENABLE_UART1 == TRUE)
INT8U receive_byte1 = 0;
BRTOS_Mutex *SerialResource1;
BRTOS_Sem *SerialTX1;
OS_QUEUE SerialPortBuffer1;
BRTOS_Queue *Serial1;
#endif

#if (ENABLE_UART2 == TRUE)
INT8U receive_byte2 = 0;
BRTOS_Mutex *SerialResource2;
BRTOS_Sem *SerialTX2;
OS_QUEUE SerialPortBuffer2;
BRTOS_Queue *Serial2;
#endif


void uart0_init(unsigned int baudrate);
void uart1_init(unsigned int baudrate);
void uart2_init(unsigned int baudrate);

void uart0_init(unsigned int baudrate)
{
	/*Set baud rate */
	UBRR0H = (INT8U)(baudrate>>8);
	UBRR0L = (INT8U)baudrate;

	/*Enable receiver and transmitter. Enable RX interrupt */
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1 << RXCIE0);
	/* Set frame format: 8 bit data, 1 stop bit */
	UCSR0C = (3<<UCSZ00); 								
}

void uart1_init(unsigned int baudrate)
{	
	/*Set baud rate */
	UBRR1H = (INT8U)(baudrate>>8);
	UBRR1L = (INT8U)baudrate;

	/*Enable receiver and transmitter. Enable RX interrupt */
	UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1 << RXCIE1);
	/* Set frame format: 8 bit data, 1 stop bit */
	UCSR1C = (3<<UCSZ10);
}

void uart2_init(unsigned int baudrate)
{
	/*Set baud rate */
	UBRR2H = (INT8U)(baudrate>>8);
	UBRR2L = (INT8U)baudrate;

	/*Enable receiver and transmitter. Enable RX interrupt */
	UCSR2B = (1<<RXEN2) | (1<<TXEN2) | (1 << RXCIE2);
	/* Set frame format: 8 bit data, 1 stop bit */
	UCSR2C = (3<<UCSZ20);
}


void _putchar_uart0(CHAR8 data)
{
   /* Wait for empty transmit buffer */
	while (!(UCSR0A & 0x20)){}
    UCSR0A = 0x20;
	// Put data into buffer, sends the data */
    UDR0 = data;
	while (!(UCSR0A & 0x20)){}
}

void _putchar_uart1(CHAR8 data)
{
	/* Wait for empty transmit buffer */
	while (!(UCSR1A & 0x20)){}
	UCSR1A = 0x20;
	// Put data into buffer, sends the data */
	UDR1 = data;
}

void _putchar_uart2(CHAR8 data)
{
	/* Wait for empty transmit buffer */
	while (!(UCSR2A & 0x20)){}
	UCSR2A = 0x20;
	// Put data into buffer, sends the data */
	UDR2 = data;
}

void uart_init(INT8U uart, INT16U baudrate, INT16U buffersize, INT8U mutex, INT8U priority)
{
#if (ENABLE_UART0 == TRUE)		
	/* check if UART 0 is already init */
	if(uart == 0 && Serial0 != NULL)
	{
		if(Serial0->OSEventAllocated == TRUE)
		{
			return;
		}
	}
#endif	

#if (ENABLE_UART1 == TRUE)	
	/* check if UART 1 is already init */
	if(uart == 1 && Serial1 != NULL)
	{
		if(Serial1->OSEventAllocated == TRUE)
		{
			return;
		}
	}
#endif	
	
#if (ENABLE_UART1 == TRUE)		
	/* check if UART 2 is already init */
	if(uart == 2 && Serial2 != NULL)
	{
		if(Serial2->OSEventAllocated == TRUE)
		{
			return;
		}
	}
#endif	

// Configure UART 0
#if (ENABLE_UART0 == TRUE)
		if (uart == 0)
		{			
			uart0_init(baudrate);			
			if (mutex == TRUE)
			{
				if (OSMutexCreate(&SerialResource0, priority) != ALLOC_EVENT_OK)
				{
					while (1){};
				}
			}

			if (OSSemCreate(0, &SerialTX0) != ALLOC_EVENT_OK)
			{
				while (1){};
			}

			if (OSQueueCreate(&SerialPortBuffer0, buffersize,
			&Serial0) != ALLOC_EVENT_OK)
			{
				while (1){};
			}
		}
#endif
			
// Configure UART 1
#if (ENABLE_UART1 == TRUE)
		if (uart == 1)
		{
			
			uart1_init(baudrate);		
			if (mutex == TRUE)
			{
				if (OSMutexCreate(&SerialResource1, priority) != ALLOC_EVENT_OK)
				{
					while (1){};
				}
			}

			if (OSSemCreate(0, &SerialTX1) != ALLOC_EVENT_OK)
			{
				while (1){};
			}

			if (OSQueueCreate(&SerialPortBuffer1, buffersize,
					&Serial1) != ALLOC_EVENT_OK)
			{
				while (1){};
			}
		}		
#endif

// Configure UART 2
#if (ENABLE_UART2 == TRUE)
		if (uart == 2)
		{
			
			uart2_init(baudrate);
			if (mutex == TRUE)
			{
				if (OSMutexCreate(&SerialResource2, priority) != ALLOC_EVENT_OK)
				{
					while (1){};
				}
			}

			if (OSSemCreate(0, &SerialTX2) != ALLOC_EVENT_OK)
			{
				while (1){};
			}

			if (OSQueueCreate(&SerialPortBuffer2, buffersize,
					&Serial2) != ALLOC_EVENT_OK)
			{
				while (1){};
			}
		}
#endif

}

// UART0 functions
#if (ENABLE_UART0 == TRUE)

void uart0_tx(void)
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_INT_ENTER();
	

	#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
	#endif


	(void) OSSemPost(SerialTX0);

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	// ************************
}

INT8U getchar_uart0(CHAR8* caracter, INT16U timeout)
{
	INT8U ret;
	ret = OSQueuePend(Serial0, (INT8U*)caracter, timeout);
	return (ret != TIMEOUT) ? TRUE:FALSE;
}

//Fun��o para adquirir direito exclusivo a porta serial
// Assim que poss�vel colocar recurso de timeout
void uart0_acquire(void)
{
#if UART0_MUTEX		
	// Aloca o recurso da porta serial
	OSMutexAcquire(SerialResource0);
#endif		
}

//Fun��o para liberar a porta serial
void uart0_release(void)
{
#if UART0_MUTEX		
	// Libera o recurso da porta serial
	OSMutexRelease(SerialResource0);
#endif	
}

void uart0_RxEnable(void)
{
	/* Enable receiver.*/
	UCSR0B = UCSR0B | (1<<RXEN0);
}

void uart0_RxDisable(void)
{	
	/* Disable RX.*/
	UCSR0B = UCSR0B & ~(1 << RXEN0);
}

void uart0_RxEnableISR(void)
{
	/* Enable RX interrupt */
	UCSR0B = UCSR0B | (1 << RXCIE0);
}

void uart0_RxDisableISR(void)
{
	
	/* Disable RX interrupt */	
	UCSR0B = UCSR0B & ~(1 << RXCIE0);	

}


void uart0_TxEnableISR(void)
{
	
}

void uart0_TxDisableISR(void)
{
	
}


char putchar_uart0(char caracter)
{
	_putchar_uart0(caracter);
	
#if 0	
	if(OSSemPend(SerialTX0, TX_TIMEOUT) == TIMEOUT)
	{
		return caracter++;
	}
#endif	
	
	return caracter;
}

void printf_uart0(CHAR8 *string)
{
	uart0_acquire();
	while (*string)
	{
		putchar_uart0(*string);
		string++;
	}
	uart0_release();
}

void printP_uart0(char const *string)
{
	char c;

	uart0_acquire();

	while((c=pgm_read_byte(string)) != 0)
	{
		putchar_uart0(c);
		string++;
	}

	uart0_release();

}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
//__attribute__ ((section (".lowtext")))
ISR(USART0_RX_vect, __attribute__ ( ( naked ) ))
#else
ISR(USART_RX_vect, __attribute__ ( ( naked ) ))
#endif
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_SAVE_ISR();
	OS_INT_ENTER();

	INT8U caracter = 0;
	caracter = UDR0;

	#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
	#endif

	if (OSQueuePost(Serial0,caracter) == BUFFER_UNDERRUN)
	{
		// Problema: Estouro de buffer
		(void)OSCleanQueue(Serial0);
	}

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	OS_RESTORE_ISR();
	// ************************
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void uart0_error(void)
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_INT_ENTER();

	#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
	#endif

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	// ************************
}

#endif

// UART1 functions
#if (ENABLE_UART1 == TRUE)

void uart1_tx(void)
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_INT_ENTER();

#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
#endif  
	
	(void) OSSemPost(SerialTX1);

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	// ************************
}

//Fun��o para adquirir direito exclusivo a porta serial
// Assim que poss�vel colocar recurso de timeout
void uart1_acquire(void)
{
#if UART1_MUTEX	
	// Aloca o recurso da porta serial
	OSMutexAcquire(SerialResource1);
#endif	
}

//Fun��o para liberar a porta serial
void uart1_release(void)
{
#if UART1_MUTEX	
	// Libera o recurso da porta serial
	OSMutexRelease(SerialResource1);
#endif		
}

void uart1_RxEnable(void)
{
	
}

void uart1_RxDisable(void)
{
	
}


void uart1_RxEnableISR(void)
{
	
}

void uart1_RxDisableISR(void)
{
	
}


void uart1_TxEnableISR(void)
{

}

void uart1_TxDisableISR(void)
{
	
}

char putchar_uart1(char caracter)
{
	_putchar_uart1(caracter);
	
	#if 0
	if(OSSemPend(SerialTX1, TX_TIMEOUT) == TIMEOUT)
	{
		return caracter++;
	}
	#endif
	
	return caracter;
}

void printf_uart1(char *string)
{

	while (*string)
	{
		putchar_uart1(*string);
		string++;
	}

}

void printP_uart1(char const *string)
{
	char c;
	while((c=pgm_read_byte(string)) != 0)
	{
		putchar_uart1(c);
		string++;
	}
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
//__attribute__ ((section (".lowtext")))
ISR(USART1_RX_vect, __attribute__ ( ( naked ) ))
#else
ISR(USART_RX_vect, __attribute__ ( ( naked ) ))
#endif
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_SAVE_ISR();
	OS_INT_ENTER();

	INT8U caracter = 0;
	caracter = UDR1;

	#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
	#endif

	if (OSQueuePost(Serial1,caracter) == BUFFER_UNDERRUN)
	{
		// Problema: Estouro de buffer
		(void)OSCleanQueue(Serial1);
	}

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	OS_RESTORE_ISR();
	// ************************
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void uart1_error(void)
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_INT_ENTER();

	

#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
#endif  

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	// ************************
}

#endif

// UART2 functions
#if (ENABLE_UART2 == TRUE)

void uart2_tx(void)
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_INT_ENTER();

	

#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
#endif  


	(void) OSSemPost(SerialTX2);

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	// ************************
}

//Fun��o para adquirir direito exclusivo a porta serial
// Assim que poss�vel colocar recurso de timeout
void uart2_acquire(void)
{
	// Aloca o recurso da porta serial
	OSMutexAcquire(SerialResource2);
}

//Fun��o para liberar a porta serial
void uart2_release(void)
{
	// Libera o recurso da porta serial
	OSMutexRelease(SerialResource2);
}

void uart2_RxEnableISR(void)
{
	
}

void uart2_RxDisableISR(void)
{
	
}


void uart2_TxEnableISR(void)
{
	
}

void uart2_TxDisableISR(void)
{
	
}


char putchar_uart2(char caracter)
{
	_putchar_uart2(caracter);
	
	#if 0
	if(OSSemPend(SerialTX2, TX_TIMEOUT) == TIMEOUT)
	{
		return caracter++;
	}
	#endif
	
	return caracter;
}

void printf_uart2(char *string)
{

	while (*string)
	{
		putchar_uart2(*string);
		string++;	
	}
}

void printP_uart2(char const *string)
{
	char c;
	while((c=pgm_read_byte(string)) != 0)
	{
		putchar_uart2(c);
		string++;
	}
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
//__attribute__ ((section (".lowtext")))
ISR(USART2_RX_vect, __attribute__ ( ( naked ) ))
#else
ISR(USART_RX_vect, __attribute__ ( ( naked ) ))
#endif
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_SAVE_ISR();
	OS_INT_ENTER();

	INT8U caracter = 0;
	caracter = UDR2;

	#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
	#endif

	if (OSQueuePost(Serial2,caracter) == BUFFER_UNDERRUN)
	{
		// Problema: Estouro de buffer
		(void)OSCleanQueue(Serial2);
	}

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	OS_RESTORE_ISR();
	// ************************
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
void uart2_error(void)
{
	// ************************
	// Entrada de interrup��o
	// ************************
	OS_INT_ENTER();

#if (NESTING_INT == 1)
	OS_ENABLE_NESTING();
#endif  

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	// ************************
}

#endif

void SerialReset(INT8U Comm)
{
	
}
