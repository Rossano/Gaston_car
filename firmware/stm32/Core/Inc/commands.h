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
