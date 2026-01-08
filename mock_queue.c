#include "mcu_mock.h"
#include <stddef.h>

/* Calculates the next index in the Circular Buffer */
RAM_FUNC static inline uint32_t next_idx(uint32_t idx) {
    return (idx + 1) % QUEUE_CAPACITY;
}

/* Gets pointer to the current write buffer at Tail (returns NULL if queue is full) */
RAM_FUNC Srec_line_t* queue_get_current_write_buffer(Queue_t* q) {
    if (next_idx(q->tail) == q->head) {
        return NULL;
    }
    return &q->buffer[q->tail];
}

/* Commits a fully written line and advances the Tail index */
RAM_FUNC void queue_commit_write(Queue_t* q) {
    /* Only commit if not full */
    if (next_idx(q->tail) != q->head) {
        q->tail = next_idx(q->tail);
        q->write_idx = 0;
    }
}

/* Pushes char to temp buffer and commits when newline (\r or \n) is detected */
RAM_FUNC void queue_push_char(Queue_t* q, char c) {
    Srec_line_t* current_line = queue_get_current_write_buffer(q);
    if (current_line == NULL) {
        return;
    }
    if (q->write_idx >= SREC_MAX_LEN - 1) {
        q->write_idx = 0;
        return;
    }
    if (c == '\n' || c == '\r') {
        if (q->write_idx > 0) {
            current_line->data[q->write_idx] = '\0';
            current_line->length = q->write_idx;
            queue_commit_write(q);
        }
    }
    else {
        current_line->data[q->write_idx] = c;
        q->write_idx++;
    }
}

/* Initializes queue variables (Head, Tail, WriteIndex) */
void queue_init(Queue_t* q) {
    q->head = 0;
    q->tail = 0;
    q->write_idx = 0;
}

/* Gets pointer to data at Head for processing (returns NULL if queue is empty) */
Srec_line_t* queue_get_read_buffer(Queue_t* q) {
    if (q->head == q->tail) {
        return NULL;
    }
    return &q->buffer[q->head];
}

/* Releases the buffer at Head after data processing is done */
void queue_release_read_buffer(Queue_t* q) {
    if (q->head != q->tail) {
        q->head = next_idx(q->head);
    }
}

