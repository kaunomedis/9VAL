/* ********************************************* */
/* (c)2022 by www.vabolis.lt, Kaunas             */
/**
  **********************************************
  * @file           : circular_buffer.c
  * @version        : 1.0
  * @brief          : circular buffer for uart/usb
**/  
/* ********************************************* */

#include "circular_buffer.h"

/* INSTRUKCIJA
#define BUFFER_SIZE 32

unsigned char testas[BUFFER_SIZE];
CCBuf cc; //struktura
//init
cc.buffer=testas;
circle_reset(&cc,BUFFER_SIZE);

//naudojimas

while(circle_available(&cc)>0) {a=circle_pull(&cc); naudoti duomenys}
					
// uart callback
circle_push(&cc , baitinisbuferis); // push received byte to circular buffer
					

************* */



void circle_reset(CCBuf *c, size_t size)
{
	c->head=0;
	c->tail=0;
	c->max=size; //sizeof(c->buffer); <-- negauna sitos informacijos
	c->count=0;
}

void circle_push(CCBuf *c,unsigned char b)
{
	c->buffer[c->head]=b;
	c->head++; if(c->head == c->max) {c->head=0;}
	c->count++;
	//if(c->count == c->max) {c->count=c->max;} //error
}

unsigned char circle_pull(CCBuf *c)
{
unsigned char b;
	b=c->buffer[c->tail];
	c->tail++;
	if(c->tail == c->max) {c->tail=0;}
	c->count--;
	//if(c->count == c->max){c->count=0;} //error
return b;
}

void circle_push_buf(CCBuf *c, unsigned char *b, size_t len)
{
size_t i;
for (i=0;i<len;i++){circle_push(c,b[i]);}
}

size_t circle_available(CCBuf *c)
{
return c->count;
}
size_t circle_free(CCBuf *c)
{
return c->max - c->count;
}




