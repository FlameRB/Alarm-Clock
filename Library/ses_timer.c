/* INCLUDES ******************************************************************/
#include "ses_timer.h"

/* DEFINES & MACROS **********************************************************/
#define TIMER1_CTRL_A			    (1 << WGM12) 			//CTC mode
#define TIMER1_CTRL_B				(1 << CS11)|(1 << CS10)	//pre_scaler of 64
#define TIMER1_INT_MASK			    (1 << OCIE1A)			//interrupt mask register for compare A
#define TIMER1_INT_FLAG_CLEAR		(1 << OCF1A)	//clear the interrupt flag
#define TIMER1_CYC_FOR_5MILLISEC    1249 			//interrupt triggered every 5ms, timer 1 is 16 bit

#define TIMER2_CTRL_A			   	(1 << WGM21) 	//CTC mode
#define TIMER2_CTRL_B				(1 << CS22) 	//prescaler of 64
#define TIMER2_INT_MASK			    (1 << OCIE2A)	//interrupt mask register for compare A
#define TIMER2_INT_FLAG_CLEAR		(1 << OCF2A)	//clear the interrupt flag
#define TIMER2_CYC_FOR_1MILLISEC	249	//250 -> 64*(1/(16e6/250)) = interval of 1ms between 2 interrupts

#define TIMER0_CTRL_A				(1 << COM0B1)|(1 << COM0B0)|(1 << WGM01)|(1 << WGM00)		//Inverted mode, set OC0B on CTC mode, fast PWM TOP = OCRA
#define TIMER0_CTRL_B				(1 << CS00)	//fast PWM, TOP = 0xFF, no prescaling

#define TIMER5_CTRL_A				(1 << WGM52)			//CTC mode
#define TIMER5_CTRL_B				(1 << CS51)|(1 << CS50)	//prescaler = 64
#define TIMER5_INT_MASK				(1 << OCIE5A)			//output Compare A match Enable
#define TIMER5_INT_FLAG_CLEAR		(1 << OCF5A)			//output Compare A match Flag
#define TIMER5_CYC_FOR_200MILISEC	24999					//timer reset after 100ms

pTimerCallback Timer5Callback 	= NULL;
pTimerCallback Timer2Callback 	= NULL;
pTimerCallback Timer1Callback 	= NULL;

/*FUNCTION DEFINITION ********************************************************/
void timer2_setCallback(pTimerCallback cb) {
	Timer2Callback = cb;
}

void timer2_start() {
	//Active control mode in Register A and B
	TCCR2A 	|= TIMER2_CTRL_A;
	TCCR2B 	|= TIMER2_CTRL_B;

	//Interrupt triggered for CTC mode at OCR2A
	TIFR2	|= TIMER2_INT_FLAG_CLEAR;
	TIMSK2	|= TIMER2_INT_MASK;

	//Value used to compared in A
	OCR2A	=	TIMER2_CYC_FOR_1MILLISEC;
}


void timer2_stop() {
	//No Clock is set means deactivated
    TCCR2A &= ~TIMER2_CTRL_A;
    TCCR2B &= ~TIMER2_CTRL_B;
}

void timer1_setCallback(pTimerCallback cb) {
	Timer1Callback = cb;
}


void timer1_start() {
	TCCR1B 	|= (TIMER1_CTRL_A | TIMER1_CTRL_B);
	TIFR1 	|= TIMER1_INT_FLAG_CLEAR;
	TIMSK1 	|= TIMER1_INT_MASK;
	OCR1A  	= TIMER1_CYC_FOR_5MILLISEC;
}


void timer1_stop() {
	TCCR1B &= ~(TIMER1_CTRL_A | TIMER1_CTRL_B);
}

void timer0_start(){
	TCCR0A |= TIMER0_CTRL_A;
	TCCR0B |= TIMER0_CTRL_B;
}

void timer0_stop(){
	TCCR0A &= ~TIMER0_CTRL_A;
	TCCR0B &= ~TIMER0_CTRL_B;
}

void timer5_setCallback(pTimerCallback cb) {
	Timer5Callback = cb;
}

void timer5_start(){
	TCCR5B |= (TIMER5_CTRL_A | TIMER5_CTRL_B);
	TIMSK5 |= TIMER5_INT_MASK;
	TIFR5  &= ~TIMER5_INT_FLAG_CLEAR;
	OCR5A   = TIMER5_CYC_FOR_200MILISEC;
}

void timer5_stop(){
	TCCR5B &= ~(TIMER5_CTRL_A | TIMER5_CTRL_B);
}


ISR(TIMER1_COMPA_vect) {
	if(Timer1Callback != NULL){
		Timer1Callback();
	}
}

ISR(TIMER2_COMPA_vect) {
	if(Timer2Callback != NULL){
		Timer2Callback();
	}
}

ISR(TIMER5_COMPA_vect){
	if(Timer5Callback != NULL){
		Timer5Callback();
	}
}

