#include "ses_button.h"
#include "ses_timer.h"

/* DEFINES & MACROS **********************************************************/

// Button wiring on SES board
#define BUTTON_ROTARY_PORT     	PORTB
#define BUTTON_ROTARY_PIN          	6

#define BUTTON_JOYSTICK_PORT  	PORTB
#define BUTTON_JOYSTICK_PIN      	7

#define BUTTON_NUM_DEBOUNCE_CHECKS	5

pButtonCallback rotaryCallback 		= NULL; //initially point to nothing
pButtonCallback joystickCallback 	= NULL;

/* FUNCTION DEFINITION *******************************************************/
void button_init(bool debouncing)
{
	//Initial BUTTON as input
	DDR_REGISTER(BUTTON_ROTARY_PORT) 	&= ~(1 << BUTTON_ROTARY_PIN) ;
	DDR_REGISTER(BUTTON_JOYSTICK_PORT) 	&= ~(1 << BUTTON_JOYSTICK_PIN) ;

	//Active Internal pull-up
	BUTTON_ROTARY_PORT 		|= 1 << BUTTON_ROTARY_PIN;
	BUTTON_JOYSTICK_PORT 	|= 1 << BUTTON_JOYSTICK_PIN;

	/*Extended function button with debouncing function*/
	if(debouncing)
	{
		//timer1_start();
		//timer1_setCallback(button_checkState);
		button_checkState(); //for using scheduler
	}

	/*Use ISR if debouncing function is not triggered*/
	else
	{
	//Enable all interrupt
	PCICR |= (1 << PCIE0);
	//All buttons of SES board trigger one pin change interrupt PCI.
	//PCMSK0: PCINT6 and PCINT7 is to check whether interrupt enabled on
	//		  Pin6 and Pin7 of PORTB respectively or Position of Rotary
	//		  and Joystick Pin (accidentally or deliberately ?)
	//Trigger interrupt if a button is pressed
	PCMSK0 |= ((1<<BUTTON_ROTARY_PIN) | (1<<BUTTON_JOYSTICK_PIN));
	}
}

bool button_isJoystickPressed(void)
{
	//Return only the value of PING, thats why "&" with MASK here.
	//Return has form : 0G00 0000, not something like this 1G00 1101.
	//Return 1 if the button is pressed.
	return !(PIN_REGISTER(BUTTON_JOYSTICK_PORT) & (1 << BUTTON_JOYSTICK_PIN));
}

bool button_isRotaryPressed(void)
{
	return !(PIN_REGISTER(BUTTON_ROTARY_PORT) & (1 << BUTTON_ROTARY_PIN));
}

void button_setRotaryButtonCallback(pButtonCallback callback)
{
	rotaryCallback = callback; //point to the address of callback function
}

void button_setJoystickButtonCallback(pButtonCallback callback)
{
	joystickCallback = callback;
}

void button_checkState() {
	static uint8_t state[BUTTON_NUM_DEBOUNCE_CHECKS] = {};
	static uint8_t index = 0;
	static uint8_t debouncedState = 0;
	uint8_t lastDebouncedState = debouncedState;
	// each bit in every state byte represents one button
	state[index] = 0;
	if(button_isJoystickPressed()) {
		state[index] |= 1;
	}
	if(button_isRotaryPressed()) {
		state[index] |= 2;
	}
	index++;
	if (index == BUTTON_NUM_DEBOUNCE_CHECKS) {
		index = 0;
	}
	// init compare value and compare with ALL reads, only if
	// we read BUTTON_NUM_DEBOUNCE_CHECKS consistent "0" in the state
	// array, the button at this position is considered pressed
	uint8_t j = 0xFF;
	for(uint8_t i = 0; i < BUTTON_NUM_DEBOUNCE_CHECKS; i++) {
		j = j & state[i];
	}
	debouncedState = j;

	// Check whether the button is really pressed or just a debouncing effect.
	if(((debouncedState & 1) == 0) && ((lastDebouncedState & 1) != 0))
	{	if(joystickCallback != NULL){
			joystickCallback();
		}
	}

	if(((debouncedState & 2) == 0) && ((lastDebouncedState & 2) != 0))
	{	if(rotaryCallback != NULL){
			rotaryCallback();
		}
	}
}

ISR(PCINT0_vect)
{
	//mask register contains 1 and valid callback was set
	if(button_isJoystickPressed() && (joystickCallback != NULL))
	{
		joystickCallback(); //execute callback function
	}

	if(button_isRotaryPressed() && (rotaryCallback != NULL))
	{
		rotaryCallback();
	}
}
