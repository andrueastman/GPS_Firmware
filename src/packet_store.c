/*******************************************************************************
* @file    packet_store.c
* @author  Kuzan
* @brief   Packet store(SPI flash) handler
******************************************************************************/
#include "stm32l0xx.h"
#include "packet_store.h"

/*******************************************************************************
	Specific defines for commands for the SPI flash
	Winbond 25Q16BVSIG / 25Q16CVSIG, 16MBit SPI Flash Memory
********************************************************************************/
#define WRITE_ENABLE_CMD		0x06
#define WRITE_DISABLE_CMD		0x04
#define READ_STATUS1_REG_CMD	0x05
#define READ_STATUS2_REG_CMD	0x35
#define WRITE_STATUS_REG_CMD	0x01
#define WRITE_PAGE_CMD			0x02
#define SECTOR_ERASE_4K_CMD		0x20
#define BLOCK_ERASE_32K_CMD		0x52
#define BLOCK_ERASE_64K_CMD		0xD8
#define CHIP_ERASE_CMD			0x60
#define POWER_DOWN_CMD			0xB9
#define READ_DATA_CMD			0x03
#define READ_UNIQUE_ID			0x4B
#define READ_MANUFACTURER_ID	0x90

/*******************************************************************************
	File Definitions
********************************************************************************/
#define SPI_DEFAULT_TIMEOUT		100
#define SPI_FLASH_PAGE_SIZE		256
#define SPI_FLASH_PAGE_COUNT	8192

/*******************************************************************************
	Local Function Prototypes
********************************************************************************/
static void local_error_handler(void);
static void get_flash_chip_id( void );
static void get_flash_chip_manufacturer_id( void );
static void flash_chip_write_enable( void );
static void wait_for_flash_to_be_ready( void );
static void erase_flash_chip_sector( uint8_t * page_address ,uint8_t size_of_page_adress);

/*******************************************************************************
	Local Variables
********************************************************************************/
static SPI_HandleTypeDef SpiHandle;//spi handler
static uint8_t manufacturer_id[2];
static uint8_t device_id[4];
static uint32_t page_index_to_send = 0;
static uint32_t page_index_to_write = 0;

/*******************************************************************************
	Public Functions
********************************************************************************/

/*******************************************************************************
* @function   	packet_store_init
* @brief		Initialize the hardware for the packetstore to be used
* @param		None
* @retval		None
******************************************************************************/
void packet_store_init( void )
{

	GPIO_InitTypeDef  GPIO_InitStruct;
	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO TX/RX clock */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	/* Enable SPI clock */
	__HAL_RCC_SPI1_CLK_ENABLE();

	/*##-2- Configure peripheral GPIO ##########################################*/
	/* SPI SCK GPIO pin configuration  */
	GPIO_InitStruct.Pin       = GPIO_PIN_5;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* SPI MISO GPIO pin configuration  */
	GPIO_InitStruct.Pin = GPIO_PIN_6;
	GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* SPI MOSI GPIO pin configuration  */
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* SPI CS pin configuration  */
	GPIO_InitStruct.Pin 	= GPIO_PIN_4;
	GPIO_InitStruct.Mode 	= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  	= GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	//set CS high
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);


	//initialize the underlying spi
	/*##-1- Configure the SPI peripheral #######################################*/
	/* Set the SPI parameters */
	SpiHandle.Instance               	= SPI1;
	SpiHandle.Init.BaudRatePrescaler 	= SPI_BAUDRATEPRESCALER_256;
	SpiHandle.Init.Direction         	= SPI_DIRECTION_2LINES;
	SpiHandle.Init.CLKPhase          	= SPI_PHASE_1EDGE;
	SpiHandle.Init.CLKPolarity       	= SPI_POLARITY_LOW;
	SpiHandle.Init.DataSize          	= SPI_DATASIZE_8BIT;
	SpiHandle.Init.FirstBit          	= SPI_FIRSTBIT_MSB;
	SpiHandle.Init.TIMode            	= SPI_TIMODE_DISABLE;
	SpiHandle.Init.CRCCalculation    	= SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial     	= 7;
//	SpiHandle.Init.CRCLength         	= SPI_CRC_LENGTH_8BIT;
	SpiHandle.Init.NSS               	= SPI_NSS_SOFT;
//	SpiHandle.Init.NSSPMode          	= SPI_NSS_PULSE_DISABLE;
	SpiHandle.Init.Mode 			 	= SPI_MODE_MASTER;


	if(HAL_SPI_Init(&SpiHandle) != HAL_OK)
	{
		/* Initialization Error */
		local_error_handler();
	}

	get_flash_chip_id();
	get_flash_chip_manufacturer_id();
}


/*******************************************************************************
* @function   	packet_store_write_buffer_to_page
* @brief		Write buffer to flash
* @param		buffer - buffer to write
* 				length - buffer size
* @retval		None
******************************************************************************/
void packet_store_write_buffer_to_page(uint8_t * buffer, uint16_t length)
{
	uint8_t page_address[3];
	uint8_t page_program_cmd = WRITE_PAGE_CMD;

	if(length > SPI_FLASH_PAGE_SIZE)
	{
		return;//buffer too big. Abort
	}

	page_address[0] = (uint8_t) ((page_index_to_write & 0xFFFF) > 8);
	page_address[1] = (uint8_t) (page_index_to_write & 0xFF);
	page_address[2] = (uint8_t) 0;

	//we are at sector edge
	if(0 == (page_index_to_write & 0x0F))
	{
		erase_flash_chip_sector(page_address,sizeof(page_address));
	}

	flash_chip_write_enable();
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
	//send the command
	HAL_SPI_Transmit(&SpiHandle,&page_program_cmd,sizeof(page_program_cmd),SPI_DEFAULT_TIMEOUT);//send command
	//send addresses
	HAL_SPI_Transmit(&SpiHandle,page_address,sizeof(page_address),SPI_DEFAULT_TIMEOUT);//send command
	//send data
	HAL_SPI_Transmit(&SpiHandle,buffer,length,SPI_DEFAULT_TIMEOUT);//send command
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);

	wait_for_flash_to_be_ready();

	page_index_to_write++;

	if(page_index_to_write >= SPI_FLASH_PAGE_COUNT)
	{
		page_index_to_write = 0;//we are at the flash end
	}
}


/*******************************************************************************
* @function   	packet_store_read_data_to_buffer
* @brief		Write buffer to flash
* @param		read_buffer - buffer to read to
* 				read_length - buffer size
* @retval		None
******************************************************************************/
uint8_t packet_store_read_data_to_buffer( uint8_t * read_buffer, uint16_t read_length)
{
	uint8_t read_data_cmd = READ_DATA_CMD;
	uint8_t page_address[3];

	if(page_index_to_send == page_index_to_write)//nothing to read
	{
		return 0;
	}

	page_address[0] = (uint8_t) ((page_index_to_send & 0xFFFF) > 8);
	page_address[1] = (uint8_t) (page_index_to_send & 0xFF);
	page_address[2] = (uint8_t) 0;

	wait_for_flash_to_be_ready();//make sure the flash is not busy

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
	//send the command
	HAL_SPI_Transmit(&SpiHandle,&read_data_cmd,sizeof(read_data_cmd),SPI_DEFAULT_TIMEOUT);//send command
	//send addresses
	HAL_SPI_Transmit(&SpiHandle,page_address,sizeof(page_address),SPI_DEFAULT_TIMEOUT);//send command
	HAL_SPI_Receive(&SpiHandle,read_buffer,read_length,SPI_DEFAULT_TIMEOUT);//read back response
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);

	page_index_to_send++;

	if(page_index_to_send >= SPI_FLASH_PAGE_COUNT)
	{
		page_index_to_send = 0;//we are at the flash end
	}

	return 1;
}

/*******************************************************************************
	Local Functions
********************************************************************************/


/*******************************************************************************
* @function   	get_flash_chip_id
* @brief		Read the device id from the the flash chip
* @param		None
* @retval		None
******************************************************************************/
static void get_flash_chip_id( void )
{
	uint8_t device_id_cmd[]= {
			READ_UNIQUE_ID,
			0x00,0x00,0x00,0x00//DUMMY BYTES
			};

	//TODO write/find generic function that takes the input and output buffer to summarize the next 4 lines
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);

	HAL_SPI_Transmit(&SpiHandle,device_id_cmd,sizeof(device_id_cmd),SPI_DEFAULT_TIMEOUT);//send command
	HAL_SPI_Receive(&SpiHandle,device_id,sizeof(device_id),SPI_DEFAULT_TIMEOUT);//read back response

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
}


/*******************************************************************************
* @function   	get_flash_chip_manufacturer_id
* @brief		Read the manufacturer id from the the flash chip
* @param		None
* @retval		None
******************************************************************************/
static void get_flash_chip_manufacturer_id( void )
{
	uint8_t device_id_cmd[]= {
			READ_MANUFACTURER_ID,
			0x00,0x00,0x00//address zero
			};

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);

	HAL_SPI_Transmit(&SpiHandle,device_id_cmd,sizeof(device_id_cmd),SPI_DEFAULT_TIMEOUT);//send command
	HAL_SPI_Receive(&SpiHandle,manufacturer_id,sizeof(manufacturer_id),SPI_DEFAULT_TIMEOUT);//read back response

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);

}

/*******************************************************************************
* @function   	flash_chip_write_enable
* @brief		prepare the flash for writing into by enabling writes
* @param		None
* @retval		None
******************************************************************************/
static void flash_chip_write_enable( void )
{
	uint8_t write_enable_command = WRITE_ENABLE_CMD;

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_SPI_Transmit(&SpiHandle,&write_enable_command,sizeof(write_enable_command),SPI_DEFAULT_TIMEOUT);//send command
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);

	wait_for_flash_to_be_ready();
}


/*******************************************************************************
* @function   	wait_for_flash_to_be_ready
* @brief		Poll the busy flag till the flash is not busy anymore.
* @param		None
* @retval		None
******************************************************************************/
static void wait_for_flash_to_be_ready( void )
{
	uint8_t read_status_cmd = READ_STATUS1_REG_CMD;
	uint8_t response[2];
	uint16_t counts = 0;

	do
	{
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
		//send the command
		HAL_SPI_Transmit(&SpiHandle,&read_status_cmd,sizeof(read_status_cmd),SPI_DEFAULT_TIMEOUT);//send command
		HAL_SPI_Receive(&SpiHandle,response,sizeof(response),SPI_DEFAULT_TIMEOUT);//read back response

		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
		counts++;
	} while(response[0] & 0x01);
}

/*******************************************************************************
* @function   	erase_flash_chip_sector
* @brief		Erase and set a sector of size 4K to 1111s
* @param		None
* @retval		None
******************************************************************************/
static void erase_flash_chip_sector( uint8_t * page_address ,uint8_t size_of_page_adress)
{
	uint8_t sector_erase_cmd = SECTOR_ERASE_4K_CMD;
	uint32_t time;
	flash_chip_write_enable();

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
	//send the command
	HAL_SPI_Transmit(&SpiHandle,&sector_erase_cmd,sizeof(sector_erase_cmd),SPI_DEFAULT_TIMEOUT);//send command
	//send addresses
	HAL_SPI_Transmit(&SpiHandle,page_address,size_of_page_adress,SPI_DEFAULT_TIMEOUT);//send command
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);

	time = uwTick;
	//TODO check why this hack is needed
	//TODO create generic delay function
	while(uwTick < time +100)
	{
		//delay for 100ms
	}
	wait_for_flash_to_be_ready();

}

/*******************************************************************************
* @function   	local_error_handler
* @brief		This function is executed in case of error occurrence.
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
