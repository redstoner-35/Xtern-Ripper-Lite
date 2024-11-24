#ifndef Strap_
#define Strap_

//外部参考
extern xdata char VbattCellCount; //系统的电池节数
extern xdata int TurboCurrent; //极亮模式下的电流
extern bit Is6VLED; //是否为6V LED

//函数
void Strap_Init(void);

#endif
