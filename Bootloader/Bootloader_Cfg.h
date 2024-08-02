#ifndef BOOTLOADER_CFG_H
#define BOOTLOADER_CFG_H

/* ------------------ Includes ------------------------------------- */

/* ------------------ Macros Declaration --------------------------- */
#define BL_DEBUG_UARTX   &huart2

#define BL_DEBUG_UART_ENABLE   0x00
#define BL_DEBUG_SPI_ENABLE    0x01
#define BL_DEBUG_CAN_ENABLE    0x02

#define BL_DEBUG_METHOD   (BL_DEBUG_UART_ENABLE)


/* ------------------ Macros Function Declaration ------------------ */


/* ------------------ Data Type Declaration ------------------------ */


/* ------------------ Interfaces Declaration ----------------------- */

#endif