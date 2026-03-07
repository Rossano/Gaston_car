/*
 * motor.h
 *
 *  Created on: 16 août 2020
 *      Author: Ross
 */

#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#define MOTOR_OFFSET 30
#define TURN_OFFSET	30
#define MOVE_OFFSET 50

typedef enum {
	STOP,
	FORWARD,
	BACKWARD
} eMotorMove_t;

typedef enum {
	NO_TURN,
	LEFT,
	RIGHT
} eMotorTurn_t;

extern eMotorTurn_t eMotorTurn;
extern eMotorMove_t eMotorMove;

/**
 *  Exported Variables
 */
eMotorMove_t moveStatus;
eMotorTurn_t turnStatus;
	//uint8_t uiMotorA_Offset;
	//uint8_t uiMotorB_Offset;
unsigned char uiMotorA_Offset;
unsigned char uiMotorB_Offset;
int pwmA;
int pwmB;

/**
 * Function Prototypes
 */
void init_motor();
void move_A(int);
void move_B(int);
void stop_A();
void stop_B();
void stop();
int getPWM_A();
int getPWM_B();


//extern _motor motor;

//
//	Function Prototypes
//
#ifndef ARDUINO_AVR_YUN
void vMotorTurn(int argc, char *argv[]); 		// Turn the Bot
void vMotorMove(int argc, char *argv[]); 		// Move the Bot
#endif !ARDUINO_AVR_YUN


#endif /* INC_MOTOR_H_ */
