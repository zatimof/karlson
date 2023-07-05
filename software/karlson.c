#include <iom8.h>
#include <intrinsics.h>
#define START 600		//напряжение включения вентилятора, 6,00В
#define UOP 720			//напряжение, при котором снята хар-ка, UOP/50=14,4В
#define BR 100			//яркость в ночном режиме 0-255

//functions declare;
void initialize(void);		//функция инициализации
//char read(void);

//variables declare
unsigned char sgt[21]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x83,0xc6,0xa1,0x86,0x8e,0xff,0x9c,0x87,0xc1,0xbf};	//коды знакогенератора
unsigned char buffer[4]={4,3,2,1};	//буфер экрана(4 символа) 
unsigned char point=0,dp=5,lvl=0,cnt0,cnt1,cnt2;
long adcv,adct,u,t,start,eestart;

//main function
int main(void)
{
	initialize();		//инициализация
	
	while(1)
	{
		u=adcv;
		u=u+u-(u>>5)-(u>>6);
				
		cnt0=0;
		cnt1=0;
		cnt2=0;
		
		dp=1;
		while(u>999)
		{	
			u-=1000;
			cnt2++;
		}
		while(u>99)
		{	
			u-=100;
			cnt1++;
		}
		while(u>9)
		{	
			u-=10;
			cnt0++;
		}
		if(u>4)
			if(cnt0!=9)
				cnt0++;
			else
			{
				cnt0=0;
				cnt1++;
				if(cnt1==10)
				{
					cnt1=0;
					cnt2++;
				}
			}
		
		buffer[0]=cnt0;
		buffer[1]=cnt1;
		buffer[2]=cnt2;
			
		start=eestart*adcv/UOP;
		t=adct;
		if((t<start)&&(t>start-200))
		{
			if(t<(start-lvl-1))
				lvl++;
			if(t>(start-lvl+1))
				lvl--;
			if(lvl>127)
				lvl=127;
			if(lvl==255)
				lvl=0;
		}
		else
			lvl=0;
		
		if(lvl>7)
			buffer[3]=(lvl>>3);
		else
			buffer[3]=19;
			
		OCR2=(lvl<<1);
			
		DDRB=0x07;
		__delay_cycles(10);
		if(PINB&0x40)
			OCR1BL=BR;
		else
			OCR1BL=0;
		DDRB=0x47;
		PORTB^=0x40;
		
		if(!(PINB&0x80))
		{
			buffer[0]=16;
			buffer[1]=16;
			buffer[2]=16;
			buffer[3]=16;
			dp=5;
			
			__delay_cycles(100000);
			if(!(PINB&0x80))
			{
				PORTB^=0x40;
				eestart=adct*UOP/adcv;
				
				while(EECR&0x02);
	  		EEAR=3;
	  		EEDR=(eestart&0x000000ff);
	  		cnt0=EEDR;
				EECR|=0x04;
	  		EECR|=0x02;
		
				while(EECR&0x02);
	  		EEAR=2;
	  		EEDR=((eestart>>8)&0x000000ff);
	  		cnt0+=EEDR;
				EECR|=0x04;
	  		EECR|=0x02;
	
				while(EECR&0x02);
	  		EEAR=1;
	  		EEDR=((eestart>>16)&0x000000ff);
	  		cnt0+=EEDR;
				EECR|=0x04;
	  		EECR|=0x02;
		
				while(EECR&0x02);
	  		EEAR=0;
	  		EEDR=((eestart>>24)&0x000000ff);
	  		cnt0+=EEDR;
				EECR|=0x04;
	  		EECR|=0x02;
		
				while(EECR&0x02);
	  		EEAR=4;
	  		EEDR=cnt0;
				EECR|=0x04;
	  		EECR|=0x02;
			
				PORTB^=0x40;
			}
		}
		__delay_cycles(1000000);
	}
}

//initialize function
void initialize(void)	//функция инициализации
{
	//init ports

  DDRD=0xff;	//порт D на выход
	PORTD=0xff;
	
	DDRC=0x0f;	//порт C на вход, кроме PC0-PC3
	PORTC=0x0f;
	
	DDRB=0x47;	//PB0-PB2, PB6 на выход, остальные на вход. 
	PORTB=0x00;
	
	//init timer indicator	
	TCCR1A=0;
	TCCR1B=0x0b;
	OCR1AH=0;
	OCR1AL=255;
	OCR1BH=0;
	OCR1BL=0;
	ICR1H=0;
	ICR1L=0;
	
	//init timer 2
	TCCR2=0x4a;
	OCR2=0x00;
	ASSR=0;
	
	//init ADC
	ADMUX=0x44;
	ADCSR=0xef;
  
	//чтение EEPROM
	EEAR=0;
	while(EECR&0x02);
	EECR|=0x01;
	cnt0=EEDR;
	eestart=(((long)EEDR)<<24);
	
	EEAR=1;
	while(EECR&0x02);
	EECR|=0x01;
	cnt0+=EEDR;
	eestart|=(((long)EEDR)<<16);
	  
	EEAR=2;
	while(EECR&0x02);
	EECR|=0x01;
	cnt0+=EEDR;
	eestart|=(((long)EEDR)<<8);
	
	EEAR=3;
	while(EECR&0x02);
	EECR|=0x01;
	cnt0+=EEDR;
	eestart|=EEDR;
	
	EEAR=4;
	while(EECR&0x02);
	EECR|=0x01;
	if(cnt0!=EEDR)
	{
		eestart=START;
		
		while(EECR&0x02);
	  EEAR=3;
	  EEDR=(eestart&0x000000ff);
	  cnt0=EEDR;
		EECR|=0x04;
	  EECR|=0x02;
		
		while(EECR&0x02);
	  EEAR=2;
	  EEDR=((eestart>>8)&0x000000ff);
	  cnt0+=EEDR;
		EECR|=0x04;
	  EECR|=0x02;
	
		while(EECR&0x02);
	  EEAR=1;
	  EEDR=((eestart>>16)&0x000000ff);
	  cnt0+=EEDR;
		EECR|=0x04;
	  EECR|=0x02;
		
		while(EECR&0x02);
	  EEAR=0;
	  EEDR=((eestart>>24)&0x000000ff);
	  cnt0+=EEDR;
		EECR|=0x04;
	  EECR|=0x02;
		
		while(EECR&0x02);
	  EEAR=4;
	  EEDR=cnt0;
		EECR|=0x04;
	  EECR|=0x02;
	}

	//init interrupt
	TIMSK=0xd8;
	SREG|=0x80;	//прерывания разрешены
	
	return;
}

//interrupts functions
#pragma vector=TIMER1_COMPA_vect
__interrupt void timer1a(void)
{
  PORTD=0xff;		//гасим символ
	point++;
	point&=0x03;
	PORTC=(0xf7>>point)&0x0F;		//сдвигаем разряд
	PORTD=sgt[buffer[point]];	//вывод нового символа
	if(point==dp)
		PORTD&=0x7f;
	ADCSR&=0x3f;
	ADMUX^=0x01;
	ADCSR|=0xc0;
	return;
}

#pragma vector=TIMER1_COMPB_vect
__interrupt void timer1b(void)
{
  if(OCR1BL)
		PORTD=0xff;		//гасим символ
	return;
}

#pragma vector=ADC_vect
__interrupt void adcp(void)
{
	long adc;
		
	adc=ADCL;
	adc|=ADCH<<8;
	//adc=adcr;
	if(ADMUX&0x01)
		adcv+=(adc-adcv)>>2;
	else
		adct+=(adc-adct)>>3;
	return;
}

#pragma vector=TIMER2_COMP_vect
__interrupt void timer2c(void)
{
	if(OCR2<254)
		PORTB&=0xf8;	
	return;
}

#pragma vector=TIMER2_OVF_vect
__interrupt void timer2o(void)
{
	if(OCR2>15)
		PORTB|=0x07;
	return;
}
