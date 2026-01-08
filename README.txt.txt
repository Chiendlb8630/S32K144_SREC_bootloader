================================================================================
DỰ ÁN : BOOTLOADER UART CHO S32K144 (S-RECORD PARSER)
ChienKD1- Khương Đình Chiến.
FPT Software Academy - MCU_Fresher_Mock Project
================================================================================
1. TỔNG QUAN 
------------------
Project này xây dựng một Bootloader cho vi điều khiển NXP S32K144. 
Hệ thống cho phép cập nhật firmware thông qua giao tiếp UART sử dụng định dạng file S-Record (S19/S28/S37).

Các tính năng nổi bật:
- Giao thức: Phân tích file S-Record (Hỗ trợ S1, S2, S3).
- Giao tiếp: LPUART1 (Baudrate 115200 bps).
- Hiệu năng cao: Kiến trúc xử lý luồng liên tục "No-Delay" .
- Ổn định: Ngăn chặn lỗi Read-While-Write (RWW) bằng cơ chế thực thi trên RAM.
- Tin cậy: Xử lý logic căn chỉnh bộ nhớ Flash và xác thực Checksum từng dòng.

2. KIẾN TRÚC HỆ THỐNG
---------------------
[ PC ] --(UART)--> [ Bộ Đệm Vòng (RAM) ] --(Pop)--> [ Trình Ghi Flash ] --(Ghi)--> [ Internal Flash ]

2.1. Bản đồ bộ nhớ (Memory Map)
   - 0x0000_0000 - 0x0000_3FFF : Bootloader 
   - 0x0000_4000 - Hết Flash   : User Application (Ứng dụng người dùng)
   - 0x1FFF_XXXX               : SRAM (Chứa ISR & Queue Logic trong khi nạp Flash)

2.2. Các quyết định thiết kế quan trọng
   - Thực thi trên RAM (RAM Execution): Toàn bộ các đoạn mã quan trọng (Ngắt UART, Queue Push/Pop) 
     được đưa xuống chạy dưới SRAM (.code_ram) bằng thuộc tính `__attribute__((section(".code_ram")))`. 
   
   - Bộ đệm vòng (Circular Buffer): Sử dụng hàng đợi giúp tách biệt tốc độ nhận 
     của UART (Producer) và tốc độ ghi của Flash (Consumer), đảm bảo không mất dữ liệu ở baudrate cao.

3. MÔ TẢ CÁC MODULE
-------------------
A. CORE LOGIC
----------------------------
1. main.c
   - Điểm bắt đầu của Bootloader.
   - Thực hiện di dời Vector Table (SCB->VTOR) xuống RAM.
   - Xóa các Sector vùng Application trước khi nhận dữ liệu mới.
   - Quản lý vòng lặp nạp chính và thực hiện nhảy (Jump) sang Application khi thành công.

2. mock_queue.c / mock_queue.h
   - Cài đặt cấu trúc dữ liệu Hàng đợi vòng (Circular FIFO Buffer) để lưu trữ các dòng S-Record đến.
   - Tất cả hàm tại đây được gắn nhãn `RAM_FUNC` để chạy trên SRAM.
   - Xử lý bảo vệ tràn bộ đệm (Overflow) và phát hiện ký tự kết thúc dòng (\n hoặc \r).

3. mock_flashHandler.c / mock_flashHandler.h
   - S-Record Parser: Giải mã loại S-Type, Độ dài, Địa chỉ và Dữ liệu từ chuỗi Hex ASCII.
   - Checksum Verification: Xác thực tính toàn vẹn dữ liệu cho từng dòng.
   - Logic Căn chỉnh (Alignment): Xử lý yêu cầu căn chỉnh 8-byte (Phrase) của Flash S32K144. 
     Thuật toán tự động phát hiện địa chỉ lẻ, tính toán độ lệch (offset) và chèn đệm (padding) 0xFF.
   - Flash Protection: Bảo vệ vùng cấu hình Flash (Flash Configuration Field: 0x400 - 0x410).

4. mock_uartDrv.c / mock_uartDrv.h
   - Driver cấp thấp cho LPUART1.
   - Cấu hình Baudrate (115200), và Pin Muxing (ALT2).
   - Cung cấp các hàm gửi để báo cáo trạng thái nạp.

B. CẤU HÌNH & DRIVER (Lớp BSP)
------------------------------
1. clock_config.c / clock_config.h
   - Cấu hình bộ tạo xung nhịp hệ thống (SCG).
   - System Core Clock: 48 MHz (Từ nguồn FIRC).
   - Bus Clock: 48 MHz.
   - Flash Clock: 24 MHz.

2. peripherals_flash_1.c / peripherals_flash_1.h
   - Cấu hình bộ điều khiển Flash C40 (FTFC) thông qua S32 SDK.
   - Thiết lập địa chỉ cơ sở và kích thước cho PFlash.

3. mcu_mock.h
   - File header trung tâm, tổng hợp tất cả các module.
   - Định nghĩa các macro đặc thù nền tảng như `RAM_FUNC` cho trình biên dịch GCC.
   - Định nghĩa địa chỉ bắt đầu của ứng dụng (`APP_START_ADDR`).

4. ĐẶC TẢ PHẦN CỨNG
-------------------
- Vi điều khiển: NXP S32K144 (ARM Cortex-M4F).
- Kết nối UART:
    - Rx: PTC6 (Port C, Pin 6)
    - Tx: PTC7 (Port C, Pin 7)
    - Tốc độ: 115200 bps
    - Parity: None, Stop bits: 1.

5. HƯỚNG DẪN BIÊN DỊCH & SỬ DỤNG
--------------------------------
1. Cấu hình Linker Script (.ld):
   A. Đối với BOOTLOADER :
      * Cần khai báo section `.code_ram` để chạy các hàm quan trọng trên RAM.
      * Thêm vào khối `SECTIONS` trong file .ld:
        .code_ram :
        {
          . = ALIGN(4);
          KEEP(*(.code_ram))
          *(.code_ram*)
          . = ALIGN(4);
        } > m_data  /* Đẩy code xuống RAM */

   B. Đối với USER APPLICATION Project (Ứng dụng cần nạp):
      * Cần sửa file Linker Script (.ld) để ánh xạ vùng nhớ phù hợp, tránh ghi đè lên Bootloader và vùng Flash Configuration.
      * Thay thế khối MEMORY bằng cấu hình sau:
        MEMORY
        {
          /* 1. Bảng Vector Table của App bắt đầu tại 0x4000 */
          m_interrupts          (RX)  : ORIGIN = 0x00004000, LENGTH = 0x00000400
          
          /* 2. Bỏ qua m_flash_config (0x400) vì vùng này thuộc về Bootloader */
          
          /* 3. Code của App bắt đầu ngay sau bảng Vector Table */
          /* Địa chỉ 0x4400 = 0x4000 + 0x400 */
          m_text                (RX)  : ORIGIN = 0x00004400, LENGTH = 0x0007BC00 
          
          /* 4. Các vùng RAM giữ nguyên không đổi */
          m_data                (RW)  : ORIGIN = 0x1FFF8000, LENGTH = 0x00008000
          m_data_2              (RW)  : ORIGIN = 0x20000000, LENGTH = 0x00007000
        }
2. Thêm Flash Component (Bắt buộc):
   Để project có thể sử dụng các hàm `FLASH_DRV_*`, bạn cần thêm driver thông qua công cụ cấu hình của S32DS:
   - Bước 1: Mở cấu hình configtoool.
   - Bước 2: Chọn tab **Peripherals**.
   - Bước 3: Trong danh sách "Components" (bên trái), tìm và nhấn dấu **(+)** tại mục **Flash (flash_c40)**.
   - Bước 4: Giữ nguyên các cấu hình mặc định của component.
   - Bước 5: Nhấn nút **Update Code** (trên thanh công cụ) để IDE tự động sinh ra file driver SDK.

3. Quy trình Build & Nạp:
   - Bước 1: Build và nạp Bootloader xuống mạch bằng Debug.
   - Bước 2: Build User Application (với file .ld đã sửa ở trên) -> Tạo file S-Record
   - Bước 3: Reset mạch để Bootloader chạy.
   - Bước 4: Dùng Terminal gửi file S-Record của Application qua UART.
   - Bước 5: Quan sát terminal xem trạng thái của chương trình
4. Các lỗi debug :
   - "Counter Error": PC gửi dữ liệu quá nhanh.
   - "CRC Error": Dữ liệu bị nhiễu trên đường truyền.
   - "TIME OUT ERROR" Hệ thống treo khi không nhận được dòng srec nào.
   - App không chạy sau khi Jump: Kiểm tra chắc chắn rằng `m_interrupts` đã được đặt tại `0x4000` và `VTOR` trong Bootloader trỏ đúng tới địa chỉ này.

5. BỘ KIỂM THỬ 
--------------------------------
Thư mục `SREC_TEST` chứa các file firmware mẫu (định dạng .srec) được thiết kế để kiểm tra các kịch bản hoạt động khác nhau của Bootloader.
6.1: BLINK_LED
       * Ứng dụng nháy LED Xanh (PTD16) chu kỳ 1 giây.
       * Kết quả mong đợi: Terminal báo "Download SUCCESS!", sau đó LED trên mạch nháy đều.
     + BLINK_LED_TEST_ERROR.srec:
       * File này đã bị cố tình sửa dữ liệu.
       * Kết quả mong đợi: Bootloader phát hiện lỗi, Terminal báo "Error: Checksum verification failed!" và dừng nạp.
6.2: LED_CHANGE (Thay đổi trạng thái)
       * Ứng dụng nháy LED với tần số nhanh hơn (hoặc kiểu nháy khác biệt so với BLINK_LED).
       * Kết quả mong đợi: LED thay đổi tốc độ nháy 
6.3: LED_LIGHTNESS (Độ sáng/PWM)
       - Ứng dụng điều khiển độ sáng LED.
       * "Download SUCCESS!", LED sáng mờ hoặc thay đổi độ sáng khi bấm nút.
     + LED_LIGHTNESS_TEST_ERROR.srec:
       * Kết quả mong đợi: Terminal báo lỗi CRC.
6.4. Hướng dẫn chạy Test
       1. Reset mạch S32K144.
       2. Chờ thông báo "READY TO DOWNLOAD".
       3. Gửi file ".srec" tương ứng qua Terminal (Send Text File).
       4. Quan sát log trên Terminal và trạng thái đèn LED trên mạch( nếu có lỗi bấm reset và nạp lại file khác).