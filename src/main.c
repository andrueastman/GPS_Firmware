/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32l0xx.h"
#include "uart_gsm.h"
#include "packet_store.h"
#include "gsm.h"

/*******************************************************************************
	File Definitions
********************************************************************************/
#define START_UP_DELAY	20000
#define TICK_PERIOD		1000

/*******************************************************************************
	Local Function Prototypes
********************************************************************************/
static void init(void);
static void run_ticks(void);
static void run_churns(void);
static void handle_ticks(void);
static void system_clock_config(void);
static void local_error_handler(void);


/*******************************************************************************
	Local Variables
********************************************************************************/

/*******************************************************************************
	Local Functions
********************************************************************************/

/*******************************************************************************
* @function   main
* @brief   	Default main function.
* @param		None
* @retval		Success status
******************************************************************************/
int main( void )
{
	//initialize
	init();

	while(uwTick < START_UP_DELAY);

	while(1)
	{
		run_churns();
	}
}

/*******************************************************************************
* @function   init
* @brief   	Initialization procedure
* @param		None
* @retval		None
******************************************************************************/
static void init( void )
{
	HAL_Init();
	system_clock_config();
	packet_store_init();
	uart_gsm_init();
	gsm_init();
}


/*******************************************************************************
* @function   run_ticks
* @brief   	Call the functions needed to be handled every tick period
* @param		None
* @retval		None
******************************************************************************/
static void run_ticks( void )
{
	//Task 1: GSM modem handling
	gsm_tick();
}


/*******************************************************************************
* @function   run_churns
* @brief   	Call the functions needed to be handled every churn period
* @param		None
* @retval		None
******************************************************************************/
static void run_churns( void )
{
	//Task 1: handle updating of tick timers
	handle_ticks();
}

/*******************************************************************************
* @function   handle_ticks
* @brief   	Call the functions needed to be handled every tick period
* @param		None
* @retval		None
******************************************************************************/
static void handle_ticks(void)
{
	//check up on whether the tick period is over to run new ticks
	static uint32_t previous_tick;

	if((previous_tick + TICK_PERIOD) < uwTick)
	{
		previous_tick = uwTick;
		run_ticks();
	}
}


/*******************************************************************************
* @function   system_clock_config
* @brief   	System Clock Configuration
*         	The system Clock is configured as follow :
*            		System Clock source            = PLL (HSI)
*            		SYSCLK(Hz)                     = 64000000
*            		HCLK(Hz)                       = 64000000
*            		AHB Prescaler                  = 1
*            		APB1 Prescaler                 = 2
*            		APB2 Prescaler                 = 1
*            		HSI Frequency(Hz)              = 8000000
*            		PREDIV                         = RCC_PREDIV_DIV2 (2)
*            		PLLMUL                         = RCC_PLL_MUL16 (16)
*            		Flash Latency(WS)              = 2
* @param		None
* @retval		None
******************************************************************************/
static void system_clock_config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* HSI Oscillator already ON after system reset, activate PLL with HSI as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL24;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
	{
		local_error_handler();
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1)!= HAL_OK)
	{
		local_error_handler();
	}
}

/*******************************************************************************
* @function   local_error_handler
* @brief   	Error handler for anthing happening in the main file
* @param		None
* @retval		None
* TODO define a global handler
******************************************************************************/
static void local_error_handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}
