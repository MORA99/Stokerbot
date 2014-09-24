/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        24.10.2007
 Description:    RS232 Routinen

 Dieses Programm ist freie Software. Sie können es unter den Bedingungen der 
 GNU General Public License, wie von der Free Software Foundation veröffentlicht, 
 weitergeben und/oder modifizieren, entweder gemäß Version 2 der Lizenz oder 
 (nach Ihrer Option) jeder späteren Version. 

 Die Veröffentlichung dieses Programms erfolgt in der Hoffnung, 
 daß es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, 
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT 
 FÜR EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License. 

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem 
 Programm erhalten haben. 
 Falls nicht, schreiben Sie an die Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. 
------------------------------------------------------------------------------*/
#ifndef _UART_H
	#include <stdio.h>
	#define _UART_H

	#define USART_ECHO	1
	

	//Die Quarzfrequenz auf dem Board (in config.h)
	/*
	#ifndef SYSCLK
			#define SYSCLK 16000000UL
	#endif //SYSCLK	
	*/
	
	//Anpassen der seriellen Schnittstellen Register wenn ein ATMega128 benutzt wird
	#if defined (__AVR_ATmega128__)
		#define USR UCSR0A
		#define UCR UCSR0B
		#define UDR UDR0
		#define UBRR UBRR0L
		#define USART_RX USART0_RX_vect 
	#endif
	
	#if defined (__AVR_ATmega644__) || defined (__AVR_ATmega644P__)
		#define USR UCSR0A
		#define UCR UCSR0B
		#define UBRR UBRR0L
		#define EICR EICRB
		#define TXEN TXEN0
		#define RXEN RXEN0
		#define RXCIE RXCIE0
		#define UDR UDR0
		#define UDRE UDRE0
		#define USART_RX USART0_RX_vect   
	#endif
	
	#if defined (__AVR_ATmega32__)
		#define USR UCSRA
		#define UCR UCSRB
		#define UBRR UBRRL
		#define EICR EICRB
		#define USART_RX USART_RXC_vect  
	#endif
	
	#if defined (__AVR_ATmega8__)
		#define USR UCSRA
		#define UCR UCSRB
		#define UBRR UBRRL
	#endif
	
	#if defined (__AVR_ATmega88__)
		#define USR UCSR0A
		#define UCR UCSR0B
		#define UBRR UBRR0L
		#define TXEN TXEN0
		#define UDR UDR0
		#define UDRE UDRE0
	#endif
	//----------------------------------------------------------------------------
	int uart_putchar(char c, FILE *stream);
	void usart_init(unsigned long baudrate); 
	//----------------------------------------------------------------------------

#endif //_UART_H
