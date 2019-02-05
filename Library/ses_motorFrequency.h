#ifndef SES_MOTORFREQUENCY_H_
#define SES_MOTORFREQUENCY_H_

/* INCLUDES ******************************************************************/

#include "ses_common.h"

/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes Frequency Measurement
 */
void motorFrequency_init(void);

/**
 * get current frequency
 */
uint16_t motorFrequency_getRecent(void);

/**
 * get median
 */
uint16_t motorFrequency_getMedian(void);

/**
 * functionality when motor stopped
 */
void motor_stopped(void);

/**
 * sorting array elements in ascending order
 */
void sorting_array(void);

#endif /* SES_MOTORFREQUENCY_H_ */
