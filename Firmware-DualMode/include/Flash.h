#ifndef _FSH_
#define _FSH_

typedef enum
	{
	Flash_Read=0x11,
	Flash_Write=0x19,
	Flash_Erase=0x1D
	}FlashOperationDef;

//º¯Êý
void SetFlashState(bit IsUnlocked);
void Flash_Operation(FlashOperationDef Operation,int ADDR,char *Data);
	
#endif