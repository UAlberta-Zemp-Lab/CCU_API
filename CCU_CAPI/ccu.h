/*
 * Header file for CCU C-API
 * CCU Version 3.0
 * Author: Tarek Kaddoura
 * Copyright CliniSonix 2025
 */
#include <libserialport.h>
#include <stdint.h>

#define SIZE_START_BYTES 	2
#define SIZE_COMMAND_BYTES 	2
#define SIZE_PAYLOAD_LENGTH 	2
#define SIZE_FOOTER_BYTES 	3

#define HEADER_BYTES 0x01AA
#define FOOTER_BYTES {0xFF, 0x0D, 0x0A}
#define COM_VERSION_GET 0x0102
#define COM_SERIAL_NUM 0x0103
#define COM_SET_VPP 0x0201
#define COM_SET_VNN 0x0202
#define COM_SET_VPP_CURRENT_LIMIT 0x0203
#define COM_SET_VNN_CURRENT_LIMIT 0x0204
#define COM_SET_NEXT_SEQUENCE 0x0205
#define COM_SET_STOP 0x0206
#define COM_SET_HIGH_IMPEDANCE_TIMEOUT 0x0401
#define COM_SET_HEARTBEAT 0x0402
#define COM_SET_TRIGGER_DELAY 0x0207
#define COM_READ_HV_STATUS 0x0301
#define COM_GET_PICARDS_HEALTH 0x0302
#define COM_READ_CURRENT_SEQUENCE 0x0303
#define COM_READ_CURRENT_FROM_HV 0x0304
#define COM_GET_MASTER_DIAGNOSTICS 0x0305
#define COM_SOFT_RESET 0x0601
#define COM_EMULATE_TRIGGER 0x0602
#define COM_SET_SEQUENCE_256 0x0701

// The header structure

#pragma pack(push, 1)
typedef struct {
	uint16_t start;
	uint16_t command;
	uint16_t payload_length;
} PacketHeader;

typedef struct {
    uint8_t ch1_64[16];
    uint8_t ch65_128[16];
    uint8_t ch129_192[16];
    uint8_t ch193_256[16];
} PackedSequence256;

/* REQUEST COMMANDS */

typedef struct {
	uint16_t version_major;
	uint16_t version_minor;
	uint16_t bug_fix;
	uint64_t stamp;
} CCU_Version_Response;
#define CCU_VERSION_RESPONSE_SIZE {2, 2, 2, 8}

typedef struct {
	char serial[3];
} CCU_Serial_Num_Response;
#define CCU_SERIAL_NUM_RESPONSE_SIZE {-1}


/* SET COMMANDS */

typedef struct {
	uint8_t vpp;
} CCU_Set_VPP_Response, CCU_Set_VPP_Request;
#define CCU_SET_VPP_RESPONSE_SIZE {1}
#define CCU_SET_VPP_REQUEST_SIZE {1}

typedef struct {
	uint8_t vnn;
} CCU_Set_VNN_Response, CCU_Set_VNN_Request;
#define CCU_SET_VNN_RESPONSE_SIZE {1}
#define CCU_SET_VNN_REQUEST_SIZE {1}

typedef struct {
	uint16_t vppcurrent;
} CCU_Set_VPP_Current_Limit_Response, CCU_Set_VPP_Current_Limit_Request;
#define CCU_SET_VPP_CURRENT_LIMIT_RESPONSE_SIZE {2}
#define CCU_SET_VPP_CURRENT_LIMIT_REQUEST_SIZE {2}

typedef struct {
	uint16_t vnncurrent;
} CCU_Set_VNN_Current_Limit_Response, CCU_Set_VNN_Current_Limit_Request;
#define CCU_SET_VNN_CURRENT_LIMIT_RESPONSE_SIZE {2}
#define CCU_SET_VNN_CURRENT_LIMIT_REQUEST_SIZE {2}

typedef struct {
	uint16_t currentSequence;
	uint16_t maxSequence;
} CCU_Set_Next_Sequence_Response, CCU_Set_Next_Sequence_Request;
#define CCU_SET_NEXT_SEQUENCE_RESPONSE_SIZE {2, 2}
#define CCU_SET_NEXT_SEQUENCE_REQUEST_SIZE {2, 2}

typedef struct {
	uint8_t stop;
} CCU_Set_Stop_Request;
#define CCU_SET_STOP_REQUEST_SIZE {1}

typedef struct {
	uint16_t Ztimer;
} CCU_Set_High_Impedance_Timeout_Response, CCU_Set_High_Impedance_Timeout_Request;
#define CCU_SET_HIGH_IMPEDANCE_TIMEOUT_RESPONSE_SIZE {2}
#define CCU_SET_HIGH_IMPEDANCE_TIMEOUT_REQUEST_SIZE {2}

typedef struct {
	uint16_t heartbeat;
} CCU_Set_Heartbeat_Response, CCU_Set_Heartbeat_Request;
#define CCU_SET_HEARTBEAT_RESPONSE_SIZE {2}
#define CCU_SET_HEARTBEAT_REQUEST_SIZE {2}

typedef struct {
	uint16_t TrOd;
} CCU_Set_Trigger_Delay_Response, CCU_Set_Trigger_Delay_Request;
#define CCU_SET_TRIGGER_DELAY_RESPONSE_SIZE {2}
#define CCU_SET_TRIGGER_DELAY_REQUEST_SIZE {2}

/* READ COMMANDS */

typedef struct {
	uint8_t VPPccu;
	uint8_t VNNccu;
	uint8_t VPPcard1;
	uint8_t VNNcard1;
	uint8_t VPPcard2;
	uint8_t VNNcard2;
	uint8_t VPPcard3;
	uint8_t VNNcard3;
	uint8_t VPPcard4;
	uint8_t VNNcard4;
} CCU_Read_HV_Status_Response;
#define CCU_READ_HV_STATUS_RESPONSE_SIZE {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}

typedef struct {
	uint8_t picard1_health;
	uint8_t picard2_health;
	uint8_t picard3_health;
	uint8_t picard4_health;
} CCU_Get_Picards_Health_Response;
#define CCU_GET_PICARDS_HEALTH_RESPONSE_SIZE {1, 1, 1, 1}

typedef struct {
	uint16_t currentSequence;
} CCU_Read_Current_Sequence_Response;
#define CCU_READ_CURRENT_SEQUENCE_RESPONSE_SIZE {2}

typedef struct {
	float vppcurrent;
	float vnncurrent;
} CCU_Read_Current_From_HV_Response;
#define CCU_READ_CURRENT_FROM_HV_RESPONSE_SIZE {4, 4}

typedef struct {
	uint8_t stop;
	uint8_t imgmode;
	uint8_t initiation;
	uint8_t operation;
	uint8_t voltage_ok;
	uint8_t HVLevelError;
	uint16_t card1Seq;
	uint16_t card2Seq;
	uint16_t card3Seq;
	uint16_t card4Seq;
} CCU_Get_Master_Diagnostics_Response;
#define CCU_GET_MASTER_DIAGNOSTICS_RESPONSE_SIZE {1, 1, 1, 1, 1, 1, 2, 2, 2, 2}

typedef struct {
	uint8_t result;
} CCU_Soft_Reset_Response;
#define CCU_SOFT_RESET_RESPONSE_SIZE {1}

typedef struct {
	uint8_t ch1_64[16];
	uint8_t ch65_128[16];
	uint8_t ch129_192[16];
	uint8_t ch193_256[16];
	uint16_t maxSequence;
	uint16_t targetSequence;
} CCU_Set_Sequence_256_Request;
#define CCU_SET_SEQUENCE_256_REQUEST_SIZE {16, 16, 16, 16, 2, 2}

typedef struct {
	uint16_t targetSequence;
} CCU_Set_Sequence_256_Response;
#define CCU_SET_SEQUENCE_256_RESPONSE_SIZE {2}

#pragma pack(pop)

/*
 * Calculates CRC16/IBM3740 of payload
 */
uint16_t crc(const uint8_t *data, size_t length);

/*
 * Wait for a port to appear.  Return a pointer to the port.
 */
struct sp_port * GetPort(void);

/*
 * Configure the serial port.
 */
int ConfigureSerialPort(struct sp_port *port);
struct sp_port * getAndInitializePort();
int closePort(struct sp_port *port);

/* 
 * Wait for an event on the serial port.
 */
int WaitForEventOnPort(struct sp_port *port);
/*
 * Read data from the port.
 */
int ReadDataFromPort(struct sp_port *port, char *buffer, int size);
/* Helper function for error handling. */
int check(enum sp_return result);
int SendDataToPort(struct sp_port *port, const char *data, int length, unsigned int timeout);
int CheckCCUSecret(struct sp_port *port);
void convert_endianness(uint8_t* array, size_t num_bytes);
uint8_t* createCCUPacket(uint16_t command, uint8_t *payload_data, uint16_t payload_length, size_t *length);
uint8_t *getPayload(uint8_t *packet);
int verifyPacket(uint8_t *packet, uint16_t expected_command, size_t num_bytes);
void convert_endianness_fields(uint8_t *ccu_struct, int ccu_struct_size[], size_t num_fields);
int sendRequestAndGetResponse(
		struct sp_port *port, uint16_t command, 
		void *request, size_t request_size, int request_fields_size[], int num_request_fields, 
		void *response, size_t response_size, int response_fields_size[], int num_response_fields);

/* CCU API Functions */
CCU_Version_Response *getVersion(struct sp_port *port);
CCU_Serial_Num_Response *getSerialNum(struct sp_port *port);
CCU_Set_VPP_Response *setVPP(struct sp_port *port, uint8_t vpp);
CCU_Set_VNN_Response *setVNN(struct sp_port *port, uint8_t vnn);
CCU_Set_VPP_Current_Limit_Response *setVPPCurrentLimit(struct sp_port *port, uint16_t limit);
CCU_Set_VNN_Current_Limit_Response *setVNNCurrentLimit(struct sp_port *port, uint16_t limit);
CCU_Set_Next_Sequence_Response *setNextSequence(struct sp_port *port, uint16_t currentSequence, uint16_t maxSequence);
CCU_Set_Stop_Request *setStop(struct sp_port *port, uint8_t stop);
CCU_Set_High_Impedance_Timeout_Response *setHighImpedanceTimeout(struct sp_port *port, uint16_t ztimer);
CCU_Set_Heartbeat_Response *setHeartbeat(struct sp_port *port, uint16_t heartbeat);
CCU_Set_Trigger_Delay_Response *setTrigger_Delay(struct sp_port *port, uint16_t TrOd);
CCU_Read_HV_Status_Response *readHVStatus(struct sp_port *port);
CCU_Get_Picards_Health_Response *getPicardsHealth(struct sp_port *port);
CCU_Read_Current_Sequence_Response *readCurrentSequence(struct sp_port *port);
CCU_Read_Current_From_HV_Response *readCurrentFromHV(struct sp_port *port);
CCU_Soft_Reset_Response *softReset(struct sp_port *port);
CCU_Get_Master_Diagnostics_Response *getMasterDiagnostics(struct sp_port *port);
CCU_Set_Sequence_256_Response *setPackedSequence256(
		struct sp_port *port, 
		PackedSequence256 packedSeq, 
		uint16_t maxseq, uint16_t target_seq);
CCU_Set_Sequence_256_Response *setSequence256(
		struct sp_port *port, const int sequence[256], uint16_t maxseq, uint16_t target_seq);
int emulateTrigger(struct sp_port *port);
