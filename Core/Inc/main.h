/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Buzzer_Pin GPIO_PIN_5
#define Buzzer_GPIO_Port GPIOE
#define MotionSensor_Pin GPIO_PIN_2
#define MotionSensor_GPIO_Port GPIOF
#define MotionSensor_EXTI_IRQn EXTI2_IRQn
#define PHOTORESISTANCE_Pin GPIO_PIN_3
#define PHOTORESISTANCE_GPIO_Port GPIOF
#define gazreader_Pin GPIO_PIN_6
#define gazreader_GPIO_Port GPIOF
#define pleutilmouille_Pin GPIO_PIN_8
#define pleutilmouille_GPIO_Port GPIOF
#define UsrBtn_Pin GPIO_PIN_0
#define UsrBtn_GPIO_Port GPIOA
#define UsrBtn_EXTI_IRQn EXTI0_IRQn
#define Servoooo_Pin GPIO_PIN_6
#define Servoooo_GPIO_Port GPIOA
#define ETLALUMIEREFUT_Pin GPIO_PIN_12
#define ETLALUMIEREFUT_GPIO_Port GPIOF
#define btnfenetre_Pin GPIO_PIN_13
#define btnfenetre_GPIO_Port GPIOF
#define btnfenetre_EXTI_IRQn EXTI15_10_IRQn
#define btnlight_Pin GPIO_PIN_14
#define btnlight_GPIO_Port GPIOF
#define btnlight_EXTI_IRQn EXTI15_10_IRQn
#define ledpourgaz_Pin GPIO_PIN_15
#define ledpourgaz_GPIO_Port GPIOF
#define LEDfenetre_Pin GPIO_PIN_9
#define LEDfenetre_GPIO_Port GPIOE
#define SCL_temperat_Pin GPIO_PIN_10
#define SCL_temperat_GPIO_Port GPIOB
#define SDA_temperat_Pin GPIO_PIN_11
#define SDA_temperat_GPIO_Port GPIOB
#define LEDG_Pin GPIO_PIN_13
#define LEDG_GPIO_Port GPIOG
#define LEDR_Pin GPIO_PIN_14
#define LEDR_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
