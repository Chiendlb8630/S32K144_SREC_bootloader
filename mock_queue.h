#ifndef MOCK_QUEUE_H_
#define MOCK_QUEUE_H_

#include <stdint.h>
#include <stdbool.h>
#include "mcu_mock.h"

#define SREC_MAX_LEN    84U
#define QUEUE_CAPACITY  64U

typedef struct {
    char data[SREC_MAX_LEN];
    uint8_t length;
} Srec_line_t;

typedef struct {
    Srec_line_t buffer[QUEUE_CAPACITY];
    volatile uint32_t head;
    volatile uint32_t tail;
    uint32_t write_idx;
} Queue_t;

/*Dành cho ISR*/
void queue_init(Queue_t* q);
Srec_line_t* queue_get_write_buffer(Queue_t* q); // Lấy chỗ để ghi
void queue_push_char(Queue_t* q, char c);
void queue_commit_write(Queue_t* q);             // Ghi xong -> Nhích Tail

/* Dành cho Main */
Srec_line_t* queue_get_read_buffer(Queue_t* q);  // Lấy con trỏ data để đọc
void queue_release_read_buffer(Queue_t* q);      // Đọc xong -> Nhích Head

#endif


