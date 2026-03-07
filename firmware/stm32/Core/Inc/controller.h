/*
 * pid.h
 *
 *  Created on: 18 oct. 2021
 *      Author: Ross
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <stdint.h>

#define 	__USE_PID__
#undef 		__USE_STATE_VARIABLE__

extern uint8_t pidActive;			// Tells if the PID is active
extern double last_error;			// Last error to the next iteration
extern double output;				// Output of the PID
extern double integral;				// Integral part of the PID memory
extern double kp, kd, ki;			// PID constants
extern uint8_t newData;				// Tells that a new data from sensors is avaialbe for motor update

/**
 * Prototypes
 */
void pid_reset();									// PID reset
void pid_init();									// PID initialization
void pid_start();									// PID start
void pid_stop();									// PID stop
double pid_calculate(double, double, double );		// PID update

void update_motor(double );							// Motor update

#endif /* CONTROLLER_H_ */
