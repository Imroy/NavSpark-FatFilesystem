#include "diskio.h"
#include "SPI.h"
#include <GNSS.h>

#if(SHOW_DEBUG)
#include "HardwareSerial.h"
#endif

#define CMD0  (0x40+0)      // GO_IDLE_STATE, Software reset.
#define CMD1  (0x40+1)      // SEND_OP_COND, Initiate initialization process.
#define CMD8  (0x40+8)      // SEND_IF_COND, For only SDC V2. Check voltage range.
#define CMD9  (0x40+9)      // SEND_CSD, Read CSD register.
#define CMD10 (0x40+10)     // SEND_CID, Read CID register.
#define CMD12 (0x40+12)     // STOP_TRANSMISSION, Stop to read data.
#define ACMD13  (0xC0+13)   // SD_STATUS (SDC)
#define CMD16 (0x40+16)     // SET_BLOCKLEN, Change R/W block size.
#define CMD17 (0x40+17)     // READ_SINGLE_BLOCK, Read a block.
#define CMD18 (0x40+18)     // READ_MULTIPLE_BLOCK, Read multiple blocks.
#define ACMD23  (0xC0+23)   // SET_WR_BLK_ERASE_COUNT (SDC)
#define CMD24 (0x40+24)     // WRITE_BLOCK, Write a block.
#define CMD25 (0x40+25)     // WRITE_MULTIPLE_BLOCK, Write multiple blocks.
//#define CMD41 (0x40+41)   // SEND_OP_COND (ACMD)
#define ACMD41  (0xC0+41)   // SEND_OP_COND (SDC)
#define CMD55 (0x40+55)     // APP_CMD, Leading command of ACMD<n> command.
#define CMD58 (0x40+58)     // READ_OCR, Read OCR.
//#define ACMD41  (0x40+41)   // SEND_OP_COND (SDC), For only SDC. Initiate initialization process.

/* Card type flags (CardType) */
#define CT_MMC				0x01	/* MMC ver 3 */
#define CT_SD1				0x02	/* SD ver 1 */
#define CT_SD2				0x04	/* SD ver 2 */
#define CT_SDC				(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK			0x08	/* Block addressing */

#define FCLK_SLOW()         /* Set slow clock (100k-400k) */
#define FCLK_FAST()         /* Set fast clock (depends on the CSD) */

#if USED_INSERT_PIN
DSTATUS Stat = STA_NOINIT | STA_NODISK;  /* Disk status */
#else
DSTATUS Stat = STA_NOINIT;  /* Disk status */
#endif

static
BYTE CardType;      /* Card type flags */

#if(SHOW_DEBUG)
//static char text_buf[64];
#endif

void xmit_spi(U08 data)
{
  spiMaster.resetTx();
  spiMaster.write(data);
  spiMaster.transfer(1);
}

uint8_t rcvr_spi()
{
  spiMaster.resetTx();
  spiMaster.resetRx();
  spiMaster.write(0xFF);
  spiMaster.transfer(1);
  return spiMaster.read();
}

void dummy_clock(int n)
{
  for(int i=n; i; i--)
  {
    rcvr_spi();
  }
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static
BYTE wait_ready (void)
{
  BYTE res;
  rcvr_spi();
  do
  {
    res = rcvr_spi();
  } while ((res != 0xFF));
  return res;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static
void deselect (void)
{
  dummy_clock(10);
}



/*-----------------------------------------------------------------------*/
/* Select the card and wait ready                                        */
/*-----------------------------------------------------------------------*/

static
BOOL select (void)  /* TRUE:Successful, FALSE:Timeout */
{
  dummy_clock(5);
  return TRUE;
}

/*-----------------------------------------------------------------------*/
/* Power Control  (Platform dependent)                                   */
/*-----------------------------------------------------------------------*/
/* When the target system does not support socket power control, there   */
/* is nothing to do in these functions and chk_power always returns 1.   */

static
void power_on (void)
{
}

static
void power_off (void)
{
  Stat |= STA_NOINIT;   /* Set STA_NOINIT */
}

static
int chk_power(void)   /* Socket power state: 0=off, 1=on */
{
  return 1;
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/

static
BOOL rcvr_datablock (
  BYTE *buff,     /* Data buffer to store received data */
  UINT btr      /* Byte count (must be multiple of 4) */
)
{
  BYTE token;

  do {              /* Wait for data packet in timeout of 200ms */
    token = rcvr_spi();
  } while (token == 0xFF);
  if(token != 0xFE) return FALSE; /* If not valid data token, retutn with error */

  do {              /* Receive the data block into buffer */
    (*buff) = rcvr_spi();
    buff++;
    (*buff) = rcvr_spi();
    buff++;
    (*buff) = rcvr_spi();
    buff++;
    (*buff) = rcvr_spi();
    buff++;
  } while (btr -= 4);
  rcvr_spi();           /* Discard CRC */
  rcvr_spi();
  return TRUE;          /* Return with success */
}

/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                             */
/*-----------------------------------------------------------------------*/
#if _READONLY == 0
static
BOOL xmit_datablock (
  const BYTE *buff, /* 512 byte data block to be transmitted */
  BYTE token      /* Data/Stop token */
)
{
  BYTE resp;
//text_len = gnss_sti_sprintf(buf, "DW1(%d, %x, %d, %d)\r\n", drv, buff, sector, count);
//gnss_uart_putline(0, (U08*)buf, text_len);

  if (wait_ready() != 0xFF)
  {
    return FALSE;
  }

  xmit_spi(token);          /* Xmit data token */
  if (token == 0xFD)
  {
    wait_ready();
    return TRUE;
  }

  /* Is data token */
  int wc = 256;
  const BYTE *p = buff;
  do
  {              /* Xmit the 512 byte data block to MMC */
    xmit_spi(*p);
    p++;
    xmit_spi(*p);
    p++;
    spiMaster.resetRx();
  } while (--wc);

  xmit_spi(0xFF);         /* CRC (Dummy) */
  xmit_spi(0xFF);
  resp = rcvr_spi();        /* Reveive data response */
  if ((resp & 0x1F) != 0x05)    /* If not accepted, return with error */
  {
    return FALSE;
  }
  //if (wait_ready() != 0xFF) return FALSE;
  wait_ready();
  return TRUE;
}
#endif /* _READONLY */

static
BYTE send_cmd (
  BYTE cmd,   /* Command byte */
  DWORD arg   /* Argument */
)
{
  BYTE n, res = 0;
  if (cmd & 0x80)
  {
    cmd &= 0x7F;
    res = send_cmd(CMD55, 0);
    if (res > 1)
    {
      return res;
    }
    else
    {
      dummy_clock(10);
    }
  }

  /* Select the card and wait for ready */
  deselect();
  if (cmd == CMD0)
  {
    select();
  }
  else
  {
    if (!select())
    {
      return 0xFF;
    }
  }

  /* Send command packet */
  xmit_spi(cmd);            /* Start + Command index */
  xmit_spi((BYTE)(arg >> 24));    /* Argument[31..24] */
  xmit_spi((BYTE)(arg >> 16));    /* Argument[23..16] */
  xmit_spi((BYTE)(arg >> 8));     /* Argument[15..8] */
  xmit_spi((BYTE)arg);        /* Argument[7..0] */
  n = 0x01;             /* Dummy CRC + Stop */
  if (cmd == CMD0) n = 0x95;      /* Valid CRC for CMD0(0) */
  if (cmd == CMD8) n = 0x87;      /* Valid CRC for CMD8(0x1AA) */
  xmit_spi(n);

  /* Receive command response */
  if (cmd == CMD12)
  {
    rcvr_spi();   /* Skip a stuff byte when stop reading */
  }

  n = 200;
  do
  {
    res = rcvr_spi();
  } while(--n && (res & 0x80));
  return res;
}

DSTATUS disk_initialize (BYTE drv)
{
  BYTE cmd, ty, ocr[4];

  if (drv) return STA_NOINIT;     /* Supports only single drive */
  if (Stat & STA_NODISK)
  {
    return Stat; /* No card in the socket */
  }
  power_on();             /* Force socket power on */
  FCLK_SLOW();

  dummy_clock(100);

  ty = 0;
  if (1 != send_cmd(CMD0, 0))
  {
    return 0xFF;
  }

  dummy_clock(80);
  if (1 == send_cmd(CMD8, 0x1AA))
  {  //SDC V2
    for (int n=0; n<4; ++n) ocr[n] = rcvr_spi();

    if (ocr[2] == 0x01 && ocr[3] == 0xAA)  //The card can work at vdd range of 2.7-3.6V
    {
      while(send_cmd(ACMD41, 1UL << 30)) {};
      dummy_clock(10);
      if(0 == send_cmd(CMD58, 0))
      {
        for (int n=0; n<4; ++n) ocr[n] = rcvr_spi();
        ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;  // SDv2
      }
    }
  }
  else
  {  //SDV or MMC, no card to test
    if (send_cmd(ACMD41, 0) <= 1)
    {
      ty = CT_SD1;
      cmd = ACMD41;
    }
    else
    {
      ty = CT_MMC;
      cmd = CMD1;
    }
    dummy_clock(10);
    while(send_cmd(cmd, 0)) {};
    dummy_clock(10);
    if (send_cmd(CMD16, 512) != 0) // Set R/W block length to 512
    {
      ty = 0;
    }
  }
  CardType = ty;
  deselect();
  if (ty)
  {     /* Initialization succeded */
    Stat &= ~STA_NOINIT;    /* Clear STA_NOINIT */
    FCLK_FAST();
  }
  else
  {      /* Initialization failed */
    power_off();
  }
  return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
  BYTE drv    /* Physical drive nmuber (0) */
)
{
  if (drv) return STA_NOINIT;   /* Supports only single drive */
  return Stat;
}


//-----------------------------------------------------------------------
// Read Sector(s)
//-----------------------------------------------------------------------

DRESULT disk_read (
  BYTE drv,     /* Physical drive nmuber (0) */
  BYTE *buff,     /* Pointer to the data buffer to store read data */
  DWORD sector,   /* Start sector number (LBA) */
  UINT count      /* Sector count (1..255) */
)
{
  if (drv || !count) return RES_PARERR;
  if (Stat & STA_NOINIT) return RES_NOTRDY;

  if (!(CardType & CT_BLOCK)) sector *= 512;  // Convert to byte address if needed

  if (count == 1) { // Single block read
    if ((send_cmd(CMD17, sector) == 0)  // READ_SINGLE_BLOCK
      && rcvr_datablock(buff, 512))
      count = 0;
  }
  else {        // Multiple block read
    if (send_cmd(CMD18, sector) == 0) { // READ_MULTIPLE_BLOCK
      do {
        if (!rcvr_datablock(buff, 512)) break;
        buff += 512;
      } while (--count);
      send_cmd(CMD12, 0);       // STOP_TRANSMISSION
    }
  }
  deselect();
  return count ? RES_ERROR : RES_OK;
}

DRESULT disk_read3 (
  BYTE drv,     /* Physical drive nmuber (0) */
  BYTE *buff,     /* Pointer to the data buffer to store read data */
  DWORD sector,   /* Start sector number (LBA) */
  BYTE count      /* Sector count (1..255) */
)
{
  if (drv || !count) return RES_PARERR;
  if (Stat & STA_NOINIT) return RES_NOTRDY;

  if (!(CardType & CT_BLOCK)) sector *= 512;  // Convert to byte address if needed

  if (count == 1) { // Single block read
    if ((send_cmd(CMD17, sector) == 0)  // READ_SINGLE_BLOCK
      && rcvr_datablock(buff, 512))
      count = 0;
  }
  else {        // Multiple block read
    if (send_cmd(CMD18, sector) == 0) { // READ_MULTIPLE_BLOCK
      do {
        if (!rcvr_datablock(buff, 512)) break;
        buff += 512;
      } while (--count);
      send_cmd(CMD12, 0);       // STOP_TRANSMISSION
    }
  }
  deselect();
  return count ? RES_ERROR : RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
#if _READONLY == 0
DRESULT disk_write (
  BYTE drv,     /* Physical drive nmuber (0) */
  const BYTE *buff, /* Pointer to the data to be written */
  DWORD sector,   /* Start sector number (LBA) */
  UINT count      /* Sector count (1..255) */
)
{
  if (drv || !count) return RES_PARERR;
  if (Stat & STA_NOINIT) return RES_NOTRDY;
  if (Stat & STA_PROTECT) return RES_WRPRT;

  if (!(CardType & CT_BLOCK))
  {
    sector *= 512;  // Convert to byte address if needed
  }

  if (count == 1)
  { // Single block write
    // WRITE_BLOCK
    if ((send_cmd(CMD24, sector) == 0) && xmit_datablock(buff, 0xFE))
    {
      count = 0;
    }
  } //if (count == 1)
  else
  { // Multiple block write
    if (CardType & CT_SDC)
    {
      send_cmd(ACMD23, count);
    }
    if (send_cmd(CMD25, sector) == 0)
    { // WRITE_MULTIPLE_BLOCK
      do
      {
        if (!xmit_datablock(buff, 0xFC))
          break;
        buff += 512;
      } while (--count);
      if (!xmit_datablock(0, 0xFD)) // STOP_TRAN token
      {
        count = 1;
      }
    } //if (send_cmd(CMD25, sector) == 0)
  } //if (count == 1) else
  return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY == 0 */


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
#if _USE_IOCTL != 0
DRESULT disk_ioctl (
  BYTE drv,   /* Physical drive nmuber (0) */
  BYTE ctrl,    /* Control code */
  void *buff    /* Buffer to send/receive control data */
)
{
  DRESULT res;
  BYTE n, csd[16], *ptr = (BYTE*)buff;
  WORD csize;


  if (drv) return RES_PARERR;

  res = RES_ERROR;

  if (ctrl == CTRL_POWER) {
    switch (*ptr) {
    case 0:   // Sub control code == 0 (POWER_OFF)
      if (chk_power())
        power_off();    // Power off
      res = RES_OK;
      break;
    case 1:   // Sub control code == 1 (POWER_ON)
      power_on();       // Power on
      res = RES_OK;
      break;
    case 2:   // Sub control code == 2 (POWER_GET)
      *(ptr+1) = (BYTE)chk_power();
      res = RES_OK;
      break;
    default :
      res = RES_PARERR;
    }
  }
  else {
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    switch (ctrl) {
    case CTRL_SYNC :    // Make sure that no pending write process. Do not remove this or written sector might not left updated.
      if (select()) {
        res = RES_OK;
        deselect();
      }
      break;

    case GET_SECTOR_COUNT : // Get number of sectors on the disk (DWORD)
      if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
        if ((csd[0] >> 6) == 1) { // SDC ver 2.00
          csize = csd[9] + ((WORD)csd[8] << 8) + 1;
          *(DWORD*)buff = (DWORD)csize << 10;
        } else {          // SDC ver 1.XX or MMC
          n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
          csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
          *(DWORD*)buff = (DWORD)csize << (n - 9);
        }
        res = RES_OK;
      }
      break;

    case GET_SECTOR_SIZE :  // Get R/W sector size (WORD)
      *(WORD*)buff = 512;
      res = RES_OK;
      break;

    case GET_BLOCK_SIZE : // Get erase block size in unit of sector (DWORD)
      if (CardType & CT_SD2) {  // SDC ver 2.00
        if (send_cmd(ACMD13, 0) == 0) { // Read SD status
          rcvr_spi();
          if (rcvr_datablock(csd, 16)) {        // Read partial block
            for (n = 64 - 16; n; n--) rcvr_spi(); // Purge trailing data
            *(DWORD*)buff = 16UL << (csd[10] >> 4);
            res = RES_OK;
          }
        }
      } else {          // SDC ver 1.XX or MMC
        if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {  // Read CSD
          if (CardType & CT_SD1) {  // SDC ver 1.XX
            *(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
          } else {          // MMC
            *(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
          }
          res = RES_OK;
        }
      }
      break;

    case MMC_GET_TYPE :   // Get card type flags (1 byte)
      *ptr = CardType;
      res = RES_OK;
      break;

    case MMC_GET_CSD :    // Receive CSD as a data block (16 bytes)
      if (send_cmd(CMD9, 0) == 0    /// READ_CSD
        && rcvr_datablock(ptr, 16))
        res = RES_OK;
      break;

    case MMC_GET_CID :    // Receive CID as a data block (16 bytes)
      if (send_cmd(CMD10, 0) == 0   // READ_CID
        && rcvr_datablock(ptr, 16))
        res = RES_OK;
      break;

    case MMC_GET_OCR :    // Receive OCR as an R3 resp (4 bytes)
      if (send_cmd(CMD58, 0) == 0) {  // READ_OCR
        for (n = 4; n; n--) *ptr++ = rcvr_spi();
        res = RES_OK;
      }
      break;

    case MMC_GET_SDSTAT : // Receive SD statsu as a data block (64 bytes)
      if (send_cmd(ACMD13, 0) == 0) { // SD_STATUS
        rcvr_spi();
        if (rcvr_datablock(ptr, 64))
          res = RES_OK;
      }
      break;

    default:
      res = RES_PARERR;
    }

    deselect();
  }

  return res;
}
#endif /* _USE_IOCTL != 0 */

#ifdef __cplusplus
extern "C" {
#endif

// --------------------
static
const WCHAR Tbl[] = { /*  CP437(0x80-0xFF) to Unicode conversion table */
  0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
  0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
  0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
  0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
  0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
  0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
  0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
  0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
  0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
  0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
  0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
  0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
  0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
  0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
  0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

WCHAR ff_convert (  /* Converted character, Returns zero on error */
  WCHAR src,  /* Character code to be converted */
  UINT  dir   /* 0: Unicode to OEMCP, 1: OEMCP to Unicode */
)
{
  WCHAR c;


  if (src < 0x80) { /* ASCII */
    c = src;

  } else {
    if (dir) {    /* OEMCP to Unicode */
      c = (src >= 0x100) ? 0 : Tbl[src - 0x80];

    } else {    /* Unicode to OEMCP */
      for (c = 0; c < 0x80; c++) {
        if (src == Tbl[c]) break;
      }
      c = (c + 0x80) & 0xFF;
    }
  }

  return c;
}

WCHAR ff_wtoupper ( // Upper converted character
  WCHAR chr   // Input character
)
{
  static const WCHAR tbl_lower[] = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0xA1, 0x00A2, 0x00A3, 0x00A5, 0x00AC, 0x00AF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0x0FF, 0x101, 0x103, 0x105, 0x107, 0x109, 0x10B, 0x10D, 0x10F, 0x111, 0x113, 0x115, 0x117, 0x119, 0x11B, 0x11D, 0x11F, 0x121, 0x123, 0x125, 0x127, 0x129, 0x12B, 0x12D, 0x12F, 0x131, 0x133, 0x135, 0x137, 0x13A, 0x13C, 0x13E, 0x140, 0x142, 0x144, 0x146, 0x148, 0x14B, 0x14D, 0x14F, 0x151, 0x153, 0x155, 0x157, 0x159, 0x15B, 0x15D, 0x15F, 0x161, 0x163, 0x165, 0x167, 0x169, 0x16B, 0x16D, 0x16F, 0x171, 0x173, 0x175, 0x177, 0x17A, 0x17C, 0x17E, 0x192, 0x3B1, 0x3B2, 0x3B3, 0x3B4, 0x3B5, 0x3B6, 0x3B7, 0x3B8, 0x3B9, 0x3BA, 0x3BB, 0x3BC, 0x3BD, 0x3BE, 0x3BF, 0x3C0, 0x3C1, 0x3C3, 0x3C4, 0x3C5, 0x3C6, 0x3C7, 0x3C8, 0x3C9, 0x3CA, 0x430, 0x431, 0x432, 0x433, 0x434, 0x435, 0x436, 0x437, 0x438, 0x439, 0x43A, 0x43B, 0x43C, 0x43D, 0x43E, 0x43F, 0x440, 0x441, 0x442, 0x443, 0x444, 0x445, 0x446, 0x447, 0x448, 0x449, 0x44A, 0x44B, 0x44C, 0x44D, 0x44E, 0x44F, 0x451, 0x452, 0x453, 0x454, 0x455, 0x456, 0x457, 0x458, 0x459, 0x45A, 0x45B, 0x45C, 0x45E, 0x45F, 0x2170, 0x2171, 0x2172, 0x2173, 0x2174, 0x2175, 0x2176, 0x2177, 0x2178, 0x2179, 0x217A, 0x217B, 0x217C, 0x217D, 0x217E, 0x217F, 0xFF41, 0xFF42, 0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48, 0xFF49, 0xFF4A, 0xFF4B, 0xFF4C, 0xFF4D, 0xFF4E, 0xFF4F, 0xFF50, 0xFF51, 0xFF52, 0xFF53, 0xFF54, 0xFF55, 0xFF56, 0xFF57, 0xFF58, 0xFF59, 0xFF5A, 0 };
  static const WCHAR tbl_upper[] = { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x21, 0xFFE0, 0xFFE1, 0xFFE5, 0xFFE2, 0xFFE3, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0x178, 0x100, 0x102, 0x104, 0x106, 0x108, 0x10A, 0x10C, 0x10E, 0x110, 0x112, 0x114, 0x116, 0x118, 0x11A, 0x11C, 0x11E, 0x120, 0x122, 0x124, 0x126, 0x128, 0x12A, 0x12C, 0x12E, 0x130, 0x132, 0x134, 0x136, 0x139, 0x13B, 0x13D, 0x13F, 0x141, 0x143, 0x145, 0x147, 0x14A, 0x14C, 0x14E, 0x150, 0x152, 0x154, 0x156, 0x158, 0x15A, 0x15C, 0x15E, 0x160, 0x162, 0x164, 0x166, 0x168, 0x16A, 0x16C, 0x16E, 0x170, 0x172, 0x174, 0x176, 0x179, 0x17B, 0x17D, 0x191, 0x391, 0x392, 0x393, 0x394, 0x395, 0x396, 0x397, 0x398, 0x399, 0x39A, 0x39B, 0x39C, 0x39D, 0x39E, 0x39F, 0x3A0, 0x3A1, 0x3A3, 0x3A4, 0x3A5, 0x3A6, 0x3A7, 0x3A8, 0x3A9, 0x3AA, 0x410, 0x411, 0x412, 0x413, 0x414, 0x415, 0x416, 0x417, 0x418, 0x419, 0x41A, 0x41B, 0x41C, 0x41D, 0x41E, 0x41F, 0x420, 0x421, 0x422, 0x423, 0x424, 0x425, 0x426, 0x427, 0x428, 0x429, 0x42A, 0x42B, 0x42C, 0x42D, 0x42E, 0x42F, 0x401, 0x402, 0x403, 0x404, 0x405, 0x406, 0x407, 0x408, 0x409, 0x40A, 0x40B, 0x40C, 0x40E, 0x40F, 0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F, 0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0 };
  int i;


  for (i = 0; tbl_lower[i] && chr != tbl_lower[i]; i++) ;

  return tbl_lower[i] ? tbl_upper[i] : chr;
}

DWORD get_fattime (void) {
#if defined(USE_UART1_FOR_NMEA) && (USE_UART1_FOR_NMEA==1)
  if (GnssInfo.fixMode() != 0)
    return ((DWORD)(GnssInfo.date.year() - 1980) << 25)
      | ((DWORD)GnssInfo.date.month() << 21)
      | ((DWORD)GnssInfo.date.day() << 16)
      | ((DWORD)GnssInfo.time.hour() << 11)
      | ((DWORD)GnssInfo.time.minute() << 5)
      | ((DWORD)GnssInfo.time.second() >> 1);
#endif

  return 0;
}

#ifdef __cplusplus
}
#endif
