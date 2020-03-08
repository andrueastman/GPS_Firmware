/*
 * uart_gsm.c
 *
 *  Created on: Feb 8, 2019
 *      Author: v-anomon
 */

#include "stm32l0xx.h"
#include "uart_gsm.h"

#define SPI_DEFAULT_TIMEOUT	1000
/* Private variables ---------------------------------------------------------*/
/* UART handler declaration */
UART_HandleTypeDef UartHandle;
__IO ITStatus UartReady = RESET;

/* Buffer used for reception */
uint8_t aRxBuffer[RXBUFFERSIZE];

static void Error_Handler(void);
/*
 * Initialize the uart for the GSM
 */
void uart_gsm_init(void) {

	/* Put the USART peripheral in the Asynchronous mode (UART Mode) */
	/* UART configured as follows:
	 - Word Length = 8 Bits
	 - Stop Bit = One Stop bit
	 - Parity = None
	 - BaudRate = 9600 baud
	 - Hardware flow control disabled (RTS and CTS signals) */
	UartHandle.Instance = USART5;
	UartHandle.Init.BaudRate = 115200;
	UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	UartHandle.Init.StopBits = UART_STOPBITS_1;
	UartHandle.Init.Parity = UART_PARITY_NONE;
	UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	UartHandle.Init.Mode = UART_MODE_TX_RX;
	UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
	UartHandle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	UartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	if (HAL_UART_DeInit(&UartHandle) != HAL_OK) {
		Error_Handler();
	}

	if (HAL_UART_Init(&UartHandle) != HAL_OK) {
		Error_Handler();
	}


	/*##-3- Configure the NVIC for UART ########################################*/
	/* NVIC for USART */
	HAL_NVIC_SetPriority(USART4_5_IRQn, 0, 1);

	__HAL_UART_ENABLE_IT(&UartHandle,UART_IT_RXNE);

	HAL_NVIC_EnableIRQ(USART4_5_IRQn);

}

void uart_gsm_send(uint8_t * tx_buffer, uint16_t send_length,
		uint16_t receive_length) {
	if (HAL_UART_Transmit_IT(&UartHandle, tx_buffer, send_length) != HAL_OK) {
		Error_Handler();
	}

	/*##-3- Wait for the end of the transfer ###################################*/


	/* Reset transmission flag */
	UartReady = RESET;

	(uint16_t) READ_REG(UartHandle.Instance->RDR); //dummy read first byte is trash

	if (receive_length != 0) {
		if (HAL_UART_Receive_IT(&UartHandle, (uint8_t *) aRxBuffer,
				receive_length) != HAL_OK) {
			Error_Handler();
		}

		/*##-5- Wait for the end of the transfer ###################################*/
		while (UartReady != SET) {
		}
	}

	/* Reset transmission flag */
	UartReady = RESET;
}

/**
 * @brief UART MSP Initialization
 *        This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 *           - Peripheral's GPIO Configuration
 *           - NVIC configuration for UART interrupt request enable
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
	GPIO_InitTypeDef GPIO_InitStruct;

	/*##-1- Enable peripherals and GPIO Clocks #################################*/

	/* Enable USARTx clock */
	__HAL_RCC_USART5_CLK_ENABLE();

	/* Enable GPIO TX/RX clock */
//	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*##-2- Configure peripheral GPIO ##########################################*/
	/* UART TX GPIO pin configuration  */
	GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF6_USART5;

	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* MODEM PWR pin configuration  */
	GPIO_InitStruct.Pin = GPIO_PIN_14;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


	//set PWR high
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);

	/* MODEM PWR pin configuration  */
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	//set PWR high
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
}

/**
 * @brief  Tx Transfer completed callback
 * @param  UartHandle: UART handle.
 * @note   This example shows a simple way to report end of IT Tx transfer, and
 *         you can add your own implementation.
 * @retval None
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle) {
	/* Set transmission flag: transfer complete */
	UartReady = SET;

}

/**
 * @brief  Rx Transfer completed callback
 * @param  UartHandle: UART handle
 * @note   This example shows a simple way to report end of DMA Rx transfer, and
 *         you can add your own implementation.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle) {
	/* Set transmission flag: transfer complete */
	UartReady = SET;

}
/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
static void Error_Handler(void) {
	/* User may add here some code to deal with this error */
	while (1) {
	}
}

