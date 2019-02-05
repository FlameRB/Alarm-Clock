#include "ses_pwm.h"
#include "ses_timer.h"
#include "ses_common.h"

/* DEFINES & MACROS **********************************************************/
#define PWM_OUTPUT_PORT 	PORTG
#define PWM_OUTPUT_PIN		5

/* FUNCTION DEFINITION *******************************************************/
void pwm_init(void)
{
	//Enable timer 0
	PRR0 &= ~(1 << PRTIM0);

	//Initilize OC0B pin as output
	DDR_REGISTER(PWM_OUTPUT_PORT) |= (1 << PWM_OUTPUT_PIN);

	//Initilize TCNT0
	TCNT0 = 0;

	//start timer0 which settings set
	timer0_start();
}

void pwm_setDutyCycle(uint8_t dutyCycle)
{
	// FAST PWM mode, inverting mode
	// OCR0B = 255*(1 - dutyCycle)
	// dutyCycle in term of % = (255 - 170)/255 = 33.3%
	OCR0B = (uint8_t) dutyCycle;
}

int16_t pid_control(uint16_t target, uint16_t current, profilesPID *pid)
{
	/*PID controller*/
	//remember last value
	int8_t lastError = pid->error;

	pid->error = current - target;
	pid->intergrate = pid->error + pid->intergrate;

	/*Check whether Intergate value ís in boundary*/
	if(pid->intergrate >= pid->f_max)
	{
		pid->intergrate = pid->f_max;
	}

	if(pid->intergrate <= pid->f_max)
	{
		pid->intergrate = pid->f_min;
	}

	int16_t manipulated_frequency = (int16_t)pid->Kp*pid->error + pid->Ki*pid->intergrate + pid->Kd*(lastError - pid->error);
	return manipulated_frequency;
}
