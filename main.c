#include "mcu_mock.h"

/* Global Circular Buffer for UART Reception */
Queue_t srecRx_queue;

/* LPUART1 Interrupt Handler: Clears errors and pushes received bytes to Queue (Executed in RAM) */
RAM_FUNC void LPUART1_RxTx_IRQHandler(void) {
    uint32_t stat = LPUART1->STAT;
    
    /* Clear Error Flags (Overrun, Noise, Framing, Parity) if set */
    if (stat & (LPUART_STAT_OR_MASK | LPUART_STAT_NF_MASK | LPUART_STAT_FE_MASK | LPUART_STAT_PF_MASK)) {
        LPUART1->STAT = (stat & (LPUART_STAT_OR_MASK | LPUART_STAT_NF_MASK | LPUART_STAT_FE_MASK | LPUART_STAT_PF_MASK));
    }
    
    /* Check Read Data Register Full flag and read data */
    if (LPUART1->STAT & LPUART_STAT_RDRF_MASK) {
        char c = (char)(LPUART1->DATA);
        queue_push_char(&srecRx_queue, c);
 }
}

int main(void)
{
    /* Initialize S-Record Reception Queue */
    queue_init(&srecRx_queue);
    
    /* Initialize Flash Driver */
    mock_flash_init();
    
    /* Copy Vector Table to RAM to avoid Read-While-Write conflicts */
    Relocate_VTOR_To_RAM();
    
    /* Disable Interrupts to safely Erase Flash */
    INT_SYS_DisableIRQGlobal();
    
    /* Erase User Application Flash Sectors */
    for(uint32_t i = 0; i < IMAGE_KB_SIZE; i++) {
        FLASH_DRV_EraseSector(&flashSSDConfig, APP_START_ADDR + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
    }
    
    /* Re-enable Interrupts for UART Communication */
    INT_SYS_EnableIRQGlobal();
    
    /* Initialize UART with RX Interrupt enabled */
    uart01_init_with_interrupt();
    
    /* Send Welcome Message */
    lpuart01_send_string("\r\n==============================");
    lpuart01_send_string("\r\n ChienKD1 S32K144 BOOTLOADER V1.0    ");
    lpuart01_send_string("\r\n==============================\r\n");
    lpuart01_send_string("State: READY TO DOWNLOAD....(Send S-Rec now)\r\n");
    
    /* Start Download and Programming Process */
    download_error_t error_downCode = fHandler_download_app(&srecRx_queue);

        switch (error_downCode) {
            case DOWNLOAD_OK:
                lpuart01_send_string("\r\n> Download SUCCESS!\r\n");
                
                /* Fetch Stack Pointer and Reset Handler from App Vector Table */
                uint32_t *appVectorTable = (uint32_t *)APP_START_ADDR;
                uint32_t appStackPointer = appVectorTable[0];
                uint32_t appResetHandler = appVectorTable[1];
                
                lpuart01_send_string("> Jumping to User Application...\r\n");
                
                /* Perform Jump to User Application */
                JumpToUserApplication(appStackPointer, appResetHandler);
                break;

            case DOWNLOAD_CRC_ERROR:
                lpuart01_send_string("\r\n> Error: Checksum verification failed (CRC Error)!\r\n");
                lpuart01_send_string("> Please reset and try again.\r\n");
                break;

            case DOWNLOAD_COUNTER_ERROR:
                lpuart01_send_string("\r\n> Error: Line count mismatch (Counter Error)!\r\n");
                lpuart01_send_string("> Data loss occurred during transmission.\r\n");
                break;

            case DOWNLOAD_TIMEOUT_ERROR:
                lpuart01_send_string("\r\n> Error: Connection timeout!\r\n");
                lpuart01_send_string("> No data received for a long time.\r\n");
                break;

            default:
                lpuart01_send_string("\r\n> Error: Unknown error code!\r\n");
                break;
        }
        
        /* Error Trap: Halt System */
        while(1) {
            lpuart01_send_string("System Halted. Please Reset.\r\n");
            for(volatile int i=0; i<5000000; i++);
        }
}