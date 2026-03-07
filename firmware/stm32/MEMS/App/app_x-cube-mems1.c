/**
  ******************************************************************************
  * File Name          : stmicroelectronics_x-cube-mems1_7_1_0.c
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

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_x-cube-mems1.h"
#include "main.h"
#include <stdio.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_nucleo.h"
#include "com.h"
#include "demo_serial.h"
#include "motion_tl_manager.h"
#include "bsp_ip_conf.h"
#include "fw_version.h"

//#include "motion_tl.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ALGO_FREQ  50U /* Algorithm frequency [Hz] */

/* Public variables ----------------------------------------------------------*/
volatile uint8_t DataLoggerActive = 0;
volatile uint32_t SensorsEnabled = 0;
char lib_version[35];
int lib_version_len;
MTL_angle_mode_t AngleMode = MODE_PITCH_ROLL_GRAVITY_INCLINATION;

MTL_output_t data_out = {.angles_array = {0.0f, 0.0f, 0.0f}};

/* USER CODE BEGIN PV 0 */
#include "controller.h"

volatile uint8_t DataRead = 0;
/* USER CODE END PV 0 */

/* Extern variables ----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static volatile uint8_t SensorReadRequest = 0;
static MOTION_SENSOR_Axes_t AccValue;

/* Private function prototypes -----------------------------------------------*/
static void MX_TiltSensing_Init(void);
static void MX_TiltSensing_Process(void);

static void Init_Sensors(void);
static void RTC_Handler(TMsg *Msg);
static void TL_Data_Handler(TMsg *Msg);
static void Accelero_Sensor_Handler(TMsg *Msg);
static void Gyro_Sensor_Handler(TMsg *Msg);
static void Magneto_Sensor_Handler(TMsg *Msg);
static void Pressure_Sensor_Handler(TMsg *Msg);
static void Humidity_Sensor_Handler(TMsg *Msg);
static void Temperature_Sensor_Handler(TMsg *Msg);
static void TIM_Config(uint32_t Freq);
static void DWT_Start(void);
static uint32_t DWT_Stop(void);

#ifdef BSP_IP_MEMS_INT1_PIN_NUM
static void MEMS_INT1_Force_Low(void);
static void MEMS_INT1_Init(void);
#endif

void MX_MEMS_Init(void)
{
  /* USER CODE BEGIN SV */

  /* USER CODE END SV */

  /* USER CODE BEGIN MEMS_Init_PreTreatment */

  /* USER CODE END MEMS_Init_PreTreatment */

  /* Initialize the peripherals and the MEMS components */

  MX_TiltSensing_Init();

  /* USER CODE BEGIN MEMS_Init_PostTreatment */

  /* USER CODE END MEMS_Init_PostTreatment */
}

/*
 * LM background task
 */
void MX_MEMS_Process(void)
{
  /* USER CODE BEGIN MEMS_Process_PreTreatment */

  /* USER CODE END MEMS_Process_PreTreatment */

  MX_TiltSensing_Process();

  /* USER CODE BEGIN MEMS_Process_PostTreatment */
  newData = 1;
  /* USER CODE END MEMS_Process_PostTreatment */
}

/* Exported functions --------------------------------------------------------*/
/**
 * @brief  Period elapsed callback
 * @param  htim pointer to a TIM_HandleTypeDef structure that contains
 *              the configuration information for TIM module.
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == BSP_IP_TIM_Handle.Instance)
  {
	/* USER CODE BEGIN HANDLER 0 */
	  if(DataLoggerActive == 1 || DataRead == 1)
	  {
		  SensorReadRequest = 1;
	  }
	/* USER CODE END HANDLER 0 */
  }
}

/**
 * @brief  Configures the current date
 * @param  y the year value to be set
 * @param  m the month value to be set
 * @param  d the day value to be set
 * @param  dw the day-week value to be set
 * @retval None
 */
void RTC_DateRegulate(uint8_t y, uint8_t m, uint8_t d, uint8_t dw)
{
  RTC_DateTypeDef sdatestructure;

  sdatestructure.Year    = y;
  sdatestructure.Month   = m;
  sdatestructure.Date    = d;
  sdatestructure.WeekDay = dw;

  if (HAL_RTC_SetDate(&hrtc, &sdatestructure, FORMAT_BIN) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/**
 * @brief  Configures the current time
 * @param  hh the hour value to be set
 * @param  mm the minute value to be set
 * @param  ss the second value to be set
 * @retval None
 */
void RTC_TimeRegulate(uint8_t hh, uint8_t mm, uint8_t ss)
{
  RTC_TimeTypeDef stimestructure;

  stimestructure.Hours          = hh;
  stimestructure.Minutes        = mm;
  stimestructure.Seconds        = ss;
  stimestructure.SubSeconds     = 0;
  stimestructure.TimeFormat     = RTC_HOURFORMAT12_AM;
  stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

  if (HAL_RTC_SetTime(&hrtc, &stimestructure, FORMAT_BIN) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/**
 * @brief  Gets Presentation string
 * @param  PresentationString the Presentation string
 * @param  Length the length of Presentation string
 * @retval None
 */
void Get_PresentationString(char *PresentationString, uint32_t *Length)
{
  char fw_id[] = {"16"}; /* FW ID for Unicleo-GUI */
  const uint8_t string_pointer_shift = strlen("ST MotionXX v"); /* Shift string pointer by this amount */
  char *lib_version_num;
  char lib_version_string[64];
  int lib_version_len = 0;

  MotionTL_manager_get_version(lib_version_string, &lib_version_len);

  /* Shorten library version string (e.g.: ST MotionXX v1.0.0) to contain version number only (e.g.: 1.0.0) */
  if (lib_version_len > string_pointer_shift)
  {
    lib_version_num = lib_version_string + string_pointer_shift;
  }
  else
  {
    lib_version_num = lib_version_string;
  }

  *Length = snprintf(PresentationString, 64, "MEMS shield demo,%s,%s,%s,%s", fw_id, FW_VERSION, lib_version_num, BOARD_NAME);
}

/**
 * @brief  Collect accelerometer data
 * @param  cal_data Pointer to 2D array of calibration data cal_data[num_records][3]
 * @param  num_records Number of records to be taken (3 axes per record)
 * @retval 0  Ok
 * @retval 1  Accelerometer error
 */
uint8_t CollectData(float cal_data[][3], uint32_t num_records)
{
  uint32_t i = 0;

  if ((SensorsEnabled & ACCELEROMETER_SENSOR) != ACCELEROMETER_SENSOR)
  {
    return 1;
  }

  /* Discard previous data */
  SensorReadRequest = 0;

  while (i < num_records)
  {
    if (SensorReadRequest == 1)
    {
      SensorReadRequest = 0;

      BSP_SENSOR_ACC_GetAxes(&AccValue);

      cal_data[i][0] = (float)AccValue.x / 1000.0f;
      cal_data[i][1] = (float)AccValue.y / 1000.0f;
      cal_data[i][2] = (float)AccValue.z / 1000.0f;
      i++;
    }
  }

  return 0;
}

/**
 * @brief  Get estimated measurement time
 * @param  time_s Pointer to time in [s]
 * @param  num_records Number of records taken
 * @retval None
 */
void GetEstimatedMeasTime(float *time_s, uint32_t num_records)
{
  float odr = 0.0f;

  if ((SensorsEnabled & ACCELEROMETER_SENSOR) != ACCELEROMETER_SENSOR)
  {
    *time_s = 0.0f;
    return;
  }

  BSP_SENSOR_ACC_Enable();

  BSP_SENSOR_ACC_GetOutputDataRate(&odr);

  if (odr > 0.001f)
  {
    *time_s = (float)num_records / odr;
  }
}

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initialize the TiltSensing application
  * @retval None
  */
static void MX_TiltSensing_Init(void)
{
#ifdef BSP_IP_MEMS_INT1_PIN_NUM
  /* Force MEMS INT1 pin of the sensor low during startup in order to disable I3C and enable I2C. This function needs
   * to be called only if user wants to disable I3C / enable I2C and didn't put the pull-down resistor to MEMS INT1 pin
   * on his HW setup. This is also the case of usage X-NUCLEO-IKS01A2 or X-NUCLEO-IKS01A3 expansion board together with
   * sensor in DIL24 adapter board where the LDO with internal pull-up is used.
   */
  MEMS_INT1_Force_Low();
#endif

  /* Initialize LED */
  BSP_LED_Init(LED2);

  /* Initialize Virtual COM Port */
  BSP_COM_Init(COM1);

  /* Initialize Timer */
  BSP_IP_TIM_Init();

  /* Configure Timer to run with desired algorithm frequency */
  TIM_Config(ALGO_FREQ);

  /* Initialize (disabled) sensors */
  Init_Sensors();

#ifdef BSP_IP_MEMS_INT1_PIN_NUM
  /* Initialize MEMS INT1 pin back to it's default state after I3C disable / I2C enable */
  MEMS_INT1_Init();
#endif

  /* TiltSensing API initialization function */
  MotionTL_manager_init();

  /* OPTIONAL */
  /* Get library version */
  MotionTL_manager_get_version(lib_version, &lib_version_len);

  BSP_LED_On(LED2);
  HAL_Delay(500);
  BSP_LED_Off(LED2);

#if 0
  /* Start receiving messages via DMA */
  UART_StartReceiveMsg();
#endif
}

#define USE_MY_COMMANDS

/**
  * @brief  Process of the TiltSensing application
  * @retval None
  */
static void MX_TiltSensing_Process(void)
{
  static TMsg msg_dat;
  static TMsg msg_cmd;

#ifdef USE_MY_COMMANDS
  SensorReadRequest = 0;

  /* Acquire data from enabled sensors and fill Msg stream */
  RTC_Handler(&msg_dat);

  if ((SensorsEnabled & ACCELEROMETER_SENSOR) == ACCELEROMETER_SENSOR)
  {
    BSP_SENSOR_ACC_GetAxes(&AccValue);
  }

  MOTION_SENSOR_Axes_t gyr_value;
  if ((SensorsEnabled & GYROSCOPE_SENSOR) == GYROSCOPE_SENSOR)
  {
    BSP_SENSOR_GYR_GetAxes(&gyr_value);
  }

  MOTION_SENSOR_Axes_t mag_value;
  if ((SensorsEnabled & MAGNETIC_SENSOR) == MAGNETIC_SENSOR)
  {
    BSP_SENSOR_MAG_GetAxes(&mag_value);
  }

  float hum_value;
  if ((SensorsEnabled & HUMIDITY_SENSOR) == HUMIDITY_SENSOR)
  {
    BSP_SENSOR_HUM_GetValue(&hum_value);
//    (void)memcpy(&Msg->Data[15], (void *)&hum_value, sizeof(float));;
  }

  float temp_value;
  if ((SensorsEnabled & TEMPERATURE_SENSOR) == TEMPERATURE_SENSOR)
  {
    BSP_SENSOR_TEMP_GetValue(&temp_value);
//    (void)memcpy(&Msg->Data[11], (void *)&temp_value, sizeof(float));
  }

  float press_value;
  if ((SensorsEnabled & PRESSURE_SENSOR) == PRESSURE_SENSOR)
  {
    BSP_SENSOR_PRESS_GetValue(&press_value);
//    (void)memcpy(&Msg->Data[7], (void *)&press_value, sizeof(float));
  }

  /* TiltSensing specific part */
  uint32_t elapsed_time_us = 0U;
  MTL_input_t data_in = {.acc_x = 0.0f, .acc_y = 0.0f, .acc_z = 0.0f, .deltatime_s = 0.0f};
  //static MTL_output_t data_out = {.angles_array = {0.0f, 0.0f, 0.0f}};

  static uint32_t tick_prev = 0;
  uint32_t tick_curr = HAL_GetTick();

  if ((SensorsEnabled & ACCELEROMETER_SENSOR) == ACCELEROMETER_SENSOR)
  {
    /* Convert acceleration from [mg] to [g] */
    data_in.acc_x = (float)AccValue.x / 1000.0f;
    data_in.acc_y = (float)AccValue.y / 1000.0f;
    data_in.acc_z = (float)AccValue.z / 1000.0f;
    data_in.deltatime_s = (float)(tick_curr - tick_prev) / 1000.0f;

    /* Run Tilt Sensing algorithm */
    BSP_LED_On(LED2);
    DWT_Start();
    MotionTL_manager_run(&data_in);
    MotionTL_manager_getAngles(&data_out, AngleMode);
    elapsed_time_us = DWT_Stop();
    BSP_LED_Off(LED2);

    //Serialize_s32(&Msg->Data[67], (int32_t)elapsed_time_us, 4);

//    tick_prev = tick_curr;
  }

#else
  if (UART_ReceivedMSG((TMsg *)&msg_cmd) != 1)
  {
    if (msg_cmd.Data[0] == DEV_ADDR)
    {
      (void)HandleMSG((TMsg *)&msg_cmd);
    }
  }

  /* USER CODE BEGIN Tilt_Sensing 0 */
  if (SensorReadRequest == 1U)
  {
    SensorReadRequest = 0;

    /* Acquire data from enabled sensors and fill Msg stream */
    RTC_Handler(&msg_dat);
    Accelero_Sensor_Handler(&msg_dat);
    Gyro_Sensor_Handler(&msg_dat);
    Magneto_Sensor_Handler(&msg_dat);
    Humidity_Sensor_Handler(&msg_dat);
    Temperature_Sensor_Handler(&msg_dat);
    Pressure_Sensor_Handler(&msg_dat);

    /* TiltSensing specific part */
    TL_Data_Handler(&msg_dat);

    if(DataRead == 1)
    {
    	DataRead = 0;
    	msg_dat.Data[0] = 0;
    }

    /* Send data stream */
    INIT_STREAMING_HEADER(&msg_dat);
    msg_dat.Len = STREAMING_MSG_LENGTH;
    UART_SendMsg(&msg_dat);
    /* USER CODE END Tilt_Sensing 0 */
  }
#endif

}

/**
 * @brief  Initialize all sensors
 * @param  None
 * @retval None
 */
static void Init_Sensors(void)
{
  BSP_SENSOR_ACC_Init();
  BSP_SENSOR_GYR_Init();
  BSP_SENSOR_MAG_Init();
  BSP_SENSOR_PRESS_Init();
  BSP_SENSOR_TEMP_Init();
  BSP_SENSOR_HUM_Init();

  /* Set accelerometer:
   *   - ODR >= 50Hz
   *   - FS   = <-4g, 4g>
   */
  BSP_SENSOR_ACC_SetOutputDataRate(50.0f);
  BSP_SENSOR_ACC_SetFullScale(4);
}

/**
 * @brief  Handles the time+date getting/sending
 * @param  Msg the time+date part of the stream
 * @retval None
 */
static void RTC_Handler(TMsg *Msg)
{
  uint8_t sub_sec = 0;
  RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructure;
  uint32_t ans_uint32;
  int32_t ans_int32;
  uint32_t RtcSynchPrediv = hrtc.Init.SynchPrediv;

  (void)HAL_RTC_GetTime(&hrtc, &stimestructure, FORMAT_BIN);
  (void)HAL_RTC_GetDate(&hrtc, &sdatestructureget, FORMAT_BIN);

  /* To be MISRA C-2012 compliant the original calculation:
     sub_sec = ((((((int)RtcSynchPrediv) - ((int)stimestructure.SubSeconds)) * 100) / (RtcSynchPrediv + 1)) & 0xFF);
     has been split to separate expressions */
  ans_int32 = (RtcSynchPrediv - (int32_t)stimestructure.SubSeconds) * 100;
  ans_int32 /= RtcSynchPrediv + 1;
  ans_uint32 = (uint32_t)ans_int32 & 0xFFU;
  sub_sec = (uint8_t)ans_uint32;

  Msg->Data[3] = (uint8_t)stimestructure.Hours;
  Msg->Data[4] = (uint8_t)stimestructure.Minutes;
  Msg->Data[5] = (uint8_t)stimestructure.Seconds;
  Msg->Data[6] = sub_sec;
}

/**
 * @brief  Tilt Sensing data handler
 * @param  Msg the Tilt Sensing data part of the stream
 * @retval None
 */
static void TL_Data_Handler(TMsg *Msg)
{
  uint32_t elapsed_time_us = 0U;
  MTL_input_t data_in = {.acc_x = 0.0f, .acc_y = 0.0f, .acc_z = 0.0f, .deltatime_s = 0.0f};
  static MTL_output_t data_out = {.angles_array = {0.0f, 0.0f, 0.0f}};

  static uint32_t tick_prev = 0;
  uint32_t tick_curr = HAL_GetTick();

  if ((SensorsEnabled & ACCELEROMETER_SENSOR) == ACCELEROMETER_SENSOR)
  {
    /* Convert acceleration from [mg] to [g] */
    data_in.acc_x = (float)AccValue.x / 1000.0f;
    data_in.acc_y = (float)AccValue.y / 1000.0f;
    data_in.acc_z = (float)AccValue.z / 1000.0f;
    data_in.deltatime_s = (float)(tick_curr - tick_prev) / 1000.0f;

    /* Run Tilt Sensing algorithm */
    BSP_LED_On(LED2);
    DWT_Start();
    MotionTL_manager_run(&data_in);
    MotionTL_manager_getAngles(&data_out, AngleMode);
    elapsed_time_us = DWT_Stop();
    BSP_LED_Off(LED2);

    FloatToArray(&Msg->Data[55], data_out.angles_array[0]);
    FloatToArray(&Msg->Data[59], data_out.angles_array[1]);
    FloatToArray(&Msg->Data[63], data_out.angles_array[2]);

    Serialize_s32(&Msg->Data[67], (int32_t)elapsed_time_us, 4);

    tick_prev = tick_curr;
  }
}

/**
 * @brief  Handles the ACC axes data getting/sending
 * @param  Msg the ACC part of the stream
 * @retval None
 */
static void Accelero_Sensor_Handler(TMsg *Msg)
{
  if ((SensorsEnabled & ACCELEROMETER_SENSOR) == ACCELEROMETER_SENSOR)
  {
    BSP_SENSOR_ACC_GetAxes(&AccValue);
    Serialize_s32(&Msg->Data[19], (int32_t)AccValue.x, 4);
    Serialize_s32(&Msg->Data[23], (int32_t)AccValue.y, 4);
    Serialize_s32(&Msg->Data[27], (int32_t)AccValue.z, 4);
  }
}

/**
 * @brief  Handles the GYR axes data getting/sending
 * @param  Msg the GYR part of the stream
 * @retval None
 */
static void Gyro_Sensor_Handler(TMsg *Msg)
{
  MOTION_SENSOR_Axes_t gyr_value;

  if ((SensorsEnabled & GYROSCOPE_SENSOR) == GYROSCOPE_SENSOR)
  {
    BSP_SENSOR_GYR_GetAxes(&gyr_value);
    Serialize_s32(&Msg->Data[31], gyr_value.x, 4);
    Serialize_s32(&Msg->Data[35], gyr_value.y, 4);
    Serialize_s32(&Msg->Data[39], gyr_value.z, 4);
  }
}

/**
 * @brief  Handles the MAG axes data getting/sending
 * @param  Msg the MAG part of the stream
 * @retval None
 */
static void Magneto_Sensor_Handler(TMsg *Msg)
{
  MOTION_SENSOR_Axes_t mag_value;

  if ((SensorsEnabled & MAGNETIC_SENSOR) == MAGNETIC_SENSOR)
  {
    BSP_SENSOR_MAG_GetAxes(&mag_value);
    Serialize_s32(&Msg->Data[43], mag_value.x, 4);
    Serialize_s32(&Msg->Data[47], mag_value.y, 4);
    Serialize_s32(&Msg->Data[51], mag_value.z, 4);
  }
}

/**
 * @brief  Handles the PRESS sensor data getting/sending.
 * @param  Msg the PRESS part of the stream
 * @retval None
 */
static void Pressure_Sensor_Handler(TMsg *Msg)
{
  float press_value;

  if ((SensorsEnabled & PRESSURE_SENSOR) == PRESSURE_SENSOR)
  {
    BSP_SENSOR_PRESS_GetValue(&press_value);
    (void)memcpy(&Msg->Data[7], (void *)&press_value, sizeof(float));
  }
}

/**
 * @brief  Handles the TEMP axes data getting/sending
 * @param  Msg the TEMP part of the stream
 * @retval None
 */
static void Temperature_Sensor_Handler(TMsg *Msg)
{
  float temp_value;

  if ((SensorsEnabled & TEMPERATURE_SENSOR) == TEMPERATURE_SENSOR)
  {
    BSP_SENSOR_TEMP_GetValue(&temp_value);
    (void)memcpy(&Msg->Data[11], (void *)&temp_value, sizeof(float));
  }
}

/**
 * @brief  Handles the HUM axes data getting/sending
 * @param  Msg the HUM part of the stream
 * @retval None
 */
static void Humidity_Sensor_Handler(TMsg *Msg)
{
  float hum_value;

  if ((SensorsEnabled & HUMIDITY_SENSOR) == HUMIDITY_SENSOR)
  {
    BSP_SENSOR_HUM_GetValue(&hum_value);
    (void)memcpy(&Msg->Data[15], (void *)&hum_value, sizeof(float));;
  }
}

/**
 * @brief  Timer configuration
 * @param  Freq the desired Timer frequency
 * @retval None
 */
static void TIM_Config(uint32_t Freq)
{
  const uint32_t tim_counter_clock = 2000; /* TIM counter clock 2 kHz */
  uint32_t prescaler_value = (uint32_t)((SystemCoreClock / tim_counter_clock) - 1);
  uint32_t period = (tim_counter_clock / Freq) - 1;

  BSP_IP_TIM_Handle.Init.Prescaler         = prescaler_value;
  BSP_IP_TIM_Handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  BSP_IP_TIM_Handle.Init.Period            = period;
  BSP_IP_TIM_Handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  BSP_IP_TIM_Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&BSP_IP_TIM_Handle) != HAL_OK)
  {
    Error_Handler();
  }
}

#ifdef BSP_IP_MEMS_INT1_PIN_NUM
/**
 * @brief  Force MEMS INT1 pin low
 * @param  None
 * @retval None
 */
static void MEMS_INT1_Force_Low(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = BSP_IP_MEMS_INT1_PIN_NUM;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BSP_IP_MEMS_INT1_GPIOX, &GPIO_InitStruct);

  HAL_GPIO_WritePin(BSP_IP_MEMS_INT1_GPIOX, BSP_IP_MEMS_INT1_PIN_NUM, GPIO_PIN_RESET);
}

/**
 * @brief  Configure MEMS INT1 pin to the default state
 * @param  None
 * @retval None
 */
static void MEMS_INT1_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = BSP_IP_MEMS_INT1_PIN_NUM;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BSP_IP_MEMS_INT1_GPIOX, &GPIO_InitStruct);
}
#endif

/**
 * @brief  Start counting clock cycles
 * @param  None
 * @retval None
 */
static void DWT_Start(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0; /* Clear count of clock cycles */
  DWT->CTRL |= 1;  /* Enable counter */
}

/**
 * @brief  Stop counting clock cycles and calculate elapsed time in [us]
 * @param  None
 * @retval Elapsed time in [us]
 */
static uint32_t DWT_Stop(void)
{
  volatile uint32_t cycles_count = 0U;
  uint32_t system_core_clock_mhz = 0U;

  DWT->CTRL = DWT->CTRL ^ 1;  /* Disable counter */
  cycles_count = DWT->CYCCNT; /* Read count of clock cycles */

  /* Calculate elapsed time in [us] */
  system_core_clock_mhz = SystemCoreClock / 1000000U;
  return cycles_count / system_core_clock_mhz;
}

#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
