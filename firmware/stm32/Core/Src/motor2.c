/*
 * motor.c
 *
 *  Created on: 16 août 2020
 *      Author: Ross
 */

//#include <Arduino.h>
//#include "board.h"
#include <motor2.h>
#include "main.h"


eMotorTurn_t eMotorTurn;
eMotorMove_t eMotorMove;

//_motor motor;

//extern void vUsage(char *str);

void motor_init() {
/*	// Configure Arduino I/Os
	pinMode(MOTOR_E1, OUTPUT);
	pinMode(MOTOR_M1, OUTPUT);
	pinMode(MOTOR_E2, OUTPUT);
	pinMode(MOTOR_M2, OUTPUT);

	// Set outputs
	digitalWrite(MOTOR_E1, LOW);
	digitalWrite(MOTOR_M1, LOW);
	digitalWrite(MOTOR_E2, LOW);
	digitalWrite(MOTOR_M2, LOW);

	pwmA = pwmB = 0;
	moveStatus = STOP;
	turnStatus = NO_TURN;
	uiMotorA_Offset = 0;
	uiMotorB_Offset = 0;
	*/
}

/*
void move_A(int pwm)
{
	// Make saturation
	if (pwm > 255)
	{
		pwmA = 255;
	}
	else if (pwm < -255)
	{
		pwmA = -255;
	}
	else
	{
		pwmA = pwm;
	}

	// Checks direction and make the motor A moving
	if (pwmA > 0)
	{
//		digitalWrite(MOTOR_M1, LOW);
//		analogWrite(MOTOR_E1, pwmA);
	}
	else if (pwmA < 0)
	{
//		digitalWrite(MOTOR_M1, HIGH);
//		analogWrite(MOTOR_E1, -pwmA);
	}
	else
	{
//		this->stop_A();
	}
}

void move_B(int pwm)
{
	// Make saturation
	if (pwm > 255)
	{
		pwmB = 255;
	}
	else if (pwm < -255)
	{
		pwmB = -255;
	}
	else
	{
		pwmB = pwm;
	}

	// Checks direction and make the motor A moving
	if (pwmB > 0)
	{
//		digitalWrite(MOTOR_M2, HIGH);
//		analogWrite(MOTOR_E2, pwmB);
	}
	else if (pwmB < 0)
	{
//		digitalWrite(MOTOR_M2, LOW);
//		analogWrite(MOTOR_E2, -pwmA);
	}
	else
	{
		stop_B();
	}
}

void stop_A()
{
//	digitalWrite(MOTOR_E1, LOW);
//	digitalWrite(MOTOR_M1, LOW);
}

void stop_B()
{
//	digitalWrite(MOTOR_E2, LOW);
//	digitalWrite(MOTOR_M2, LOW);
}

void stop()
{
	stop_A();
	stop_B();
}

int getPWM_A()
{
	return pwmA;
}

int getPWM_B()
{
	return pwmB;
}



void vMotorTurn(int argc, char *argv[]) 		// Turn the Bot
{
	if(argc != 1) {
		vUsage("turn <0=NO_TURN, 1=LEFT, 2=RIGHT");
	}
	else {
		uint8_t val = atoi(argv[0]);

		if(!val) {
//			motor.turnStatus = NO_TURN;
			motor.uiMotorA_Offset = 0;
			motor.uiMotorB_Offset = 0;
		}
		else {
			if(val == 1 && motor.uiMotorA_Offset != LEFT) {
				motor.turnStatus = LEFT;
				motor.uiMotorA_Offset += MOTOR_OFFSET;
			}
			else if (motor.uiMotorB_Offset != RIGHT) {
				motor.turnStatus = RIGHT;
				motor.uiMotorB_Offset += MOTOR_OFFSET;
			}
		}
	}
}

void vMotorMove(int argc, char *argv[]) 		// Move the Bot
{
	if(argc != 2) {
		vUsage("move <0=STOP 1=FORWARD 2=BACKWARD");
	}
	else {
		uint8_t val = atoi(argv[0]);
		uint8_t pwm = atoi(argv[1]);
		if(!val) {
//			motor.moveStatus = STOP;
			motor.uiMotorA_Offset = 0;
			motor.uiMotorB_Offset = 0;
		}
		else {
			if(val == 1 && motor.moveStatus != FORWARD) {
				motor.moveStatus = FORWARD;
				motor.uiMotorA_Offset += MOTOR_OFFSET;
				motor.uiMotorB_Offset += MOTOR_OFFSET;
			}
			else if(val == 2 && motor.moveStatus != BACKWARD) {
				motor.moveStatus = BACKWARD;
				motor.uiMotorA_Offset -= MOTOR_OFFSET;
				motor.uiMotorB_Offset -= MOTOR_OFFSET;
			}
		}
		motor.move_A(pwm);
		motor.move_B(pwm);
	}
}

*/
