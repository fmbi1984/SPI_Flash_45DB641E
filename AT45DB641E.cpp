#include "AT45DB641E.h"

#include "hardware.h"
#include <SPI_Master.h>

//opcodes
#define WREN  6
#define WRDI  4
#define RDSR  5
#define WRSR  1
#define READ  3
#define WRITE 2

#define DATAFLASH_FLASH_SIZE (1024UL*100UL) //100KB

#define DATAFLASH_PAGE_SIZE  264 // NEW PROTOTYPE
//#define DATAFLASH_PAGE_SIZE  1024 // OLD PROTOTYPE

#define DATAFLASH_BLOCK_SIZE 2048

dataflashID_t dataflashID;

static unsigned char PageBits = 0;
static unsigned int  PageSize = 0;

const uint8_t DF_pagebits[] PROGMEM ={9,  9,  9,  9,  9,  10,  10,  11};
const uint16_t DF_pagesize[] PROGMEM ={264, 264, 264, 264, 264, 528, 528,1056};
                      
void dataflash_begin(void)
{
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  pinMode(SS, OUTPUT);
  pinMode(CS_FLASH, OUTPUT);
  digitalWrite(SS, HIGH); //disable device
  digitalWrite(CS_FLASH, HIGH);
  
  SPI_Master.begin();
  SPI_Master.setFrequency(SPI_2M);
  SPI_Master.setSPIMode(SPI_MODE3);
  SPI_Master.setBitORDER(MSBFIRST);
  
  dataflash_readID();
  dataflash_readStatus();
  dataflash_readSizeSpecification();
}

void dataflash_readID(void)
{
  dataflash_deselect(); //make sure to toggle CS signal in order
  dataflash_select();   //to reset Dataflash command decoder
  
  // Send status read command 
  SPI_Master.transfer( DATAFLASH_READ_MANUFACTURER_AND_DEVICE_ID); //send status register read op-code
  
  // Manufacturer ID 
  dataflashID.manufacturer = SPI_Master.transfer( 0); //dummy write to get result
  // Device ID (part 1) 
  dataflashID.device[0] = SPI_Master.transfer( 0); //dummy write to get result
  // Device ID (part 2) 
  dataflashID.device[1] = SPI_Master.transfer( 0); //dummy write to get result
  // Extended Device Information String Length 
  dataflashID.extendedInfoLength = SPI_Master.transfer( 0); //dummy write to get result
  
  for(int i=0; i<dataflashID.extendedInfoLength; i++)
  {
    SPI_Master.transfer( 0); //dummy write
  }
  Serial.println();
  Serial.print("Manufacturer: 0x");
  Serial.println(dataflashID.manufacturer,HEX);
  Serial.print("Device: 0x");
  Serial.print(dataflashID.device[0],HEX);
  Serial.print(" 0x");
  Serial.println(dataflashID.device[1],HEX);
  return;
}

void dataflash_readSizeSpecification(void)
{
  unsigned char byte1;
  //unsigned char byte2;
  int result;
  
  dataflash_deselect(); //make sure to toggle CS signal in order
  dataflash_select();   //to reset Dataflash command decoder
  
  // Send status read command 
  SPI_Master.transfer( StatusReg); //send status register read op-code
  
  byte1 = SPI_Master.transfer( 0x00); //dummy write to get result
  //byte2 = SPI_Master.transfer( 0x00); //dummy write to get result
  SPI_Master.transfer( 0x00); //dummy write to get result
  
  result = byte1 & 0x01;
  switch(result)
  {
    case 0:
    if(dataflashID.extendedInfoLength == 0x00) // AT45DB642D
    {
      // Device is configured for standard DataFlash page size (1056 bytes).
      PageSize = 1056;
    }
    
    if(dataflashID.extendedInfoLength == 0x01) // AT45DB641E
    {
      // Device is configured for standard DataFlash page size (264 bytes).
      PageSize = 264;
    }
    break;
    case 1:
    if(dataflashID.extendedInfoLength == 0x00) // AT45DB642D
    {
      // Device is configured for �power of 2� binary page size (1024 bytes).
      PageSize = 1024;
    }
    
    if(dataflashID.extendedInfoLength == 0x01) // AT45DB641E
    {
      // Device is configured for �power of 2� binary page size (256 bytes).
      PageSize = 256;
    }
    
    break;
    default:
    break;
  }
  
  return;
}

unsigned char dataflash_readStatus(void)
{
  unsigned char result;
  unsigned char index_copy;
  dataflash_deselect();     //make sure to toggle CS signal in order
  dataflash_select();       //to reset Dataflash command decoder
  result = SPI_Master.transfer( StatusReg); //send status register read op-code
  result = SPI_Master.transfer( 0x00);    //dummy write to get result
  index_copy = ((result & 0x38) >> 3);  //get the size info from status register

  PageBits   = pgm_read_byte(&DF_pagebits[index_copy]); //get number of internal page address bits from look-up table
  //PageSize   = pgm_read_word(&DF_pagesize[index_copy]);   //get the size of the page (in bytes)

  return result;        //return the read status register value
}

/*****************************************************************************
 * 
 *  Function name : Page_To_Buffer
 *  
 *  Returns :   None
 *  
 *  Parameters :  BufferNo  ->  Decides usage of either buffer 1 or 2
 * 
 *      PageAdr   ->  Address of page to be transferred to buffer
 * 
 *  Purpose : Transfers a page from flash to Dataflash SRAM buffer
 *          
 * 
 ******************************************************************************/
void dataflash_pageToBuffer (unsigned int PageAdr, unsigned char BufferNo)
{
  dataflash_deselect();     //make sure to toggle CS signal in order
  dataflash_select();       //to reset Dataflash command decoder
  if (1 == BufferNo)        //transfer flash page to buffer 1
  {
    SPI_Master.transfer( FlashToBuf1Transfer);        //transfer to buffer 1 op-code
    SPI_Master.transfer( (unsigned char)(PageAdr >> (16 - PageBits)));  //upper part of page address
    SPI_Master.transfer( (unsigned char)(PageAdr << (PageBits - 8))); //lower part of page address
    SPI_Master.transfer( 0x00);           //don't cares
  }
#ifdef USE_BUFFER2
  else  
    if (2 == BufferNo)            //transfer flash page to buffer 2
  {
    SPI_Master.transfer( FlashToBuf2Transfer);        //transfer to buffer 2 op-code
    SPI_Master.transfer( (unsigned char)(PageAdr >> (16 - PageBits)));  //upper part of page address
    SPI_Master.transfer( (unsigned char)(PageAdr << (PageBits - 8))); //lower part of page address
    SPI_Master.transfer( 0x00);           //don't cares
  }
#endif
  dataflash_select(); //initiate the transfer
  dataflash_deselect();
  while(!(dataflash_readStatus() & 0x80)); //monitor the status register, wait until busy-flag is high
}

/*****************************************************************************
 *  
 *  Function name : Buffer_Read_Byte
 *  
 *  Returns :   One read byte (any value)
 *
 *  Parameters :  BufferNo  ->  Decides usage of either buffer 1 or 2
 * 
 *          IntPageAdr  ->  Internal page address
 *  
 *  Purpose :   Reads one byte from one of the Dataflash
 * 
 *          internal SRAM buffers
 * 
 ******************************************************************************/
unsigned char dataflash_bufferReadByte (unsigned char BufferNo, unsigned int IntPageAdr)
{
  unsigned char data;
  data='0'; // mt 
  dataflash_deselect();       //make sure to toggle CS signal in order
  dataflash_select();       //to reset Dataflash command decoder
  if (1 == BufferNo)        //read byte from buffer 1
  {
    SPI_Master.transfer( Buf1Read);     //buffer 1 read op-code
    SPI_Master.transfer( 0x00);       //don't cares
    SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
    SPI_Master.transfer( 0x00);       //don't cares
    data = SPI_Master.transfer( 0x00);      //read byte
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)        //read byte from buffer 2
    {
      SPI_Master.transfer( Buf2Read);         //buffer 2 read op-code
      SPI_Master.transfer( 0x00);           //don't cares
      SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
      SPI_Master.transfer( 0x00);           //don't cares
      data = SPI_Master.transfer( 0x00);          //read byte
    }
#endif
  return data;              //return the read data byte
}

/*****************************************************************************
 *  
 *  Function name : Buffer_Read_Str
 * 
 *  Returns :   None
 *  
 *  Parameters :  BufferNo  ->  Decides usage of either buffer 1 or 2
 * 
 *          IntPageAdr  ->  Internal page address
 * 
 *          No_of_bytes ->  Number of bytes to be read
 * 
 *          *BufferPtr  ->  address of buffer to be used for read bytes
 * 
 *  Purpose :   Reads one or more bytes from one of the Dataflash
 * 
 *          internal SRAM buffers, and puts read bytes into
 * 
 *          buffer pointed to by *BufferPtr
 * 
 * 
 ******************************************************************************/
void dataflash_bufferReadStr (unsigned char BufferNo, unsigned int IntPageAdr, unsigned int No_of_bytes, char *BufferPtr)
{
  unsigned int i;
  dataflash_deselect();           //make sure to toggle CS signal in order
  dataflash_select();             //to reset Dataflash command decoder
  if (1 == BufferNo)            //read byte(s) from buffer 1
  {
    SPI_Master.transfer( Buf1Read);       //buffer 1 read op-code
    SPI_Master.transfer( 0x00);         //don't cares
    SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
    SPI_Master.transfer( 0x00);         //don't cares
    for( i=0; i<No_of_bytes; i++)
    {
      *(BufferPtr) = SPI_Master.transfer( 0x00);    //read byte and put it in AVR buffer pointed to by *BufferPtr
      BufferPtr++;          //point to next element in AVR buffer
    }
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)          //read byte(s) from buffer 2
    {
      SPI_Master.transfer( Buf2Read);       //buffer 2 read op-code
      SPI_Master.transfer( 0x00);         //don't cares
      SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
      SPI_Master.transfer( 0x00);         //don't cares
      for( i=0; i<No_of_bytes; i++)
      {
        *(BufferPtr) = SPI_Master.transfer( 0x00);    //read byte and put it in AVR buffer pointed to by *BufferPtr
        BufferPtr++;          //point to next element in AVR buffer
      }
    }
#endif
}

/*****************************************************************************
 * 
 * 
 *  Function name : Buffer_Write_Enable
 * 
 *  Returns :   None
 *  
 *  Parameters :  IntPageAdr  ->  Internal page address to start writing from
 * 
 *      BufferAdr ->  Decides usage of either buffer 1 or 2
 *  
 *  Purpose : Enables continous write functionality to one of the Dataflash buffers
 * 
 *      buffers. NOTE : User must ensure that CS goes high to terminate
 * 
 *      this mode before accessing other Dataflash functionalities 
 * 
 ******************************************************************************/
void dataflash_bufferWriteEnable (unsigned char BufferNo, unsigned int IntPageAdr)
{
  dataflash_deselect();       //make sure to toggle CS signal in order
  dataflash_select();       //to reset Dataflash command decoder
  if (1 == BufferNo)        //write enable to buffer 1
  {
    SPI_Master.transfer( Buf1Write);      //buffer 1 write op-code
    SPI_Master.transfer( 0x00);       //don't cares
    SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)        //write enable to buffer 2
    {
      SPI_Master.transfer( Buf2Write);      //buffer 2 write op-code
      SPI_Master.transfer( 0x00);       //don't cares
      SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
    }
#endif
}

/*****************************************************************************
 *  
 *  Function name : Buffer_Write_Byte
 * 
 *  Returns :   None
 *  
 *  Parameters :  IntPageAdr  ->  Internal page address to write byte to
 * 
 *      BufferAdr ->  Decides usage of either buffer 1 or 2
 * 
 *      Data    ->  Data byte to be written
 *  
 *  Purpose :   Writes one byte to one of the Dataflash
 * 
 *          internal SRAM buffers
 *
 ******************************************************************************/
void dataflash_bufferWriteByte (unsigned char BufferNo, unsigned int IntPageAdr, unsigned char Data)
{
  dataflash_deselect();       //make sure to toggle CS signal in order
  dataflash_select();       //to reset Dataflash command decoder
  if (1 == BufferNo)        //write byte to buffer 1
  {
    SPI_Master.transfer( Buf1Write);      //buffer 1 write op-code
    SPI_Master.transfer( 0x00);       //don't cares
    SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
    SPI_Master.transfer( Data);       //write data byte
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)        //write byte to buffer 2
    {
      SPI_Master.transfer( Buf2Write);      //buffer 2 write op-code
      SPI_Master.transfer( 0x00);       //don't cares
      SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
      SPI_Master.transfer( Data);       //write data byte
    }   
#endif
}
/*****************************************************************************
 * 
 * 
 *  Function name : Buffer_Write_Str
 *  
 *  Returns :   None
 * 
 *  Parameters :  BufferNo  ->  Decides usage of either buffer 1 or 2
 * 
 *      IntPageAdr  ->  Internal page address
 * 
 *      No_of_bytes ->  Number of bytes to be written
 * 
 *      *BufferPtr  ->  address of buffer to be used for copy of bytes
 * 
 *      from AVR buffer to Dataflash buffer 1 (or 2)
 * 
 *  Purpose :   Copies one or more bytes to one of the Dataflash
 * 
 *        internal SRAM buffers from AVR SRAM buffer
 * 
 *      pointed to by *BufferPtr
 * 
 ******************************************************************************/
void dataflash_bufferWriteStr (unsigned char BufferNo, unsigned int IntPageAdr, unsigned int No_of_bytes, char *BufferPtr)
{
  unsigned int i;
  dataflash_deselect();       //make sure to toggle CS signal in order
  dataflash_select();       //to reset Dataflash command decoder
  if (1 == BufferNo)        //write byte(s) to buffer 1
  {
    SPI_Master.transfer( Buf1Write);      //buffer 1 write op-code
    SPI_Master.transfer( 0x00);       //don't cares
    SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
    for( i=0; i<No_of_bytes; i++)
    {
      SPI_Master.transfer( *(BufferPtr));     //write byte pointed at by *BufferPtr to Dataflash buffer 1 location
      BufferPtr++;        //point to next element in AVR buffer
    }
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)        //write byte(s) to buffer 2
    {
      SPI_Master.transfer( Buf2Write);      //buffer 2 write op-code
      SPI_Master.transfer( 0x00);       //don't cares
      SPI_Master.transfer( (unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      SPI_Master.transfer( (unsigned char)(IntPageAdr));  //lower part of internal buffer address
      for( i=0; i<No_of_bytes; i++)
      {
        SPI_Master.transfer( *(BufferPtr));   //write byte pointed at by *BufferPtr to Dataflash buffer 2 location
        BufferPtr++;        //point to next element in AVR buffer
      }
    }
#endif
}

/*****************************************************************************
 * 
 * 
 *  Function name : Buffer_To_Page
 * 
 *  Returns :   None
 *  
 *  Parameters :  BufferAdr ->  Decides usage of either buffer 1 or 2
 * 
 *      PageAdr   ->  Address of flash page to be programmed
 *  
 *  Purpose : Transfers a page from Dataflash SRAM buffer to flash
 * 
 *       
 ******************************************************************************/
void dataflash_bufferToPage (unsigned char BufferNo, unsigned int PageAdr)
{
  dataflash_deselect();         //make sure to toggle CS signal in order
  dataflash_select();           //to reset Dataflash command decoder
  if (1 == BufferNo)            //program flash page from buffer 1
  {
    SPI_Master.transfer( Buf1ToFlashWE);          //buffer 1 to flash with erase op-code
    SPI_Master.transfer( (unsigned char)(PageAdr >> (16 - PageBits)));  //upper part of page address
    SPI_Master.transfer( (unsigned char)(PageAdr << (PageBits - 8))); //lower part of page address
    SPI_Master.transfer( 0x00);                   //don't cares
  }
#ifdef USE_BUFFER2
  else  
    if (2 == BufferNo)            //program flash page from buffer 2
  {
    SPI_Master.transfer( Buf2ToFlashWE);          //buffer 2 to flash with erase op-code
    SPI_Master.transfer( (unsigned char)(PageAdr >> (16 - pPageBits))); //upper part of page address
    SPI_Master.transfer( (unsigned char)(PageAdr << (pPageBits - 8)));  //lower part of page address
    SPI_Master.transfer( 0x00);           //don't cares
  }
#endif
  dataflash_deselect();         //initiate flash page programming
  dataflash_select();                       
  while(!(dataflash_readStatus() & 0x80));        //monitor the status register, wait until busy-flag is high
}

/*****************************************************************************
 * 
 * 
 *  Function name : Cont_Flash_Read_Enable
 * 
 *  Returns :   None
 * 
 *  Parameters :  PageAdr   ->  Address of flash page where cont.read starts from
 * 
 *      IntPageAdr  ->  Internal page address where cont.read starts from
 *
 *  Purpose : Initiates a continuous read from a location in the DataFlash
 * 
 ******************************************************************************/
void dataflash_contFlashReadEnable (unsigned int PageAdr, unsigned int IntPageAdr)
{
  dataflash_deselect();                               //make sure to toggle CS signal in order
  dataflash_select();                           //to reset Dataflash command decoder
  SPI_Master.transfer( ContArrayRead);                          //Continuous Array Read op-code
  SPI_Master.transfer( (unsigned char)(PageAdr >> (16 - PageBits)));      //upper part of page address
  SPI_Master.transfer( (unsigned char)((PageAdr << (PageBits - 8))+ (IntPageAdr>>8)));  //lower part of page address and MSB of int.page adr.
  SPI_Master.transfer( (unsigned char)(IntPageAdr));                    //LSB byte of internal page address
  SPI_Master.transfer( 0x00);                             //perform 4 dummy writes
  SPI_Master.transfer( 0x00);                             //in order to intiate DataFlash
  SPI_Master.transfer( 0x00);                             //address pointers
  SPI_Master.transfer( 0x00);
}

//#ifdef MTEXTRAS
/*****************************************************************************
 *  
 *  Function name : Page_Buffer_Compare
 *  
 *  Returns :   0 match, 1 if mismatch
 *  
 *  Parameters :  BufferAdr ->  Decides usage of either buffer 1 or 2
 * 
 *      PageAdr   ->  Address of flash page to be compared with buffer
 * 
 *  Purpose : comparte Buffer with Flash-Page
 *  
 *   added by Martin Thomas, Kaiserslautern, Germany. This routine was not 
 * 
 *   included by ATMEL
 * 
 ******************************************************************************/
unsigned char dataflash_pageBufferCompare(unsigned char BufferNo, unsigned int PageAdr)
{
  unsigned char stat;
  dataflash_deselect();         //make sure to toggle CS signal in order
  dataflash_select();         //to reset Dataflash command decoder
  if (1 == BufferNo)                  
  {
    SPI_Master.transfer( FlashToBuf1Compare); 
    SPI_Master.transfer( (unsigned char)(PageAdr >> (16 - PageBits)));  //upper part of page address
    SPI_Master.transfer( (unsigned char)(PageAdr << (PageBits - 8))); //lower part of page address and MSB of int.page adr.
    SPI_Master.transfer( 0x00); // "dont cares"
  }
#ifdef USE_BUFFER2
  else if (2 == BufferNo)                     
  {
    SPI_Master.transfer( FlashToBuf2Compare);           
    SPI_Master.transfer( (unsigned char)(PageAdr >> (16 - PageBits)));  //upper part of page address
    SPI_Master.transfer( (unsigned char)(PageAdr << (PageBits - 8))); //lower part of page address
    SPI_Master.transfer( 0x00);           //don't cares
  }
#endif
  dataflash_deselect();                     
  dataflash_select();
  do {
    stat=dataflash_readStatus();
  } 
  while(!(stat & 0x80));          //monitor the status register, wait until busy-flag is high
  return (stat & 0x40);
}

/*****************************************************************************
 * 
 * 
 *  Function name : Page_Erase
 * 
 *  Returns :   None
 * 
 *  Parameters :  PageAdr   ->  Address of flash page to be erased
 * 
 *  Purpose :   Sets all bits in the given page (all bytes are 0xff)
 * 
 *  function added by mthomas. 
 *
 ******************************************************************************/
void dataflash_pageErase (unsigned int PageAdr)
{
  dataflash_deselect();                             //make sure to toggle CS signal in order
  dataflash_select();                           //to reset Dataflash command decoder
  SPI_Master.transfer(PageErase);            //Page erase op-code
  SPI_Master.transfer((unsigned char)(PageAdr >> (16 - PageBits)));  //upper part of page address
  SPI_Master.transfer((unsigned char)(PageAdr << (PageBits - 8))); //lower part of page address and MSB of int.page adr.
  SPI_Master.transfer(0x00); // "dont cares"
  dataflash_deselect();           //initiate flash page erase
  dataflash_select();
  while(!(dataflash_readStatus() & 0x80));        //monitor the status register, wait until busy-flag is high
}
// MTEXTRAS

void dataflash_select()
{
  digitalWrite(CS_FLASH, LOW); // Select Dataflash
}

void dataflash_deselect()
{
  digitalWrite(CS_FLASH, HIGH);
}



int dataflash_writeBytes(uint32_t address, char *data, uint32_t length)
{
  
  uint32_t size = DATAFLASH_FLASH_SIZE; //DATAFLASH_PAGE_SIZE*20;
  uint32_t pageSize = DATAFLASH_PAGE_SIZE;
  //uint16_t size = PageSize*20;
  //uint16_t pageSize = PageSize;
  
  int page;
  int bytesToWrite;
  int pageOffset;
  int dataOffset;
  int bytesOnPage;
  
  if(address>size)
  {
    /* address is beyond the flash size */
  }
  
  if(address+length>size)
  {
    /* 
       We write on the last page so we 
       have to truncate the data. 
    */
    length = (uint32_t)size-address;
    }
  
  
    if(length<=0) 
  {
    /* nothing to write */ 
    return 0;
    }
  
    bytesToWrite=length;
  
    page = (int)(address/(uint32_t)pageSize);
    pageOffset = (int)(address%(uint32_t)pageSize);
    dataOffset = 0;
  
    while(bytesToWrite>0)
  {
    /* load the current page into the buffer */
    dataflash_select();
    dataflash_pageToBuffer(page, BUFFER_ID_1);
    dataflash_deselect();
    if((pageSize-pageOffset)>=bytesToWrite)
    {
      /* the remaining data to be written fits on the current page */
      bytesOnPage = bytesToWrite;
      bytesToWrite = 0;
    }
    else 
    {
      bytesOnPage = pageSize-pageOffset;
      bytesToWrite -= bytesOnPage;
    }
    dataflash_select();
    dataflash_bufferWriteStr(BUFFER_ID_1, pageOffset, bytesOnPage, data+dataOffset);
    dataflash_bufferToPage(BUFFER_ID_1, page);
    dataflash_deselect();
    dataOffset += bytesOnPage;
    page++;
    
    /* offset is always 0 on all but the first page */
    pageOffset = 0;
  }
  
    /*
      Page Refresh
      Each one of 8192 pages on the chip has to be refreshed 
      from time to time (at max after 10000 writes to any of 
      the pages in the sekctor it belongs to). We use a slow and 
      simple strategy here. Whenever we write a page we also 
      refresh one. This guarantees that every page was refreshed 
      at least once every 8192 writes.
      The downside to this strategy is that every page-write action
      takes twice as long (~40ms)
    */
    //spiRefreshPage(refreshCounter);
    //refreshCounter++;
  
    return length;
}

int dataflash_readBytes(uint32_t address, char *data, int length)
{
  uint32_t size = DATAFLASH_FLASH_SIZE; //DATAFLASH_PAGE_SIZE*20;
  uint32_t pageSize = DATAFLASH_PAGE_SIZE;
  //uint16_t size = PageSize*20;
  //uint16_t pageSize = PageSize;
    
  int page;
  int bytesToRead;
  int pageOffset;
  int dataOffset;
  int bytesOnPage;
  
  if(address>size)
  {
    /* address is beyond the flash size */
  }
  
  if(address+length>size)
  {
    /* 
       We write on the last page so we 
       have to truncate the data. 
    */
    length = (uint32_t)size-address;
    }
  
  
    if(length<=0) 
  {
    /* nothing to write */ 
    return 0;
    }
  
    bytesToRead=length;
  
    page = (int)(address/(uint32_t)pageSize);
    pageOffset = (int)(address%(uint32_t)pageSize);
    dataOffset = 0;
  
    while(bytesToRead>0)
  {
    /* load the current page into the buffer */
    dataflash_select();
    dataflash_pageToBuffer(page, BUFFER_ID_1);
    dataflash_deselect();
    if((pageSize-pageOffset)>=bytesToRead)
    {
      /* the remaining data to be written fits on the current page */
      bytesOnPage = bytesToRead;
      bytesToRead = 0;
    }
    else 
    {
      bytesOnPage = pageSize-pageOffset;
      bytesToRead -= bytesOnPage;
    }
    dataflash_select();
    dataflash_bufferReadStr(BUFFER_ID_1, pageOffset, bytesOnPage, data+dataOffset);
    dataflash_deselect();
    dataOffset += bytesOnPage;
    page++;
    
    /* offset is always 0 on all but the first page */
    pageOffset = 0;
  }
  
  //data[length]='\0';
  
  return length;
}
