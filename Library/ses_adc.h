#ifndef SES_ADC_H
#define SES_ADC_H

/*INCLUDES *******************************************************************/

#include <inttypes.h>
#include <avr/io.h>
#include "ses_common.h"

/* DEFINES & MACROS **********************************************************/

/* to signal that the given channel was invalid */
#define ADC_INVALID_CHANNEL    	0xFFFF
#define RIGHT_DEF			200
#define UP_DEF				400
#define LEFT_DEF			600
#define DOWN_DEF			800
#define NO_DEF				1000	/* joystick is not moved */
#define TOLERANT			25

#define ADC_TEMP_MAX		4000//313*ADC_TEMP_FACTOR
#define ADC_TEMP_MIN		2000//293*ADC_TEMP_FACTOR
#define ADC_TEMP_RAW_MAX	401//256//401//0.401*1000
#define ADC_TEMP_RAW_MIN	752//481//752 //0.752*1000
#define ADC_TEMP_FACTOR		10

#define ADC_VREF_SRC	(3 << REFS0) //Vref = 1.6V, right adjust
#define ADC_PRESCALE	(1 << ADPS0 | 1 << ADPS1) //8

enum ADCChannels {
  ADC_MIC_NEG_CH=0,                     /* ADC0 */
  ADC_MIC_POS_CH,                       /* ADC1 */
  ADC_TEMP_CH,                          /* ADC2 */
  ADC_RESERVED1_CH,                     /* ADC3 */
  ADC_LIGHT_CH,                         /* ADC4 */
  ADC_JOYSTICK_CH,                      /* ADC5 */
  ADC_RESERVED2_CH,                     /* ADC6 */
  ADC_RESERVED3_CH,                     /* ADC7 */
  ADC_NUM                               /* number of ADC channels*/
};

enum JoystickDirections {
  RIGHT = 0,
  UP 	,
  LEFT 	,
  DOWN 	,
  NO_DIRECTION
};

/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes the ADC
 */
void adc_init(void);

/**
 * Read the raw ADC value of the given channel
 * @adc_channel The channel as element of the ADCChannels enum
 * @return The raw ADC value
 */
uint16_t adc_read(uint8_t adc_channel);

/**
 * Read the current joystick direction
 * @return The direction as element of the JoystickDirections enum
 */
uint8_t adc_getJoystickDirection();

/**
 * Read the current temperature
 * @return Temperature in tenths of degree celsius
 */
int16_t adc_getTemperature();

#endif /* SES_ADC_H */
