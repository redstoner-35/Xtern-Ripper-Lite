#ifndef _RampCfg_
#define _RampCfg_

#include "ModeControl.h"

//�洢��������
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

//����
void ReadRampConfig(void);
void SaveRampConfig(bit IsForceSave);	
void RestoreToMinimumRampCurrent(void);	
	
#endif
