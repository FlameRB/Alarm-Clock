#ifndef SES_BUTTON_H_
#define SES_BUTTON_H_

/* INCLUDES ******************************************************************/

#include "ses_common.h"

/* FUNCTION PROTOTYPES *******************************************************/
typedef void (*pButtonCallback)();

/**
 * Initializes rotary encoder and joystick button
 *
 * @param toggle debouncing function or not
 */
void button_init(bool debouncing);

/** 
 * Get the state of the joystick button.
 */
bool button_isJoystickPressed(void);

/** 
 * Get the state of the rotary button.
 */
bool button_isRotaryPressed(void);

/**
 * Set the function to be called when Rotatry button is pressed.
 *
 * @param valid pointer to callback function
 */
void button_setRotaryButtonCallback(pButtonCallback callback);

/**
 * Set the function to be called when Joystick button is pressed.
 *
 * @param valid pointer to callback function
 */
void button_setJoystickButtonCallback(pButtonCallback callback);

/**
 * to check bouncing effect
 */
void button_checkState(void);

#endif /* SES_BUTTON_H_ */
