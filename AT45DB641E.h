#ifndef AT45DB641E_H_
#define AT45DB641E_H_

#include <inttypes.h>

#define SetBit(x,y)		(x |= (y))
#define ClrBit(x,y)		(x &=~(y))
#define ChkBit(x,y)		(x  & (y))
//Dataflash opcodes
#define FlashPageRead			0x52	// Main memory page read
#define FlashToBuf1Transfer 	0x53	// Main memory page to buffer 1 transfer
#define Buf1Read				0x54	// Buffer 1 read
#define FlashToBuf2Transfer 	0x55	// Main memory page to buffer 2 transfer
#define Buf2Read				0x56	// Buffer 2 read
#define StatusReg				0x57	// Status register
#define AutoPageReWrBuf1		0x58	// Auto page rewrite through buffer 1
#define AutoPageReWrBuf2		0x59	// Auto page rewrite through buffer 2
#define FlashToBuf1Compare    	0x60	// Main memory page to buffer 1 compare
#define FlashToBuf2Compare	    0x61	// Main memory page to buffer 2 compare
#define ContArrayRead			0x68	// Continuous Array Read (Note : Only A/B-parts supported)
#define FlashProgBuf1			0x82	// Main memory page program through buffer 1
#define Buf1ToFlashWE   		0x83	// Buffer 1 to main memory page program with built-in erase
#define Buf1Write				0x84	// Buffer 1 write
#define FlashProgBuf2			0x85	// Main memory page program through buffer 2
#define Buf2ToFlashWE   		0x86	// Buffer 2 to main memory page program with built-in erase
#define Buf2Write				0x87	// Buffer 2 write
#define Buf1ToFlash     		0x88	// Buffer 1 to main memory page program without built-in erase
#define Buf2ToFlash		        0x89	// Buffer 2 to main memory page program without built-in erase
#define PageErase               0x81	// Page erase, added by Martin Thomas

#define DATAFLASH_READ_MANUFACTURER_AND_DEVICE_ID 0x9F

#define BUFFER_ID_1				1
#define BUFFER_ID_2				2

typedef struct __attribute__ ((__packed__))
{
	uint8_t manufacturer;       /**< Manufacturer id **/
	uint8_t device[2];          /**< Device id **/
	uint8_t extendedInfoLength; /**< Extended device information string length **/
	
} dataflashID_t;

void dataflash_readID(void);
void dataflash_readSizeSpecification(void);
unsigned char dataflash_readStatus(void);

void dataflash_begin(void);
void dataflash_pageToBuffer (unsigned int PageAdr, unsigned char BufferNo);
unsigned char dataflash_bufferReadByte (unsigned char BufferNo, unsigned int IntPageAdr);
void dataflash_bufferReadStr (unsigned char BufferNo, unsigned int IntPageAdr, unsigned int No_of_bytes, char *BufferPtr);
void dataflash_bufferWriteEnable (unsigned char BufferNo, unsigned int IntPageAdr);
void dataflash_bufferWriteByte (unsigned char BufferNo, unsigned int IntPageAdr, unsigned char Data);
void dataflash_bufferWriteStr (unsigned char BufferNo, unsigned int IntPageAdr, unsigned int No_of_bytes, char *BufferPtr);
void dataflash_bufferToPage (unsigned char BufferNo, unsigned int PageAdr);
void dataflash_contFlashReadEnable (unsigned int PageAdr, unsigned int IntPageAdr);
//#ifdef MTEXTRAS
void dataflash_pageErase (unsigned int PageAdr); // added by mthomas
unsigned char dataflash_pageBufferCompare(unsigned char BufferNo, unsigned int PageAdr); // added by mthomas
void dataflash_deselect();
void dataflash_select();
int dataflash_readBytes(uint32_t address, char *data, int length);
int dataflash_writeBytes(uint32_t address, char *data, uint32_t length);

#endif /* AT45DB641E_H_ */
