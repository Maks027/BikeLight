#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define POWER_CTRL_PORT		PORTA3
#define POWER_CTRL_DDR		DDA3

#define BUTTON_BACK_PIN		PINA4
#define BUTTON_BACK_PORT	PORTA4
#define BUTTON_FRONT_PIN	PINB2
#define BUTTON_FRONT_PORT	PORTB2

#define FRONT_BUTTON_PRESSED	(!(PINB & (1 << BUTTON_FRONT_PIN)))
#define FRONT_BUTTON_RELEASED	(PINB & (1 << BUTTON_FRONT_PIN))

#define BACK_BUTTON_PRESSED		(!(PINA & (1 << BUTTON_BACK_PIN)))
#define BACK_BUTTON_RELEASED	(PINA & (1 << BUTTON_BACK_PIN))

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

#define FRONT_LIGHT_STATE	(TCCR1A & (1 << COM1A1))
#define BACK_LIGHT_STATE	(TCCR1A & (1 << COM1B1))

#define POWER_ON()			(PORTA |= (1 << POWER_CTRL_PORT))
#define POWER_OFF()			(PORTA &= ~(1 << POWER_CTRL_PORT))

#define ADC_ENABLE()		(ADCSRA |= (1 << ADEN))
#define ADC_DISABLE()		(ADCSRA &= ~(1 << ADEN))

#define AIN_ENABLE()		(ACSR &= ~(1 << ACD))
#define AIN_DISABLE()		(ACSR |= (1 << ACD))

#define ENABLE_INT0()		(GIMSK |= (1 << INT0))
#define DISABLE_INT0()		(GIMSK &= ~(1 << INT0))



#define OFF					0
#define ON					1

#define	SET					1
#define RESET				0

#define LONG_PRESS_DELAY	1000	//ms
#define DOUBLE_PRESS_DELAY	500	//ms
#define DEBOUNCE_DELAY		50

#define INCREMENT_DELAY		50


#define MODE_ON				0
#define MODE_BLINK_SHORT	1
#define MODE_BLINK_LONG		2
#define MODE_OFF			3



volatile uint32_t ms = 0; 
volatile uint16_t f_cnt = 0, b_cnt = 0;
uint8_t f_blink_flag = 0, f_mode = 0;

void initSystem()
{
	//////////////////////////////////////////////////////////////////////////
	DDRA |= (1 << POWER_CTRL_DDR);
	PORTA |= (1 << BUTTON_FRONT_PORT);	//enable pullup

	PORTB |= (1 << BUTTON_FRONT_PORT);	//enable pullup 
	
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

ISR(EXT_INT0_vect)
{
	DISABLE_INT0();
	sleep_disable();

	PRR = 0x00;
	
	ADC_ENABLE();
	AIN_ENABLE();
}

void goToSleep(void)
{
	ENABLE_INT0();
	sleep_bod_disable();
	ADC_DISABLE();
	AIN_DISABLE();
	
	DDRA = 0x00;
	DDRB = 0x00;
	
	PORTA = 0xFF;//LED pins ????
	PORTB = 0xFF;
	
	PRR |= (1 << PRTIM1) | (1 << PRTIM0) | (1 << PRUSI) | (1 << PRADC);
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	sleep_enable();
	
	sleep_cpu();
	
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

void frontButtonHandler(void)
{
	static uint8_t press_flag = RESET, press_cnt = 0, f_b_long_cnt = 0, db_flag = RESET;
	static uint16_t front_pwm = 500, incr_cnt = 0;
	static uint32_t last_ms = 0;

	if (FRONT_BUTTON_PRESSED)
	{
		if (press_flag == RESET)
		{
			press_flag = SET;
			last_ms = ms;
		}
		if (db_flag == RESET)
		{
			db_flag = SET;
		}
		
		if ((ms - last_ms) >= LONG_PRESS_DELAY)
		{
			incr_cnt++;
			if (incr_cnt > INCREMENT_DELAY)
			{
				incr_cnt = 0;
				if (!f_b_long_cnt)
				{
					//////////////////////////////////////////////////////////////////////////
					if (front_pwm < PWM_FREQ)
					front_pwm++;
					else
					front_pwm = PWM_FREQ;
					
					setFrontPwm(front_pwm);
					//////////////////////////////////////////////////////////////////////////
				}
				
				if (f_b_long_cnt)
				{
					//////////////////////////////////////////////////////////////////////////
					if (front_pwm > 0)
					front_pwm--;
					else
					front_pwm = 0;
					
					setFrontPwm(front_pwm);
					//////////////////////////////////////////////////////////////////////////
				}
			}
		}
	}
	
	if ((press_flag == SET) && FRONT_BUTTON_RELEASED)
	{
		if ((ms - last_ms) >= LONG_PRESS_DELAY)
		{
			press_flag = RESET;
			f_b_long_cnt ^= 1;
		}
		else
		{
			if ((ms - last_ms) >= DEBOUNCE_DELAY)
			{
				if ((ms - last_ms) >= DOUBLE_PRESS_DELAY)
				{
					press_flag = RESET;
					press_cnt = 0;
					//////////////////////////////////////////////////////////////////////////
					if (f_blink_flag)
					{
						FRONT_LIGHT_OFF();
					}
					else
					{
						
						if (FRONT_LIGHT_STATE == OFF)
						{
							FRONT_LIGHT_ON();	
						}
						else
						{
							FRONT_LIGHT_OFF();
						}
					}
					f_blink_flag = 0;
					//////////////////////////////////////////////////////////////////////////
				}
				else
				{
					if (db_flag == SET)
					{
						press_cnt++;
						db_flag = RESET;
					}
				}
			}
		}
		
		if (press_cnt >= 2)
		{
			press_flag = RESET;
			press_cnt = 0;
			//////////////////////////////////////////////////////////////////////////
			f_blink_flag = 1;
			//////////////////////////////////////////////////////////////////////////
		}
	}	

}

//void backButtonHandler(void)
//{
	//static uint8_t press_flag = RESET, press_cnt = 0, f_b_long_cnt = 0, db_flag = RESET;
	//static uint16_t back_pwm = 500, incr_cnt = 0;;
	//static uint32_t last_ms = 0;
//
	//if (BACK_BUTTON_PRESSED)
	//{
		//if (press_flag == RESET)
		//{
			//press_flag = SET;
			//last_ms = ms;
		//}
		//if (db_flag == RESET)
		//{
			//db_flag = SET;
		//}
		//
		//if ((ms - last_ms) >= LONG_PRESS_DELAY)
		//{
			//incr_cnt++;
			//if (incr_cnt > INCREMENT_DELAY)
			//{
				//incr_cnt = 0;
				//if (!f_b_long_cnt)
				//{
					////////////////////////////////////////////////////////////////////////////
					//if (back_pwm < PWM_FREQ)
					//back_pwm++;
					//else
					//back_pwm = PWM_FREQ;
					//
					//setBackPwm(back_pwm);
					////////////////////////////////////////////////////////////////////////////
				//}
				//
				//if (f_b_long_cnt)
				//{
					////////////////////////////////////////////////////////////////////////////
					//if (front_pwm > 0)
					//front_pwm--;
					//else
					//front_pwm = 0;
					//
					//setFrontPwm(front_pwm);
					////////////////////////////////////////////////////////////////////////////
				//}
			//}
		//}
	//}
	//
	//if ((press_flag == SET) && FRONT_BUTTON_RELEASED)
	//{
		//if ((ms - last_ms) >= LONG_PRESS_DELAY)
		//{
			//press_flag = RESET;
			//f_b_long_cnt ^= 1;
		//}
		//else
		//{
			//if ((ms - last_ms) >= DOUBLE_PRESS_DELAY)
			//{
				//press_flag = RESET;
				//press_cnt = 0;
				////////////////////////////////////////////////////////////////////////////
				//if (FRONT_LIGHT_STATE == OFF)
				//{
					//FRONT_LIGHT_ON();
				//}
				//else
				//{
					//FRONT_LIGHT_OFF();
				//}
				////////////////////////////////////////////////////////////////////////////
			//}
			//else
			//{
				//if (db_flag == SET)
				//{
					//press_cnt++;
					//db_flag = RESET;
				//}
			//}
		//}
		//
		//if (press_cnt >= 2)
		//{
			//press_flag = RESET;
			//press_cnt = 0;
			////////////////////////////////////////////////////////////////////////////
			//front_pwm = 100;
			//setFrontPwm(front_pwm);
			////////////////////////////////////////////////////////////////////////////
		//}
	//}
//}

int main(void)
{
	initSystem();
	
	
	FRONT_LIGHT_ON();
	setFrontPwm(500);
	
    while(1)
    {
		frontButtonHandler();
		if (f_blink_flag)
		{
			blinkFrontLight(50, 300);
		}
	}
}