#ifndef BOOTLOADER_H
#define BOOTLOADER_H

/* ------------------ Includes ------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "usart.h"
#include "crc.h"
#include "Bootloader_Cfg.h"

/* ------------------ Macros Declaration --------------------------- */

#define BL_HOST_RECIEVER_BUFFER_SIZE    (200)


#define CRC_SIZE_BYTE   (4)
#define CRC_NOK					(0)
#define CRC_OK 					(1)

#define CBL_REPLY_NACK   (0)
#define CBL_REPLY_ACK    (1)

#define CBL_ADDRESS_NOT_VALID   (0)
#define CBL_ADDRESS_VALID       (1)

#define CBL_ERASE_FAILED           (0)
#define CBL_ERASE_SUCESSED         (1)

#define CBL_FLASH_WRITE_FAILED           (0)
#define CBL_FLASH_WRITE_SUCESSED         (1)

#define CBL_GET_RDB_FAILED           (0)
#define CBL_GET_RDB_SUCESSED         (1)

#define CBL_CHANGE_RDB_FAILED           (0)
#define CBL_CHANGE_RDB_SUCESSED         (1)

#define CBL_GET_VER_CMD              0x10
#define CBL_GET_HELP_CMD             0x11
#define CBL_GET_CID_CMD              0x12
/* Get Read Protection Status */
#define CBL_GET_RDP_STATUS_CMD       0x13
#define CBL_GO_TO_ADDR_CMD           0x14
#define CBL_FLASH_ERASE_CMD          0x15
#define CBL_MEM_WRITE_CMD            0x16
/* Enable/Disable Write Protection */
#define CBL_ED_W_PROTECT_CMD         0x17
#define CBL_MEM_READ_CMD             0x18
/* Get Sector Read/Write Protection Status */
#define CBL_READ_SECTOR_STATUS_CMD   0x19
#define CBL_OTP_READ_CMD             0x20
/* Change Read Out Protection Level */
#define CBL_CHANGE_ROP_Level_CMD     0x21


#define CBL_GET_VER_VENDOR_ID             (100)
#define CBL_GET_VER_MAJOR_VERSION         (1)
#define CBL_GET_VER_MINOR_VERSION         (0)
#define CBL_GET_VER_PATCH_VERSION         (0)

#define FLASH_SECTOR2_START_ADDRESS       (0x8008000)

#define SRAM1_SIZE                        ( 112  * 1024  )
#define SRAM2_SIZE                        ( 16   * 1024  )
#define SRAM3_SIZE                        ( 64   * 1024  )
#define FLASH_SIZE                        ( 1024 * 1024  )

#define SRAM1_END                         ( SRAM1_SIZE + SRAM1_BASE )   
#define SRAM2_END                         ( SRAM2_SIZE + SRAM1_BASE ) 
#define SRAM3_END                         ( CCMDATARAM_BASE + SRAM1_BASE ) 
#define FlASH_END                         ( FLASH_SIZE + SRAM1_BASE )

#define FLASH_MAX_SECTORS_NUMBERS         (12)
#define FLASH_LAST_SECTOR_NUMBER          (11)
#define FLASH_FIRST_SECTOR_NUMBER         (0)

#define CBL_FLASH_MASS_ERASE               (0xFF)
#define HAL_SUCCESSFUL_ERASE               (0xFFFFFFFFU)



/* ------------------ Macros Function Declaration ------------------ */


/* ------------------ Data Type Declaration ------------------------ */
typedef enum 
{
	BL_NACK = 0 ,
	BL_OK
}BL_Status;

typedef void(*pMainApp)(void) ;
typedef void(*JumpPtr)(void) ;

/* ------------------ Interfaces Declaration ----------------------- */
void BL_Print_Message(char *format , ...);
BL_Status BL_UART_Fetch_Host_Command(void);

#endif