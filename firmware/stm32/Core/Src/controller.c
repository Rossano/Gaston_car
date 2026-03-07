/*
 * pid.c
 *
 *  Created on: 18 oct. 2021
 *      Author: Ross
 */

#include "x_nucleo_ihmxx.h"
#include <stdint.h>
#include "controller.h"

/**
 * Variables
 */
uint8_t pidActive;
double last_error;
double output;
double integral;
double kp, kd, ki;
uint8_t newData = 0;

/**
 * Function Implementation
 */

void pid_reset()
{
	last_error = 0;
	integral = 0;
}

void pid_init()
{
	kp = 0;
	ki = 0;
	kd = 0;
	last_error = 0;
	integral = 0;
}

void pid_start()
{
	// Enable the pid
	pidActive = 1;
}

void pid_stop()
{
	// Disable the pid
	pidActive = 0;

}

double pid_calculate(double inp, double dt, double set_point)
{
	double error;
	// check if the pid is active
	if(pidActive)
	{
		if (inp >= 0)
		{
			error = set_point - inp;
		}
		else
		{
			error = -(180 + inp);
		}
		integral += error * dt;
		double foo = kp * error + kd * (error - last_error) / dt + ki * integral;
		last_error = error;
		return foo;
	}
	else
	{
		return 0;
	}
}

void update_motor(double pwm)
{

	uint16_t foo;
#ifdef __USE_PID__
	//uint16_t _pwm = (uint16_)

	if( pwm >= 0 )
	{
		foo = (uint16_t)pwm;
		if(foo > 100) foo = 100;
		BSP_MotorControl_SetMaxSpeed(0, foo); //(uint16_t)pwm);
		BSP_MotorControl_SetMaxSpeed(1, foo); //(uint16_t)pwm);
		BSP_MotorControl_Run(0, FORWARD);
		BSP_MotorControl_Run(1, FORWARD);
	}
	else
	{
		foo = (uint16_t)(-pwm);
		if(foo > 100) foo = 100;
		BSP_MotorControl_SetMaxSpeed(0, foo); //(uint16_t)(-pwm));
		BSP_MotorControl_SetMaxSpeed(1, foo); //(uint16_t)(-pwm));
		BSP_MotorControl_Run(0, BACKWARD);
		BSP_MotorControl_Run(1, BACKWARD);
	}
#endif

	newData = 0;
}
