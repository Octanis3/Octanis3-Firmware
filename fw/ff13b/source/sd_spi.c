/*
 * sd_spi.c
 *
 *  Created on: 20 Aug 2018
 *      Author: raffael
 *
 *  Inspired by ChaN's STM32F100: MMCv3/SDv1/SDv2 (SPI mode) control module
 *  (example code)
 */
#include "../../ADS1220/spi.h"

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>
#include <xdc/runtime/Timestamp.h>
#include <ti/sysbios/hal/Seconds.h>

#include "diskio.h"

/* Definitions of physical drive number for each drive */
#define DEV_MMC     0   /* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB     2   /* Example: Map USB MSD to physical drive 2 */
#define DEV_RAM     1   /* Example: Map Ramdisk to physical drive 0 */


extern SPI_Handle  nestbox_spi_handle;

static volatile
DSTATUS Stat = STA_NOINIT;  /* Physical drive status */
static
BYTE CardType;          /* Card type flags */


/* MMC/SD command */
#define CMD0    (0)         /* GO_IDLE_STATE */
#define CMD1    (1)         /* SEND_OP_COND (MMC) */
#define ACMD41  (0x80+41)   /* SEND_OP_COND (SDC) */
#define CMD8    (8)         /* SEND_IF_COND */
#define CMD9    (9)         /* SEND_CSD */
#define CMD10   (10)        /* SEND_CID */
#define CMD12   (12)        /* STOP_TRANSMISSION */
#define ACMD13  (0x80+13)   /* SD_STATUS (SDC) */
#define CMD16   (16)        /* SET_BLOCKLEN */
#define CMD17   (17)        /* READ_SINGLE_BLOCK */
#define CMD18   (18)        /* READ_MULTIPLE_BLOCK */
#define CMD23   (23)        /* SET_BLOCK_COUNT (MMC) */
#define ACMD23  (0x80+23)   /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24   (24)        /* WRITE_BLOCK */
#define CMD25   (25)        /* WRITE_MULTIPLE_BLOCK */
#define CMD32   (32)        /* ERASE_ER_BLK_START */
#define CMD33   (33)        /* ERASE_ER_BLK_END */
#define CMD38   (38)        /* ERASE */
#define CMD55   (55)        /* APP_CMD */
#define CMD58   (58)        /* READ_OCR */

/*-----------------------------------------------------------------------*/
/* SPI controls (Platform dependent)                                     */
/*-----------------------------------------------------------------------*/

/* Initialize MMC interface */
static
void init_spi (void)
{
    spi1_arch_init();
}


/* Exchange a byte */
static
BYTE xchg_spi (
    BYTE dat    /* Data to send */
)
{
    SPI_Transaction     spiTransaction;
    BYTE                rxBuf = 0xff;

    spiTransaction.count = 1;
    spiTransaction.txBuf = &dat;
    spiTransaction.rxBuf = &rxBuf;
    SPI_transfer(nestbox_spi_handle, &spiTransaction);

    return rxBuf;       /* Return received byte */
}


/* Receive multiple byte */
static
void rcvr_spi_multi (
    BYTE *buff,     /* Pointer to data buffer */
    UINT btr        /* Number of bytes to receive (even number) */
)
{
    SPI_Transaction     spiTransaction;
    BYTE                txBuf[2] = {0xff, 0xff};

    spiTransaction.count = 2;
    spiTransaction.txBuf = txBuf;

    do {                    /* Receive the data block into buffer */
        spiTransaction.rxBuf = buff;
        SPI_transfer(nestbox_spi_handle, &spiTransaction);
        buff += 2;
    } while (btr -= 2);
}


#if _USE_WRITE
/* Send multiple byte */
static
void xmit_spi_multi (
    /*const*/ BYTE *buff,   /* Pointer to the data */
    UINT btx            /* Number of bytes to send (even number) */
)
{
    SPI_Transaction     spiTransaction;
    BYTE                rxBuf[2];

    spiTransaction.count = 2;
    spiTransaction.rxBuf = rxBuf;

    do {                    /* Receive the data block into buffer */
        spiTransaction.txBuf = buff;
        SPI_transfer(nestbox_spi_handle, &spiTransaction);
        buff += 2;
    } while (btx -= 2);
}
#endif


/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static
int wait_ready (    /* 1:Ready, 0:Timeout */
    UINT wt         /* Timeout [ms] */
)
{
    BYTE d;
    unsigned int i = 0;

    for(i=0;i<(wt>>2);i++)
    {
        d = xchg_spi(0xFF);
        Task_sleep(4);
        /* This loop takes a time. Insert rot_rdq() here for multitask environment. */
    } while (d != 0xFF);  /* Wait for card goes ready or timeout */

    return (d == 0xFF) ? 1 : 0;
}



/*-----------------------------------------------------------------------*/
/* Deselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/

static
void deselect (void)
{
    spi_slave_unselect(nbox_spi_cs_n);
    xchg_spi(0xFF); /* Dummy clock (force DO hi-z for multiple slave SPI) */
    // TODO:check if needed

}



/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/

static
int select (void)   /* 1:OK, 0:Timeout */
{
    spi_slave_select(nbox_spi_cs_n);  /* Set CS# low */
    xchg_spi(0xFF); /* Dummy clock (force DO enabled) */
    if (wait_ready(500)) return 1;  /* Wait for card ready */

    deselect();
    return 0;   /* Timeout */
}



/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/

static
int rcvr_datablock (    /* 1:OK, 0:Error */
    BYTE *buff,         /* Data buffer */
    UINT btr            /* Data block length (byte) */
)
{
    BYTE token;


    unsigned int i=0;
    for(i=0; i<100; i++)                        /* Wait for DataStart token in timeout of 200ms */
    {
        token = xchg_spi(0xFF);
        /* This loop will take a time. Insert rot_rdq() here for multitask envilonment. */
        if(token == 0xFE)
            break;
    }
    if(token != 0xFE) return 0;     /* Function fails if invalid DataStart token or timeout */

    rcvr_spi_multi(buff, btr);      /* Store trailing data to the buffer */
    xchg_spi(0xFF); xchg_spi(0xFF);         /* Discard CRC */

    return 1;                       /* Function succeeded */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to the MMC                                         */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
static
int xmit_datablock (    /* 1:OK, 0:Failed */
    BYTE *buff,   /* Ponter to 512 byte data to be sent */
    BYTE token          /* Token */
)
{
    BYTE resp,resp1,resp2,resp3;


    if (!wait_ready(500)) return 0;     /* Wait for card ready */

    xchg_spi(token);                    /* Send token */
    if (token != 0xFD) {                /* Send data if token is other than StopTran */
        xmit_spi_multi(buff, 512);      /* Data */
        xchg_spi(0xFF); xchg_spi(0xFF); /* Dummy CRC */

        resp = xchg_spi(0xFF);              /* Receive data resp */
//        resp1 = xchg_spi(0xFF);              /* Receive data resp */
//        resp2 = xchg_spi(0xFF);              /* Receive data resp */
//        resp3 = xchg_spi(0xFF);              /* Receive data resp */

        // TODO: receives wrong response here!! (0xff)
        if(     ((resp & 0x1F)!= 0x05) //&&
//                ((resp1 & 0x1F)!= 0x05) &&
//                ((resp2 & 0x1F)!= 0x05) &&
//                ((resp3 & 0x1F)!= 0x05)
          )
        /*if ((resp & 0x1F) != 0x05)*/ return 0;    /* Function fails if the data packet was not accepted */
    }
    return 1;
}
#endif


/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (     /* Return value: R1 resp (bit7==1:Failed to send) */
    BYTE cmd,       /* Command index */
    DWORD arg       /* Argument */
)
{
    BYTE n, res;


    if (cmd & 0x80) {   /* Send a CMD55 prior to ACMD<n> */
        cmd &= 0x7F;
        res = send_cmd(CMD55, 0);
        if (res > 1) return res;
    }

    /* Select the card and wait for ready except to stop multiple block read */
    if (cmd != CMD12) {
        deselect();
        if (!select()) return 0xFF;
    }

    /* Send command packet */
    xchg_spi(0x40 | cmd);               /* Start + command index */
    xchg_spi((BYTE)(arg >> 24));        /* Argument[31..24] */
    xchg_spi((BYTE)(arg >> 16));        /* Argument[23..16] */
    xchg_spi((BYTE)(arg >> 8));         /* Argument[15..8] */
    xchg_spi((BYTE)arg);                /* Argument[7..0] */
    n = 0x01;                           /* Dummy CRC + Stop */
    if (cmd == CMD0) n = 0x95;          /* Valid CRC for CMD0(0) */
    if (cmd == CMD8) n = 0x87;          /* Valid CRC for CMD8(0x1AA) */
    xchg_spi(n);

    /* Receive command resp */
    if (cmd == CMD12) xchg_spi(0xFF);   /* Diacard following one byte when CMD12 */
    n = 10;                             /* Wait for response (10 bytes max) */
    do {
        res = xchg_spi(0xFF);
    } while ((res & 0x80) && --n);

    return res;                         /* Return received response */
}




/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------*/
/* Initialize disk drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS fat_disk_initialize (
    BYTE drv        /* Physical drive number (0) */
)
{
    BYTE n, cmd, ty, ocr[4];


    if (drv) return STA_NOINIT;         /* Supports only drive 0 */
    init_spi();                         /* Initialize SPI */

    if (Stat & STA_NODISK) return Stat; /* Is card existing in the soket? */

    // TODO: FCLK_SLOW();
    for (n = 10; n; n--) xchg_spi(0xFF);    /* Send 80 dummy clocks */

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) {           /* Put the card SPI/Idle state */
        uint32_t timeout = Seconds_get()+30;    /* Initialization timeout = 1 sec */
        if (send_cmd(CMD8, 0x1AA) == 1) {   /* SDv2? */
            for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);    /* Get 32 bit return value of R7 resp */
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {             /* Is the card supports vcc of 2.7-3.6V? */
                while ((timeout>Seconds_get()) && send_cmd(ACMD41, 1UL << 30)) ; /* Wait for end of initialization with ACMD41(HCS) */
                uint32_t time2 = Seconds_get();
                if ((timeout>time2) && send_cmd(CMD58, 0) == 0) {        /* Check CCS bit in the OCR */
                    for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;  /* Card id SDv2 */
                }
            }
        } else {    /* Not SDv2 card */
            if (send_cmd(ACMD41, 0) <= 1)   {   /* SDv1 or MMC? */
                ty = CT_SD1; cmd = ACMD41;  /* SDv1 (ACMD41(0)) */
            } else {
                ty = CT_MMC; cmd = CMD1;    /* MMCv3 (CMD1(0)) */
            }
            while ((timeout>Seconds_get()) && send_cmd(cmd, 0)) ;        /* Wait for end of initialization */
            if ((timeout<Seconds_get()) || send_cmd(CMD16, 512) != 0)   /* Set block length: 512 */
                ty = 0;
        }
    }
    CardType = ty;  /* Card type */
    deselect();

    if (ty) {           /* OK */
        //TODO: FCLK_FAST();            /* Set fast clock */
        Stat &= ~STA_NOINIT;    /* Clear STA_NOINIT flag */
    } else {            /* Failed */
        Stat = STA_NOINIT;
    }

    return Stat;
}



/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS fat_disk_status (
    BYTE drv        /* Physical drive number (0) */
)
{
    if (drv) return STA_NOINIT;     /* Supports only drive 0 */

    return Stat;    /* Return disk status */
}



/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT fat_disk_read (
    BYTE drv,       /* Physical drive number (0) */
    BYTE *buff,     /* Pointer to the data buffer to store read data */
    DWORD sector,   /* Start sector number (LBA) */
    UINT count      /* Number of sectors to read (1..128) */
)
{
    if (drv || !count) return RES_PARERR;       /* Check parameter */
    if (Stat & STA_NOINIT) return RES_NOTRDY;   /* Check if drive is ready */

    if (!(CardType & CT_BLOCK)) sector *= 512;  /* LBA ot BA conversion (byte addressing cards) */

    if (count == 1) {   /* Single sector read */
        if ((send_cmd(CMD17, sector) == 0)  /* READ_SINGLE_BLOCK */
            && rcvr_datablock(buff, 512)) {
            count = 0;
        }
    }
    else {              /* Multiple sector read */
        if (send_cmd(CMD18, sector) == 0) { /* READ_MULTIPLE_BLOCK */
            do {
                if (!rcvr_datablock(buff, 512)) break;
                buff += 512;
            } while (--count);
            send_cmd(CMD12, 0);             /* STOP_TRANSMISSION */
        }
    }
    deselect();

    return count ? RES_ERROR : RES_OK;  /* Return result */
}



/*-----------------------------------------------------------------------*/
/* Write sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT fat_disk_write (
    BYTE drv,           /* Physical drive number (0) */
    const BYTE *buff,   /* Ponter to the data to write */
    DWORD sector,       /* Start sector number (LBA) */
    UINT count          /* Number of sectors to write (1..128) */
)
{
    if (drv || !count) return RES_PARERR;       /* Check parameter */
    if (Stat & STA_NOINIT) return RES_NOTRDY;   /* Check drive status */
    if (Stat & STA_PROTECT) return RES_WRPRT;   /* Check write protect */

    if (!(CardType & CT_BLOCK)) sector *= 512;  /* LBA ==> BA conversion (byte addressing cards) */

    if (count == 1) {   /* Single sector write */
        if ((send_cmd(CMD24, sector) == 0)  /* WRITE_BLOCK */
            && xmit_datablock(buff, 0xFE)) {
            count = 0;
        }
    }
    else {              /* Multiple sector write */
        if (CardType & CT_SDC) send_cmd(ACMD23, count); /* Predefine number of sectors */
        if (send_cmd(CMD25, sector) == 0) { /* WRITE_MULTIPLE_BLOCK */
            do {
                if (!xmit_datablock(buff, 0xFC)) break;
                buff += 512;
            } while (--count);
            if (!xmit_datablock(0, 0xFD)) count = 1;    /* STOP_TRAN token */
        }
    }
    deselect();

    return count ? RES_ERROR : RES_OK;  /* Return result */
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous drive controls other than data read/write               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT fat_disk_ioctl (
    BYTE drv,       /* Physical drive number (0) */
    BYTE cmd,       /* Control command code */
    void *buff      /* Pointer to the conrtol data */
)
{
    DRESULT res;
    BYTE n, csd[16];
    DWORD *dp, st, ed, csize;


    if (drv) return RES_PARERR;                 /* Check parameter */
    if (Stat & STA_NOINIT) return RES_NOTRDY;   /* Check if drive is ready */

    res = RES_ERROR;

    switch (cmd) {
    case CTRL_SYNC :        /* Wait for end of internal write process of the drive */
        if (select()) res = RES_OK;
        break;

    case GET_SECTOR_COUNT : /* Get drive capacity in unit of sector (DWORD) */
        if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
            if ((csd[0] >> 6) == 1) {   /* SDC ver 2.00 */
                csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
                *(DWORD*)buff = csize << 10;
            } else {                    /* SDC ver 1.XX or MMC ver 3 */
                n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
                *(DWORD*)buff = csize << (n - 9);
            }
            res = RES_OK;
        }
        break;

    case GET_BLOCK_SIZE :   /* Get erase block size in unit of sector (DWORD) */
        if (CardType & CT_SD2) {    /* SDC ver 2.00 */
            if (send_cmd(ACMD13, 0) == 0) { /* Read SD status */
                xchg_spi(0xFF);
                if (rcvr_datablock(csd, 16)) {              /* Read partial block */
                    for (n = 64 - 16; n; n--) xchg_spi(0xFF);   /* Purge trailing data */
                    *(DWORD*)buff = 16UL << (csd[10] >> 4);
                    res = RES_OK;
                }
            }
        } else {                    /* SDC ver 1.XX or MMC */
            if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {  /* Read CSD */
                if (CardType & CT_SD1) {    /* SDC ver 1.XX */
                    *(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
                } else {                    /* MMC */
                    *(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
                }
                res = RES_OK;
            }
        }
        break;

    case CTRL_TRIM :    /* Erase a block of sectors (used when _USE_ERASE == 1) */
        if (!(CardType & CT_SDC)) break;                /* Check if the card is SDC */
        if (fat_disk_ioctl(drv, MMC_GET_CSD, csd)) break;   /* Get CSD */
        if (!(csd[0] >> 6) && !(csd[10] & 0x40)) break; /* Check if sector erase can be applied to the card */
        dp = buff; st = dp[0]; ed = dp[1];              /* Load sector block */
        if (!(CardType & CT_BLOCK)) {
            st *= 512; ed *= 512;
        }
        if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 && send_cmd(CMD38, 0) == 0 && wait_ready(30000)) { /* Erase sector block */
            res = RES_OK;   /* FatFs does not check result of this command */
        }
        break;

    default:
        res = RES_PARERR;
    }

    deselect();

    return res;
}
#endif


/*-----------------------------------------------------------------------*/
/* Device timer function                                                 */
/*-----------------------------------------------------------------------*/
/* This function must be called from timer interrupt routine in period
/  of 1 ms to generate card control timing.
*/

void fat_disk_timerproc (void)
{
//    WORD n;
    BYTE s;


//    n = Timer1;                     /* 1kHz decrement timer stopped at 0 */
//    if (n) Timer1 = --n;
//    n = Timer2;
//    if (n) Timer2 = --n;

    s = Stat;
    /** Micro SD card cannot be write protected **/
//    if (MMC_WP) {   /* Write protected */
//        s |= STA_PROTECT;
//    } else {        /* Write enabled */
//        s &= ~STA_PROTECT;
//    }

    //TODO: check if SD card is present (on new PCB)!

//    if (MMC_CD) {   /* Card is in socket */
//        s &= ~STA_NODISK;
//    } else {        /* Socket empty */
//        s |= (STA_NODISK | STA_NOINIT);
//    }
    Stat = s;
}

/*---------------------------------------------------------*/
/* User provided RTC function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called back     */
/* from FatFs module.                                      */

#if !FF_FS_NORTC && !FF_FS_READONLY
DWORD fat_get_fattime (void)
{
    return 0;
    //TODO:
//    RTCTIME rtc;
//
//    /* Get local time */
//    if (!rtc_gettime(&rtc)) return 0;
//
//    /* Pack date and time into a DWORD variable */
//    return    ((DWORD)(rtc.year - 1980) << 25)
//            | ((DWORD)rtc.month << 21)
//            | ((DWORD)rtc.mday << 16)
//            | ((DWORD)rtc.hour << 11)
//            | ((DWORD)rtc.min << 5)
//            | ((DWORD)rtc.sec >> 1);
}
#endif



