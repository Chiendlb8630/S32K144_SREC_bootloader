#include "mcu_mock.h"

__attribute__((aligned(1024))) uint32_t __ram_vector_table[256];

void Relocate_VTOR_To_RAM(void) {
    uint32_t *current_vectors = (uint32_t *)VTOR_LOCATE;
    uint32_t i;
    INT_SYS_DisableIRQGlobal();
    for (i = 0; i < 256; i++) {
        __ram_vector_table[i] = current_vectors[i];
    }
    S32_SCB->VTOR = (uint32_t)__ram_vector_table;
    INT_SYS_EnableIRQGlobal();
}

void JumpToUserApplication(uint32_t userSP, uint32_t userStartup) {
    if (userSP == 0xFFFFFFFF || userStartup == 0xFFFFFFFF) return;
    LPUART1->CTRL = 0;

    __asm volatile ("cpsid i" : : : "memory");
    S32_SysTick->CSR = 0;

    for (uint8_t i = 0; i < 3; i++) {
        S32_NVIC->ICER[i] = 0xFFFFFFFF;
        S32_NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    S32_SCB->VTOR = (uint32_t)APP_START_ADDR;
    __asm volatile ("msr msp, %0" : : "r" (userSP) : "memory");
    __asm volatile ("dsb");
    __asm volatile ("isb");

    void (*app_reset_handler)(void) = (void (*)(void))userStartup;
    app_reset_handler();
}
