#ifndef _WDOG_
#define _WDOG_

//����
void WDog_Init(void);
void WDog_DeInit(void);
void WDog_Feed(void);
bit GetIfWDogCauseRST(void);

#endif
