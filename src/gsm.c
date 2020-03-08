/*
 * gsm.c
 *
 *  Created on: Feb 10, 2019
 *      Author: Kuzan
 */

#include "stm32l0xx.h"
#include "uart_gsm.h"
#include "packet_store.h"
#include <string.h>

#define GPS_READ_LENGTH			80

uint8_t ok_response [] = "\r\nOK\r\n";
uint8_t creg_ok [] = "\r\n+CREG: 2,1,"; //just check the start
uint8_t gps_info_resp [] = "\r\n+CGPSINFO:"; //just check the start
uint8_t network_opened [] = "\r\nNetwork opened\r\n\r\nOK\r\n";
uint8_t network_closed [] = "\r\nNetwork closed\r\n\r\nOK\r\n";
uint8_t connect_ok [] = "\r\nConnect ok\r\n\r\nOK\r\n";
uint8_t factory_reset[]= "AT&F\r";
uint8_t disable_echo[]= "ATE0\r";
uint8_t set_creg[] = "AT+CREG=2\r";
uint8_t query_creg[] = "AT+CREG?\r";
uint8_t read_imei[]= "AT+CGSN\r";
uint8_t read_iccid[] = "AT+CCID\r";
uint8_t read_CSQ[] = "AT+CSQ\r";
uint8_t gprs_attach[] = "AT+CGATT=1\r";
uint8_t gprs_dettach[] = "AT+CGATT=0\r";
uint8_t set_CGDCONT[] = "AT+CGDCONT=1,\"IP\",\"safaricom\",\"0.0.0.0\",0,0\r";
uint8_t get_GPS [] = "AT+CGPSINFO\r";
uint8_t set_AGPS_URL[] ="AT+CGPSURL=\"supl.google.com:7276\"\r";
uint8_t set_AGPS_SSL[] = "AT+CGPSSSL=0\r";
uint8_t turn_off_AGPS[] = "AT+CGPS=0\r";
uint8_t turn_on_AGPS[] = "AT+CGPS=1,2\r";
uint8_t get_GPS_STATE[] = "AT+CGPS?\r";
uint8_t open_socket[] = "AT+CHTTPSSTART\r";
uint8_t close_socket[] = "AT+NETCLOSE\r";
uint8_t establish_connection[] = "AT+TCPCONNECT=\"iot.accetly.com\",80\r";

uint8_t start_https[] = "AT+CHTTPSSTART\r";
uint8_t open_https[] = "AT+CHTTPSOPSE=\"iot.accetly.com\",443\r";
uint8_t receive_http_response[] = "AT+CHTTPSRECV=1000\r";
uint8_t close_https[] = "AT+CHTTPSCLSE\r";
uint8_t stop_https[] = "AT+CHTTPSSTOP\r";

uint8_t ready_to_send []= "\r\n>";
uint8_t gsm_state;

uint8_t gsm_IMEI [16];
uint32_t gps_read_period = 0;
uint32_t gps_reads = 0;
uint32_t file_count = 0;
enum gsm_states
{
	STATE_RESET,
	STATE_DISABLE_ECHO,
	STATE_IMEI,
	STATE_CCID,
	STATE_SET_CREG,
	STATE_GET_CREG,
	STATE_READ_CSQ,
	STATE_SET_AGPS_URL,
	STATE_SET_AGPS_SSL,
	STATE_GPS_STOP,
	STATE_GPS_START,
	STATE_GET_GPS,
	STATE_ATTACH_GPRS,
	STATE_CGDCONT,

	STATE_START_HTTPS_SESSION,
	STATE_OPEN_HTTPS_SESSION,
	STATE_SEND_HTTPS_DATA,
	STATE_RECEIVE_HTTPS_RESPONSE,
	STATE_CLOSE_HTTPS_SESSION,
	STATE_STOP_HTTPS_SERVICE,

	STATE_DETTACH_GPRS,

	//last
	STATE_OFFLINE
};


void parse_gps_info();
void build_http_message(char * message  ,uint16_t data_length , uint8_t * filedata);

void gsm_init()
{
	gsm_state = STATE_RESET;
}

void gsm_tick()
{
	uint8_t temp_buffer[58];
	uint8_t send_flag;
	switch (gsm_state) {
		case STATE_RESET:
			uart_gsm_send(factory_reset,sizeof(factory_reset)-1 ,sizeof(factory_reset) + 5);//expect command back plus OK
			if(0 == memcmp(aRxBuffer+sizeof(factory_reset)-1,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_DISABLE_ECHO:
			uart_gsm_send(disable_echo,sizeof(disable_echo)-1, sizeof(disable_echo) + 5);//expect commandd back plus OK
			if(0 == memcmp(aRxBuffer+sizeof(disable_echo)-1,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_IMEI:
			uart_gsm_send(read_imei,sizeof(read_imei)-1 , 25);
			if(0 == memcmp(aRxBuffer+19,ok_response,sizeof(ok_response)-1))
			{
				memcpy((void *)gsm_IMEI, (const void *)&aRxBuffer[2], 15);
				gsm_IMEI[15] = 0;
				//cop in index 2 to 16 as imei
				gsm_state++;
			}
			break;
		case STATE_CCID:
			uart_gsm_send(read_iccid,sizeof(read_iccid)-1,39);
			if(0 == memcmp(aRxBuffer+33,ok_response,sizeof(ok_response)-1))
			{
				//cop in index 10 to 29 as imei
				gsm_state++;
			}
			break;
		case STATE_SET_CREG:
			uart_gsm_send(set_creg,sizeof(set_creg)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_GET_CREG:
			uart_gsm_send(query_creg,sizeof(query_creg)-1,sizeof(creg_ok)-1);
			if(0 == memcmp(aRxBuffer,creg_ok,sizeof(creg_ok)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_READ_CSQ:
			uart_gsm_send(read_CSQ,sizeof(read_CSQ)-1,21);
			if(0 == memcmp(aRxBuffer+15,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;//if this passes we have a double digit csq
			}
			if(0 == memcmp(aRxBuffer+14,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;//if this passes we have a double digit csq
			}
			break;
		case STATE_SET_AGPS_URL:
			uart_gsm_send(set_AGPS_URL,sizeof(set_AGPS_URL)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_SET_AGPS_SSL:
			uart_gsm_send(set_AGPS_SSL,sizeof(set_AGPS_SSL)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_GPS_STOP:
			uart_gsm_send(turn_off_AGPS,sizeof(turn_off_AGPS)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_GPS_START:
			uart_gsm_send(turn_on_AGPS,sizeof(turn_on_AGPS)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				uint32_t default_timeout = 10000;
				uint32_t time = uwTick;
				//GIVE MODULE 5 SECONDS
				while ((uwTick < time +default_timeout));

				gsm_state++;
			}
			break;
		case STATE_GET_GPS:
			if(gps_reads < 50)
			{
				if(gps_read_period%5 == 0)
				{
					uart_gsm_send(get_GPS,sizeof(get_GPS)-1,GPS_READ_LENGTH);//wait for about 80 bytes
					if(0 == memcmp(aRxBuffer,gps_info_resp,sizeof(gps_info_resp)-1))
					{
						parse_gps_info();
						gps_reads++;
					}
				}
				gps_read_period++;
			}
			else
			{
				gps_reads = 0;
				gps_read_period = 0;
				gsm_state++;
			}
			break;
		case STATE_ATTACH_GPRS:
			//try to detech first
			uart_gsm_send(gprs_dettach,sizeof(gprs_dettach)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				uart_gsm_send(gprs_attach,sizeof(gprs_attach)-1,sizeof(ok_response)-1);
				if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
				{
					gsm_state++;
				}
			}

			break;
		case STATE_CGDCONT:
			uart_gsm_send(set_CGDCONT,sizeof(set_CGDCONT)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_START_HTTPS_SESSION:
			uart_gsm_send(start_https,sizeof(start_https)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			else
			{
				uart_gsm_send(stop_https,sizeof(stop_https)-1,sizeof(ok_response)-1);
				gsm_state =STATE_ATTACH_GPRS;
			}
			break;
		case STATE_OPEN_HTTPS_SESSION:
			uart_gsm_send(open_https,sizeof(open_https)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			else
			{
				uart_gsm_send(stop_https,sizeof(stop_https)-1,sizeof(ok_response)-1);
				gsm_state =STATE_ATTACH_GPRS;
			}
			break;
		case STATE_SEND_HTTPS_DATA:
			send_flag = packet_store_read_data_to_buffer(temp_buffer,sizeof(temp_buffer)-1);
			if(1 == send_flag)
			{
				char message[300];
				char command[50];

				memset(message,0,sizeof(message));
				temp_buffer[sizeof(temp_buffer)-1] = 0;
				build_http_message(message,sizeof(temp_buffer)-1,temp_buffer);

				const char* write_data = "AT+CHTTPSSEND=%d\r";
				uint16_t message_length = strlen(message);
				sprintf(command,write_data,message_length);
				uart_gsm_send((uint8_t *)command,strlen(command),sizeof(ready_to_send)-1);

				if(0 == memcmp(aRxBuffer,ready_to_send,sizeof(ready_to_send)-1))
				{
					//memcpy(&message[strlen(message)],temp_buffer,sizeof(temp_buffer));
					uart_gsm_send((uint8_t *)message,strlen(message),sizeof(connect_ok)-1);
					gsm_state++;
				}
			}
			else
			{
				gsm_state = STATE_CLOSE_HTTPS_SESSION;
			}
			break;
		case STATE_RECEIVE_HTTPS_RESPONSE:
			uart_gsm_send(receive_http_response,sizeof(receive_http_response)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state = STATE_SEND_HTTPS_DATA;
			}
			break;
		case STATE_CLOSE_HTTPS_SESSION:
			uart_gsm_send(receive_http_response,sizeof(receive_http_response)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state++;
			}
			break;
		case STATE_STOP_HTTPS_SERVICE:
			uart_gsm_send(stop_https,sizeof(stop_https)-1,sizeof(ok_response)-1);
			//if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			//{
				gsm_state++;
			//}
			break;
		case STATE_DETTACH_GPRS:
			uart_gsm_send(gprs_dettach,sizeof(gprs_dettach)-1,sizeof(ok_response)-1);
			if(0 == memcmp(aRxBuffer,ok_response,sizeof(ok_response)-1))
			{
				gsm_state= STATE_GET_GPS; //go pack to reading gps stuff
			}
			break;
		case STATE_OFFLINE:
			break;
		default:
			break;
	}

}

//called when we have the right header :)
void parse_gps_info()
{
	uint8_t end_character = '\r';
	uint8_t end_character_2 = '\n';
	uint8_t record_length=0;

	if(aRxBuffer[12] == ',')
	{
		//we have an empty pay load
		return;
	}

	for(uint8_t i = 12;i<GPS_READ_LENGTH-2;i++,record_length++)
	{
		if(aRxBuffer[i] == end_character)
		{
			if(aRxBuffer[i+1] == end_character_2)
			{
				break;
			}
		}
	}

	// save this to the packet store
	packet_store_write_buffer_to_page(aRxBuffer+12,record_length);

}

void build_http_message(char * message  ,uint16_t data_length , uint8_t * filedata)
{
	uint8_t url [] = "/file/upload";
	uint8_t host [] = "iot.accetly.com";
	char temp_buffer[300];
	const char * http_messsage = "POST %s HTTP/1.1\r\n"//url here
			"Host: %s\r\n"//host here
			"Content-Type: multipart/form-data; boundary=firmware\r\n"
			"Content-Length: %d\r\n\r\n"//content length
			"%s";//this is where the content goes now
	const char * content =
			"--firmware\r\n"
			"Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n\r\n"
			"Content-Type: text/plain\r\n\r\n"
			"%s : %s"
			"\r\n\r\n--firmware--\r\n\r\n";
	sprintf(temp_buffer,content,gsm_IMEI,filedata);
	uint8_t content_length = strlen(temp_buffer);
	sprintf(message,http_messsage,url,host,content_length,temp_buffer);
	file_count++;
}
