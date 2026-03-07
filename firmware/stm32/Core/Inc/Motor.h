/*
 * Motor.h
 *
 *  Created on: Dec 27, 2022
 *      Author: rossa
 */

#ifndef SRC_MOTOR_H_
#define SRC_MOTOR_H_

///////////////////////////////////////
///				DEFINES				///
///////////////////////////////////////

#define __USE_FREERTOS__

#define PWM_CLK_FREQ		84000000

/// Interrupt line used for Stspin240 Fault interrupt
#define EXTI_FAULT_IRQn           (EXTI15_10_IRQn)

/// Timer used for PWMA
#define BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_A      (TIM3)

/// Timer used for PWMB
#define BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_B      (TIM3)

   /// Timer used for REF
#define BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_REF      (TIM2)

/// Channel Timer used for PWMA
#define BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_A      (TIM_CHANNEL_1)

/// Channel Timer used for PWMB
#define BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_B      (TIM_CHANNEL_2)

/// Channel Timer used for REF
#define BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM_REF     (TIM_CHANNEL_1)

/// HAL Active Channel Timer used for PWMA
#define BSP_MOTOR_CONTROL_BOARD_HAL_ACT_CHAN_TIMER_PWM_A      (HAL_TIM_ACTIVE_CHANNEL_1)

/// HAL Active Channel Timer used for PWMB
#define BSP_MOTOR_CONTROL_BOARD_HAL_ACT_CHAN_TIMER_PWM_B      (HAL_TIM_ACTIVE_CHANNEL_2)

/// HAL Active Channel Timer used for REF
#define BSP_MOTOR_CONTROL_BOARD_HAL_ACT_CHAN_TIMER_PWM_REF      (HAL_TIM_ACTIVE_CHANNEL_1)

/// Timer Clock Enable for PWMA
#define __BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_A_CLCK_ENABLE()  __TIM3_CLK_ENABLE()

/// Timer Clock Enable for PWMB
#define __BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_B_CLCK_ENABLE()  __TIM3_CLK_ENABLE()

/// Timer Clock Enable for REF
#define __BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_REF_CLCK_ENABLE()   __TIM2_CLK_ENABLE()

   /// Timer Clock Enable for PWMA
#define __BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_A_CLCK_DISABLE()  __TIM3_CLK_DISABLE()

/// Timer Clock Enable for PWMB
#define __BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_B_CLCK_DISABLE()  __TIM3_CLK_DISABLE()

   /// Timer Clock Enable for REF
#define __BSP_MOTOR_CONTROL_BOARD_TIMER_PWM_REF_CLCK_DISABLE()  __TIM2_CLK_DISABLE()

/// PWMA GPIO alternate function
#define BSP_MOTOR_CONTROL_BOARD_AFx_TIMx_PWM_A  (GPIO_AF2_TIM3)

/// PWMB GPIO alternate function
#define BSP_MOTOR_CONTROL_BOARD_AFx_TIMx_PWM_B  (GPIO_AF2_TIM3)

/// REF GPIO alternate function
#define BSP_MOTOR_CONTROL_BOARD_AFx_TIMx_PWM_REF  (GPIO_AF1_TIM2)

/// GPIO Pin used for the ref pin of the Stspin240
#define BSP_MOTOR_CONTROL_BOARD_REF_PIN  (GPIO_PIN_0)
/// GPIO Port used for the ref pin of the Stspin240
#define BSP_MOTOR_CONTROL_BOARD_REF_PORT  (GPIOA)
/// GPIO Pin used for the  input PWM A of the Stspin240
#define BSP_MOTOR_CONTROL_BOARD_PWM_A_PIN  (GPIO_PIN_4)
/// GPIO Port sed for the  input PWM A of the Stspin240
#define BSP_MOTOR_CONTROL_BOARD_PWM_A_PORT  (GPIOB)

/// GPIO Pin used for the PWM B of the Stspin240
#define BSP_MOTOR_CONTROL_BOARD_PWM_B_PIN  (GPIO_PIN_5)
/// GPIO Port used for the PWM B of the Stspin240
#define BSP_MOTOR_CONTROL_BOARD_PWM_B_PORT  (GPIOB)

   /// GPIO Pin used for the direction of the Stspin240 Brige A
#define BSP_MOTOR_CONTROL_BOARD_DIR_A_PIN  (GPIO_PIN_10)
/// GPIO Port used for the direction of the Stspin240 Brige A
#define BSP_MOTOR_CONTROL_BOARD_DIR_A_PORT  (GPIOB)

/// GPIO Pin used for the direction of the Stspin240 Brige B
#define BSP_MOTOR_CONTROL_BOARD_DIR_B_PIN  (GPIO_PIN_8)
/// GPIO Port used for the direction of the Stspin240 Brige B
#define BSP_MOTOR_CONTROL_BOARD_DIR_B_PORT  (GPIOA)

/// GPIO Pin used for the Stspin240  Enable pin and Faults (over current detection and thermal shutdown)
#define BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PIN  (GPIO_PIN_10)
/// GPIO port used for the Stspin240  Enable pin and Faults (over current detection and thermal shutdown)
#define BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PORT (GPIOA)
/// Flag interrupt priority
#define BSP_MOTOR_CONTROL_BOARD_EN_AND_FAULT_PRIORITY  (3)

/// GPIO Pin used for the standy/reset pin of the Stspin240
#define BSP_MOTOR_CONTROL_BOARD_RESET_PIN  (GPIO_PIN_7)
/// GPIO Port used for the standy/reset pin of the Stspin240
#define BSP_MOTOR_CONTROL_BOARD_RESET_PORT  (GPIOC)

/// Timer Prescaler
#define TIMER_PRESCALER (1)

/// MCU wait time in ms after power bridges are enabled
#define BSP_MOTOR_CONTROL_BOARD_BRIDGE_TURN_ON_DELAY    (1)

/// MCU wait time in ms after exit standby mode
#define BSP_MOTOR_CONTROL_BOARD_EXIT_STANDBY_MODE_DELAY    (1)
/// The Number of Stspin240 and Stspin250 devices required for initialisation is not supported
#define STSPIN240_250_ERROR_0   (0xC000)
/// Error: Access a motor index greater than the one of the current brigde configuration
#define STSPIN240_250_ERROR_1   (0xC001)
/// Error: Use of a bridgeId greater than BRIDGE_B
#define STSPIN240_250_ERROR_2   (0xC002)

/// Maximum frequency of the PWMs in Hz
#define STSPIN240_250_MAX_PWM_FREQ   (100000)

/// Minimum frequency of the PWMs in Hz
#define STSPIN240_250_MIN_PWM_FREQ   (2)

/// Bridge A
#define BRIDGE_A         (0)
/// Bridge B
#define BRIDGE_B         (1)

/// PWM id for PWM_A
#define PWM_A         (0)
/// PWM id for PWM_B
#define PWM_B         (1)
/// PWM id for PWM_REF
#define PWM_REF       (2)

/// PWM TURNING OFFSET
#define PWM_TURN_OFFSET	10

#include <stdint.h>
#include "stm32f4xx.h"


///////////////////////////////////////
///				ENUM				///
///////////////////////////////////////
typedef enum {
	MOTOR_A = 0,
	MOTOR_B =1
} Motor_t;

typedef enum {
	FORWARD = 1,
	BACKWARD = 0,
	UNKNOWN = (uint8_t)0xFF
} motorDir_t;

typedef enum {
	INACTIVE = 0,
	STOP = 1,
	STEADY = 2,
	RUN = 3
} MotorStatus_t;

typedef enum {
	DISABLED = 0,
	HiZ = 1,
	ENABLED = (uint8_t) 0xFF
} BridgeStatus_t;

typedef enum {false, true} bool;

///////////////////////////////////////
///			MOTOR CLASS				///
///////////////////////////////////////
/*class MotorClass {

private:*/
	extern BridgeStatus_t bridgeA;
	extern BridgeStatus_t bridgeB;
	extern MotorStatus_t stateA;
	extern MotorStatus_t stateB;
	extern uint8_t vref;
	extern uint16_t vref_freq;
	extern uint16_t mot_pwm_freq;
	extern motorDir_t dirA;
	extern motorDir_t dirB;
	extern int16_t speed_A;
	extern int16_t speed_B;
	extern bool isError;
	/// Timer handler for input PWM of Bridge A
	extern TIM_HandleTypeDef hTimPwmA;
	/// Timer handler for input PWM of Bridge B
	extern TIM_HandleTypeDef hTimPwmB;
	/// Timer handler for REF PWM
	extern TIM_HandleTypeDef hTimPwmRef;

/*public:
	MotorClass();
	virtual ~MotorClass();*/
	void motor_init();
	void motor_Board_Reset(void); //Reset the Stspin240
	void motor_Board_Release_Reset(void);
	bool motor_SetVref(uint8_t val);
	bool motor_SetVrefFreq(uint32_t freq, uint8_t val);
	bool motor_StopVref(void);
	uint8_t motor_GetVref();
	bool motor_SetSpeed(Motor_t motor, int16_t speed);
	int16_t motor_GetSpeed(Motor_t motor);
	bool motor_EnableBridge(void);
	bool motor_DisableBridge(void);
	bool motor_HardStop();
	uint8_t motor_getError(void);
	bool motor_getReset(void);
	void motor_Board_Delay(uint32_t delay);         //Delay of the requested number of milliseconds
	bool motor_SetMotorFreq(uint32_t freq, uint16_t val);
	bool motor_StartMotor(Motor_t motor);
	bool motor_StopMotorA(void);
	bool motor_StopMotorB(void);
	//bool RunMotorA(uint16_t val);
	//bool RunMotorB(uint16_t val);
	void motor_Board_PwmSetFreq(uint8_t pwmId, uint32_t newFreq, uint8_t duty); //Set PWM frequency and start it
	void motor_Board_PwmStop(uint8_t pwmId);   //Stop the specified PWM
	void motor_Board_SetDirection(uint8_t bridgeId, uint8_t gpioState); //Set direction of the specified bridge
/*private:
	void Board_PwmSetFreq(uint8_t pwmId, uint32_t newFreq, uint8_t duty); //Set PWM frequency and start it
	void Board_PwmStop(uint8_t pwmId);   //Stop the specified PWM
	void Board_SetDirection(uint8_t bridgeId, uint8_t gpioState); //Set direction of the specified bridge
};*/


///Timer State
extern uint8_t timerState[3]; // =  {0, 0, 0 };


//void Stspin240_250_Board_Delay(uint32_t delay);         //Delay of the requested number of milliseconds
//void Stspin240_250_Board_DisableBridge(void);     //Disable the bridges
//void Stspin240_250_Board_EnableBridge(uint8_t addDelay);      //Enable the specified bridge
//uint8_t Stspin240_250_Board_GetFaultPinState(void); //Get the status of the Enable and Fault pin
//uint8_t Stspin240_250_Board_GetResetPinState(void); //Get the status of the reset pin
void Stspin240_250_Board_GpioInit(uint8_t deviceId);   //Initialise GPIOs used for Stspin240s
void Stspin240_250_Board_PwmDeInit(uint8_t pwmId); ///Deinitialise the specified PWM
void Stspin240_250_Board_PwmInit(uint8_t pwmId, uint8_t onlyChannel);    //Init the specified PWM
//void Stspin240_250_Board_PwmSetFreq(uint8_t pwmId, uint32_t newFreq, uint8_t duty); //Set PWM frequency and start it
//void Stspin240_250_Board_PwmStop(uint8_t pwmId);   //Stop the specified PWM
//void Stspin240_250_Board_ReleaseReset(uint8_t deviceId);   //Release the reset pin of the Stspin240
//void Stspin240_250_Board_Reset(uint8_t deviceId); //Reset the Stspin240
void Stspin240_250_Board_SetDirectionGpio(uint8_t bridgeId, uint8_t gpioState); //Set direction of the specified bridge
/**
  * @}
  */

/**
 * 	Function Prototypes for class wrapper functions
 */
int InitMotorVref();
int InitMotorPwm();
void MotorEnableBridge();
void MotorDisableBridge();
uint8_t MotorGetError();
void MotorHardStop();
void MotorReset();
void MotorReleaseReset();
void SetVrefPWMFreq(uint16_t freq);
void SetVref(TIM_HandleTypeDef *hTimPwmRef, uint16_t vref);
void SetMotorPWMFreq(uint16_t freq);
void SetMotorPWM(Motor_t, int16_t);
void _LinkTimersHandler(TIM_HandleTypeDef , TIM_HandleTypeDef );
void setDutyCycle(TIM_HandleTypeDef *htim, uint32_t ch, /*float*/ uint16_t duty_cycle);
void ErrorHandler();
///*extern */void MotorRun(Motor_t mot, motorDir_t dir, uint16_t val);

#endif /* SRC_MOTORCLASS_H_ */
