#ifndef _RampCfg_
#define _RampCfg_

#include "ModeControl.h"

//存储类型声明
typedef struct
	{
	int RampCurrent;
	int SideLEDBrightness;
	bool IsRampEnabled;
	MoonLightBrightnessDef MoonCfg;
	}RampStorDef;
	
typedef union
	{
	RampStorDef Data;
	char ByteBuf[sizeof(RampStorDef)];
	}RampDataUnion;

typedef struct
	{
	RampDataUnion RampConfig;
	char CheckSum;
	}RampROMImageDef;

typedef union
	{
	RampROMImageDef Data;
	char ByteBuf[sizeof(RampROMImageDef)];
	}RampROMImg;

//函数
void ReadRampConfig(void);
void SaveRampConfig(bit IsForceSave);	
void RestoreToMinimumRampCurrent(void);	
	
#endif
