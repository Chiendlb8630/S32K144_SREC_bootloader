#include "mcu_mock.h"
#include <string.h>

/* --- GLOBAL VARIABLES --- */
uint32_t start_address = 0;
uint32_t g_total_image_size = 0;
uint8_t  is_mem_init = 0;

/* --- STATIC VARIABLES FOR CACHE BUFFER --- */
static uint8_t  s_cacheBuf[8];           // Buffer containing 1 Phrase (8 bytes)
static uint32_t s_cacheAddr = 0xFFFFFFFF;// Current address of the Buffer
static bool     s_isDirty = false;       // Flag indicating data has not been written yet

/* Converts a byte array (Big Endian) to a 32-bit integer value */
static inline uint32_t SRec_Hex_To_Uint32(uint8_t *addrPtr, uint8_t len) {
    uint32_t val = 0;
    for (uint8_t i = 0; i < len; i++) {
        val = (val << 8) | addrPtr[i];
    }
    return val;
}

/* Converts a Hex character (0-9, A-F) to its corresponding numeric value */
static inline uint8_t HexCharToVal(char c) {
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
    return 0;
}

/* Converts a 2-character Hex string to a 1-byte value */
static inline uint8_t Str2Byte(const char *pStr) {
    return (HexCharToVal(pStr[0]) << 4) | HexCharToVal(pStr[1]);
}

/* Flushes data from the temporary buffer (Cache) to Flash memory if new data exists */
static void fHandler_flush_cache(void) {
    /* Only write if there is new data and the address is valid */
    if (s_isDirty && (s_cacheAddr != 0xFFFFFFFF)) {
        FLASH_DRV_Program(&flashSSDConfig, s_cacheAddr, 8, s_cacheBuf);
        s_isDirty = false;
    }
}

/* Clears data in the temporary buffer and resets the Cache address to the initial state */
static void fHandler_clear_cache(void) {
    s_cacheAddr = 0xFFFFFFFF;
    s_isDirty = false;
    memset(s_cacheBuf, 0xFF, 8); // Fill with 0xFF
}

/* Retrieves and parses an S-Record line from the queue into the data structure */
uint8_t fHandler_getDat_fromQueue(Queue_t *q, boot_phrase_t* BP){
	Srec_line_t *line = queue_get_read_buffer(q);
	if(line == NULL) return 0;
	char *ptr = line->data;
	if(*ptr != 'S'){
		queue_release_read_buffer(q);
		return 0;
	}
	ptr++;
	BP->F.PhraseType = *ptr;
	ptr++;
	uint8_t count = Str2Byte(ptr);
	BP-> F.PhraseSize = count;
	ptr += 2;
	uint8_t addrLen = 2;
	switch (BP->F.PhraseType) {
	    case '2': case '6': case '8':
	         addrLen = 3;
	         break;
	    case '3': case '7':
	         addrLen = 4;
	         break;
	    }

	    /* Parse Address */
	    for (uint8_t i = 0; i < addrLen; i++) {
	        BP->F.PhraseAddress[i] = Str2Byte(ptr);
	        ptr += 2;
	    }

	    int8_t dataLen = count - addrLen - 1;
	    memset(BP->F.PhraseData, 0xFF, MAX_DATA_BP);

	    if (dataLen > 0 && dataLen <= MAX_DATA_BP) {
	        for (uint8_t i = 0; i < dataLen; i++) {
	            BP->F.PhraseData[i] = Str2Byte(ptr);
	            ptr += 2;
	        }
	    }
	    BP->F.PhraseCRC = Str2Byte(ptr);
	    queue_release_read_buffer(q);
	    return 1;
}

/* Calculates and verifies the Checksum of the S-Record line to ensure data integrity */
static uint8_t fHandler_verify_checksum(const boot_phrase_t *BP){
    uint32_t checksum = 0;
    uint8_t count_remaining = 0;
    uint8_t expected_crc = 0;
    uint8_t addr_len = 2;
    uint8_t i;

    if ((BP->F.PhraseType < '0') || (BP->F.PhraseType > '9')) {
        return VER_ERROR;
    }
    /* Check Size limit */
    if ((BP->F.PhraseSize > MAX_PHSIZE_BP) || (BP->F.PhraseSize > (MAX_DATA_BP + 5))) {
        return VER_ERROR;
    }
    /* Start Summing: Count Byte */
    count_remaining = BP->F.PhraseSize;
    checksum += count_remaining;

    /* Determine Address Length & Sum Address */
    /* S2, S6, S8 use 24-bit address (3 bytes) */
    if ((BP->F.PhraseType == '2') || (BP->F.PhraseType == '6') || (BP->F.PhraseType == '8')) {
        addr_len = 3;
    }
    /* S3, S7 use 32-bit address (4 bytes) */
    else if ((BP->F.PhraseType == '3') || (BP->F.PhraseType == '7')) {
        addr_len = 4;
    }
    /* S0, S1, S5, S9 use 16-bit address (2 bytes) - Default */

    /* Sum Address Bytes */
    for (i = 0; i < addr_len; i++) {
        checksum += BP->F.PhraseAddress[i];
    }

    /* Update counter to track Data bytes only */
    /* count_remaining currently holds (Addr + Data + 1 CRC). Subtract Addr. */
    if (count_remaining < addr_len + 1) return VER_ERROR; /* Avoid Underflow */
    count_remaining -= addr_len;

    /* Loop until < count - 1 (subtracting the final CRC byte) */
    for (i = 0; i < (count_remaining - 1); i++) {
        checksum += BP->F.PhraseData[i];
    }

    /* Final Calculation */
    /* Keep LSB */
    expected_crc = (uint8_t)(checksum & 0xFF);
    /* One's Complement */
    expected_crc = ~expected_crc;

    /* Compare */
    if (expected_crc == BP->F.PhraseCRC) {
        return VER_OK;
    } else {
        return VER_ERROR;
    }
}

/* Accumulates data into the 8-byte cache and performs Flash write when full or block switching (Alignment handling) */
static void fHandler_write_toMem(boot_phrase_t *BP){
    uint32_t dest_address = 0;
    uint16_t data_len = 0;
    uint8_t  addr_len = 0;

    /* Parse address and length */
    if (BP->F.PhraseType == '3') addr_len = 4;
    else if (BP->F.PhraseType == '2') addr_len = 3;
    else addr_len = 2;

    dest_address = SRec_Hex_To_Uint32(BP->F.PhraseAddress, addr_len);

    if (BP->F.PhraseSize > (addr_len + 1)) {
        data_len = BP->F.PhraseSize - addr_len - 1;
        g_total_image_size += data_len;
    } else {
        return;
    }

    /* PROTECT FLASH CONFIG (0x400 - 0x410) */
    if ((dest_address >= 0x400) && (dest_address <= 0x40F)) return;
    /* Only write to the App region */
    if (dest_address < APP_START_ADDR) return;

    /* Iterate through each data byte in the S-Rec line */
    for (uint16_t i = 0; i < data_len; i++) {
        uint32_t current_byte_addr = dest_address + i;

        /* Calculate the start address of the 8-byte Block */
        uint32_t aligned_base = current_byte_addr & ~7U;
        /* Calculate offset within the Block (0-7) */
        uint8_t  offset = current_byte_addr & 7U;

        /* If moving to a new Block -> Write the old Block to Flash */
        if (aligned_base != s_cacheAddr) {
            fHandler_flush_cache();

            /* Reset Cache for the new Block */
            s_cacheAddr = aligned_base;
            memset(s_cacheBuf, 0xFF, 8);
        }

        /* Copy data byte to Cache */
        s_cacheBuf[offset] = BP->F.PhraseData[i];
        s_isDirty = true;
    }
}

/* Main loop managing the process of receiving, verifying checksum, and programming firmware from Queue to Flash */
download_error_t fHandler_download_app(Queue_t *q) {
    boot_phrase_t BP;
    uint32_t DataPhrase_counter = 0;
    uint8_t  verify_status;
    uint8_t  end_records = 0;
    uint32_t timeout = 0;

    if (is_mem_init == 0) {
    	mock_flash_init();
        is_mem_init = 1;
    }

    /* Clear cache before starting download */
    fHandler_clear_cache();

    do {
        if (fHandler_getDat_fromQueue(q, &BP) == 0) {
            timeout++;
            if (timeout > 900000000) return DOWNLOAD_TIMEOUT_ERROR;
            continue;
        }

        timeout = 0;
        /* Verify Checksum */
        verify_status = fHandler_verify_checksum(&BP);
        if (verify_status != VER_OK) {
            return DOWNLOAD_CRC_ERROR;
        }

        /* Process packet */
        switch (BP.F.PhraseType) {
            case '1':
            case '2':
            case '3':
                fHandler_write_toMem(&BP); /* Push to Cache */
                DataPhrase_counter++;
                break;
            case '5':
            case '6':
                {
                    uint32_t count = SRec_Hex_To_Uint32(BP.F.PhraseAddress, (BP.F.PhraseType == '5') ? 2 : 3);
                    if (count != DataPhrase_counter) {
                        return DOWNLOAD_COUNTER_ERROR;
                    }
                }
                break;
            case '7':
            case '8':
            case '9':
                {
                    uint8_t len = 2;
                    if (BP.F.PhraseType == '7') len = 4;
                    if (BP.F.PhraseType == '8') len = 3;
                    start_address = SRec_Hex_To_Uint32(BP.F.PhraseAddress, len);
                    end_records = 1;
                    /* Write remaining data in Cache to Flash */
                    fHandler_flush_cache();
                }
                break;
            default:
            	break;
        }
    } while (!end_records);
    return DOWNLOAD_OK;
}