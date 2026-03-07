/*
 * MotorClass.cpp
 *
 *  Created on: Oct 6, 2022
 *      Author: rossa
 */

#include "stdint.h"
#include "stm32f4xx.h"
#include "../Inc/Motor.h"

#ifdef __USE_FREERTOS__
#include "cmsis_os.h"
#endif

uint8_t timerState[3] =  {0, 0, 0 };

BridgeStatus_t bridgeA;
BridgeStatus_t bridgeB;
MotorStatus_t stateA;
MotorStatus_t stateB;
uint8_t vref;
uint16_t vref_freq;
uint16_t mot_pwm_freq;
motorDir_t dirA;
motorDir_t dirB;
int16_t speed_A;
int16_t speed_B;
bool isError;
/// Timer handler for input PWM of Bridge A
TIM_HandleTypeDef hTimPwmA;
/// Timer handler for input PWM of Bridge B
TIM_HandleTypeDef hTimPwmB;
/// Timer handler for REF PWM
TIM_HandleTypeDef hTimPwmRef;

//MotorClass *motor = new MotorClass();

//void MotorRun(Motor_t mot, motorDir_t dir, uint16_t val);

/*MotorClass::MotorClass() {
	// TODO Auto-generated constructor stub
	bridgeA = DISABLED;
	bridgeB = DISABLED;
	stateA = INACTIVE;
	stateB = INACTIVE;
	speed_A = 0;
	speed_B = 0;
	dirA = UNKNOWN;
	dirB = UNKNOWN;
	vref = 0;
	isError = false;
	vref_freq = 15000;
	mot_pwm_freq = 10000;

	// Initialize the bridge
	this->init();
}

MotorClass::~MotorClass() {
	// TODO Auto-generated destructor stub
}
*/

/**
 * 	Function Prototypes
 */
void setPWMFrequency(TIM_HandleTypeDef *htim, uint32_t ch, float freq);
void ErrorHandler(void);

/**
 * Configure the Vref PWM
 */
int InitMotorVref()
{
	HAL_TIM_PWM_Init(&hTimPwmRef);
	HAL_TIM_PWM_Start(&hTimPwmRef, TIM_CHANNEL_1);

	TIM_OC_InitTypeDef sConfigOC = {0};
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 4000;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

	HAL_TIM_PWM_ConfigChannel(&hTimPwmRef, &sConfigOC, TIM_CHANNEL_1);
	if(HAL_TIM_PWM_ConfigChannel(&hTimPwmRef, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		return -1;
	}

	HAL_TIM_PWM_Start(&hTimPwmRef, TIM_CHANNEL_1);
	setPWMFrequency(&hTimPwmRef, TIM_CHANNEL_1, 10000);

	return 0;
}

/**
 * Configure the Motor PWM
 */
int InitMotorPwm()
{
//	HAL_TIM_PWM_Init(&hTimPwmA);
//	HAL_TIM_PWM_Init(&hTimPwmB);

	 TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	  TIM_MasterConfigTypeDef sMasterConfig = {0};
	  TIM_OC_InitTypeDef sConfigOC = {0};

	  /* USER CODE BEGIN TIM3_Init 1 */

	  /* USER CODE END TIM3_Init 1 */
	  hTimPwmA.Instance = TIM3;
	  hTimPwmA.Init.Prescaler = 1;
	  hTimPwmA.Init.CounterMode = TIM_COUNTERMODE_UP;
	  hTimPwmA.Init.Period = 65535;
	  hTimPwmA.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	  hTimPwmA.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	  if (HAL_TIM_Base_Init(&hTimPwmA) != HAL_OK)
	  {
	    ErrorHandler();
	  }
	  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	  if (HAL_TIM_ConfigClockSource(&hTimPwmA, &sClockSourceConfig) != HAL_OK)
	  {
	    ErrorHandler();
	  }
	  if (HAL_TIM_PWM_Init(&hTimPwmA) != HAL_OK)
	  {
	    ErrorHandler();
	  }
	  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	  if (HAL_TIMEx_MasterConfigSynchronization(&hTimPwmA, &sMasterConfig) != HAL_OK)
	  {
	    ErrorHandler();
	  }
	  sConfigOC.OCMode = TIM_OCMODE_PWM1;
	  sConfigOC.Pulse = 0;
	  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	  if (HAL_TIM_PWM_ConfigChannel(&hTimPwmA, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	  {
	    ErrorHandler();
	  }
	  if (HAL_TIM_PWM_ConfigChannel(&hTimPwmA, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	  {
	    ErrorHandler();
	  }
	  /* USER CODE BEGIN TIM3_Init 2 */

	  /* USER CODE END TIM3_Init 2 */
//	  HAL_TIM_MspPostInit(&hTimPmwA);
	  return 0;
}

void motor_init()
{
	bridgeA = DISABLED;
	bridgeB = DISABLED;
	stateA = INACTIVE;
	stateB = INACTIVE;
	speed_A = 0;
	speed_B = 0;
	dirA = UNKNOWN;
	dirB = UNKNOWN;
	vref = 0;
	isError = false;
	vref_freq = 15000;
	mot_pwm_freq = 10000;

	InitMotorPwm();

	// Enable the Bridge
	if(!motor_EnableBridge())
	{
		ErrorHandler();
	}
	// Reset the bridge
	if(motor_getReset() != false)
	{
		motor_Board_Reset();
		motor_Board_Release_Reset();
	}
	// Configure the Vref
/*	if(!motor_SetVrefFreq(15000, 50))
	{
		motor_HardStop();
	}
	if (!motor_SetVref(100))
	{
		motor_HardStop();
	}
*/
	motor_StartMotor(MOTOR_A);
	motor_StartMotor(MOTOR_B);

	if(InitMotorVref())
	{
		ErrorHandler();
	}
	// Set Vref to 80%
	SetVref(&hTimPwmRef, 80);

	// Configure the PWM
	if(!motor_SetMotorFreq(10000, 0))
	{
		motor_HardStop();
	}
	if(!motor_SetSpeed(MOTOR_A, 0))
	{
		motor_HardStop();
	}
	if(!motor_SetSpeed(MOTOR_B, 0))
	{
		motor_HardStop();
	}
}

bool motor_SetVref(uint8_t val)
{
	//uint8_t foo;

	if(val > 100)
	{
		vref = 100;
	}
	vref = val;

	if(!motor_getError())
	{
		motor_SetMotorFreq(vref_freq, vref);
	}

	return true;
}

uint8_t motor_GetVref()
{
	return vref;
}

bool motor_SetSpeed(Motor_t motor, int16_t speed)
{
	/// If there is an error do nothing
	if(!motor_getError())
	{
		return false;
	}

	/// If the bridge is not enabled do nothing
	if(motor == MOTOR_A)
	{
		// If bridge A is not enabled do nothing
		if(stateA == INACTIVE)
		{
			return false;
		}

		/// Release reset if needed
		if(motor_getReset())
		{
			motor_Board_Release_Reset();
		}

		/// Check if direction has to be changed
		motorDir_t dir;
		if(speed >= 0)
		{
			dir = FORWARD;
		}
		else
		{
			speed = BACKWARD;
		}
		if(dirA != dir)
		{
			// Direction has changed
			dirA = dir;
			motor_StopMotorA();
			motor_Board_SetDirection(MOTOR_A, dir);
			stateA = STEADY;
		}

		speed_A = speed;
		if(speed_A >= 0)
		{
			stateA = RUN;
			dirA = FORWARD;
			motor_Board_SetDirection(MOTOR_A, FORWARD);
			motor_Board_PwmSetFreq(MOTOR_A, mot_pwm_freq, speed_A);
		}
		else
		{
			stateA = RUN;
			dirA = BACKWARD;
			motor_Board_SetDirection(MOTOR_A, BACKWARD);
			motor_Board_PwmSetFreq(MOTOR_A, mot_pwm_freq, -speed_A);
		}
		return true;
	}
	else if(motor == MOTOR_B)
	{
		// If bridge B is not enabled do nothing
		if(stateB == INACTIVE)
		{
			return false;
		}

		/// Release reset if needed
		if(motor_getReset())
		{
			motor_Board_Release_Reset();
		}

		/// Check if direction has to be changed
		motorDir_t dir;
		if(speed >= 0)
		{
			dir = FORWARD;
		}
		else
		{
			speed = BACKWARD;
		}
		if(dirB != dir)
		{
			// Direction has changed
			dirB = dir;
			motor_StopMotorB();
			motor_Board_SetDirection(MOTOR_B, dir);
			stateB = STEADY;
		}

		speed_B = speed;
		if(speed_B >= 0)
		{
			stateB = RUN;
			dirB = FORWARD;
			motor_Board_SetDirection(MOTOR_B, FORWARD);
			motor_Board_PwmSetFreq(MOTOR_B, mot_pwm_freq, speed_B);
		}
		else
		{
			stateB = RUN;
			dirB = BACKWARD;
			motor_Board_SetDirection(MOTOR_B, BACKWARD);
			motor_Board_PwmSetFreq(MOTOR_B, mot_pwm_freq, -speed_B);
		}
		return true;
	}
	return false;
}

int16_t motor_GetSpeed(Motor_t motor)
{
	if(motor == MOTOR_A)
		return speed_A;
	else if (motor == MOTOR_B)
		return speed_B;
	else return 0;
}

bool motor_EnableBridge(void)
{
	  GPIO_InitTypeDef GPIO_InitStruct;
	  uint32_t gpioPin;
	  GPIO_TypeDef* gpioPort;
	  IRQn_Type flagIrqn;

	  gpioPin = BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PIN;
	  gpioPort = BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PORT;
	  flagIrqn = EXTI_FAULT_IRQn;

	  HAL_GPIO_WritePin(gpioPort, gpioPin, GPIO_PIN_SET);

	  if (true) //addDelay != 0)
	  {
	    HAL_Delay(BSP_MOTOR_CONTROL_BOARD_BRIDGE_TURN_ON_DELAY);
	  }
	  /* Configure the GPIO connected to EN pin to take interrupt */
	  GPIO_InitStruct.Pin = gpioPin;
	  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	  GPIO_InitStruct.Pull = GPIO_PULLUP;
	  GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
	  HAL_GPIO_Init(gpioPort, &GPIO_InitStruct);

	  __HAL_GPIO_EXTI_CLEAR_IT(gpioPin);
	  HAL_NVIC_ClearPendingIRQ(flagIrqn);
	  HAL_NVIC_EnableIRQ(flagIrqn);

	  return true;
}

bool motor_DisableBridge(void)
{
	  GPIO_InitTypeDef GPIO_InitStruct;
	  uint32_t gpioPin;
	  GPIO_TypeDef* gpioPort;

	  gpioPin = BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PIN;
	  gpioPort = BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PORT;

	  /* Configure the GPIO connected to EN pin as an output */
	  GPIO_InitStruct.Pin = gpioPin;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
	  HAL_GPIO_Init(gpioPort, &GPIO_InitStruct);

	  __disable_irq();
	  HAL_GPIO_WritePin(gpioPort, gpioPin, GPIO_PIN_RESET);
	  __HAL_GPIO_EXTI_CLEAR_IT(gpioPin);
	  __enable_irq();

	  return true;
}

bool motor_HardStop()
{
	//Board_PwmStop(0);
	//Board_PwmStop(1);
	motor_DisableBridge();
	return true;
}

uint8_t motor_getError()
{
	  return (uint8_t)(HAL_GPIO_ReadPin(BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PORT, BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PIN));
}

bool motor_getReset(void)
{
	  return (bool)HAL_GPIO_ReadPin(BSP_MOTOR_CONTROL_BOARD_RESET_PORT, BSP_MOTOR_CONTROL_BOARD_RESET_PIN);
}

void motor_Board_Reset(void)
{
	HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_RESET_PORT, BSP_MOTOR_CONTROL_BOARD_RESET_PIN, GPIO_PIN_RESET);

#ifdef __USE_FREERTOS__
	osDelay(BSP_MOTOR_CONTROL_BOARD_EXIT_STANDBY_MODE_DELAY);
#else
	HAL_Delay(BSP_MOTOR_CONTROL_BOARD_EXIT_STANDBY_MODE_DELAY);
#endif
}

bool motor_SetVrefFreq(uint32_t freq, uint8_t val)
{
	vref_freq = freq;

	motor_Board_PwmSetFreq(2, freq, 0);//val);
	return true;
}

bool motor_StopVref(void)
{
	motor_Board_PwmStop(2);
	return true;
}


void motor_Board_Release_Reset(void)
{
    //Set reset pin high
    HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_RESET_PORT, BSP_MOTOR_CONTROL_BOARD_RESET_PIN, GPIO_PIN_SET);

    //Let some time to the motor driver to exit standby mode
//    HAL_Delay(BSP_MOTOR_CONTROL_BOARD_EXIT_STANDBY_MODE_DELAY);
}

void motor_Board_Delay(uint32_t delay)
{
#ifdef __USE_FREERTOS__
	osDelay(delay);
#else
	HAL_Delay(delay);
#endif
}

bool motor_SetMotorFreq(uint32_t freq, uint16_t val)
{
	motor_Board_PwmSetFreq(MOTOR_A, freq, val);
	motor_Board_PwmSetFreq(MOTOR_B, freq, val);
	return true;
}

bool motor_StartMotor(Motor_t motor)
{
	if(motor == MOTOR_A)
	{
		bridgeA = ENABLED;
		stateA = STEADY;
		return true;
	}
	if(motor == MOTOR_B)
	{
		bridgeB = ENABLED;
		stateB = STEADY;
		return true;
	}
	return false;
}

bool motor_StopMotorA(void)
{
	motor_Board_PwmStop(MOTOR_A);
	return true;
}

bool motor_StopMotorB(void)
{
	motor_Board_PwmStop(MOTOR_B);
	return true;
}

/*
bool MotorClass::RunMotorA(uint16_t val)
{
	if(this->bridgeA != ENABLED)
	{
		return false;
	}

	/// Check motor A direction
	motorDir_t dir;
	if(val > 0)
	{
		dir = FORWARD;
	}
	if(val < 0)
	{
		dir = BACKWARD;
	}
	else
	{
		dir = UNKNOWN;
	}
	if(this->stateA == STOP || dir != this->dirA)
	{
		/// Motor is stopped or the direction has changed

		/// Release reset if necessary
		if(this->getReset())
		{
			this->Board_Release_Reset();
		}
		/// Eventually deactivate the motor A
		if(this->stateA != STOP)
		{
			this->StopMotorA();
		}
		/// Set the motor A direction
		this->Board_SetDirection(bridgeA, dir);
		/// Set Motor A to be ready to move
		this->stateA = STEADY;
		/// Enable bridge
		if(this->bridgeA != ENABLED)
		{
			this->EnableBridge();
		}
	}
	/// Update the PWM
	this->speed_A = val;
	this->Board_PwmSetFreq(bridgeA, mot_pwm_freq, speed_A);

	return true;
}

bool MotorClass::RunMotorB(uint16_t val) {
}
*/

void motor_Board_PwmSetFreq(uint8_t pwmId, uint32_t newFreq,
		uint8_t duty)
{
	uint32_t sysFreq = HAL_RCC_GetSysClockFreq();
	  TIM_HandleTypeDef *pHTim;
	  uint32_t period;
	  uint32_t pulse;
	  uint32_t channel;
/*
	  if ((pwmId == 0) || (pwmId == 1))
	  {
	    if ((timerState[0] == 0) &&
	        (timerState[1] == 0))
	    {

	      Stspin240_250_Board_PwmInit(pwmId , 0);
	    }
	    else if (timerState[pwmId] == 0)
	    {
	      Stspin240_250_Board_PwmInit(pwmId , 1);
	    }
	  }
*/
	  if(pwmId > 2)
	  {
		  while (true) ;
	  }

	  if(newFreq > STSPIN240_250_MAX_PWM_FREQ)
		  newFreq = STSPIN240_250_MAX_PWM_FREQ;
	  mot_pwm_freq = newFreq;
	  if(stateA == INACTIVE || stateB == INACTIVE)
		  return;

	  switch (pwmId)
	  {
	    case 0:
	    default:
	      pHTim = &hTimPwmA;
	      pHTim->Instance = BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_A;
	      channel = BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_A;
	      break;
	    case  1:
	      pHTim = &hTimPwmB;
	      pHTim->Instance = BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_B;
	      channel = BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_B;
	      break;
	    case  2:
	      pHTim = &hTimPwmRef;
	      pHTim->Instance = BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_REF;
	      channel = BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_REF;
	      break;
	  }

	   period = (sysFreq/ (TIMER_PRESCALER * newFreq)) - 1;

	  if (duty == 0)
	  {
	    pulse = 0 ;
	  }
	  else
	  {
	    if (duty > 100) duty = 100;
	    pulse = period * duty /100 +1;
	  }

	  if ((pwmId == 2) ||
	      ((pwmId == 1) && (timerState[0] == 0)) ||
	      ((pwmId == 0) && (timerState[1] == 0)))
	  {
	    // for PWMs which share the same timer
	    // only update frequency if the other channel is disabled
	    __HAL_TIM_SetAutoreload(pHTim, period);
	  }
	   __HAL_TIM_SetCompare(pHTim, channel, pulse);
	   HAL_TIM_PWM_Start(pHTim, channel);

	   timerState[pwmId] = 1;
}

void motor_Board_PwmStop(uint8_t pwmId)
{
	 GPIO_InitTypeDef  GPIO_InitStruct;

	  switch (pwmId)
	  {
	    case 0:
	    default:
	      if (hTimPwmA.Instance == BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_A)
	      {
	      HAL_TIM_PWM_Stop(&hTimPwmA,BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_A);
	      }
	      if (timerState[1] == 0)
	      {
//	        Stspin240_250_Board_PwmDeInit(0);
//	        Stspin240_250_Board_PwmDeInit(1);
	      }
	      else
	      {
	        HAL_GPIO_DeInit(BSP_MOTOR_CONTROL_BOARD_PWM_A_PORT, BSP_MOTOR_CONTROL_BOARD_PWM_A_PIN);

	        // Reconfigure PWMA pin as output pull down
	        GPIO_InitStruct.Pin = BSP_MOTOR_CONTROL_BOARD_PWM_A_PIN;
	        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	        GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
	        HAL_GPIO_Init(BSP_MOTOR_CONTROL_BOARD_PWM_A_PORT, &GPIO_InitStruct);
	        HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_PWM_A_PORT, BSP_MOTOR_CONTROL_BOARD_PWM_A_PIN, GPIO_PIN_RESET);
	      }
	      break;
	    case  1:
	      if (hTimPwmA.Instance == BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_B)
	      {
	      HAL_TIM_PWM_Stop(&hTimPwmB,BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_B);
	      }
	      if (timerState[0] == 0)
	      {
//	        Stspin240_250_Board_PwmDeInit(0);
//	        Stspin240_250_Board_PwmDeInit(1);
	      }
	      else
	      {
	        HAL_GPIO_DeInit(BSP_MOTOR_CONTROL_BOARD_PWM_B_PORT, BSP_MOTOR_CONTROL_BOARD_PWM_B_PIN);

	        // Reconfigure PWMB pin  as output pull down
	       GPIO_InitStruct.Pin = BSP_MOTOR_CONTROL_BOARD_PWM_B_PIN;
	       GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	       GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	       GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
	       HAL_GPIO_Init(BSP_MOTOR_CONTROL_BOARD_PWM_B_PORT, &GPIO_InitStruct);
	       HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_PWM_B_PORT, BSP_MOTOR_CONTROL_BOARD_PWM_B_PIN, GPIO_PIN_RESET);
	      }
	      break;
	    case  2:
	      HAL_TIM_PWM_Stop(&hTimPwmRef,BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_REF);
	      break;
	  }
	  timerState[pwmId] = 0;
}

void MotorEnableBridge()
{
	motor_EnableBridge();
}

void MotorDisableBridge()
{
	motor_DisableBridge();
}

uint8_t MotorGetError()
{
	return motor_getError();
}

void MotorHardStop()
{
	motor_HardStop();
}

void MotorReset()
{
	motor_Board_Reset();
}

void MotorReleaseReset()
{
	motor_Board_Release_Reset();
}

void SetVrefPWMFreq(uint16_t freq)
{
	motor_SetVrefFreq(freq, 50);
}

void SetVref(TIM_HandleTypeDef *hTimPwmRef, uint16_t uVref)
{
	if(uVref > 100) return;

	HAL_TIM_PWM_Stop(hTimPwmRef, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(hTimPwmRef, TIM_CHANNEL_1);
	setDutyCycle(hTimPwmRef, TIM_CHANNEL_1, uVref);
	vref = uVref;
}

void SetMotorPWMFreq(uint16_t freq)
{
	motor_SetMotorFreq(freq, 0);
}

void setPWMFrequency(TIM_HandleTypeDef *htim, uint32_t ch, float freq)
{
	uint32_t counter = (uint32_t)(PWM_CLK_FREQ / freq);

	htim->Init.Period = counter;
}

uint32_t getPWMFrequency(TIM_HandleTypeDef *htim)
{
	return (uint32_t) (PWM_CLK_FREQ / htim->Init.Period);
}

/*
void MotorRun(Motor_t mot, MotorStatus_t dir, uint16_t val)
{
	if(mot == MOTOR_A)
	{
		motor->SetSpeed(MOTOR_A, val);
	}
	else if (mot == MOTOR_B)
	{
		motor->SetSpeed(MOTOR_B, val);
	}
	else
	{
		// No nothing
	}
}
*/

void _LinkTimersHandler(TIM_HandleTypeDef TIM_MotorA, TIM_HandleTypeDef TIM_MotorB)
{
	hTimPwmA = TIM_MotorA;
	hTimPwmB = TIM_MotorB;
	//TIM_MotorA = &hTimPwmA;
	//TIM_MotorB = &hTimPwmB;
}

void motor_Board_SetDirection(uint8_t bridgeId, uint8_t gpioState)
{
	  if (bridgeId == 0)
	  {
	    HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_DIR_A_PORT, BSP_MOTOR_CONTROL_BOARD_DIR_A_PIN, (GPIO_PinState)gpioState);
	  }
	  else
	  {
	    HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_DIR_B_PORT, BSP_MOTOR_CONTROL_BOARD_DIR_B_PIN, (GPIO_PinState)gpioState);
	  }
}

void SetMotorPWM(Motor_t motor, int16_t val)
{
	if(motor == MOTOR_A)
	{
		if(bridgeA != ENABLED) return;
		if(val == 0)
		{
			motor_StopMotorA();
			stateA = STOP;
			return;
		}
		else if (val > 0)
		{
			if (val > 100) val = 100;
			motor_Board_SetDirection (0, FORWARD);
			HAL_TIM_PWM_Stop(&hTimPwmA, TIM_CHANNEL_1);
			HAL_TIM_PWM_Start(&hTimPwmA, TIM_CHANNEL_1);
			setDutyCycle(&hTimPwmA, TIM_CHANNEL_1, val);
			speed_A = val;
			stateA = RUN;
		}
		else if (val < 0)
		{
			if (val < -100) val = -100;
			motor_Board_SetDirection (0, BACKWARD);
			HAL_TIM_PWM_Stop(&hTimPwmA, TIM_CHANNEL_1);
			HAL_TIM_PWM_Start(&hTimPwmA, TIM_CHANNEL_1);
			setDutyCycle(&hTimPwmA, TIM_CHANNEL_1, -val);
			speed_A = val;
			stateA = RUN;
		}
	}
	else if(motor == MOTOR_B)
	{
		if(bridgeB != ENABLED) return;
		if(val == 0)
		{
			motor_StopMotorB();
			stateB = STOP;
			return;
		}
		else if (val > 0)
		{
			if (val > 100) val = 100;
			motor_Board_SetDirection (1, FORWARD);
			HAL_TIM_PWM_Stop(&hTimPwmB, TIM_CHANNEL_2);
			HAL_TIM_PWM_Start(&hTimPwmB, TIM_CHANNEL_2);
			setDutyCycle(&hTimPwmB, TIM_CHANNEL_2, val);
			speed_B = val;
			stateB = RUN;
		}
		else if (val < 0)
		{
			if (val < -100) val = -100;
			motor_Board_SetDirection (1, BACKWARD);
			HAL_TIM_PWM_Stop(&hTimPwmB, TIM_CHANNEL_2);
			HAL_TIM_PWM_Start(&hTimPwmB, TIM_CHANNEL_2);
			setDutyCycle(&hTimPwmB, TIM_CHANNEL_2, -val);
			speed_B = val;
			stateB = RUN;
		}
	}
	else return;
}

/**
 * Error Handler
 * Enters in forever loop
 */
void ErrorHandler(void)
{
	__disable_irq();
	while(true)
	{

	}
}

void setDutyCycle(TIM_HandleTypeDef *htim, uint32_t ch, /*float*/ uint16_t duty_cycle)
{
	if(duty_cycle > 100) duty_cycle = 100;
	if(duty_cycle < 0) duty_cycle = 0;

	float pw_resolution = (((float)(*htim).Init.Period + 1.0f) / 100.0f);

	uint16_t pw_desired = pw_resolution * duty_cycle;
	__HAL_TIM_SET_COMPARE(htim, ch, pw_desired);
}

//
//void init_motor()
//{
//	  //----- Init of the Motor control library
//	  /* Set the Stspin240_250 library to use 1 device */
//
///*	BSP_MotorControl_SetNbDevices(BSP_MOTOR_CONTROL_BOARD_ID_STSPIN240, 1); */
//
////	  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
//
//	  HAL_NVIC_SetPriority(SysTick_IRQn, 0,0);
//
//	  /* Set the Stspin240_250 library to use 1 device */
////	  BSP_MotorControl_SetNbDevices(BSP_MOTOR_CONTROL_BOARD_ID_STSPIN240, 1);
////	  motorDrvHandle = (motorDrv_t *)Stspin240_250_GetMotorHandle();
////	  motorDrvHandle->SetNbDevices(1);
//
//	  /* When BSP_MotorControl_Init is called with NULL pointer,                  */
//	  /* the Stspin240_250 library parameters are set with the predefined values from file   */
//	  /* stspin240_250_target_config.h, otherwise the registers are set using the   */
//	  /* Stspin240_250_Init_t pointer structure                */
//	  /* Uncomment the call to BSP_MotorControl_Init below to initialize the      */
//	  /* device with the structure gStspin240_250InitParams declared in the the main.c file */
//	  /* and comment the subsequent call having the NULL pointer                   */
//	  BSP_MotorControl_Init(BSP_MOTOR_CONTROL_BOARD_ID_STSPIN240, &gStspin240_250InitParams);
////	  motorDrvHandle->Init(&gStspin240_250InitParams);
//
//	  //BSP_MotorControl_Init(BSP_MOTOR_CONTROL_BOARD_ID_STSPIN240, NULL);
//
//	  /* Set dual bridge enabled as two motors are used*/
//	  BSP_MotorControl_SetDualFullBridgeConfig(1);
////	  motorDrvHandle->SetDualFullBridgeConfig(1);
//
//	  /* Attach the function MyFlagInterruptHandler (defined below) to the flag interrupt */
//	  BSP_MotorControl_AttachFlagInterrupt(MyFlagInterruptHandler);
////	  motorDrvHandle->AttachFlagInterrupt(MyFlagInterruptHandler);
//
//	  /* Attach the function Error_Handler (defined below) to the error Handler*/
//	  BSP_MotorControl_AttachErrorHandler(Error_Handler);
////	  motorDrvHandle->AttachErrorHandler(Error_Handler);
//
//	  /* Set PWM Frequency of Ref to 15000 Hz */
//	  BSP_MotorControl_SetRefFreq(0,VREF_FREQ);
////	  motorDrvHandle->SetRefFreq(0, 15000);
//
//	  /* Set PWM duty cycle of Ref to 60% */
//	  BSP_MotorControl_SetRefDc(0,VREF_PWM);
////	  motorDrvHandle->SetRefDc(0, 100);
//
//	  /* Set PWM Frequency of bridge A inputs to 10000 Hz */
//	  BSP_MotorControl_SetBridgeInputPwmFreq(0,10000);
////	  motorDrvHandle->SetBridgeInputPwmFreq(0, 10000);
//
//	  /* Set PWM Frequency of bridge B inputs to 10000 Hz */
//	  /* On X-NUCLEO-IHM12A1 expansion board PWM_A and PWM_B shares the same */
//	  /* timer, so frequency must be the same */
//	  BSP_MotorControl_SetBridgeInputPwmFreq(1,10000);
////	  motorDrvHandle->SetBridgeInputPwmFreq(1, 10000);
//}
