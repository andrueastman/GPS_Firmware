/*
 * packet_store.h
 *
 *  Created on: Feb 3, 2019
 *      Author: Kuzan
 */

#ifndef PACKET_STORE_H_
#define PACKET_STORE_H_

/*
 *	FUNCTION PROTOTYPES
 */
void packet_store_init(void);
uint8_t packet_store_read_data_to_buffer(uint8_t * read_buffer,
		uint16_t read_length);
void packet_store_write_buffer_to_page(uint8_t * buffer, uint16_t length);

#endif /* PACKET_STORE_H_ */
