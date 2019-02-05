#include "ses_adc.h"
#include "ses_uart.h"
#include "ses_lcd.h"

/* DEFINES & MACROS **********************************************************/

// ADC wiring on SES board
#define ADC_TEMPERATURE_PORT     	PORTF
#define ADC_TEMPERATURE_PIN         	2

#define ADC_LIGHT_PORT  			PORTF
#define ADC_LIGHT_PIN     				4

#define ADC_JOYSTICK_PORT  			PORTF
#define ADC_JOYSTICK_PIN     			5

#define ADC_MICROPHONE_PORT  		PORTF
#define ADC_MICROPHONE_POS_PIN     		1
#define ADC_MICROPHONE_NEG_PIN     		0

void adc_init(void)
{
	//Init ADC SENSORS as input
	DDR_REGISTER(ADC_TEMPERATURE_PORT) 	&= ~(1 << ADC_TEMPERATURE_PIN);
	DDR_REGISTER(ADC_LIGHT_PORT) 		&= ~(1 << ADC_LIGHT_PIN);
	DDR_REGISTER(ADC_JOYSTICK_PORT) 	&= ~(1 << ADC_JOYSTICK_PIN);
	DDR_REGISTER(ADC_MICROPHONE_PORT) 	&= ~((1 << ADC_MICROPHONE_POS_PIN) | (1 << ADC_MICROPHONE_NEG_PIN));

	//Deactive internal pull-up register
	ADC_TEMPERATURE_PORT 	&= ~(1 << ADC_TEMPERATURE_PIN);
	ADC_LIGHT_PORT			&= ~(1 << ADC_LIGHT_PIN);
	ADC_JOYSTICK_PORT		&= ~(1 << ADC_JOYSTICK_PIN);
	ADC_MICROPHONE_PORT		&= ~((1 << ADC_MICROPHONE_POS_PIN) | (1 << ADC_MICROPHONE_NEG_PIN));

	//Power reduction mode OFF
	PRR0 	&= ~(1<<PRADC);

	//ADC reference voltage 1.6v
	ADMUX 	|= ADC_VREF_SRC;
	ADMUX 	&= ~(1 << ADLAR);

	//ADC prescaler 8, enable ADC
	ADCSRA 	|= (ADC_PRESCALE | (1 << ADEN));
	ADCSRA 	&= ~(1 << ADATE);
}

uint16_t adc_read(uint8_t adc_channel)
{
	//adc_channel_check
	if(adc_channel >= ADC_NUM)
	{
		return ADC_INVALID_CHANNEL;
	}

	//read selected channel
	ADMUX =((ADMUX & (0b11100000)) | adc_channel);

	//Start Conversion
	ADCSRA |= (1 << ADSC);

	//Wait conversion to complete
	while(ADCSRA & (1<<ADSC))
	{
		asm volatile("nop");
	}

	//read result, but read ADCL first then ADCH:
	//0000 00CH AADD CCLL 10bit resolution
	return ADC;
	//return ADCL + (ADCH << 8);
}

uint8_t adc_getJoystickDirection(void)
{
	uint16_t adc = adc_read(ADC_JOYSTICK_CH); //data TYPE

	if(abs(adc - RIGHT_DEF)	<= TOLERANT) 		{return RIGHT;}
	if(abs(adc - UP_DEF)	<= TOLERANT) 		{return UP;}
	if(abs(adc - LEFT_DEF)	<= TOLERANT)		{return LEFT;}
	if(abs(adc - DOWN_DEF)	<= TOLERANT) 		{return DOWN;}
	//if(abs(adc - NO_DEF)	<= TOLERANT) 		{return NO_DIRECTION;}

	return NO_DIRECTION;
}

int16_t adc_getTemperature(void)
{
	int16_t adc = adc_read(ADC_TEMP_CH);
	int16_t slope = (ADC_TEMP_MAX - ADC_TEMP_MIN) / (ADC_TEMP_RAW_MAX - ADC_TEMP_RAW_MIN);
	int16_t offset = ADC_TEMP_MAX - (ADC_TEMP_RAW_MAX * slope);
	return ((adc * slope + offset) / ADC_TEMP_FACTOR);
}
