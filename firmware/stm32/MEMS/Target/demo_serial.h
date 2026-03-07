/**
 *******************************************************************************
 * @file    demo_serial.h
 * @author  MEMS Software Solutions Team
 * @brief   Header for demo_serial.c
 *******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed under Software License Agreement
 * SLA0077, (the "License"). You may not use this file except in compliance
 * with the License. You may obtain a copy of the License at:
 *
 *     www.st.com/content/st_com/en/search.html#q=SLA0077-t=keywords-page=1
 *
 *******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef DEMO_SERIAL_H
#define DEMO_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "serial_protocol.h"
#include "serial_cmd.h"
#include "motion_tl_manager.h"
#include "bsp_ip_conf.h"

/* Private defines -----------------------------------------------------------*/
#define SENDER_UART  0x01
#define SENDER_USB   0x02
#define SENDER_SPI   0x03

#define DEV_ADDR                   50U
#define I2C_DATA_MAX_LENGTH_BYTES  16
#define STREAMING_MSG_LENGTH       71
#define MIN(A,B) ((A)<(B)?(A):(B))

/* Algorithm-based defines */
#define CMD_ANGLE_MODE_PITCH_ROLL_GRAVITY_INCLINATION  0x01
#define CMD_ANGLE_MODE_THETA_PSI_PHI                   0x02
#define CMD_CAL_POS_LYING_NORMAL_ON_TABLE              0x03
#define CMD_CAL_POS_LYING_UPSIDEDOWN_ON_TABLE          0x04
#define CMD_CAL_POS_NUCLEO_CONNECTOR_DOWN              0x05
#define CMD_CAL_POS_NUCLEO_CONNECTOR_LEFT              0x06
#define CMD_CAL_POS_NUCLEO_CONNECTOR_UP                0x07
#define CMD_CAL_POS_NUCLEO_CONNECTOR_RIGHT             0x08
#define CMD_GET_CALIBRATION_COEFFICIENTS               0x09
#define CMD_GET_ESTIMATED_MEASUREMENT_TIME             0x0A

/* Exported defines ----------------------------------------------------------*/
/* Enable sensor masks */
#define PRESSURE_SENSOR                         0x00000001U
#define TEMPERATURE_SENSOR                      0x00000002U
#define HUMIDITY_SENSOR                         0x00000004U
#define UV_SENSOR                               0x00000008U /* for future use */
#define ACCELEROMETER_SENSOR                    0x00000010U
#define GYROSCOPE_SENSOR                        0x00000020U
#define MAGNETIC_SENSOR                         0x00000040U

/* Exported variables --------------------------------------------------------*/
extern volatile uint8_t DataLoggerActive;
extern volatile uint32_t SensorsEnabled;
extern MTL_angle_mode_t AngleMode;

/* Exported functions ------------------------------------------------------- */
void BUILD_REPLY_HEADER(TMsg *Msg);
void INIT_STREAMING_HEADER(TMsg *Msg);
void INIT_STREAMING_MSG(TMsg *Msg);
int HandleMSG(TMsg *Msg);

#ifdef __cplusplus
}
#endif

#endif /* DEMO_SERIAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
