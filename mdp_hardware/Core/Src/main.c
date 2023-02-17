/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gyro.h"
#include "i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim8;

UART_HandleTypeDef huart3;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for MotorTask */
osThreadId_t MotorTaskHandle;
const osThreadAttr_t MotorTask_attributes = {
  .name = "MotorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for EncoderTask */
osThreadId_t EncoderTaskHandle;
const osThreadAttr_t EncoderTask_attributes = {
  .name = "EncoderTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for GyroTask */
osThreadId_t GyroTaskHandle;
const osThreadAttr_t GyroTask_attributes = {
  .name = "GyroTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */

//Queue struct
//to define the nodes for the queue
typedef struct _listnode{
    uint8_t msg[4]; //type for every instruction
    struct _listnode *next;
} ListNode;

typedef ListNode QueueNode; //define QueueNode as a ListNode structure

typedef struct _queue{
   int size; //amount of instructions in the queue
   ListNode *head;
   ListNode *tail;
} Queue;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_I2C1_Init(void);
void StartDefaultTask(void *argument);
void motor(void *argument);
void encoder_task(void *argument);
void gyro_task(void *argument);

/* USER CODE BEGIN PFP */
void show(void);
void resetVal();

//Prototypes of Interface functions for Queue structure
void enqueue(Queue *qPtr, uint8_t msg[4]);
int dequeue(Queue *qPtr);
void getFront(Queue q);
int isEmptyQueue(Queue q);
void deleteQueue(Queue *qPtr);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t aRxBuffer[50]; // UART Buffer

uint8_t frontback = 'w';	// Front/back character
uint8_t fb_speed = '0';		// Front/back speed
uint8_t leftright = 'a';	// Left/right character
uint8_t lr_speed = '0';		// Left/right speed

double curAngle = 0; 		// angle via gyro

int encoder_offset = 0; // Adds to motor pwm for straight movement
int encoder_error = 0;	// Current error value for encoder
uint32_t encoder_dist = 0;	// Used for distance estimation

uint16_t pwmVal_servo = 150; // servo centre

Queue q;


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM8_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM1_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  OLED_Init();
  gyroInit();
  HAL_UART_Receive_IT(&huart3, (uint8_t *) aRxBuffer, 4);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of MotorTask */
  MotorTaskHandle = osThreadNew(motor, NULL, &MotorTask_attributes);

  /* creation of EncoderTask */
  EncoderTaskHandle = osThreadNew(encoder_task, NULL, &EncoderTask_attributes);

  /* creation of GyroTask */
  GyroTaskHandle = osThreadNew(gyro_task, NULL, &GyroTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 160;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 10;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 10;
  if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 7199;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, OLED_SCL_Pin|OLED_SDA_Pin|OLED_RST_Pin|OLED_DC_Pin
                          |LED3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, AIN2_Pin|AIN1_Pin|BIN1_Pin|BIN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : OLED_SCL_Pin OLED_SDA_Pin OLED_RST_Pin OLED_DC_Pin
                           LED3_Pin */
  GPIO_InitStruct.Pin = OLED_SCL_Pin|OLED_SDA_Pin|OLED_RST_Pin|OLED_DC_Pin
                          |LED3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : AIN2_Pin AIN1_Pin BIN1_Pin BIN2_Pin */
  GPIO_InitStruct.Pin = AIN2_Pin|AIN1_Pin|BIN1_Pin|BIN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
/**
  * @brief  Function called during Serial interrupt
  * @param  argument: UART_HandleTypeDef
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// Prevent unused argument compiled warning

	UNUSED(huart);

	enqueue(&q,aRxBuffer);
//	frontback = (uint8_t)(aRxBuffer[0]);
//	fb_speed = (uint8_t)(aRxBuffer[1]);
//	leftright = (uint8_t)(aRxBuffer[2]);
//	lr_speed = (uint8_t)(aRxBuffer[3]);


	HAL_UART_Receive_IT(&huart3,(uint8_t *) aRxBuffer,4);
//	sprintf(hello, "Dir %c : %d\0", frontback, fb_speed-48);
//	OLED_ShowString(10, 20, hello);
//
//	sprintf(hello, "Turn %c: %d\0", leftright, lr_speed-48);
//	OLED_ShowString(10, 30, hello);
	HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
}

void reset_motorVal(){
	// Reset Values
	frontback = 'w';	// Front/back character
	fb_speed = '0';	// Front/back speed
	leftright = 'a';	// Left/right character
	lr_speed = '0';	// Left/right speed
}

//input stuff into the queue
void enqueue(Queue *qPtr, uint8_t msg[4]){
    QueueNode *newNode;
    newNode = (QueueNode *) malloc(sizeof(QueueNode));
    for(int i=0; i<4; i++){
        newNode->msg[i] = msg[i];
    }
    newNode->next = NULL;

    if(isEmptyQueue(*qPtr))
        qPtr->head=newNode;
    else
        qPtr->tail->next = newNode;

    qPtr->tail = newNode;
    qPtr->size++;
}

int dequeue(Queue *qPtr){
    if(qPtr==NULL || qPtr->head==NULL){ //Queue is empty or NULL pointer
        return 0;
    }
    else{
       QueueNode *temp = qPtr->head;
       qPtr->head = qPtr->head->next;
       if(qPtr->head == NULL) //Queue is emptied
           qPtr->tail = NULL;

       free(temp);
       qPtr->size--;
       return 1;
    }
}

//get the front of the queue (not sure if working)
void getFront(Queue q){
        frontback = (uint8_t)(q.head->msg[0]);
        fb_speed = (uint8_t)(q.head->msg[1]);
        leftright = (uint8_t)(q.head->msg[2]);
        lr_speed = (uint8_t)(q.head->msg[3]);
}

//check if queue is empty (output 1 if empty, 0 if not empty)
int isEmptyQueue(Queue q){
    if(q.size==0) return 1;
    else return 0;
}

//delete the whole queue
void deleteQueue(Queue *qPtr)
{
    while(dequeue(qPtr));
}


/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
//	  HAL_UART_Transmit(&huart3, (uint8_t *)&ch,1, 0xFFFF);
//	  if(ch<'Z')
//		  ch++;
//	  else ch = 'A';
//	  if(aRxBuffer != NULL){
//		  if(aRxBuffer == 'w'){
//			  HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
//			  aRxBuffer = NULL;
//		  }
//	  }
//	  HAL_UART_Transmit(&huart3, (uint8_t *)txData, strlen(txData), 10);
//	  if(curAngle > 0){
//		  curAngle -= 1;	// Account for gyro drift
//	  }
	  osDelay(1000);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_motor */
/**
* @brief Function implementing the MotorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_motor */
void motor(void *argument)
{
  /* USER CODE BEGIN motor */
	uint16_t servo_max = 5;		// Servo_max * (0-9) = servo_value

	// For differential steering
	double motor_offset_r = 1;
	double motor_offset_l = 1;

	uint16_t pwmVal_motor = 0;	// Current motor pwm value
	uint16_t motor_min = 1000;	// Min value for pwm to complete 2 instruction without stopping
	uint16_t motor_increment = 100;
	uint8_t accelerate;

	uint16_t motor_reference;	// Reference pwm value for motor
	uint32_t target_dist;	// Distance to travel

	// PID Values
	int current_encoder_error = 0;
	uint8_t kp = 50;
	uint8_t ki = 1.3;
	int eintegral = 0;	// Integral error

	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);



  /* Infinite loop */
  for(;;)
  {
	  if(isEmptyQueue(q) != 1){
		  	  uint8_t hello[20];

			  getFront(q);			// Setting values according to queue head
			  encoder_dist = 0;		// Reset Encoder distance measurement

		  	  accelerate = 1; 		// Default always start with acceleration
		  	  target_dist = (int) ((fb_speed - 48)*129 - 40);
		  	  if(target_dist <= 0)
		  		  target_dist = 0;

		  	  // Display direction of movement
		  	  sprintf(hello, "Dir %c : %3d\0", frontback, (fb_speed-48));
		  	  OLED_ShowString(10, 30, hello);

		  	  if(lr_speed == '0'){
		  		motor_reference = 2400;
		  	  }

		  	  else
		  		  motor_reference = 2000;

		  	  // Turn Servo to desired position
		  	  // Centre - offset for left turn
		  	  if(leftright == 'a'){
		  		  htim1.Instance->CCR4 = pwmVal_servo - 1.1*(lr_speed-48) *servo_max;
		  		  // right motor offset
		  		  // right motor have to spin more due to differential steering
		  		  motor_offset_r = 0.03*(lr_speed-48)+1;
		  		  motor_offset_l = 1;

		  	  }
		  	  else if(leftright =='d'){
		  		  htim1.Instance->CCR4 = pwmVal_servo + 1.7*(lr_speed-48) *servo_max;
		  		  // left motor offset
		  		  motor_offset_r = 1;
		  		  motor_offset_l = 0.05*(lr_speed-48)+1;
		  	  }

		  	  pwmVal_motor = motor_min;

		  	  // Move Motor forward (Normal)
		  	  if(frontback == 'w'){
		  		  do
		  		  	  {
		  		  		  // H-Bridge Circuit for AINx; 1 turn on, the other turns off
		  		  		  // MOTOR A
		  		  		  HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_SET);
		  		  		  HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_RESET);

		  		  		  // MOTOR B
		  		  		  HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_SET);
		  		  		  HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_RESET);

		  		  		  if(accelerate == 1){
		  		  			  pwmVal_motor+=motor_increment;	// Accelerating

							  if(pwmVal_motor > motor_reference){
								  accelerate = 0;
								  while(encoder_dist < (int)target_dist*0.95){
									  osDelay(1);
								  }
							  }

		  		  		  }

		  		  		  else {		// Decelerate
		  		  			  if(pwmVal_motor > motor_min)
							  	  pwmVal_motor-=5*motor_increment;
		  		  		  }


		  				  // Modify comparison value for duty cycle
		  		  		  // Motor speed = motor_offset from servo turn * pwm_value + encoder to ensure its going straight

		  		  		  // PID for straight movement
		  		  		  if(lr_speed == '0'){
		  		  			  // PID for encoder offset
							  current_encoder_error = encoder_error - 0;
							  eintegral += current_encoder_error;		// Integral error

							  // PID equation
							  encoder_offset = kp*current_encoder_error+ ki*eintegral;
		  		  		  }

		  		  		  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
		  		  		  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);
		  		  		  osDelay(10);

		  		  	  }while(encoder_dist < target_dist);
		  	  }

		  	// Move Motor backwards(Normal)
		  	  else if(frontback == 's'){
		  		  do
		  		  	  {
		  		  		  // MOTOR A
		  		  		  HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
		  		  		  HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);

		  		  		  // MOTOR B
		  		  		  HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
		  		  		  HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);

		  		  		if(accelerate == 1){
							  pwmVal_motor+=motor_increment;	// Accelerating

							  if(pwmVal_motor > motor_reference){
								  accelerate = 0;
								  while(encoder_dist < (int)target_dist*0.95){
									  osDelay(1);
								  }
							  }

						  }

						  else {		// Decelerate
							  if(pwmVal_motor > motor_min)
								  pwmVal_motor-=5*motor_increment;
						  }


						  // Modify comparison value for duty cycle
						  // Motor speed = motor_offset from servo turn * pwm_value + encoder to ensure its going straight

						  // PID for straight movement
						  if(lr_speed == '0'){
							  // PID for encoder offset
							  current_encoder_error = encoder_error - 0;
							  eintegral += current_encoder_error;		// Integral error

							  // PID equation
							  encoder_offset = kp*current_encoder_error+ ki*eintegral;
						  }

						  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);
						  osDelay(10);

					  }while(encoder_dist < target_dist);
		  	  }

		  	  // No motor/reset values and start gyro
		  	  else if(frontback == 'k'){
		  		  // Reset all values
		  		  encoder_offset = 0;
		  		  encoder_error = 0;
		  		  curAngle = 0;
		  		  pwmVal_motor = 0;

		  		  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwmVal_motor);
		  		  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwmVal_motor);

		  		  gyroStart();					// Start Gyro Calibration

		  		  osDelay(100);					// Required Delay for calibration
		  		  osDelay((fb_speed-48)*50);	// Additional delay if required
		  	  }

		  	  // Move backwards (Slow)
		  	  else if(frontback == 'y'){
		  		  // Slow down reference
		  		  pwmVal_motor = 1600;

		  		  // MOTOR A
		  		  HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
		  		  HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);

				  // MOTOR B
				  HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
				  HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);

				  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwmVal_motor);
				  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwmVal_motor);

				  osDelay((fb_speed - 48)*40);
		  	  }

		  	// Move forwards (Slow)
		  	else if(frontback == 'u'){
		  		// Slow down reference
		  		pwmVal_motor = 1600;

		  		// MOTOR A
		  		HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_SET);
		  		HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_RESET);

		  		// MOTOR B
		  		HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_SET);
		  		HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_RESET);

		  		__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwmVal_motor);
		  		__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwmVal_motor);

		  		osDelay((fb_speed - 48)*40);
		  }

		  	  osDelay(10);
		  	  dequeue(&q);


	  }	// ENDIF Queue not empty

	  // Queue empty
	  else{
		  reset_motorVal();	//Reset the values
		  encoder_offset = 0;
		  encoder_error = 0;
		  encoder_dist = 0;
		  curAngle = 0;
		  pwmVal_motor = 0;

		  htim1.Instance->CCR4 = pwmVal_servo;	// Reset Servo values

		  // Stop motor
		  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwmVal_motor);
		  __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwmVal_motor);
	  }

  }
  /* USER CODE END motor */
}

/* USER CODE BEGIN Header_encoder_task */
/**
* @brief Function implementing the EncoderTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_encoder_task */
void encoder_task(void *argument)
{
  /* USER CODE BEGIN encoder_task */
  /* Infinite loop */
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_ALL);

	int cnt1, diffa=0;
	int cnt2, diffb=0;
	uint32_t tick;

	tick = HAL_GetTick();

	uint8_t msg[20];

	for(;;)
	{
		// Every 1000 ticks, get reading(How fast wheel turn)
		if(HAL_GetTick()-tick > 10){
			// At rising edge, counter increase by 1
			cnt1 = __HAL_TIM_GET_COUNTER(&htim2);
			cnt2 = __HAL_TIM_GET_COUNTER(&htim3);

			/* Motor A */
			// Counting up; Motor moving forward
			// 32500 is the max tick
			if(cnt1 - 32500 > 0){
				diffa = cnt1 - 65535;
			}
			// Counting down; Motor moving backward
			else if(cnt1 - 32500 < 0){
				diffa = cnt1;
			}

			/* Motor B */
			// Counting up; Motor moving backward
			if(cnt2 - 32500 > 0){
				diffb = (cnt2 - 65535);
			}
			// Counting down; Motor moving forward
			else if(cnt2 - 32500 < 0){
				diffb = cnt2;
			}

			encoder_error = diffa + diffb;
			encoder_dist += (abs(diffa) + abs(diffb));

			// Display difference
			sprintf(msg, "Diff : %3d\0", encoder_error);
			OLED_ShowString(10,40,msg);

			sprintf(msg, "Dist : %3d\0", encoder_dist);
			OLED_ShowString(10,20,msg);

			OLED_Refresh_Gram();
			// Reset base tick
			__HAL_TIM_SET_COUNTER(&htim2, 0);
			__HAL_TIM_SET_COUNTER(&htim3, 0);

			tick = HAL_GetTick();

			osDelay(10);
		}
	}
  /* USER CODE END encoder_task */
}

/* USER CODE BEGIN Header_gyro_task */
/**
* @brief Function implementing the GyroTask thread
* gyro is only used for turning, the value should be around 28
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_gyro_task */
void gyro_task(void *argument)
{
  /* USER CODE BEGIN gyro_task */
  /* Infinite loop */
	uint8_t val[2] = { 0, 0 }; // To store ICM gyro values
	gyroInit();
	int16_t angular_speed = 0;
	int16_t angle = 0;

	// PID values
	uint8_t kp = 5;
	uint8_t ki = 0.8;
	int16_t eintegral = 0;	// Integral error

	int32_t err;			// To total error for Integral

	// Set servo value to centre
	uint8_t servo_val = pwmVal_servo;

	int turn_angle = 105;	// gyro turn threshold for 90deg
	osDelay(100);
	for(;;)
	{
		uint8_t msg[8];
		curAngle = 0;

		// Gyro Function for turning
		if(lr_speed > '0'){

			if(leftright == 'a')
				turn_angle = 115;
			else
				turn_angle = 125;

			// Continue Reading Gyro until hit threshold
			while(abs((int) curAngle) < turn_angle){
				readByte(0x37, val);
				angular_speed = (val[0] << 8) | val[1];	// appending the 2 bytes together
				angle = ((double)(angular_speed*(100) - 2) / 16400.0)*1.1 ; //1.69

				curAngle += angle;

				sprintf(msg, "gyro : %3d\0", (int)curAngle);
				OLED_ShowString(10, 10, msg);

				if(fb_speed == '0')
					break;
				osDelay(50);
			}

			// Once Threshold reached, turn servo centre
			htim1.Instance->CCR4 = pwmVal_servo;	// Turn servo to the centre

			curAngle = 0;
		}

		// Ensures robot goes straight
		else if((lr_speed == '0')&&(fb_speed > '0')){
			eintegral = 0;

			do{
				// Read Gyro
				readByte(0x37, val);
				angular_speed = (val[0] << 8) | val[1];	// appending the 2 bytes together
				angle = ((double)(angular_speed*(100) - 2) / 16400.0)*1.5 ; //1.69

				// Prevent gyro drift by ignoring small angle change
				if(abs(angle) > 2)
					curAngle += angle;

				// Print Gyro
				sprintf(msg, "gyro : %3d\0", (int)curAngle);
				OLED_ShowString(10, 10, msg);

				// PID for error adjustment
				err = curAngle - 0;		// Proportional error
				eintegral += err;		// Integral error

				// PID equation
				servo_val = (uint8_t)(pwmVal_servo + kp*err + ki*eintegral);

				// Set servo value
				htim1.Instance->CCR4 = servo_val;	// Turn servo to correct error

				osDelay(40);
			}while((lr_speed == '0')&&(fb_speed > '0'));

			curAngle = 0;							// Reset Angle value

		}

		osDelay(10);
	}
  /* USER CODE END gyro_task */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
