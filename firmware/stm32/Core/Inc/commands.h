<<<<<<< HEAD
/*
 * commands.h
 *
 *  Created on: Dec 17, 2023
 *      Author: rossa
 */

#ifndef INC_COMMANDS_H_
#define INC_COMMANDS_H_

#include <stdio.h>

int toggle_button(int argc, char **argv);

/**
 * Commands to move motor
 */
int forward(int argc, char **argv);
int backward(int argc, char **argv);
int stop(int argc, char **argv);
int left(int argc, char **argv);
int right(int argc, char **argv);
int straight(int argc, char **argv);
int get_status(int argc, char **argv);

#endif /* INC_COMMANDS_H_ */
=======
/*
 * commands.h
 *
 *  Created on: 18 avr. 2021
 *      Author: Ross
 */

#ifndef SRC_COMMANDS_H_
#define SRC_COMMANDS_H_

#define UARTBUFFERSIZE	80
#define true			1
#define false			0

extern uint8_t command[UARTBUFFERSIZE];
extern uint8_t msg[UARTBUFFERSIZE];
extern uint8_t newCmd;
extern uint8_t sbt_rx_p;
extern uint8_t sbt_tx_p;
extern uint8_t rx_char[1];
extern uint8_t tx_char[1];

HAL_StatusTypeDef printError(char *);
void print_welcome();
int parse_execute(void);

#endif /* SRC_COMMANDS_H_ */
>>>>>>> 6503406ba0a9f92bfb0325a558ed63026d2a0e5d
