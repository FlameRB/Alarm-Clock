#ifndef SES_PWM_H_
#define SES_PWM_H_

/* INCLUDES ******************************************************************/

#include "ses_common.h"

/**PID control structure
 */
typedef struct profilesPID_s{
	/* Max and Min manipulated frequency */
	uint16_t f_max;
	uint16_t f_min;

	/* Proportaional, Intergrator and Differential constants */
	int8_t Kp;
	int8_t Ki;
	int8_t Kd;

	/*Error*/
	int16_t error;
	int16_t intergrate;

}profilesPID;

/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes PWM
 */
void pwm_init(void);

/**
 * Set rotation speed of PWM
 * @param dutyCycle value in term of 8-bit value
 */
void pwm_setDutyCycle(uint8_t dutyCycle);

/**
 * updating the PID_profiles
 * @param1 target frequency
 * @param2 current frequency
 * @param3 pid profiles structure
 */
int16_t pid_control(uint16_t target, uint16_t current, profilesPID *pid);

/**
 * Map frequency value to OCR0B value
 * @param frequency
 */
int8_t frequency_to_dutyCycle (int16_t frequency);

#endif /* SES_PWM_H_ */
