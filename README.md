# S32K144 UART Bootloader (S-Record Parser)

A high-performance, reliable Bootloader for the **NXP S32K144** microcontroller. This project enables seamless firmware updates over **UART** using the industry-standard **S-Record (S19/S28/S37)** file format.

---

## 1. Key Features
- **S-Record Parsing**: Full support for S1 (16-bit), S2 (24-bit), and S3 (32-bit) address records.
- **Communication**: Optimized for **LPUART1** at a baudrate of **115200 bps**.
- **"No-Delay" Architecture**: Continuous stream processing using a circular buffer to ensure high-speed data handling without packet loss.
- **RAM Execution**: Critical sections (UART Interrupts, Queue logic, and Flash drivers) are executed from **SRAM** using `__attribute__((section(".code_ram")))` to prevent **Read-While-Write (RWW)** errors.
- **Data Integrity**: 
    - Per-line **Checksum** verification.
    - Automatic **8-byte (Phrase) Alignment** logic with `0xFF` padding for S32K144 Flash constraints.
    - **Flash Protection**: Specifically configured to safeguard the Flash Configuration Field (`0x400 - 0x410`).

---

## 2. System Architecture

### 2.1. Logic Flow
The system decouples data reception (Producer) from Flash programming (Consumer) to handle high-speed transfers.

`[ PC / Host ] --(UART)--> [ Circular Buffer (RAM) ] --(Pop)--> [ Flash Handler ] --(Write)--> [ Internal Flash ]`



### 2.2. Memory Map
| Region | Address Range | Description |
| :--- | :--- | :--- |
| **Bootloader** | `0x0000_0000 - 0x0000_3FFF` | Core Bootloader logic & Vector Table |
| **User Application** | `0x0000_4000 - End of Flash` | Application space |
| **SRAM** | `0x1FFF_8000 - 0x2000_6FFF` | ISR, Queue Logic, and RAM Functions |

---

## 3. Project Structure
- **`main.c`**: Entry point. Manages Vector Table relocation (`SCB->VTOR`), sector erasing, and the jump to Application.
- **`mock_queue.c/h`**: Thread-safe Circular FIFO Buffer implementation.
- **`mock_flashHandler.c/h`**: Decodes ASCII Hex to Binary, validates checksums, and handles 8-byte phrase alignment.
- **`mock_uartDrv.c/h`**: Low-level peripheral driver for LPUART1.
- **`clock_config.c/h`**: System clock configuration (48MHz FIRC).

---

## 4. Hardware Specifications
- **Microcontroller**: NXP S32K144 (ARM Cortex-M4F).
- **Interface**: UART (LPUART1).
- **Pin Mapping**: 
    - **Rx**: PTC6 (ALT2)
    - **Tx**: PTC7 (ALT2)
- **Settings**: 115200 bps, 8 Data bits, No Parity, 1 Stop bit.

---

## 5. Getting Started

### Step 1: Linker Script Setup
**For the Bootloader:**
Map the `.code_ram` section to SRAM in your `.ld` file:
```ld
.code_ram : {
    . = ALIGN(4);
    KEEP(*(.code_ram))
    *(.code_ram*)
    . = ALIGN(4);
} > m_data

For the User Application: Shift the memory origin to 0x4000 to avoid overwriting the bootloader:
Đoạn mã

MEMORY {
    m_interrupts (RX) : ORIGIN = 0x00004000, LENGTH = 0x00000400
    m_text       (RX) : ORIGIN = 0x00004400, LENGTH = 0x0007BC00
}

Step 2: Build & Flash

    Flash the Bootloader project to the S32K144EVB.

    Build your User Application to generate an .srec file.

    Open a Serial Terminal (e.g., TeraTerm, Hercules).

    Reset the board. When "READY TO DOWNLOAD" appears, send the .srec file.

6. Testing (/SREC_TEST folder)

The project includes pre-compiled .srec files for validation:

    BLINK_LED.srec: Standard test (Blinks Blue LED at 1Hz).

    BLINK_LED_TEST_ERROR.srec: Corrupted file to verify Checksum Error detection.

    LED_LIGHTNESS.srec: Advanced test for PWM-based LED control.
