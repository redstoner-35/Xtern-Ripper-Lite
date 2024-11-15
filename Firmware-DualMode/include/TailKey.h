#ifndef _TK_
#define _TK_

//内部包含
#include "ModeControl.h"

//尾按参数定义
#define TailKeyRelTime 2 //尾按放开的时间

//外部引用
extern xdata char TailKeyTIM;

//函数
void TailMemory_Recall(void);
void TailMemory_Save(ModeIdxDef Mode);
void TailKey_Init(void); //尾部初始化
void TailKey_Handler(void); //侧按初始化
void TailKeyCounter(void); //计时器
char GetTailKeyCount(void); //获取尾按状态

#endif
