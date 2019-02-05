#include "ses_motorFrequency.h"
#include "ses_timer.h"
#include "ses_led.h"
#include <util/atomic.h>

/* DEFINES & MACROS **********************************************************/
#define SPIKE_OUTPUT_PORT		PORTD
#define SPIKE_OUTPUT_PIN			0

#define	EXTERNAL_INT0_CTRL		(1 << ISC01) | (1 << ISC00) //rising edge triggered interrupt
#define EXTERNAL_INT0_MASK		(1 << INT0)		//enable external interrupt INT0
#define EXTERNAL_INT0_FLAG		(1 << INTF0)	//flag of external interrupt INT0
#define FACTOR					10				//due to overflow reasons
#define N						31				//N element, limited to 256 (uint8)

bool motorStatus 			= 0;				//stopped at beginning
uint16_t currentFrequency 	= 0;
uint16_t frequency[N] 		= {0};				//to store N lastest frequency value

/* FUNCTION DEFINITION *******************************************************/
void motorFrequency_init(void)
{
	//Control bit setting
	EICRA |= EXTERNAL_INT0_CTRL;

	//Masking
	EIMSK |= EXTERNAL_INT0_MASK;

	//Flag is clear at the beginning
	EIFR &= ~EXTERNAL_INT0_FLAG;

	/*Initilizing the timer5*/
	timer5_start();
	TCNT5 = 0;
	timer5_setCallback(motor_stopped);
}

uint16_t motorFrequency_getRecent(void)
{
	/* the FACTOR is calculated by Rule of three, given max frequency 16Mhz/64
	   corresponding to max period 2^16*/

	uint16_t temp = 0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	/** using rule of three to calculate frequency, with 100ms corresponds to TCNT5 = 25000
	 *  FACTOR is being used to convert the frequency to revolution per seconds or Hz
	 */
	temp = (uint16_t) FACTOR*currentFrequency;
	}

	return temp;
}

uint16_t motorFrequency_getMedian(void)
{
	/* if motor is stopped, set all value to zero*/
	if(motorStatus == 0)
	{
	  for(uint8_t i = 0; i < N; i++)
		{
		 ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		 frequency[i] = 0;
		 }
		}
	}

	uint16_t temp = 0;
	sorting_array();
	if (N%2 == 0){
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		temp = (uint16_t)  FACTOR*((frequency[N/2]+frequency[N/2 - 1])/2);	//N is even, then median is average of 2 values of 2 middle elements of array
		}
	}
	else{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		temp =  (uint16_t) FACTOR*frequency[(N - 1)/2];	//N is odd, the median = value of middle element of array
		}
	}
	return temp;
}

void motor_stopped(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	motorStatus = 0;					// motorStatus is 0 if motor is off
	currentFrequency = 0;
	led_yellowOff();
	led_greenOn();
	}
}

void sorting_array(void)
{
	//uint16_t temp = frequency; // implementing a copy of array
    uint16_t array = 0;
    // the following two loops sort the array in ascending order
    for(uint8_t i = 0; i < N-1; i++) {
        for(uint8_t j = 1; j < N; j++) {
        	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        	if(frequency[j] < frequency[i]) {
                // swap elements
                array = frequency[i];
                frequency[i] = frequency[j];
                frequency[j] = array;
            }
            }
        }
    }
}

ISR(INT0_vect){
	static uint8_t frequencyIndex 	= 0; // index of array frequency
	static uint8_t spikeCounter 	= 0; // counting spikes
	motorStatus = 1;					 // motorStatus is 1 if motor is running
	led_yellowToggle();
	led_greenOff();

	/*0 -> 5 : 6 spikes =  1 revolution*/
	if(spikeCounter == 5)
	{
		/*if motor is not running, avoid division of TNCT5 = 0 */
		if(TCNT5 == 0||motorStatus == 0){
			frequency[frequencyIndex] = 0;
			currentFrequency = 0;
		}
		else{
			/*need to edit this too, using uint32_t anf remove FACTOR*/
			currentFrequency = (uint16_t)25000/TCNT5;		//current frequency of Motor
			frequency[frequencyIndex] = currentFrequency;	//store value of frequency in array of length N
			TCNT5 = 0;										//reset counter after 6 spikes
		}
		frequencyIndex = (frequencyIndex+1)%N;			//index run from 0 -> N-1
		spikeCounter = 0;
	}
	spikeCounter++;
}
