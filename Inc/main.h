/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "stm32f1xx_hal.h"

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
#define LED1_Pin GPIO_PIN_13
#define LED1_GPIO_Port GPIOC
#define REMOTE_CHOOSE_Pin GPIO_PIN_0
#define REMOTE_CHOOSE_GPIO_Port GPIOA
#define REMOTE_ADC_Pin GPIO_PIN_1
#define REMOTE_ADC_GPIO_Port GPIOA
#define RELAY_H8_Pin GPIO_PIN_2
#define RELAY_H8_GPIO_Port GPIOA
#define RELAY_H7_Pin GPIO_PIN_3
#define RELAY_H7_GPIO_Port GPIOA
#define RELAY_H6_Pin GPIO_PIN_4
#define RELAY_H6_GPIO_Port GPIOA
#define RELAY_H5_Pin GPIO_PIN_5
#define RELAY_H5_GPIO_Port GPIOA
#define RELAY_H4_Pin GPIO_PIN_6
#define RELAY_H4_GPIO_Port GPIOA
#define RELAY_H3_Pin GPIO_PIN_7
#define RELAY_H3_GPIO_Port GPIOA
#define RELAY_H2_Pin GPIO_PIN_0
#define RELAY_H2_GPIO_Port GPIOB
#define RELAY_H1_Pin GPIO_PIN_1
#define RELAY_H1_GPIO_Port GPIOB
#define KC_L8_Pin GPIO_PIN_12
#define KC_L8_GPIO_Port GPIOB
#define KC_L7_Pin GPIO_PIN_13
#define KC_L7_GPIO_Port GPIOB
#define KC_L6_Pin GPIO_PIN_14
#define KC_L6_GPIO_Port GPIOB
#define KC_L5_Pin GPIO_PIN_15
#define KC_L5_GPIO_Port GPIOB
#define KC_L4_Pin GPIO_PIN_8
#define KC_L4_GPIO_Port GPIOA
#define KC_L3_Pin GPIO_PIN_9
#define KC_L3_GPIO_Port GPIOA
#define KC_L2_Pin GPIO_PIN_10
#define KC_L2_GPIO_Port GPIOA
#define KC_L1_Pin GPIO_PIN_15
#define KC_L1_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
