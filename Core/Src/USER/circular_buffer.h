/* ********************************************* */
/* (c)2022 by www.vabolis.lt, Kaunas             */
/**
  **********************************************
  * @file           : circular_buffer.h
  * @version        : 1.0
  * @brief          : circular buffer for uart/usb
**/  
/* ********************************************* */
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H



// strukturis aprasas
struct circular_buf_t {
	uint8_t * buffer;
	size_t head;
	size_t tail;
	size_t count;
	size_t max; //of the buffer
};




//supaprastintas pavadinimas be kurio miliojonas warningu
typedef struct circular_buf_t CCBuf;

// prototipai
void circle_reset(CCBuf *c, size_t size);
void circle_push(CCBuf *c, unsigned char b);
unsigned char circle_pull(CCBuf *c);
size_t circle_available(CCBuf *c);
size_t circle_free(CCBuf *c);
void circle_push_buf(CCBuf *c, unsigned char *b, size_t len);


#endif
