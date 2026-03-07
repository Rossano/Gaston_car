/**
  ******************************************************************************
  * File Name          : stmicroelectronics_x-cube-mems1_7_1_0.h
  * Description        : This file provides code for the configuration
  *                      of the STMicroelectronics.X-CUBE-MEMS1.7.1.0 instances.
  ******************************************************************************
  *
  * COPYRIGHT 2020 STMicroelectronics
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_X_CUBE_MEMS1_H
#define __APP_X_CUBE_MEMS1_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "iks01a2_mems_control.h"
#include "iks01a2_mems_control_ex.h"

#include "motion_tl.h"

/* Exported variables --------------------------------------------------------*/
extern MTL_output_t data_out;// = {.angles_array = {0.0f, 0.0f, 0.0f}};

/* Exported defines ----------------------------------------------------------*/

#define BOARD_NAME "IKS01A2"

/* Exported functions --------------------------------------------------------*/
void MX_MEMS_Init(void);
void MX_MEMS_Process(void);

void RTC_DateRegulate(uint8_t y, uint8_t m, uint8_t d, uint8_t dw);
void RTC_TimeRegulate(uint8_t hh, uint8_t mm, uint8_t ss);
void Get_PresentationString(char *PresentationString, uint32_t *Length);

#ifdef __cplusplus
}
#endif

#endif /* __APP_X_CUBE_MEMS1_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
