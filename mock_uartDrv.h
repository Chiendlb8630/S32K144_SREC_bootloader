
#ifndef MOCK_UARTDRV_H_
#define MOCK_UARTDRV_H_

#include "S32K144.h"
#include <stdint.h>
#include <stddef.h>

#define REG_FIELD_WRITE(REG, MASK, VALUE)  ((REG) = ((REG) & ~(MASK)) | (VALUE))
#define UART_MUX_RX							2U
#define UART_MUX_TX							2U

void uart01_init(void);
void lpuart01_send_string(const char * str);
void lpuart01_send_char(const char c);
void uart01_init_with_interrupt(void);


#endif /* MOCK_UARTDRV_H_ */

