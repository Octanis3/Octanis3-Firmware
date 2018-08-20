/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various existing      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */

/* Definitions of physical drive number for each drive */
#define DEV_MMC		0	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */
#define DEV_RAM     1   /* Example: Map Ramdisk to physical drive 0 */


///*-----------------------------------------------------------------------*/
///* Get Drive Status                                                      */
///*-----------------------------------------------------------------------*/
//
//DSTATUS fat_disk_status (
//	BYTE pdrv		/* Physical drive nmuber to identify the drive */
//)
//{
//	DSTATUS stat;
//	int result;
//
//	if(pdrv == DEV_MMC)
//	{
//		result = MMC_disk_status();
//
//		// translate the reslut code here
//
//		return stat;
//	}
//	else
//	    return STA_NOINIT;
//}
//
//
//
///*-----------------------------------------------------------------------*/
///* Inidialize a Drive                                                    */
///*-----------------------------------------------------------------------*/
//
//DSTATUS fat_disk_initialize (
//	BYTE pdrv				/* Physical drive nmuber to identify the drive */
//)
//{
//	DSTATUS stat;
//	int result;
//
//	if(pdrv == DEV_MMC)
//	{
//		result = MMC_disk_initialize();
//
//		// translate the reslut code here
//
//		return stat;
//	}
//	else
//	    return STA_NOINIT;
//}
//
//
//
///*-----------------------------------------------------------------------*/
///* Read Sector(s)                                                        */
///*-----------------------------------------------------------------------*/
//
//DRESULT fat_disk_read (
//	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
//	BYTE *buff,		/* Data buffer to store read data */
//	DWORD sector,	/* Start sector in LBA */
//	UINT count		/* Number of sectors to read */
//)
//{
//	DRESULT res;
//	int result;
//
//	if(pdrv == DEV_MMC)
//	{
//		// translate the arguments here
//
//		result = MMC_disk_read(buff, sector, count);
//
//		// translate the reslut code here
//
//		return res;
//	}
//	else
//	    return RES_PARERR;
//}
//
//
//
///*-----------------------------------------------------------------------*/
///* Write Sector(s)                                                       */
///*-----------------------------------------------------------------------*/
//
//DRESULT fat_disk_write (
//	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
//	const BYTE *buff,	/* Data to be written */
//	DWORD sector,		/* Start sector in LBA */
//	UINT count			/* Number of sectors to write */
//)
//{
//	DRESULT res;
//	int result;
//
//	if (pdrv==DEV_MMC) {
//		// translate the arguments here
//
//		result = MMC_disk_write(buff, sector, count);
//
//		// translate the reslut code here
//
//		return res;
//	}
//	else
//	    return RES_PARERR;
//}
//
//
//
///*-----------------------------------------------------------------------*/
///* Miscellaneous Functions                                               */
///*-----------------------------------------------------------------------*/
//
//DRESULT fat_disk_ioctl (
//	BYTE pdrv,		/* Physical drive nmuber (0..) */
//	BYTE cmd,		/* Control code */
//	void *buff		/* Buffer to send/receive control data */
//)
//{
//	DRESULT res;
//	int result;
//
//	if(pdrv == DEV_MMC) {
//		// Process of the command for the MMC/SD card
//
//		return res;
//	}
//	else
//	    return RES_PARERR;
//}
//
