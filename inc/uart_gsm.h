/*
 * uart_gsm.h
 *
 *  Created on: Feb 8, 2019
 *      Author: v-anomon
 */

#ifndef UART_GSM_H_
#define UART_GSM_H_

/*
 *	DEFINES
 */
#define RXBUFFERSIZE 250

/*
 *	GLOBAL VARIABLES
 */
extern uint8_t aRxBuffer[RXBUFFERSIZE];

/*
 *	FUNCTION PROTOTYPES
 */
void uart_gsm_init( void );
void uart_gsm_send(uint8_t * tx_buffer, uint16_t length ,uint16_t receive_length);

#endif /* UART_GSM_H_ */
