#include "mcu_mock.h"

/* Initialize Fast Internal Reference Clock (FIRC) to 48 MHz */
static void init_firc_48MHz(void){
    SCG->FIRCDIV = SCG_FIRCDIV_FIRCDIV1(1) | SCG_FIRCDIV_FIRCDIV2(1);
    SCG->FIRCCFG = SCG_FIRCCFG_RANGE(0);
    while(SCG->FIRCCSR & SCG_FIRCCSR_LK_MASK); /* Wait if Locked */
    SCG->FIRCCSR |= SCG_FIRCCSR_FIRCEN_MASK;   /* Enable FIRC */
    while(!(SCG->FIRCCSR & SCG_FIRCCSR_FIRCVLD_MASK)); /* Wait for Valid status */
}

/* Configure Clock Source for LPUART1 */
static void init_uartclk_source(void){

    PCC->PCCn[PCC_LPUART1_INDEX] &= ~PCC_PCCn_CGC_MASK;
    /* Select Clock Source (PCS) as FIRC --- */
    /* Option 1: SOSC, Option 2: SIRCDIV2, Option 3: FIRCDIV2, Option 6: SPLLDIV2 */
    /* Select PCS = 3 (FIRC - 48 MHz) */
    PCC->PCCn[PCC_LPUART1_INDEX] |= PCC_PCCn_PCS(3) | PCC_PCCn_CGC_MASK;

    /* Enable Clock for Port C (Rx/Tx pins) */
    PCC->PCCn[PCC_PORTC_INDEX] |= PCC_PCCn_CGC_MASK;
}

/* Configure Port C Pins for LPUART1 function */
static void init_uart01_port(void){
    /* PTC6 as RX (ALT2) */
    uint32_t u32_temp = PORTC->PCR[6];
    REG_FIELD_WRITE(u32_temp, PORT_PCR_MUX_MASK, PORT_PCR_MUX(UART_MUX_RX));
    PORTC->PCR[6] = u32_temp;

    /* PTC7 as TX (ALT2) */
    u32_temp = PORTC->PCR[7];
    REG_FIELD_WRITE(u32_temp, PORT_PCR_MUX_MASK, PORT_PCR_MUX(UART_MUX_TX));
    PORTC->PCR[7] = u32_temp;
}

/* Initialize LPUART1 Baudrate to 115200 */
static void init_baud_lpuart01(void){
    /* Disable Tx/Rx for configuration */
    LPUART1->CTRL &= ~(LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK);
    
    LPUART1->BAUD &= ~LPUART_BAUD_OSR_MASK;
    LPUART1->BAUD |= LPUART_BAUD_OSR(15);    /* OSR = 15 (Over Sampling 16x) */
    
    LPUART1->BAUD &= ~LPUART_BAUD_SBR_MASK;
    LPUART1->BAUD |= LPUART_BAUD_SBR(26);    /* SBR = 26 (Calculated for ~115200 baud) */
    
    /* Enable Tx/Rx */
    LPUART1->CTRL |= (LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK);
}

/* Initialize UART without Interrupts */
void uart01_init(void){
    init_firc_48MHz();
    init_uartclk_source();
    init_uart01_port();
    init_baud_lpuart01();
}

/* Configure LPUART1 Interrupts (NVIC) */
static void init_uart01_interrupt(void){
    LPUART1->CTRL |= LPUART_CTRL_RIE_MASK; /* Enable Receiver Interrupt */
    S32_NVIC->IP[33] = (8 << 4);           /* Set Priority */
    S32_NVIC->ISER[33 / 32] = (1 << (33 % 32)); /* Enable IRQ in NVIC */
}

/* Initialize UART with Interrupts enabled */
void uart01_init_with_interrupt(void){
    uart01_init();
    init_uart01_interrupt();
}

/* Send a single character via LPUART1 (Blocking) */
void lpuart01_send_char(const char c){
    /* Wait until Transmit Data Register Empty (TDRE) */
    while((LPUART1->STAT & (LPUART_STAT_TDRE_MASK)) == 0);
    LPUART1->DATA = c;
}

/* Send a string via LPUART1 (Blocking) */
void lpuart01_send_string(const char * str){
    while(*str != '\0'){
        lpuart01_send_char(*str);
        str++;
    }
}