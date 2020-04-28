#include <avr/io.h>
#include <avr/interrupt.h>

#define POWER_CTRL_PORT		PORTA3
#define POWER_CTRL_DDR		DDA3

#define BUTTON_FRONT_PIN	PINA4
#define BUTTON_FRONT_PORT	PORTA4
#define BUTTON_BACK_PIN		PINB2
#define BUTTON_BACK_PORT	PORTB2

#define FRONT_LIGHT_DDR		DDA5
#define FRONT_LIGHT_PORT	PORTA5
#define BACK_LIGHT_DDR		DDA6
#define BACK_LIGHT_PORT		PORTA6

#define FRONT_LIGHT			0
#define BACK_LIGHT			1

#define PWM_FREQ			1000	//1kHz

#define FRONT_LIGHT_ON()	(TCCR1A |= (1 << COM1A1))
#define FRONT_LIGHT_OFF()	(TCCR1A &= ~(1 << COM1A1)), (PORTA &= ~ (1 << FRONT_LIGHT_PORT))

#define BACK_LIGHT_ON()		(TCCR1A |= (1 << COM1B1))
#define BACK_LIGHT_OFF()	(TCCR1A &= ~(1 << COM1B1)), (PORTA &= ~(1 << BACK_LIGHT_PORT))

#define POWER_ON()			(PORTA |= (1 << POWER_CTRL_PORT))
#define POWER_OFF()			(PORTA &= ~(1 << POWER_CTRL_PORT))

#define ADC_ENABLE()		(ADCSRA |= (1 << ADEN))
#define ADC_DISABLE()		(ADCSRA &= ~(1 << ADEN))

#define AIN_ENABLE()		(ACSR &= ~(1 << ACD))
#define AIN_DISABLE()		(ACSR |= (1 << ACD))

#define OFF					0
#define ON					1

volatile uint32_t ms = 0; 
volatile uint16_t f_cnt = 0, b_cnt = 0;

void initSystem()
{
	//////////////////////////////////////////////////////////////////////////
	DDRA |= (1 << POWER_CTRL_DDR);
	PORTA |= (1 << BUTTON_FRONT_PORT);	//enable pullup
	PORTB |= (1 << BUTTON_BACK_PORT);	//enable pullup 
	
	//////////////////////////////////////////////////////////////////////////
	
	//////////////////////////////////////////////////////////////////////////
	ADC_ENABLE();
	ADMUX = 0;
	ADMUX |= (1 << REFS1);
	ADCSRA |= (1 << ADPS1) | (1 << ADPS0);
	DIDR0 |= (1 << ADC0D);
	//////////////////////////////////////////////////////////////////////////
	
	//////////////////////////////////////////////////////////////////////////
	DDRA |= (1 << FRONT_LIGHT_DDR) | (1 << BACK_LIGHT_DDR);
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS10);
	ICR1 = PWM_FREQ;
	//////////////////////////////////////////////////////////////////////////
	
	//////////////////////////////////////////////////////////////////////////
	TCCR0A |= (1 << WGM01);
	TCCR0B |= (1 << CS01);
	OCR0A = 125; // 1ms
	TIMSK0 |= (1 << OCIE0A);
	//////////////////////////////////////////////////////////////////////////
	
	//////////////////////////////////////////////////////////////////////////
	ACSR |= (1 << ACIE) | (1 << ACBG) | (1 << ACIS0) | ( 1 << ACIS1);
	//////////////////////////////////////////////////////////////////////////
	
	sei();	
}

void blinkFrontLight(uint16_t on_time, uint16_t off_time)
{
	static uint8_t last_state = ON;
	
	
	if (last_state == ON)
	{
		if (f_cnt <= off_time)
		{
			FRONT_LIGHT_OFF();
		}
		else
		{
			last_state = OFF;
			f_cnt = 0;
		}
	}
	
	if (last_state == OFF)
	{
		if (f_cnt <= on_time)
		{
			FRONT_LIGHT_ON();
		}
		else
		{
			last_state = ON;
			f_cnt = 0;
		}
	}
}

void blinkBackLight(uint16_t on_time, uint16_t off_time)
{
	static uint8_t last_state = ON;
	
	
	if (last_state == ON)
	{
		if (b_cnt <= off_time)
		{
			BACK_LIGHT_OFF();
		}
		else
		{
			last_state = OFF;
			b_cnt = 0;
		}
	}
	
	if (last_state == OFF)
	{
		if (b_cnt <= on_time)
		{
			BACK_LIGHT_ON();
		}
		else
		{
			last_state = ON;
			b_cnt = 0;
		}
	}
}

ISR(TIM0_COMPA_vect)
{
	ms++;
	f_cnt++;
	b_cnt++;
}

ISR(ANA_COMP_vect)
{
	
	
}

void setFrontPwm(uint16_t val)
{
	if (val >= PWM_FREQ)
		val = PWM_FREQ;
	else if(val < 1)
		val = 0;
		
	OCR1A = val;
}

void setBackPwm(uint16_t val)
{
	if (val >= PWM_FREQ)
	val = PWM_FREQ;
	else if(val < 1)
	val = 0;
	
	OCR1B = val;
}

uint16_t readAdc()
{
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	return ADC;
}

int main(void)
{
	initSystem();
	
	setFrontPwm(1000);
	setBackPwm(1000);
    while(1)
    {
		blinkFrontLight(500, 1000);
		blinkBackLight(1000,500);
        
    }
}