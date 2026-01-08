#ifndef MOCK_FLASHHANDLER_H_
#define MOCK_FLASHHANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "mcu_mock.h"

#define MAX_ADDRESS_BP  	4U
#define MAX_DATA_BP     	32
#define MAX_PHSIZE_BP   	40U
#define VER_ERROR       	0x00U
#define VER_OK          	0x01U

typedef union
{
    uint8_t Byte[MAX_PHSIZE_BP];
    struct
    {
        char    PhraseType;      /* S0, S1, ... */
        uint8_t PhraseSize;      /* Length field from S-Rec */
        uint8_t PhraseAddress[MAX_ADDRESS_BP]; /* Raw Address bytes (Big Endian) */
        uint8_t PhraseData[MAX_DATA_BP];       /* Raw Data bytes */
        uint8_t PhraseCRC;
    }F;
} boot_phrase_t;

typedef enum {
    DOWNLOAD_OK = 0,
	DOWNLOAD_CRC_ERROR = 1,
	DOWNLOAD_COUNTER_ERROR = 2,
	DOWNLOAD_TIMEOUT_ERROR = 3
} download_error_t;

download_error_t fHandler_download_app(Queue_t *q);
extern uint32_t start_address;
extern uint32_t g_total_image_size;
extern uint8_t  is_mem_init;

#endif /* MOCK_FLASHHANDLER_H_ */
