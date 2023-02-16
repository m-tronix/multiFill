/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "stm32l0xx_hal.h"

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
#define led1_Pin GPIO_PIN_14
#define led1_GPIO_Port GPIOC
#define led2_Pin GPIO_PIN_15
#define led2_GPIO_Port GPIOC
#define time_ref_Pin GPIO_PIN_0
#define time_ref_GPIO_Port GPIOA
#define run_fwd_Pin GPIO_PIN_6
#define run_fwd_GPIO_Port GPIOA
#define run_rev_Pin GPIO_PIN_7
#define run_rev_GPIO_Port GPIOA
#define run_flush_Pin GPIO_PIN_1
#define run_flush_GPIO_Port GPIOB
#define estop_Pin GPIO_PIN_9
#define estop_GPIO_Port GPIOA
#define start_Pin GPIO_PIN_10
#define start_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
