/* *****************************************************************************
* File Name          : ssd1306_oled_i2c.c
* Author             : www.vabolis.lt + kmm CHR(0x40) rmlabs.net
* Version            : V1.2
* Date               : 2020
* Description        : OLED I2C display module from China
********************************************************************************
* converterted STM32F MCU, compatible with STM32CubeMX, HAL, gcc
****************************************************************************** */
#include <stdlib.h>
#include <string.h>


/* IC2 handlers from CubeMX */
extern I2C_HandleTypeDef hi2c2;
#define HI2C &hi2c2

/* some includes */
#include "ssd1306_oled.h" // configuration number from datasheet
#include "font_c64_lower.h" // Commodore 64 font


/* Configure display: size, voltage, I2C address */
#define SSD1306_ADR 0x78>>1
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 32


#define SSD1306_VCS SSD1306_SWITCHCAPVCC //all China made modules use internal Vcc


  #if SSD1306_HEIGHT == 64
    #define SSD_MAGIC 7
  #endif
  #if SSD1306_HEIGHT == 32
    #define SSD_MAGIC 3 // Page end address
  #endif
  #if SSD1306_HEIGHT == 16
    #define SSD_MAGIC 1 // Page end address
  #endif





#if SSD1306_WIDTH == 128 && SSD1306_HEIGHT == 32
    #define SSD_COMPINS 0x02
	#define SSD_CONTRAST 0x8F
#elif SSD1306_WIDTH == 128 && SSD1306_HEIGHT == 64
	#define SSD_COMPINS 0x12
		#if SSD1306_VCS == SSD1306_EXTERNALVCC
			#define SSD_CONTRAST 0x9F
		#else
			#define SSD_CONTRAST 0xCF
		#endif
#elif SSD1306_WIDTH == 96 && SSD1306_HEIGHT == 16
	#define SSD_COMPINS 0x02
		#if SSD1306_VCS == SSD1306_EXTERNALVCC
			#define SSD_CONTRAST 0x10
		#else
			#define SSD_CONTRAST 0xAF
		#endif
#else
	#error "nezinomas ekrnas"
#endif


#if SSD1306_VCS == SSD1306_EXTERNALVCC
	#define CHRPUMP 0x10
	#define PRECHARGE 0x22
#else
	#define CHRPUMP 0x14
	#define PRECHARGE 0xF1
#endif






/* some crazy stuff from kmm */
#define SSD1306_swap(a, b)   (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation 


/* ******** GLOBALS ****************** */
unsigned char oled_line=0; //holds current text line, used from autoscroll
unsigned char getRotation(void) { return 0;} //for comaptibility

/* ********************* I2C blokai = I2C modules ****************** */

/* send command via i2c to ssd1306. first byte is command/data flag. for I2C only */
void SSD1306_command1(uint8_t c)
{
unsigned char a[2];
	a[0]=0; // DC=0, CO=0
	a[1]=c;
	HAL_I2C_Master_Transmit(HI2C, SSD1306_ADR<<1, a, 2,SSD_MAX_DELAY);
}

/* transmit buffer to device using HAL. First byte in buffer is command/data flag */
void SSD1306_sendbuffer(uint8_t *c, uint8_t n)
{
	HAL_I2C_Master_Transmit(HI2C, SSD1306_ADR<<1, c, n,SSD_MAX_DELAY);
}

/* for compatibility with kmm software. Write data or command to i2c device */
void SSD1306_write(unsigned char dc, unsigned char data)
{
unsigned char a[2];
/* copypaste from datasheet:
A control byte mainly consists of Co and D/C# bits following by six “0” ‘s.
a. If the Co bit is set as logic “0”, the transmission of the following information will contain
data bytes only.
b. The D/C# bit determines the next data byte is acted as a command or a data. If the D/C# bit is
set to logic “0”, it defines the following data byte as a command. If the D/C# bit is set to
logic “1”, it defines the following data byte as a data which will be stored at the GDDRAM.
The GDDRAM column address pointer will be increased by one automatically after each
data write.
*/
	if(dc) {a[0]=0b01000000;} else {a[0]=0;} // ar abu bitai 1?
	a[1]=data;
	HAL_I2C_Master_Transmit(HI2C, SSD1306_ADR<<1, a, 2,SSD_MAX_DELAY);
}
static char SSD1306_present(void)
{
	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady (HI2C, SSD1306_ADR<<1, 1,SSD_MAX_DELAY);
	if(status==HAL_OK) {return 1;}else{return 0;} 
}
/* ********* no more STM stuff bellow this line ****************************** */


/* Init OLED screen */



char SSD1306_Init(void)
{
SSD1306_command1(SSD1306_DISPLAYOFF);
HAL_Delay(2000); //test on hardware
if(SSD1306_present()==0) {return 0;}

// Configurations are send to device in chunks of data. It is faster and smaller.
// For final products (with static configuration) it is possible to set all config
// in one array and send all data in one block.

static unsigned char init1[]= {0x00, // first byte command !
	SSD1306_DISPLAYOFF,
	SSD1306_SETDISPLAYCLOCKDIV,	0x80, //7:4 bitai-freq (default 0b1000). 3:0-div(1-16)
	SSD1306_SETMULTIPLEX, SSD1306_HEIGHT-1,  //
	SSD1306_SETDISPLAYOFFSET,             // 0xD3
	0x0,                                  // no offset
	SSD1306_SETSTARTLINE | 0x0,           // line #0
	SSD1306_CHARGEPUMP,CHRPUMP,           // 0x8D

	SSD1306_MEMORYMODE,                   // 0x20
	0x00,                                 // 0x0 act like ks0108
	SSD1306_SEGREMAP | 0x1,
	SSD1306_COMSCANDEC,

	SSD1306_SETCOMPINS,SSD_COMPINS,
	SSD1306_SETCONTRAST,SSD_CONTRAST,
	SSD1306_SETPRECHARGE,PRECHARGE, //((vccstate == SSD1306_EXTERNALVCC) ? 0x22 : 0xF1)};

	SSD1306_SETVCOMDETECT,               // 0xDB
	0x40,
	SSD1306_DISPLAYALLON_RESUME,         // 0xA4
	SSD1306_NORMALDISPLAY,               // 0xA6
	SSD1306_DEACTIVATE_SCROLL,
	SSD1306_DISPLAYON };                 // Main screen turn on
	SSD1306_sendbuffer(init1, sizeof(init1));

return 1;
}

/* small subroutines to make world brighter */
void SSD1306_off(void){SSD1306_command1(SSD1306_DISPLAYOFF);}
void SSD1306_on(void){SSD1306_command1(SSD1306_DISPLAYON);}
void SSD1306_invert(void){SSD1306_command1(SSD1306_INVERTDISPLAY);}
void SSD1306_normal(void){SSD1306_command1(SSD1306_NORMALDISPLAY);}
void SSD1306_flip(unsigned char m){SSD1306_command1((m==0)? SSD1306_COMSCANINC : SSD1306_COMSCANDEC);}

void SSD1306_set_addr_mode(uint8_t mode) {
	// 0 -> horizontal (write column, increment column pointer, at column overrun reset column pointer and increment page pointer)
	// 1 -> vertical (write column, increment page pointer, at page overrun reset page pointer and increment column pointer)
	// 2 -> page (write column, increment column pointer, reset column pointer at overrun)
	unsigned char data[]={0,SSD1306_MEMORYMODE,((mode < 2) ? mode : 0)};
	SSD1306_sendbuffer(data, sizeof(data));	
}
void SSD1306_scroll(unsigned char n)
{
	unsigned char data[]={0,SSD1306_SETDISPLAYOFFSET,n & 0b01111111};
	SSD1306_sendbuffer(data, sizeof(data));
}
/* send buffer to device. We need to add first "command/data" byte in the buffer.
   This gives us all rush with allocation, copying and flushing memory. */
void SSD1306_put_tile(uint8_t *tile, uint8_t limit) {
	unsigned char *tempbuf=(unsigned char *) malloc(limit+1);
	memcpy(tempbuf+1,tile,limit); tempbuf[0]=0b01000000; //data mode
	SSD1306_sendbuffer(tempbuf,limit+1);
	free(tempbuf);
}

/* print char (raw mode)-according font table */
void SSD1306_putc_raw(char c) {
	SSD1306_put_tile(font+(c<<3),8);
}

/* print char from ASCII */
void SSD1306_putc(char c) {
	// remap from petscii to ascii, shifts drawing characters into the lower 32 ascii cells
	if(c > 'A' && c < 'Z') { }               // upper-case ascii range
	else if(c > 'a' && c < 'z') { c -= 96; } // lower-case ascii range
	else if(c > 31 && c < 64) { }            // numbers and symbols
	else if(c < 32) { c += 96; }             // low ascii
	SSD1306_putc_raw(c);
}

/* move to position in pixels/bytes */
void SSD1306_move_raw(uint8_t row, uint8_t col) {
	if(col > 127) { col = 127; }
	if(row > 7) { row = 7; }
	unsigned char data[]={0,SSD1306_COLUMNADDR,col,0x7F,SSD1306_PAGEADDR,row,SSD_MAGIC};
	SSD1306_sendbuffer(data, sizeof(data));
}

/* move to position in char positions */
void SSD1306_move(uint8_t row, uint8_t col) {
	if(col > 15) { col = 15; }
	if(row > 7) { row = 7; }	
	SSD1306_move_raw(row, col << 3);
}

/* move to "home" positions. Just make it easier to type */
void SSD1306_home() {SSD1306_move_raw(0,0);}

/* clear screen/fill screen with byte pattern */
void SSD1306_clear(unsigned char c) {
	SSD1306_home();
	// just fill 128*64 pixels = 8192 bits = 1024bytes =1kb.
	// I do it in 8 bytes blocks= 128 times.
unsigned char tmp[]={0b01000000,c,c,c,c,c,c,c,c};
	for(uint8_t i = 128; i > 0; i--) {SSD1306_sendbuffer(tmp, sizeof(tmp));}
	SSD1306_scroll(0);
	oled_line=0;
}

/* I don't know what this is doint. Ask kmm */
void SSD1306_fill(uint8_t row, uint8_t col, uint8_t count, uint8_t max, uint32_t pattern, int8_t pshift) {
	SSD1306_move(row, col);
	uint8_t pstate = 0;
	unsigned char i;
	for(i=max; i > 0; i--) {
		if(count > max) {
			if(pshift < 0) {
				SSD1306_write(1, (pattern >> (pstate++ << 3)) & 0xFF);
			} 
			else 
			{
				SSD1306_write(1, (pattern >> (pstate++ + pshift)) & 0xFF);
			}			
			pstate = (pstate > 3) ? 0 : pstate;
		}
		else {
			SSD1306_write(1, 0x00);
		}		
	}
}


// Draw a tile at an arbitrary pixel location (top, left) using an 8 byte tile buffer referenced by *tile.
// Slower than oled_putc(), potentially substantially so; only use for things that need
// finer grained positioning than is possible with tile cells, like sprites.
// Clips right and bottom edges properly; untested and not expected to work with negative positions.
void SSD1306_putxy(uint8_t left_pxl, uint8_t top_pxl, uint8_t *tile) {
	uint8_t tbuf[8], obuf[8];
	uint8_t top_cell = top_pxl >> 3;
	uint8_t left_cell = left_pxl >> 3;
	int8_t voff = top_pxl - ((top_cell << 3) - 1);
	int8_t hoff = left_pxl - ((left_cell << 3) - 1);
	
	if(voff == 0 && hoff == 0) {
		SSD1306_move(top_cell, left_pxl >> 3);
		SSD1306_put_tile(tile, 8);
		return;
	}
	else {
		for(uint8_t tcol = 0; tcol < 8; tcol++) { // tile column
			tbuf[tcol] = (tile[tcol]) << ((uint8_t)voff); // shift left (down) by voff
			obuf[tcol] = (tile[tcol]) >> (8 - (uint8_t)voff); //shift right (up) by voff
		}
		
		SSD1306_move_raw(top_cell, left_pxl); // move_raw(row[0:7], column[0:127]) rows and pixels for extra confusion
		SSD1306_put_tile(tbuf, (left_pxl > (SSD1306_WIDTH - 8)) ? 8 - hoff : 8);
		if(top_pxl < (SSD1306_HEIGHT - 8)) {
			SSD1306_move_raw((top_cell + 1), left_pxl);
			SSD1306_put_tile(obuf, (left_pxl > (SSD1306_WIDTH - 8)) ? 8 - hoff : 8);
		}		
	}	
}

/* print string at current position */
void SSD1306_puts(char *str) {
	while(*str != 0) {
		SSD1306_putc(*str++);
	}
}

// print bigger digits using Commodore 64 symbols.
// box graphics digit (a single digit, not byte or word; use this to render output of an int->bcd conversion etc)
	const uint8_t chartable[]  = { 0x10, 0x0E, 0x5D, 0x5D, 0x0D, 0x1D, // zero
							              0x20, 0x0E, 0x20, 0x5D, 0x20, 0x11, // one
		                                  0x10, 0x0E, 0x10, 0x1D, 0x0D, 0x1D, // two
							              0x10, 0x0E, 0x20, 0x13, 0x0D, 0x1D, // three
							              0x5F, 0x5F, 0x0D, 0x13, 0x20, 0x5E, // four
							              0x10, 0x0E, 0x0D, 0x0E, 0x0D, 0x1D, // five
							              0x10, 0x0E, 0x0B, 0x0E, 0x0D, 0x1D, // six
							              0x10, 0x0E, 0x20, 0x5B, 0x20, 0x5E, // seven
							              0x10, 0x0E, 0x0B, 0x13, 0x0D, 0x1D, // eight
							              0x10, 0x0E, 0x0d, 0x13, 0x0D, 0x1D  // nine
						                };
void SSD1306_bigdigit(uint8_t top, uint8_t left, uint8_t num) {
	if(num > 9) { return; }
	SSD1306_move(top, left);
	for(uint8_t i = 0; i < 6; i++) {
		if(i == 2 || i == 4) { SSD1306_move(++top, left); }
		SSD1306_putc(chartable[(num * 6) + i]);
	}		
}

/* draw box from PET symbols */
void SSD1306_box(uint8_t top, uint8_t left, uint8_t width, uint8_t height) {
	SSD1306_move(top, left);
	SSD1306_putc(BOX_TL); for(uint8_t i = 0; i < width - 2; i++) { SSD1306_putc(BOX_HL);} SSD1306_putc(BOX_TR);
	for(uint8_t i = top+1; i < top+height-1; i++) {
		SSD1306_move(i, left); 
		SSD1306_putc(BOX_VL); 
		SSD1306_move(i, left + width - 1);
		SSD1306_putc(BOX_VL);
	}
	SSD1306_move(top + height - 1, left);
	SSD1306_putc(BOX_BL); for(uint8_t i = 0; i < width - 2; i++) { SSD1306_putc(BOX_HL);} SSD1306_putc(BOX_BR);		
}

/* 
"I tell you, we are here on Earth to fart around, and don't let anybody tell you different." - Kurt Vonnegut
*/

/* www.vabolis.lt adds */
/* scroll stuff up and print on bottom of the screen. "listing" emulation */ 
void SSD1306_scroll_print(char *n)
{
unsigned char tmp;
	SSD1306_scroll(oled_line*8);
	if(oled_line==0){tmp=7;} else {tmp=oled_line-1;}
	SSD1306_move(tmp, 0);
	SSD1306_puts(n);
	oled_line++; if(oled_line>7)oled_line=0;
}

void SSD1306_stopscroll(void){
  SSD1306_command1(SSD1306_DEACTIVATE_SCROLL);
}


// Dim the display
// dim = true: display is dimmed
// dim = false: display is normal
void SSD1306_dim(unsigned char dim) {
  uint8_t contrast;

  if (dim) {
			contrast = 0; // Dimmed display
			} else {
//				if (_vccstate == SSD1306_EXTERNALVCC) {
//						contrast = 0x9F;
//					} else {
			contrast = 0xCF;
//    }
  }
  // the range of contrast to too small to be really useful
  // it is useful to dim the display
  SSD1306_command1(SSD1306_SETCONTRAST);
  SSD1306_command1(contrast);
}


// startscrollright
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void SSD1306_startscrollright(uint8_t start, uint8_t stop){
  SSD1306_command1(SSD1306_RIGHT_HORIZONTAL_SCROLL);
  SSD1306_command1(0X00);
  SSD1306_command1(start);
  SSD1306_command1(0X00);
  SSD1306_command1(stop);
  SSD1306_command1(0X00);
  SSD1306_command1(0XFF);
  SSD1306_command1(SSD1306_ACTIVATE_SCROLL);
}

// startscrollleft
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void SSD1306_startscrollleft(uint8_t start, uint8_t stop){
  SSD1306_command1(SSD1306_LEFT_HORIZONTAL_SCROLL);
  SSD1306_command1(0X00);
  SSD1306_command1(start);
  SSD1306_command1(0X00);
  SSD1306_command1(stop);
  SSD1306_command1(0X00);
  SSD1306_command1(0XFF);
  SSD1306_command1(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagright
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void SSD1306_startscrolldiagright(uint8_t start, uint8_t stop){
  SSD1306_command1(SSD1306_SET_VERTICAL_SCROLL_AREA);
  SSD1306_command1(0X00);
  SSD1306_command1(SSD1306_HEIGHT);
  SSD1306_command1(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
  SSD1306_command1(0X00);
  SSD1306_command1(start);
  SSD1306_command1(0X00);
  SSD1306_command1(stop);
  SSD1306_command1(0X01);
  SSD1306_command1(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagleft
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void SSD1306_startscrolldiagleft(uint8_t start, uint8_t stop){
  SSD1306_command1(SSD1306_SET_VERTICAL_SCROLL_AREA);
  SSD1306_command1(0X00);
  SSD1306_command1(SSD1306_HEIGHT);
  SSD1306_command1(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
  SSD1306_command1(0X00);
  SSD1306_command1(start);
  SSD1306_command1(0X00);
  SSD1306_command1(stop);
  SSD1306_command1(0X01);
  SSD1306_command1(SSD1306_ACTIVATE_SCROLL);
}



/* programinis dugnas - this must be last line in file */
