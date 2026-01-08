
#ifndef MCU_MOCK_H_
#define MCU_MOCK_H_

#include "S32K144.h"
#include "mock_queue.h"
#include "mock_uartDrv.h"
#include "mock_flashHandler.h"
#include "mock_flashDrv.h"
#include "sdk_project_config.h"
#include <stdint.h>

#define FLASH_SECTOR_SIZE   4096U
#define VTOR_LOCATE 		0x00000000U
#define APP_START_ADDR     	0x00004000U
#define IMAGE_KB_SIZE		16U

/* * Macro RAM_FUNC: Ép hàm chạy trên RAM thay vì Flash. */
#if defined(__GNUC__)
    #define RAM_FUNC __attribute__((section(".code_ram")))
#else
    #define RAM_FUNC
#endif

void Relocate_VTOR_To_RAM(void);
void JumpToUserApplication(uint32_t userSP, uint32_t userStartup);

#endif
