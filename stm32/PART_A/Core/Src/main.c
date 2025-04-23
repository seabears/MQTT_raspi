/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef void (*pFunction)(void);

typedef enum {
  FW_UPDATE_IDLE,
  FW_UPDATE_REQUESTED,
  FW_UPDATE_IN_PROGRESS,
  FW_UPDATE_COMPLETE
} FirmwareUpdateStat_t;

typedef struct {
  CAN_RxHeaderTypeDef rxHeader;
  uint8_t rxData[8];
} CAN_Message_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APPLICATION_A_ADDRESS 0x08008000 /* Partition A address */
#define APPLICATION_B_ADDRESS 0x08014000 /* Partition B address */
#define BOOT_FLAG_ADDR BKP->DR1 /* Boot flag address */

#define NUMBER_OF_PAGES_IN_PARTITION 48 /* Number of partition pages */

#define BOOT_FLAG_A 0x01 /* Partition A flag */
#define BOOT_FLAG_B 0x02 /* Partition B flag */

#define CAN_ID_FILE 0x70 /* 112 ID of receive file data */
#define CAN_ID_SIZE 0x71 /* 113 ID of receive file size */
#define CAN_ID_SEND 0x7A /* 122 ID of send CAN message */
#define CAN_ID_CONTROL 0x7B /* 123 ID of receive control */

#define MESSAGE_BUFFER_SIZE 512
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
CAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8] = {0x00,};
uint8_t command[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' }; //{0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00};
uint32_t TxMailbox;

FirmwareUpdateStat_t fwUpdateState = FW_UPDATE_IDLE;
volatile uint8_t fwUpdateRequested = 0;
volatile uint8_t fwUpdateComplete = 0;
uint32_t fwUpdateAddress = 0;
uint32_t fwUpdateSize = 0;
uint32_t fwUpdateReceivedBytes = 0;
uint8_t LD2Counter = 0;
uint8_t ledState = 0;

CAN_Message_t messageBuffer[MESSAGE_BUFFER_SIZE];
volatile uint16_t messageBufferHead = 0;
volatile uint16_t messageBufferTail = 0;

CAN_TxHeaderTypeDef txHeader;
uint8_t txData[8];
uint32_t txMailbox;


uint8_t uart_rxbuf[10];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SetBootFlag(void);
void NVIC_Config(void);
/* 펌웨어 업데이트 관련 함수 */
void FirmwareUpdateStateMachine(void);
void StartFirmwareUpdate(void);
void EraseFlashMemory(void);
void SendInactivePartitionAddress(void);
void SendUpdateState(void);

/* 메시지 버퍼 관리 함수 */
uint8_t MessageBufferIsFull(void);
uint8_t MessageBufferIsEmpty(void);
void MessageBufferPut(CAN_RxHeaderTypeDef *rxHeader, uint8_t *rxData);
void MessageBufferGet(CAN_RxHeaderTypeDef *rxHeader, uint8_t *rxData);

/* 추가: 펌웨어 시작 알림 함수 */
void SendFirmwareStartedMessage(void);


/* 함수 프로토타입 추가 */
void ProcessFirmwareSizeMessage(CAN_RxHeaderTypeDef *rxHeader, uint8_t *rxData);
void LD2Flip(void);
void Error_Handler(void);
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
  NVIC_Config();

  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_BKP_CLK_ENABLE();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  if (HAL_CAN_Start(&hcan) != HAL_OK){
	  Error_Handler();
  }

  CAN_FilterTypeDef canFilterConfig;
  canFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
  canFilterConfig.FilterBank = 0;
  canFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  canFilterConfig.FilterIdHigh = 0x0000;
  canFilterConfig.FilterIdLow = 0x0000;
  canFilterConfig.FilterMaskIdHigh = 0x0000;
  canFilterConfig.FilterMaskIdLow = 0x0000;
  canFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

  if (HAL_CAN_ConfigFilter(&hcan, &canFilterConfig) != HAL_OK)
  {
	  Error_Handler();
  }

  if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK){
	  Error_Handler();
  }

  SendFirmwareStartedMessage();
  TxHeader.DLC = 8;
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.StdId = 0x80;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

	if (fwUpdateState == FW_UPDATE_IDLE){
		if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK){
			Error_Handler();
		}

		TxData[0] += 0x11;
		TxData[1] += 0x12;
		TxData[2] += 0x13;
		HAL_Delay(100);

		LD2Counter++;
		if (LD2Counter > 30)
			LD2Flip();
	}

//	HAL_UART_Receive(&huart2, (uint8_t*)uart_rxbuf, 1, HAL_MAX_DELAY);
//	HAL_UART_Transmit(&huart2, (uint8_t*)uart_rxbuf, 1, HAL_MAX_DELAY);
//	if(uart_rxbuf[0]=='1'){
//		uart_rxbuf[0] = '0';
//		HAL_UART_Transmit(&huart2, "updateB", sizeof("updateB"), HAL_MAX_DELAY);
//		fwUpdateRequested = 1;
//		//fwUpdateAddress = APPLICATION_B_ADDRESS;
//		//BKP->DR1 = 0x02;
//
//	}



	FirmwareUpdateStateMachine();
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV8;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

/* USER CODE BEGIN 4 */
void LD2Flip(void){
	if (ledState == 0)
	{
		ledState = 1;
		HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_SET);
	}
	else
	{
		ledState = 0;
		HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_RESET);
	}
	LD2Counter = 0;
}
void SetBootFlag(void){
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_BKP_CLK_ENABLE();
	uint32_t boot_flag = BOOT_FLAG_ADDR;
	if (boot_flag == BOOT_FLAG_A)
		BOOT_FLAG_ADDR = BOOT_FLAG_B;
	else
		BOOT_FLAG_ADDR = BOOT_FLAG_A;
}

void SendFirmwareStartedMessage(void)
{

    txHeader.StdId = CAN_ID_SEND;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.IDE = CAN_ID_STD;
    txHeader.DLC = 1;

    txData[0] = 0x01; // 펌웨어 시작 알림 신호

    if (HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox) != HAL_OK)
    {
    	Error_Handler();
    }

    while (HAL_CAN_IsTxMessagePending(&hcan, txMailbox)){

    }
}

void NVIC_Config(void)
{
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

void ProcessFirmwareSizeMessage(CAN_RxHeaderTypeDef *rxHeader, uint8_t *rxData)
{
    if (rxHeader->DLC == 4)
    {
        // 펌웨어 크기 수신
        fwUpdateSize = (rxData[0] << 24) | (rxData[1] << 16) | (rxData[2] << 8) | rxData[3];
    }

}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK)
    {
    	Error_Handler();
        return;
    }

    if (rxHeader.StdId == CAN_ID_CONTROL && fwUpdateRequested == 0)
    {
    	//HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_SET);

    	if (!memcmp(command, rxData, sizeof(command))){
    		HAL_UART_Transmit(&huart2, "req", sizeof("req"), HAL_MAX_DELAY);
    		fwUpdateRequested = 1;
    		//HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_SET);

    	}
    }
    else if (fwUpdateRequested == 1 && rxHeader.StdId == CAN_ID_SIZE)	//143360byte
    {
    	ProcessFirmwareSizeMessage(&rxHeader, rxData);

    	HAL_UART_Transmit(&huart2, "size", sizeof("size"), HAL_MAX_DELAY);
    }

    else if (fwUpdateRequested == 1 && rxHeader.StdId == CAN_ID_FILE)
    {
    	//HAL_UART_Transmit(&huart2, "file_start", sizeof("file_start"), HAL_MAX_DELAY);
        MessageBufferPut(&rxHeader, rxData);
        //HAL_UART_Transmit(&huart2, "file_end", sizeof("file_end"), HAL_MAX_DELAY);
    }
}

void EraseFlashMemory(void)
{
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t pageError = 0;

    eraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInitStruct.PageAddress = fwUpdateAddress;
    eraseInitStruct.NbPages = NUMBER_OF_PAGES_IN_PARTITION;

    if (HAL_FLASHEx_Erase(&eraseInitStruct, &pageError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        fwUpdateState = FW_UPDATE_IDLE;
        NVIC_SystemReset();
    }

    HAL_FLASH_Lock();

}

void StartFirmwareUpdate(void)
{

	if (BOOT_FLAG_ADDR == BOOT_FLAG_A)
		fwUpdateAddress = APPLICATION_B_ADDRESS;
	else
		fwUpdateAddress = APPLICATION_A_ADDRESS;

    fwUpdateReceivedBytes = 0;

    EraseFlashMemory();

    SendInactivePartitionAddress();
}

void SendInactivePartitionAddress(void)
{

    txHeader.StdId = CAN_ID_SEND;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.IDE = CAN_ID_STD;
    txHeader.DLC = 4; // 주소 전송

    uint32_t targetAddress = fwUpdateAddress;

    txData[0] = (targetAddress >> 24) & 0xFF;
    txData[1] = (targetAddress >> 16) & 0xFF;
    txData[2] = (targetAddress >> 8) & 0xFF;
    txData[3] = (targetAddress) & 0xFF;

    if (HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox) != HAL_OK)
    {
        Error_Handler();
    }
    while (HAL_CAN_IsTxMessagePending(&hcan, txMailbox)){
	}
}

void SendUpdateState(void)
{

    txHeader.StdId = CAN_ID_SEND;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.IDE = CAN_ID_STD;
    txHeader.DLC = 1;

    if (fwUpdateComplete == 1)
    	txData[0] = 0x02; // 완료 신호
    else
    	txData[0] = 0x03; // 실패 신호

    if (HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox) != HAL_OK)
    {
        Error_Handler();
    }
    while (HAL_CAN_IsTxMessagePending(&hcan, txMailbox)){
    }
}

uint8_t MessageBufferIsFull(void)
{
    return ((messageBufferHead + 1) % MESSAGE_BUFFER_SIZE) == messageBufferTail;
}

uint8_t MessageBufferIsEmpty(void)
{
    return messageBufferHead == messageBufferTail;
}

void MessageBufferPut(CAN_RxHeaderTypeDef *rxHeader, uint8_t *rxData)
{
    if (!MessageBufferIsFull())
    {
        messageBuffer[messageBufferHead].rxHeader = *rxHeader;
        memcpy(messageBuffer[messageBufferHead].rxData, rxData, 8);
        messageBufferHead = (messageBufferHead + 1) % MESSAGE_BUFFER_SIZE;
    }
}

void MessageBufferGet(CAN_RxHeaderTypeDef *rxHeader, uint8_t *rxData)
{
    if (!MessageBufferIsEmpty())
    {
        *rxHeader = messageBuffer[messageBufferTail].rxHeader;
        memcpy(rxData, messageBuffer[messageBufferTail].rxData, 8);
        messageBufferTail = (messageBufferTail + 1) % MESSAGE_BUFFER_SIZE;
    }
}

void FirmwareUpdateStateMachine(void)
{
    switch (fwUpdateState)
    {
        case FW_UPDATE_IDLE:
            if (fwUpdateRequested)
            {
                StartFirmwareUpdate();
                fwUpdateState = FW_UPDATE_IN_PROGRESS;

            }
            break;

        case FW_UPDATE_IN_PROGRESS:
        	CAN_RxHeaderTypeDef rxHeader;
        	uint32_t currentAddress = fwUpdateAddress;
        	uint8_t rxData[8];

        	HAL_UART_Transmit(&huart2, "file_start", sizeof("file_start"), HAL_MAX_DELAY);
            while (fwUpdateSize > fwUpdateReceivedBytes)
            {
            	//HAL_UART_Transmit(&huart2, "file_start", sizeof("file_start"), HAL_MAX_DELAY);
            	if(!MessageBufferIsEmpty()){
					MessageBufferGet(&rxHeader, rxData);

					uint8_t dataLength = rxHeader.DLC;

					HAL_FLASH_Unlock();

					for (uint8_t i = 0; i < dataLength; i += 2)
					{
						uint16_t data16 = rxData[i];
						if (i + 1 < dataLength)
						{
							data16 |= rxData[i + 1] << 8;
						}


						if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, currentAddress, data16) != HAL_OK)
						{
							HAL_FLASH_Lock();
							fwUpdateRequested = 0;
							fwUpdateState = FW_UPDATE_IDLE;
							return;
						}
						currentAddress += 2;
						fwUpdateReceivedBytes += (i + 1 < dataLength) ? 2 : 1;

                        // 디버깅: 수신된 바이트 수 출력
						HAL_UART_Transmit(&huart2, "p", sizeof("p"), HAL_MAX_DELAY);

					}
            	}
            }
            fwUpdateRequested = 0;
        	HAL_UART_Transmit(&huart2, "file_end", sizeof("file_end"), HAL_MAX_DELAY);
			if (fwUpdateReceivedBytes != fwUpdateSize)
			{
				HAL_UART_Transmit(&huart2, "failed", sizeof("failed"), HAL_MAX_DELAY);
				SendUpdateState();
				fwUpdateState = FW_UPDATE_IDLE;

				//
	        	SetBootFlag();
				HAL_Delay(1000);
				NVIC_SystemReset();
			}
			else{
				HAL_UART_Transmit(&huart2, "ccomplete", sizeof("ccomplete"), HAL_MAX_DELAY);
				fwUpdateComplete = 1;
				SendUpdateState();
				fwUpdateState = FW_UPDATE_COMPLETE;
			}
			fwUpdateSize = 0;
            break;

        case FW_UPDATE_COMPLETE:
            HAL_UART_Transmit(&huart2, "complete", sizeof("complete"), HAL_MAX_DELAY);
        	SetBootFlag();
			HAL_Delay(1000);
			NVIC_SystemReset();
            break;

        default:
        	fwUpdateState = FW_UPDATE_IDLE;
            break;
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_SET);
  while (1)
  {
	  SetBootFlag();
	  NVIC_SystemReset();
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
