/*
 * CCU C-API
 * CCU Version 3.0
 * Author: Tarek Kaddoura
 * Copyright CliniSonix 2025
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ccu.h"

// Configuration
static const int baudRate = 9600;
static const int bits = 8;
static const int stopBits = 1;
static const int parity = SP_PARITY_NONE;
static const int vid = 0x16C0;
static const int pid = 0x0483;
static const char secret_phrase[] = "6ziieuJlUe7kamhNa4shpRfgzCbjB7Tvw0DAidJ1fG9dQgRP87WnO2rbuslTn2e2";
static const char expected_response[] = "6ettuJlUe7kamhNa4shpRfgzCbjB7ery0DAidJ1fG9dQgRP87WnO2rcarlTn2eo2";
static const uint16_t header_bytes = 0x01AA;
static const uint8_t footer_bytes[] = {0xFF, 0x0D, 0x0A};

/*
 * Calculates CRC16/IBM3740 of payload
 */
uint16_t crc(const uint8_t *data, size_t length) {
	uint16_t crc = 0xFFFF; // Initial value

	for (size_t i = 0; i < length; i++) {
		crc ^= (uint16_t)data[i] << 8; // XOR with current byte, shifted to MSB

		for (int j = 0; j < 8; j++) {
			if (crc & 0x8000) { // Check if MSB is set
				crc = (crc << 1) ^ 0x1021; // Shift left and XOR with polynomial
			} else {
				crc <<= 1; // Just shift left
			}
		}
	}
	return crc;
}

/*
 * Wait for a port to appear.  Return a pointer to the port.
 */
struct sp_port * GetPort(void) {
	struct sp_port **portList;
	int retval;
	struct sp_port *port = NULL;
	int i;
	char * nameMatch; 
	char * portName;

	port = NULL;

	do {
		retval = sp_list_ports(&portList);

		if (retval == SP_OK) {
			nameMatch = NULL;
			for(i=0; portList[i] != NULL; i++) {
				int usb_vid, usb_pid;
				sp_get_port_usb_vid_pid(portList[i], &usb_vid, &usb_pid);
				//printf("VID: %04X PID: %04X\n", usb_vid, usb_pid);
				if (usb_vid == vid && usb_pid == pid) {
					nameMatch = sp_get_port_name(portList[i]);
					break;
				}
			}
			if (nameMatch != NULL) {
				sp_copy_port(portList[i], &port);
			} else {
				printf("\rWaiting for CCU port to appear...");
			}
		}

		sp_free_port_list(portList);
	} while (port == NULL);

	return port;
}

/*
 * Configure the serial port.
 */
int ConfigureSerialPort(struct sp_port *port) {
	int retval = 0;

	if (SP_OK != sp_set_baudrate(port, baudRate))
	{
		puts("Unable to set port baudrate.");
		retval = -1;
	}
	if(SP_OK != sp_set_bits(port, bits)) {
		puts("Unable to set port width.");
		retval = -1;
	}
	if (SP_OK != sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE)) {
		puts("Unable to set flow control.");
		retval = -1;
	}
		/*} else if (SP_OK !=  sp_set_parity(port, parity)) {
		  puts("Unable to set port parity.");
		  retval = -1;
		  } else if (SP_OK != sp_set_stopbits(port, stopBits)) {
		  puts("Unable to set port stop bits.");
		  retval = -1;*/

	return retval;
}

/*
* Get and initialize port.
*/
struct sp_port * getAndInitializePort() {
	static struct sp_port *port = NULL;

	if (port) 
		closePort(port);

	port = GetPort();

	if (port == NULL) {
		puts("Did not find a suitable port.");
	} else {
		//printf("Using %s\r\n", sp_get_port_name(port));
		int retval = sp_open(port, SP_MODE_READ | SP_MODE_WRITE);
		if (retval == SP_OK) {
			if (ConfigureSerialPort(port)) {
				puts("Could not configure serial port.");
				port = NULL;
			}
		} else {
			port = NULL;
		}
	}

	return port;
}

/*
* Close the CCU port @port.
*/
int closePort(struct sp_port *port) {
	int retval = sp_close(port);
	if(retval == SP_OK) {
		//puts("Serial port closed.");
	} else {
		puts("Unable to close serial port.");
	}
	return retval;
}

/* 
 * Wait for an event on the serial port.
 */
int WaitForEventOnPort(struct sp_port *port) {
	int retval;
	struct sp_event_set *eventSet = NULL;

	retval = sp_new_event_set(&eventSet);
	if (retval == SP_OK) {
		retval = sp_add_port_events(eventSet, port, SP_EVENT_RX_READY | SP_EVENT_ERROR);
		if (retval == SP_OK) {
			retval = sp_wait(eventSet, 10000);
		} else {
			puts("Unable to add events to port.");
			retval = -1;
		}
	} else {
		puts("Unable to create new event set.");
		retval = -1;
	}
	sp_free_event_set(eventSet);

	return retval;
}

/* Reads @size bytes from @port into @buffer. */
int ReadDataFromPort(struct sp_port *port, char *buffer, int size) {
	//WaitForEventOnPort(port);
	// Blocking read
	if (!port)
		return -1;

	int bytes_read = sp_blocking_read(port, buffer, size, 100);
	if (bytes_read > 0) {
		printf("Received: %d\n", bytes_read);
		//for (int i = 0; i < bytes_read; i++) {
		//	printf("ReceivedS: %02x\n", buffer[i]);
		//}
	} else if (bytes_read < 0) {
		fprintf(stderr, "Error reading: %d\n", bytes_read);
	}
	return bytes_read;
}

/*
* Check if the termination sequence has been found in the buffer
*/
int check_for_termination(uint8_t *buffer, int buffer_index) {
    if (buffer_index >= SIZE_FOOTER_BYTES) {
        for (int i = 0; i < SIZE_FOOTER_BYTES; i++) {
            if (buffer[buffer_index - SIZE_FOOTER_BYTES + i] != footer_bytes[i])
                return 0;
        }
        return 1;
    }
    return 0;
}

/*
* Read data from the serial port until the termination sequence is found
*/
int ReadDataFromPortUntilFooter(struct sp_port* port, uint8_t *buffer, int max_size, int timeout) {
	if (!port)
		return -1;

    int buffer_index = 0;
    uint8_t temp_buffer[256];
    int temp_buffer_size = 256;
    int temp_data_size = 0;
	int count = 0;

    // Loop until termination sequence is found
    while (1) {
        // Get the next available chunk of data from the serial port
        temp_data_size = sp_blocking_read_next(port, temp_buffer, temp_buffer_size, timeout);
		//printf("Read %d\n", temp_data_size);
        if (temp_data_size > 0) {
			count += temp_data_size;
            for (int i = 0; i < temp_data_size; i++) {
                buffer[buffer_index++] = temp_buffer[i];

                // Check if the termination sequence is found
                if (check_for_termination(buffer, buffer_index)) {
                    //printf("Termination sequence detected.\n");
                   	return count;
                }

                // If the buffer is full, reset the index
                if (buffer_index >= max_size) {
                    buffer_index = 0;
                }
            }
        } else if (temp_data_size < 0) {
            // Handle error (negative value means error)
            printf("Error reading from the serial port\n");
            return temp_data_size;
        }
    }
	return -1;
}

/* Helper function for error handling. */
int check(enum sp_return result)
{
	/* We'll just exit on any error by calling abort(). */
	char *error_message;

	switch (result) {
		case SP_ERR_ARG:
			printf("Error: Invalid argument.\n");
			abort();
		case SP_ERR_FAIL:
			error_message = sp_last_error_message();
			printf("Error: Failed: %s\n", error_message);
			sp_free_error_message(error_message);
			abort();
		case SP_ERR_SUPP:
			printf("Error: Not supported.\n");
			abort();
		case SP_ERR_MEM:
			printf("Error: Couldn't allocate memory.\n");
			abort();
		case SP_OK:
		default:
			return result;
	}
}

int SendDataToPort(struct sp_port *port, const char *data, int length, unsigned int timeout) {
	if (!port)
		return -1;

	/* On success, sp_blocking_write() and sp_blocking_read()
	 * return the number of bytes sent/received before the
	 * timeout expired. We'll store that result here. */
	int result = -1;

	/* Send data. */
	//printf("Sending '%s' (%d bytes) on port %s.\n",
	//		data, length, sp_get_port_name(port));
	result = check(sp_blocking_write(port, data, length, timeout));

	return result;
}

int CheckCCUSecret(struct sp_port *port) {
	char buf[256];
	SendDataToPort(port, secret_phrase, strlen(secret_phrase), 100);
	ReadDataFromPort(port, buf, 256);
	if (!strncmp(buf, expected_response, 64)) {
		printf("CCU got expected secret.\n");
		return 1;
	}
	return 0;
}

void convert_endianness(uint8_t* array, size_t num_bytes) {
	// Iterate only up to half the array length, swapping pairs
	for (size_t i = 0; i < num_bytes / 2; ++i) {
		uint8_t temp = array[i];
		array[i] = array[num_bytes - 1 - i];
		array[num_bytes - 1 - i] = temp;
	}
}

uint8_t *createCCUPacket(uint16_t command, uint8_t *payload_data, uint16_t payload_length, size_t *length) {
	// Total Size = Header + Command + Payload_Length + Payload + CRC + Footer
	int total_packet_size = 2+2+2+payload_length+2+3;

	// Allocate memory for the entire packet
	uint8_t* packet_buffer = (uint8_t*)malloc(total_packet_size);
	if (packet_buffer == NULL) {
		// Handle allocation error
		return NULL;
	}

	// Cast the buffer to the header type to easily access header fields
	PacketHeader* header = (PacketHeader*)packet_buffer;

	// Populate header fields
	header->start = header_bytes;
	header->command = command;
	header->payload_length = payload_length; // Set the actual payload length

	convert_endianness((uint8_t *) &header->start, 2);
	convert_endianness((uint8_t *) &header->command, 2);
	convert_endianness((uint8_t *) &header->payload_length, 2);

	// Copy the payload data into the buffer after the header
	memcpy(packet_buffer + sizeof(PacketHeader), payload_data, payload_length);
	uint16_t crc_v = crc(packet_buffer, total_packet_size-(3+2));
	convert_endianness((uint8_t *) &crc_v, 2);

	//printf("CRC is 0x%04X\n", crc_v);
	memcpy(packet_buffer + sizeof(PacketHeader) + payload_length, &crc_v, 2);
	memcpy(packet_buffer + sizeof(PacketHeader) + payload_length + 2, footer_bytes, 3);

	*length = total_packet_size;
	return packet_buffer;
}

uint8_t *getPayload(uint8_t *packet) {
	return (packet + SIZE_START_BYTES + SIZE_COMMAND_BYTES + SIZE_PAYLOAD_LENGTH);
}

int verifyPacket(uint8_t *packet, uint16_t expected_command, size_t num_bytes) {
	uint16_t header = *((uint16_t *) (packet + 0));
	uint16_t command = *((uint16_t *) (packet + SIZE_START_BYTES));
	uint16_t payload_length = *((uint16_t *) (packet + SIZE_START_BYTES + SIZE_COMMAND_BYTES));
	uint8_t *payload = (packet + SIZE_START_BYTES + SIZE_COMMAND_BYTES + SIZE_PAYLOAD_LENGTH);

	convert_endianness((uint8_t *) &header, 2);
	convert_endianness((uint8_t *) &command, 2);
	convert_endianness((uint8_t *) &payload_length, 2);

	if (header != header_bytes) {
		printf("Header is incorrect\n");
		return 1;
	}
	if (command != expected_command && command != 0x0101) {
		printf("Command is incorrect. Got 0x%04x, Expected: 0x%04x\n", command, expected_command);
		return 2;
	}
	if (payload_length != num_bytes-(2+2+2+3+2)) {
		printf("Payload length is incorrect\n");
		return 3;
	}

	uint16_t crc_p = *((uint16_t *) (packet + SIZE_START_BYTES + SIZE_COMMAND_BYTES + SIZE_PAYLOAD_LENGTH + payload_length));
	convert_endianness((uint8_t *) &crc_p, 2);

	uint16_t crc_v = crc(packet, SIZE_START_BYTES + SIZE_COMMAND_BYTES + SIZE_PAYLOAD_LENGTH + payload_length);
	if (crc_v != crc_p) {
		printf("CRC is incorrect\n");
		return 4;
	}
	return 0;
}

void convert_endianness_fields(uint8_t *ccu_struct, int ccu_struct_size[], size_t num_fields) {
	int index = 0;
	for (int i = 0; i < num_fields; i++) {
		if (ccu_struct_size[i] > 0) {
			convert_endianness(ccu_struct+index, ccu_struct_size[i]);
		}
		index += ccu_struct_size[i];
	}
}

int sendRequestAndGetResponse(
		struct sp_port *port, uint16_t command, 
		void *request, size_t request_size, int request_fields_size[], int num_request_fields, 
		void *response, size_t response_size, int response_fields_size[], int num_response_fields) {
	size_t length;
	uint8_t resp[256];


	if (!port) {
		printf("Port is not valid.\n");
		return 1;
	}
	convert_endianness_fields((uint8_t *) request, request_fields_size, num_request_fields);
	uint8_t *packet = createCCUPacket(command, request, request_size, &length);
	if (packet == NULL) {
		printf("Could not create CCU packet.\n");
		return 2;
	}
	SendDataToPort(port, packet, length, 1000);
	int n = ReadDataFromPortUntilFooter(port, resp, 256, 1000);
	if (!verifyPacket(resp, command, n)) {
		uint8_t *payload = getPayload(resp);
		memcpy(response, payload, response_size);
		convert_endianness_fields((uint8_t *) response, response_fields_size, num_response_fields);
	} else {
		printf("Did not receive correct packet.\n");
		free(packet);
		return 3;
	}

	free(packet);
	return 0;
}

CCU_Version_Response *getVersion(struct sp_port *port) {
	uint16_t command = COM_VERSION_GET;
	int ccu_struct_size[] = CCU_VERSION_RESPONSE_SIZE;
	static CCU_Version_Response ccu_struct = {};

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			&ccu_struct, sizeof(ccu_struct), ccu_struct_size, sizeof(ccu_struct_size)/sizeof(int)))
		return NULL;

	return &ccu_struct;
}

CCU_Serial_Num_Response *getSerialNum(struct sp_port *port) {
	uint16_t command = COM_SERIAL_NUM;
	int ccu_struct_size[] = CCU_SERIAL_NUM_RESPONSE_SIZE;
	static CCU_Serial_Num_Response ccu_struct = {};

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			&ccu_struct, sizeof(ccu_struct), ccu_struct_size, sizeof(ccu_struct_size)/sizeof(int)))
		return NULL;

	return &ccu_struct;
}

CCU_Set_VPP_Response *setVPP(struct sp_port *port, uint8_t vpp) {
	uint16_t command = COM_SET_VPP;
	static CCU_Set_VPP_Response response = {};
	CCU_Set_VPP_Request request = {};
	request.vpp = vpp;
	int response_fields_size[] = CCU_SET_VPP_RESPONSE_SIZE;
	int request_fields_size[] = CCU_SET_VPP_RESPONSE_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Set_VNN_Response *setVNN(struct sp_port *port, uint8_t vnn) {
	uint16_t command = COM_SET_VNN;
	static CCU_Set_VNN_Response response = {};
	CCU_Set_VNN_Request request = {};
	request.vnn = vnn;
	int response_fields_size[] = CCU_SET_VNN_RESPONSE_SIZE;
	int request_fields_size[] = CCU_SET_VNN_REQUEST_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Set_VPP_Current_Limit_Response *setVPPCurrentLimit(struct sp_port *port, uint16_t limit) {
	uint16_t command = COM_SET_VPP_CURRENT_LIMIT;
	static CCU_Set_VPP_Current_Limit_Response response = {};
	CCU_Set_VPP_Current_Limit_Request request = {};
	request.vppcurrent = limit;
	int response_fields_size[] = CCU_SET_VPP_CURRENT_LIMIT_RESPONSE_SIZE;
	int request_fields_size[] = CCU_SET_VPP_CURRENT_LIMIT_REQUEST_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Set_VNN_Current_Limit_Response *setVNNCurrentLimit(struct sp_port *port, uint16_t limit) {
	uint16_t command = COM_SET_VNN_CURRENT_LIMIT;
	static CCU_Set_VNN_Current_Limit_Response response = {};
	CCU_Set_VNN_Current_Limit_Request request = {};
	request.vnncurrent = limit;
	int response_fields_size[] = CCU_SET_VNN_CURRENT_LIMIT_RESPONSE_SIZE;
	int request_fields_size[] = CCU_SET_VNN_CURRENT_LIMIT_REQUEST_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Set_Next_Sequence_Response *setNextSequence(struct sp_port *port, uint16_t currentSequence, uint16_t maxSequence) {
	uint16_t command = COM_SET_NEXT_SEQUENCE;
	static CCU_Set_Next_Sequence_Response response = {};
	CCU_Set_Next_Sequence_Request request = {};
	request.currentSequence = currentSequence;
	request.maxSequence = maxSequence;
	int response_fields_size[] = CCU_SET_NEXT_SEQUENCE_RESPONSE_SIZE;
	int request_fields_size[] = CCU_SET_NEXT_SEQUENCE_REQUEST_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Set_Stop_Request *setStop(struct sp_port *port, uint8_t stop) {
	uint16_t command = COM_SET_STOP;
	static CCU_Set_Stop_Request response = {};
	CCU_Set_Stop_Request request = {};
	request.stop = stop;
	int request_fields_size[] = CCU_SET_STOP_REQUEST_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			NULL, 0, NULL, 0
			))
		return NULL;

	response.stop = stop;

	return &response;
}

CCU_Set_High_Impedance_Timeout_Response *setHighImpedanceTimeout(struct sp_port *port, uint16_t ztimer) {
	uint16_t command = COM_SET_HIGH_IMPEDANCE_TIMEOUT;
	static CCU_Set_High_Impedance_Timeout_Response response = {};
	CCU_Set_High_Impedance_Timeout_Request request = {};
	request.Ztimer = ztimer;
	int response_fields_size[] = CCU_SET_HIGH_IMPEDANCE_TIMEOUT_RESPONSE_SIZE;
	int request_fields_size[] = CCU_SET_HIGH_IMPEDANCE_TIMEOUT_REQUEST_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Set_Heartbeat_Response *setHeartbeat(struct sp_port *port, uint16_t heartbeat) {
	uint16_t command = COM_SET_HEARTBEAT;
	static CCU_Set_Heartbeat_Response response = {};
	CCU_Set_Heartbeat_Request request = {};
	request.heartbeat = heartbeat;
	int response_fields_size[] = CCU_SET_HEARTBEAT_RESPONSE_SIZE;
	int request_fields_size[] = CCU_SET_HEARTBEAT_REQUEST_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Set_Trigger_Delay_Response *setTrigger_Delay(struct sp_port *port, uint16_t TrOd) {
	uint16_t command = COM_SET_TRIGGER_DELAY;
	static CCU_Set_Trigger_Delay_Response response = {};
	CCU_Set_Trigger_Delay_Request request = {};
	request.TrOd = TrOd;
	int response_fields_size[] = CCU_SET_TRIGGER_DELAY_RESPONSE_SIZE;
	int request_fields_size[] = CCU_SET_TRIGGER_DELAY_REQUEST_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}


CCU_Read_HV_Status_Response *readHVStatus(struct sp_port *port) {
	uint16_t command = COM_READ_HV_STATUS;
	static CCU_Read_HV_Status_Response response = {};
	int response_fields_size[] = CCU_READ_HV_STATUS_RESPONSE_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	printf("HV Status: %d\n", response.VPPccu);

	return &response;
}

CCU_Get_Picards_Health_Response *getPicardsHealth(struct sp_port *port) {
	uint16_t command = COM_GET_PICARDS_HEALTH;
	static CCU_Get_Picards_Health_Response response = {};
	int response_fields_size[] = CCU_GET_PICARDS_HEALTH_RESPONSE_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Read_Current_Sequence_Response *readCurrentSequence(struct sp_port *port) {
	uint16_t command = COM_READ_CURRENT_SEQUENCE;
	static CCU_Read_Current_Sequence_Response response = {};
	int response_fields_size[] = CCU_READ_CURRENT_SEQUENCE_RESPONSE_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Read_Current_From_HV_Response *readCurrentFromHV(struct sp_port *port) {
	uint16_t command = COM_READ_CURRENT_FROM_HV;
	static CCU_Read_Current_From_HV_Response response = {};
	int response_fields_size[] = CCU_READ_CURRENT_FROM_HV_RESPONSE_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Soft_Reset_Response *softReset(struct sp_port *port) {
	uint16_t command = COM_SOFT_RESET;
	static CCU_Soft_Reset_Response response = {};
	int response_fields_size[] = CCU_SOFT_RESET_RESPONSE_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

CCU_Get_Master_Diagnostics_Response *getMasterDiagnostics(struct sp_port *port) {
	uint16_t command = COM_GET_MASTER_DIAGNOSTICS;
	static CCU_Get_Master_Diagnostics_Response response = {};
	int response_fields_size[] = CCU_GET_MASTER_DIAGNOSTICS_RESPONSE_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}

int emulateTrigger(struct sp_port *port) {
	uint16_t command = COM_EMULATE_TRIGGER;

	if (sendRequestAndGetResponse(
			port, command, 
			NULL, 0, NULL, 0, 
			NULL, 0, NULL, 0
			))
		return 0;

	return 1;
}

CCU_Set_Sequence_256_Response *setPackedSequence256(
		struct sp_port *port, 
		PackedSequence256 packedSeq, 
		uint16_t maxseq, uint16_t target_seq) {
	uint16_t command = COM_SET_SEQUENCE_256;
	static CCU_Set_Sequence_256_Response response = {};
	CCU_Set_Sequence_256_Request request = {};
	memcpy(request.ch1_64, packedSeq.ch1_64, 16);
	memcpy(request.ch65_128, packedSeq.ch65_128, 16);
	memcpy(request.ch129_192, packedSeq.ch129_192, 16);
	memcpy(request.ch193_256, packedSeq.ch193_256, 16);
	request.maxSequence = maxseq;
	request.targetSequence = target_seq;
	int request_fields_size[] = CCU_SET_SEQUENCE_256_REQUEST_SIZE;
	int response_fields_size[] = CCU_SET_SEQUENCE_256_RESPONSE_SIZE;

	if (sendRequestAndGetResponse(
			port, command, 
			&request, sizeof(request), request_fields_size, sizeof(request_fields_size)/sizeof(int), 
			&response, sizeof(response), response_fields_size, sizeof(response_fields_size)/sizeof(int)
			))
		return NULL;

	return &response;
}


static inline uint8_t encode2bit(int v)
{
    if (v == -1)  return 1; // 01
    if (v == 1) return 2; // 10
	if (v == 0) return 3; // 11
    return 0;             // 00
}

PackedSequence256 packSequence256(const int sequence[256])
{
    PackedSequence256 out;

    /* Array of pointers to output blocks */
    uint8_t *dst[4] = {
        out.ch1_64,
        out.ch65_128,
        out.ch129_192,
        out.ch193_256
    };

    for (int i = 0; i < 64; i++) {
        uint8_t byte =
            (encode2bit(sequence[4*i    ]) << 6) |
            (encode2bit(sequence[4*i + 1]) << 4) |
            (encode2bit(sequence[4*i + 2]) << 2) |
             encode2bit(sequence[4*i + 3]);

        dst[i >> 4][15 - (i & 15)] = byte;
    }

    return out;
}

CCU_Set_Sequence_256_Response *setSequence256(
		struct sp_port *port, const int sequence[256], uint16_t maxseq, uint16_t target_seq) {
	return setPackedSequence256(port, packSequence256(sequence), maxseq, target_seq);
}

