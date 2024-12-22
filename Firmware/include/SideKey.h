#ifndef _SideKey_
#define _SideKey_

//按键检测延时(每个单位=0.125秒)
#define LongPressTime 5 //长按按键检测延时(按下时间超过这个数值则判定为长按)
#define ContShortPressWindow 4 //连续多次按下时侧按的检测释抑时间(在该时间以内按下的短按才算入短按次数内)
#define KeyReleaseDetectMask 0xFF //按键按下的监测Mask

//按键处理宏定义
#define ClearKeyIntFlag() P2EXTIF=0 //清除GPIO Flag

typedef enum
{
HoldEvent_None=0,
HoldEvent_H=1, //长按
HoldEvent_1H=2, //单击+长按
HoldEvent_2H //双击+长按
}HoldEventDef;

//按键事件结构体定义
typedef struct
{
char LongPressDetected;
char ShortPressCount;
char ShortPressEvent;
HoldEventDef HoldStat;
}KeyEventStrDef;

//函数
void SideKeyInit(void);
int getSideKeyShortPressCount(bit IsRemoveResult);//获取侧按按键的单击和连击次数
bit getSideKeyLongPressEvent(void);//获得侧按按钮长按的事件
bit getSideKeyHoldEvent(void);//获得侧按按钮一直按住的事件
bit IsKeyEventOccurred(void); //检测是否有事件发生
char getSideKeyNClickAndHoldEvent(void); //获取侧按按下N次+长按的按键数
void MarkAsKeyPressed(void); //标记按键按下
bit getSideKey1HEvent(void); //获取侧按按键单击

//回调处理
void SideKey_Int_Callback(void);//侧按中断的处理
void SideKey_TIM_Callback(void);//连按检测计时的回调处理
void SideKey_LogicHandler(void);//逻辑处理

#endif
