/*
 * commands.c
 *
 *  Created on: Dec 17, 2023
 *      Author: rossa
 */

/**
 * Includes Files
 */
#include "Motor.h"
#include <stdio.h>
#include <stdlib.h>

// Button Variable
int btn = 0;

int toggle_button(int argc, char **argv)
{
	if(btn == 0)
	{
		printf("ON!!!\r\n");
		btn = 1;
	}
	else
	{
		btn = 0;
		printf("OFF!!!\r\n");
	}
	return 0;
}

/**
 * Commands to move motor
 */
int forward(int argc, char **argv)
{
	if(argc < 2) return -1;
	else
	{
		int speed = atoi(argv[1]);
		if (speed != 0)
		{
			SetMotorPWM(MOTOR_A, speed);
			SetMotorPWM(MOTOR_B, speed);
			return 0;
		}
	}
	return -2;
}

int backward(int argc, char **argv)
{
	if(argc < 2) return -1;
	else
	{
		int speed = atoi(argv[1]);
		if (speed != 0)
		{
			SetMotorPWM(MOTOR_A, -speed);
			SetMotorPWM(MOTOR_B, -speed);
			return 0;
		}
	}
	return -2;
}

int stop(int argc, char **argv)
{
	motor_StopMotorA();
	motor_StopMotorB();
	return 0;
}


/**
 * Command to turn left
 */
int left(int argc, char **argv)
{
	if (argc < 1) return -1;
	else
	{
		if (bridgeA != ENABLED && bridgeB != ENABLED) return -3;
		if (stateA == stateB && stateA == RUN)
		{
			int valA, valB;
			if (speed_A < speed_B)
			{
				// Do nothing it is already turning left
				return 0;
			}
			else if (dirA == dirB && dirA == FORWARD)
			{
				valA = speed_A - PWM_TURN_OFFSET;
				valB = speed_B + PWM_TURN_OFFSET;
			}
			else if (dirA == dirB && dirA == BACKWARD)
			{
				valA = speed_A + PWM_TURN_OFFSET;
				valB = speed_B - PWM_TURN_OFFSET;
			}
			else
			{
				// Direction Unknown or bridge A and B diverging
				return -4;
			}
			// All checks passed, PWM may be set
			SetMotorPWM(MOTOR_A, valA);
			SetMotorPWM(MOTOR_B, valB);
			speed_A = valA;
			speed_B = valB;
			return 0;
		}
		else
		{
			// Bridge status diverging or not in RUN state
			return -5;
		}
	}
	return -2;
}

/**
 * Command to turn Right
 */
int right(int argc, char **argv)
{
	if (argc < 1) return -1;
	else
	{
		if (bridgeA != ENABLED && bridgeB != ENABLED) return -3;
		if (stateA == stateB && stateA == RUN)
		{
			int valA, valB;
			if (speed_A > speed_B)
			{
				// Do nothing it is already turning Right
				return 0;
			}
			else if (dirA == dirB && dirA == FORWARD)
			{
				valA = speed_A + PWM_TURN_OFFSET;
				valB = speed_B - PWM_TURN_OFFSET;
			}
			else if (dirA == dirB && dirA == BACKWARD)
			{
				valA = speed_A - PWM_TURN_OFFSET;
				valB = speed_B + PWM_TURN_OFFSET;
			}
			else
			{
				// Direction Unknown or bridge A and B diverging
				return -4;
			}
			// All checks passed, PWM may be set
			SetMotorPWM(MOTOR_A, valA);
			SetMotorPWM(MOTOR_B, valB);
			speed_A = valA;
			speed_B = valB;
			return 0;
		}
		else
		{
			// Bridge status diverging or not in RUN state
			return -5;
		}
	}
	return -2;
}

/**
 * Command to stop turning
 */
int straight(int argc, char **argv)
{
	if (argc < 2) return -1;
	else
	{
		if (bridgeA != ENABLED && bridgeB != ENABLED) return -3;
		if (stateA == stateB && stateA == RUN)
		{
			int val;
			if (speed_A == speed_B)
			{
				// Do nothing it is already going straight
				return 0;
			}
			else if (speed_A > speed_B)
			{
				val = speed_A - PWM_TURN_OFFSET;
			}
			else
			{
				val = speed_A + PWM_TURN_OFFSET;
			}
			// All checks passed, PWM may be set
			SetMotorPWM(MOTOR_A, val);
			SetMotorPWM(MOTOR_B, val);
			speed_A = val;
			speed_B = val;
			return 0;
		}
		else
		{
			// Bridge status diverging or not in RUN state
			return -5;
		}
	}
	return -2;
}

/**
 * Command to get motor speeds and direction
 */
int get_status(int argc, char **argv)
{
	if (argc < 1) return -1;
	else
	{
		printf("Gaston status: SpeedA: %d SpeedB: %d\r\n", speed_A, speed_B);
		return 0;
	}
}
