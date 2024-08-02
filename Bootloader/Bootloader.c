#include "Bootloader.h"

static uint8_t Bootloader_CRC_Calculation(uint8_t *data , uint16_t data_len , uint32_t Host_CRC );
static void Bootloader_Send_ACK(uint16_t Reply_Len);
static void Bootloader_Send_NACK(void);
static void Bootloader_Jump_To_User_APP(void);
static uint8_t Bootloader_Address_Verfication(uint32_t Host_Address );
static uint8_t Perform_Flash_Erase(uint8_t Sector_Number , uint8_t Number_Of_Sectors );
static uint8_t FLASH_Write_Payload(uint8_t *Host_Payload , uint32_t Host_Address , uint16_t Payload_Len);
static uint8_t Bootloader_Get_RDB_Level(void);
static uint8_t Bootloader_Change_RDB_Level(uint32_t RDB_Level);


static void Bootloader_Get_Version(uint8_t *Host_Buffer);
static void Bootloader_Get_Help(uint8_t *Host_Buffer);
static void Bootloader_Get_Chip_Identification_Number(uint8_t *Host_Buffer);
static void Bootloader_Read_Protection_Level(uint8_t *Host_Buffer);
static void Bootloader_Jump_To_Address(uint8_t *Host_Buffer);
static void Bootloader_Erase_Flash(uint8_t *Host_Buffer);
static void Bootloader_Memory_Write(uint8_t *Host_Buffer);
static void Bootloader_Enable_RW_Protection(uint8_t *Host_Buffer);
static void Bootloader_Memory_Read(uint8_t *Host_Buffer);
static void Bootloader_Get_Sector_Protection_Status(uint8_t *Host_Buffer);
static void Bootloader_Read_OTP(uint8_t *Host_Buffer);
static void Bootloader_Change_Read_Protection_Level(uint8_t *Host_Buffer);



static uint8_t BL_Host_Buffer[BL_HOST_RECIEVER_BUFFER_SIZE] ;
static uint8_t BL_Supported_CMD[12] ={
		CBL_GET_VER_CMD ,             
		CBL_GET_HELP_CMD ,            
		CBL_GET_CID_CMD   ,          
		CBL_GET_RDP_STATUS_CMD  ,     
		CBL_GO_TO_ADDR_CMD   ,        
		CBL_FLASH_ERASE_CMD  ,        
		CBL_MEM_WRITE_CMD    ,        
		CBL_ED_W_PROTECT_CMD  ,       
		CBL_MEM_READ_CMD      ,       
		CBL_READ_SECTOR_STATUS_CMD ,  
		CBL_OTP_READ_CMD          ,   
		CBL_CHANGE_ROP_Level_CMD     
};


/**********************************************************************************************************************/

void BL_Print_Message(char *format , ...)
{
	char Message[100] = {0} ;
	va_list list ;
	va_start (list , format ) ;
	vsprintf(Message , format , list ) ; 
	#if(BL_DEBUG_METHOD == BL_DEBUG_UART_ENABLE )
	HAL_UART_Transmit(BL_DEBUG_UARTX , (uint8_t *)Message , sizeof(Message) , HAL_MAX_DELAY ); 
	#endif
	va_end(list);
}


BL_Status BL_UART_Fetch_Host_Command(void)
{
	BL_Status ret = BL_OK ;
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	uint8_t Command_Length = 0 ;
	
	/* make values of buffer equal zero */  
	memset(BL_Host_Buffer , 0 , BL_HOST_RECIEVER_BUFFER_SIZE ) ;
	/* recieving first byte only to know length of command */
	HAL_Status = HAL_UART_Receive(BL_DEBUG_UARTX , BL_Host_Buffer , 1 , HAL_MAX_DELAY ) ;
	if( HAL_Status == HAL_OK )
	{
		Command_Length = BL_Host_Buffer[0] ;
		/* recieveing rest of command */
		HAL_Status = HAL_UART_Receive(BL_DEBUG_UARTX , &BL_Host_Buffer[1] , Command_Length  , HAL_MAX_DELAY ) ;
		/* Doing command details */ 
		if( HAL_Status == HAL_OK )
		{
			switch(BL_Host_Buffer[1])
			{
				case CBL_GET_VER_CMD :
								Bootloader_Get_Version(BL_Host_Buffer);
					break;
				case CBL_GET_HELP_CMD :
								Bootloader_Get_Help(BL_Host_Buffer);
					break;
				case CBL_GET_CID_CMD :
								Bootloader_Get_Chip_Identification_Number(BL_Host_Buffer);
					break;
				case CBL_GET_RDP_STATUS_CMD :
								Bootloader_Read_Protection_Level(BL_Host_Buffer);
					break;
				case CBL_GO_TO_ADDR_CMD :
								Bootloader_Jump_To_Address(BL_Host_Buffer) ;
					break;
				case CBL_FLASH_ERASE_CMD :
								Bootloader_Erase_Flash(BL_Host_Buffer);
					break;
				case CBL_MEM_WRITE_CMD :
								Bootloader_Memory_Write(BL_Host_Buffer);
					break;
				case CBL_ED_W_PROTECT_CMD :
					
					break;
				case CBL_MEM_READ_CMD :
					
					break;
				case CBL_READ_SECTOR_STATUS_CMD :
					
					break;
				case CBL_OTP_READ_CMD :
					
					break;
				case CBL_CHANGE_ROP_Level_CMD :
								Bootloader_Change_Read_Protection_Level(BL_Host_Buffer);
					break;
			}
		}
		else{
			ret = BL_NACK ;
		}
	}
	else{
		ret = BL_NACK ;
	}
	return ret ;
}

/***********************************************************************************************************************/


static uint8_t Bootloader_CRC_Calculation(uint8_t *data , uint16_t data_len , uint32_t Host_CRC )
{
	uint8_t CRC_Status = CRC_NOK , counter = 0 ;
	uint32_t CRC_Value = 0 , data_buffer = 0 ;
	
	for( counter = 0 ; counter <= data_len ; counter++ )
	{
		data_buffer = (uint32_t)data[counter] ;
		CRC_Value = HAL_CRC_Accumulate(&hcrc , &data_buffer , 1 ) ;
	}
	
	__HAL_CRC_DR_RESET(&hcrc);
	
	if( CRC_Value == Host_CRC )
	{
		CRC_Status = CRC_OK ;
	}
	else{
		CRC_Status = CRC_NOK ;
	}
	return CRC_Status ;
}

static void Bootloader_Send_ACK(uint16_t Reply_Len)
{
	uint8_t Reply_data[2] = {0} ;
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	
	Reply_data[0] = CBL_REPLY_ACK ;
	Reply_data[1] = Reply_Len ;
	
	HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , Reply_data , 2 , HAL_MAX_DELAY ) ;
}

static void Bootloader_Send_NACK(void)
{
	uint8_t Reply_data[1] = {0} ;
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;

	Reply_data[0] = CBL_REPLY_NACK ;
	
	HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , Reply_data , 1 , HAL_MAX_DELAY ) ;

}


static void Bootloader_Jump_To_User_APP(void)
{
	/* MSP is the first 4 byte of vector table which is the first address of flash */
	uint32_t MSP_Value  =  *((uint32_t *)(FLASH_SECTOR2_START_ADDRESS))  ;
	/* Reset Handler function is the first address of application */
	uint32_t MainApp_Address =  *((uint32_t *)(FLASH_SECTOR2_START_ADDRESS + 4 )) ;
	/* save address of reset handler at pointer to function to call it */
	pMainApp  Reset_Handler_Address = (pMainApp)MainApp_Address ;
	/* Set main MSP */ 
	__set_MSP(MSP_Value) ;
	/* Deintialize of modules */
	HAL_RCC_DeInit();
	/* Jump to application */
	Reset_Handler_Address();
}

static uint8_t Bootloader_Address_Verfication(uint32_t Host_Address )
{
	uint8_t Address_Verfication = CBL_ADDRESS_NOT_VALID ; ;
	
	if( (Host_Address >= SRAM1_BASE )  &&  (Host_Address <= SRAM1_END ) ) 
	{
		Address_Verfication = CBL_ADDRESS_VALID ;
	}
	else if( (Host_Address >= SRAM2_BASE )  &&  (Host_Address <= SRAM2_END ) )
	{
		Address_Verfication = CBL_ADDRESS_VALID ;
	}
	else if( (Host_Address >= CCMDATARAM_BASE )  &&  (Host_Address <= SRAM3_END ) )
	{
		Address_Verfication = CBL_ADDRESS_VALID ;
	}
	else if( (Host_Address >= FLASH_BASE )  &&  (Host_Address <= FlASH_END ) )
	{
		Address_Verfication = CBL_ADDRESS_VALID ;
	}
	else{
		Address_Verfication = CBL_ADDRESS_NOT_VALID ; 
	}
	
	return Address_Verfication ;
}


static uint8_t Perform_Flash_Erase(uint8_t Sector_Number , uint8_t Number_Of_Sectors )
{
	uint8_t Erase_Status = CBL_ERASE_FAILED ;
	uint8_t Remaining_Sectors = 0 ;
	FLASH_EraseInitTypeDef pEraseInit ;
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	uint32_t Sector_Error = 0 ; 
	
	if( Number_Of_Sectors > FLASH_MAX_SECTORS_NUMBERS )
	{
		Erase_Status = CBL_ERASE_FAILED ;
	}
	else{
		if( ( Sector_Number <= FLASH_LAST_SECTOR_NUMBER )  ||  ( Sector_Number == CBL_FLASH_MASS_ERASE ) )
		{
			if( Sector_Number == CBL_FLASH_MASS_ERASE )
			{
				pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE ;
				pEraseInit.NbSectors = Number_Of_Sectors ;
			}
			else
			{
				/*  Force Number of sectors to finish at 11 */ 
				Remaining_Sectors = FLASH_MAX_SECTORS_NUMBERS - Sector_Number ;
				
				/* Configure Data Type */
				if( Number_Of_Sectors > Remaining_Sectors  )
				{
					Number_Of_Sectors = Remaining_Sectors ;
				}
				
				pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS ;
				pEraseInit.NbSectors = Number_Of_Sectors ;
				pEraseInit.Sector = Sector_Number ;
			}
			pEraseInit.Banks = FLASH_BANK_1 ;
			pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3 ;
			
			/* Unlock Flash */ 
			HAL_Status = HAL_FLASH_Unlock() ;
			
			/* Start Erasing */ 
			HAL_Status = HAL_FLASHEx_Erase(&pEraseInit , &Sector_Error ) ; 
			
			if( Sector_Error == HAL_SUCCESSFUL_ERASE )
			{
				Erase_Status = CBL_ERASE_SUCESSED ;
			}
			else{
				Erase_Status = CBL_ERASE_FAILED ;
			}	
			/* Lock Flash */
			HAL_Status = HAL_FLASH_Lock() ;
		}
		else{
			Erase_Status = CBL_ERASE_FAILED ;
		}
	}
	return Erase_Status ;
}


static uint8_t FLASH_Write_Payload(uint8_t *Host_Payload , uint32_t Host_Address , uint16_t Payload_Len)
{
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	uint16_t counter = 0 ;
	uint8_t Write_Payload_Status = 0 ;
	
	/* Unlock Flash */ 
	HAL_Status = HAL_FLASH_Unlock() ;
	
	if( HAL_Status != HAL_OK )
	{
		Write_Payload_Status = CBL_FLASH_WRITE_FAILED ;
	}
	else{
		/* Looping For Payload byte by byte */
		for( counter = 0 ; counter <= Payload_Len ; counter++ )
		{
		/* Write Payload */
		HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE , Host_Address + counter , Host_Payload[counter] ) ;
		if( HAL_Status != HAL_OK )
		{
			Write_Payload_Status = CBL_FLASH_WRITE_FAILED ;
			break ;
		}
		else{
			Write_Payload_Status = CBL_FLASH_WRITE_SUCESSED ;
		}	
	}
	}
	/* Lock Flash */
	HAL_Status = HAL_FLASH_Lock() ;
	
	return Write_Payload_Status ;
	
}

static uint8_t Bootloader_Get_RDB_Level(void)
{
	FLASH_OBProgramInitTypeDef pOBInit ;
	
	HAL_FLASHEx_OBGetConfig(&pOBInit);

	
	return (uint8_t)(pOBInit.RDPLevel) ;
}

static uint8_t Bootloader_Change_RDB_Level(uint32_t RDB_Level)
{
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	uint8_t Change_RDB_Status = 0 ;
	FLASH_OBProgramInitTypeDef pOBInit ;
	
	/* Unlock Flash OB */ 
	HAL_Status = HAL_FLASH_OB_Unlock() ;
	
	if( HAL_Status != HAL_OK )
	{
		Change_RDB_Status = CBL_CHANGE_RDB_FAILED ;
	}
	else{
		/* Change RDB Level */
		pOBInit.OptionType = OPTIONBYTE_RDP ;
		pOBInit.Banks = FLASH_BANK_1 ;
		pOBInit.RDPLevel = (uint32_t)RDB_Level ;
		
		HAL_Status = HAL_FLASHEx_OBProgram(&pOBInit) ;
		
		if( HAL_Status != HAL_OK )
		{
			Change_RDB_Status = CBL_CHANGE_RDB_FAILED ;
		}
		else{
			/* Launch Flash OB */
			HAL_Status = HAL_FLASH_OB_Launch() ;
			
			if( HAL_Status != HAL_OK )
			{
				Change_RDB_Status = CBL_CHANGE_RDB_FAILED ;
			}
			else{
				Change_RDB_Status = CBL_CHANGE_RDB_SUCESSED ;
			}
		}
	}	
	/* Lock Flash OB  */
	HAL_Status = HAL_FLASH_OB_Lock() ;
	
	return Change_RDB_Status ;
}

/***************************************************************************************************************************/
static void Bootloader_Get_Version(uint8_t *Host_Buffer)
{
	uint8_t BL_Version[4] =  {CBL_GET_VER_VENDOR_ID,CBL_GET_VER_MAJOR_VERSION,CBL_GET_VER_MINOR_VERSION,CBL_GET_VER_PATCH_VERSION } ;
	
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	uint16_t Host_CMD_Length = 0 ;
	uint32_t Host_CRC = 0 ;
	uint8_t ACK = 0 ;
	
 /* Save CRC of the command to compare it with CRC that I will calculate */ 	
  Host_CMD_Length = Host_Buffer[0] + 1 ;
	Host_CRC = *( ( Host_Buffer + Host_CMD_Length ) - CRC_SIZE_BYTE ) ;
	
	/* CRC Verfication */ 
	ACK = Bootloader_CRC_Calculation(Host_Buffer , Host_CMD_Length - CRC_SIZE_BYTE , Host_CRC ) ;
	
	
	/* Send Reply to Host */
	if( ACK == CRC_OK )
	{
		Bootloader_Send_ACK(4);
		
		/* Send detail of command */ 
		HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , BL_Version , 4 , HAL_MAX_DELAY ) ;
	}
	else if( ACK == CRC_NOK )
	{
		Bootloader_Send_NACK();
	}
	
}




static void Bootloader_Get_Help(uint8_t *Host_Buffer)
{
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	uint16_t Host_CMD_Length = 0 ;
	uint32_t Host_CRC = 0 ;
	uint8_t ACK = 0 ;
	
 /* Save CRC of the command to compare it with CRC that I will calculate */ 	
  Host_CMD_Length = Host_Buffer[0] + 1 ;
	Host_CRC = *( ( Host_Buffer + Host_CMD_Length ) - CRC_SIZE_BYTE ) ;
	
	/* CRC Verfication */ 
	ACK = Bootloader_CRC_Calculation(Host_Buffer , Host_CMD_Length - CRC_SIZE_BYTE , Host_CRC ) ;
	
	
	/* Send Reply to Host */
	if( ACK == CRC_OK )
	{
		Bootloader_Send_ACK(12);
		
		/* Send detail of command */ 
		HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , BL_Supported_CMD , 4 , HAL_MAX_DELAY ) ;
	}
	else if( ACK == CRC_NOK )
	{
		Bootloader_Send_NACK();
	}
}



static void Bootloader_Get_Chip_Identification_Number(uint8_t *Host_Buffer)
{

	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	uint16_t Host_CMD_Length = 0 ;
	uint16_t ID_Number = 0 ;
	uint32_t Host_CRC = 0 ;
	uint8_t ACK = 0 ;
	
 /* Save CRC of the command to compare it with CRC that I will calculate */ 	
  Host_CMD_Length = Host_Buffer[0] + 1 ;
	Host_CRC = *( ( Host_Buffer + Host_CMD_Length ) - CRC_SIZE_BYTE ) ;
	
	/* CRC Verfication */ 
	ACK = Bootloader_CRC_Calculation(Host_Buffer , Host_CMD_Length - CRC_SIZE_BYTE , Host_CRC ) ;
	
	
	/* Send Reply to Host */
	if( ACK == CRC_OK )
	{
		/* GET ID number */ 
		
		ID_Number = (uint16_t )( DBGMCU->IDCODE  & 0xFFF ) ;
		
		/* Send detail of command */ 
		Bootloader_Send_ACK(2);
		
		HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , (uint8_t *)&ID_Number , 2 , HAL_MAX_DELAY ) ;
	}
	else if( ACK == CRC_NOK )
	{
		Bootloader_Send_NACK();
	}
	
}



static void Bootloader_Read_Protection_Level(uint8_t *Host_Buffer)
{
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	uint16_t Host_CMD_Length = 0 ;
	uint32_t Host_CRC = 0 ;
	uint8_t ACK = 0 ;
	uint8_t RDB_Level = 0 ;
	uint8_t Get_RDB_Status = 0 ;
	
 /* Save CRC of the command to compare it with CRC that I will calculate */ 	
  Host_CMD_Length = Host_Buffer[0] + 1 ;
	Host_CRC = *( ( Host_Buffer + Host_CMD_Length ) - CRC_SIZE_BYTE ) ;
	
	/* CRC Verfication */ 
	ACK = Bootloader_CRC_Calculation(Host_Buffer , Host_CMD_Length - CRC_SIZE_BYTE , Host_CRC ) ;
	
	
	/* Send Reply to Host */
	if( ACK == CRC_OK )
	{		
		Bootloader_Send_ACK(1);
		
	  /* Read protection Level */ 
		RDB_Level = Bootloader_Get_RDB_Level() ;
		
		/* Send detail of command */
		HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , (uint8_t *)&RDB_Level , 1 , HAL_MAX_DELAY ) ;
	}
	else if( ACK == CRC_NOK )
	{
		Bootloader_Send_NACK();
	}
}

static void Bootloader_Jump_To_Address(uint8_t *Host_Buffer)
{
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	uint16_t Host_CMD_Length = 0 ;
	uint32_t Host_CRC = 0 ;
	uint32_t Host_Jump_Address = 0 ;
	uint8_t ACK = 0 ;
	uint8_t Address_Verfication = 0 ;
	
 /* Save CRC of the command to compare it with CRC that I will calculate */ 	
  Host_CMD_Length = Host_Buffer[0] + 1 ;
	Host_CRC = *( ( Host_Buffer + Host_CMD_Length ) - CRC_SIZE_BYTE ) ;
	
	/* CRC Verfication */ 
	ACK = Bootloader_CRC_Calculation(Host_Buffer , Host_CMD_Length - CRC_SIZE_BYTE , Host_CRC ) ;
	
	
	/* Send Reply to Host */
	if( ACK == CRC_OK )
	{
		Bootloader_Send_ACK(1);
		
		/* Extract Jump Address */
		Host_Jump_Address = *((uint32_t *)&Host_Buffer[2]) ;
		
		/* Adress Verfication */
		Address_Verfication = Bootloader_Address_Verfication(Host_Jump_Address);
		
		if( Address_Verfication == CBL_ADDRESS_VALID )
		{
			/* Send detail of command that address is valid */
			HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , &Address_Verfication , 1 , HAL_MAX_DELAY ) ;
		}
		else{
			/* Send detail of command that address is not valid */
			HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , &Address_Verfication , 1 , HAL_MAX_DELAY ) ;
			/* Jump to Address */
			JumpPtr Jump_Address = (JumpPtr)Host_Jump_Address ;
			Jump_Address();
		}
				
	}
	else if( ACK == CRC_NOK )
	{
		Bootloader_Send_NACK();
	}
	
}

static void Bootloader_Erase_Flash(uint8_t *Host_Buffer)
{
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	uint16_t Host_CMD_Length = 0 ;
	uint32_t Host_CRC = 0 ;
	uint8_t ACK = 0 ;
	uint8_t Erase_Status = 0 ;
	
 /* Save CRC of the command to compare it with CRC that I will calculate */ 	
  Host_CMD_Length = Host_Buffer[0] + 1 ;
	Host_CRC = *( ( Host_Buffer + Host_CMD_Length ) - CRC_SIZE_BYTE ) ;
	
	/* CRC Verfication */ 
	ACK = Bootloader_CRC_Calculation(Host_Buffer , Host_CMD_Length - CRC_SIZE_BYTE , Host_CRC ) ;
	
	
	/* Send Reply to Host */
	if( ACK == CRC_OK )
	{
		Bootloader_Send_ACK(1);
		
		/* Erase Flash */
		
		Erase_Status = Perform_Flash_Erase( Host_Buffer[2] , Host_Buffer[3] ) ;
		
		if( Erase_Status == CBL_ERASE_SUCESSED )
		{	
			/* Send detail of command that Erase is done  */
			HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , &Erase_Status , 1 , HAL_MAX_DELAY ) ;
		}
		else if( Erase_Status == CBL_ERASE_FAILED )
		{
			/* Send detail of command that erase is failed */
			HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , &Erase_Status , 1 , HAL_MAX_DELAY ) ;
		}
	}
	else if( ACK == CRC_NOK )
	{
		Bootloader_Send_NACK();
	}
	
	
}

static void Bootloader_Memory_Write(uint8_t *Host_Buffer)
{	
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	uint16_t Host_CMD_Length = 0 ;
	uint32_t Host_CRC = 0 ;
	uint8_t ACK = 0 ;
	uint32_t Host_Address = 0 ;
	uint8_t Payload_Len = 0 ;
	uint8_t Address_Verfication = 0 ;
	uint8_t Write_Payload_Status = 0 ;
	
 /* Save CRC of the command to compare it with CRC that I will calculate */ 	
  Host_CMD_Length = Host_Buffer[0] + 1 ;
	Host_CRC = *( ( Host_Buffer + Host_CMD_Length ) - CRC_SIZE_BYTE ) ;
	
	/* CRC Verfication */ 
	ACK = Bootloader_CRC_Calculation(Host_Buffer , Host_CMD_Length - CRC_SIZE_BYTE , Host_CRC ) ;
	
	
	/* Send Reply to Host */
	if( ACK == CRC_OK )
	{
		Bootloader_Send_ACK(4);
		
		/* Extract Host address and Payload length */
		Host_Address = *((uint32_t *)&Host_Buffer[2]) ;
		Payload_Len = Host_Buffer[6] ;
		
		/* Adress Verfication */
		Address_Verfication = Bootloader_Address_Verfication(Host_Address);
		
		if( Address_Verfication == CBL_ADDRESS_VALID )
		{
			/* Write data at memory */
			Write_Payload_Status = FLASH_Write_Payload(&Host_Buffer[7] , Host_Address , (uint16_t)Payload_Len ) ;
			
			if( Write_Payload_Status == CBL_FLASH_WRITE_SUCESSED )
			{
				/* Send detail of command that data has been written */
				HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , &Write_Payload_Status , 1 , HAL_MAX_DELAY ) ;
			}
			else{
				/* Send detail of command that data has not been written */
				HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , &Write_Payload_Status , 1 , HAL_MAX_DELAY ) ;
			}
		}
		else{
			/* Send detail of command that address is not valid */
			HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , &Address_Verfication , 1 , HAL_MAX_DELAY ) ;
		}
	}
	else if( ACK == CRC_NOK )
	{
		Bootloader_Send_NACK();
	}
}

static void Bootloader_Enable_RW_Protection(uint8_t *Host_Buffer)
{
}

static void Bootloader_Memory_Read(uint8_t *Host_Buffer)
{
}

static void Bootloader_Get_Sector_Protection_Status(uint8_t *Host_Buffer)
{
}

static void Bootloader_Read_OTP(uint8_t *Host_Buffer)
{
}

static void Bootloader_Change_Read_Protection_Level(uint8_t *Host_Buffer)
{
	HAL_StatusTypeDef  HAL_Status = HAL_OK ;
	
	uint16_t Host_CMD_Length = 0 ;
	uint32_t Host_CRC = 0 ;
	uint8_t ACK = 0 ;
	uint32_t RDB_Level = 0 ;
	uint8_t Get_RDB_Status = 0 ;
	
 /* Save CRC of the command to compare it with CRC that I will calculate */ 	
  Host_CMD_Length = Host_Buffer[0] + 1 ;
	Host_CRC = *( ( Host_Buffer + Host_CMD_Length ) - CRC_SIZE_BYTE ) ;
	
	/* CRC Verfication */ 
	ACK = Bootloader_CRC_Calculation(Host_Buffer , Host_CMD_Length - CRC_SIZE_BYTE , Host_CRC ) ;
	
	
	/* Send Reply to Host */
	if( ACK == CRC_OK )
	{		
		Bootloader_Send_ACK(1);
		
		/* Get RDB Level */
		RDB_Level = Host_Buffer[2] ;
		
		if( RDB_Level == 0 )
		{
			RDB_Level = 0xAA ;
		}
		else if( RDB_Level == 1 )
		{
			RDB_Level = 0x55 ;
		}
		else if( RDB_Level == 2 )
		{
			RDB_Level = 0xCC ;
		}
		
	  /* Change protection Level */ 
		Get_RDB_Status = Bootloader_Change_RDB_Level(RDB_Level) ;
		
		if( Get_RDB_Status == CBL_GET_RDB_SUCESSED )
		{
			/* Send detail of command that changing RDB level successed */
			HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , (uint8_t *)&Get_RDB_Status , 1 , HAL_MAX_DELAY ) ;
		}
		else{
				/* Send detail of command that changing RDB level failed */
				HAL_Status = HAL_UART_Transmit(BL_DEBUG_UARTX , (uint8_t *)&Get_RDB_Status , 1 , HAL_MAX_DELAY ) ;
		}
		
	
	}
	else if( ACK == CRC_NOK )
	{
		Bootloader_Send_NACK();
	}
}
