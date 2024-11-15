#include "cms8s6990.h"
#include "Watchdog.h"

//���Ź���ʼ��
#pragma OT(0)      //�ⲿ�ִ����ʱ��������Ҫ����0�����Ż�
void WDog_Init(void)
	{
	unsigned char buf;
	//���ÿ��Ź����ʱ��Ϊ48MHz/2^24=0.3495S
	buf=CKCON&0x1F;
	buf|=0xC0;  //��WTS[2:0]=110,ѡ��2^24��Ƶ��
	CKCON = buf;
	//�������Ź�
	EA = 0;			//�ر��ж�
	_nop_();
	TA = 0xAA;
	TA = 0x55;
	WDCON |= 0x02; //��WDTRE=1�����Ź�����
	_nop_();
	EA = 1;  //���´��ж�
	}

//�رտ��Ź�
#pragma OT(0)      //�ⲿ�ִ����ʱ��������Ҫ����0�����Ż�
void WDog_DeInit(void)
	{
	EA = 0;			//�ر��ж�
	_nop_();
	TA = 0xAA;
	TA = 0x55;
	WDCON &= 0xFD; //��WDTRE=0�����Ź���ֹ
	_nop_();
	EA = 1;  //���´��ж�
	}
	
//ι��
#pragma OT(0)      //�ⲿ�ִ����ʱ��������Ҫ����0�����Ż�
void WDog_Feed(void)
	{
	EA = 0;			//�ر��ж�
	_nop_();
	TA = 0xAA;
	TA = 0x55;
	WDCON |= 0x01; //��WDTCLR=1���������
	_nop_();
	EA = 1;  //���´��ж�
	}
	
//��ȡ�Ƿ��ǿ��Ź����µĸ�λ
#pragma OT(0)      //�ⲿ�ִ����ʱ��������Ҫ����0�����Ż�
bit GetIfWDogCauseRST(void)
	{
	unsigned char buf;
	//��ʼ��	
	EA = 0;			//�ر��ж�
	_nop_();
	TA = 0xAA;
	TA = 0x55;
	buf=WDCON; //��ȡ���Ź��Ĵ���
	_nop_();
	EA = 1;  //���´��ж�
	//���ؽ��
	return buf&0x04?1:0;
	}
