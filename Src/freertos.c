/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     
#include <stdio.h>
#include <string.h>
#include "iwdg.h"
#include "usbd_cdc_if.h"
#include "gpio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    uint8_t	FrameLen;
    uint8_t	FrameBuf[256];
    uint8_t	FrameAck[64];
    uint8_t	FrameCheckSum;
    uint8_t	FrameLenMax;
    uint8_t	FramStatus;
    void (*ProtocolDecode)(uint8_t *Buf, uint16_t Len);
}tHostUartProtocolInfo;

typedef struct
{
    GPIO_TypeDef        *Port;
    uint16_t            Pin;
}tGPIODef;

#if DEBUG
#define dbgprintf(fmt, ...)                do{\
                                                printf(fmt, ##__VA_ARGS__);\
                                                }while(0)
#else
#define dbgprintf(fmt, ...)                do{}while(0)
#endif

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t UartTxBuf[256];
uint32_t UartTxBufPtOut = 0;
tHostUartProtocolInfo uartRxDecodeInfo;
static uint8_t IwdgTaskTable = 0;


/* USER CODE END Variables */
osThreadId HostDecodeTaskHandle;
osMessageQId uartRxQueueHandle;
osMutexId UartWriteMutexHandle;
osMutexId IwdgWriteMutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
uint8_t TaskFeedIwdgRegister(void);
void TaskFeedIwdg(uint8_t taskId);
void USBD_CDC_ReceiveData(uint8_t * Buf, uint32_t Len);
BaseType_t UartTransmitDataToHost(uint8_t * Buf, uint16_t Len);
BaseType_t UsbCdcTransmitDataToHost(uint8_t * Buf, uint16_t Len);
void HostProtocolDecode(tHostUartProtocolInfo * Protocol, uint8_t data);
void HostCmdProcess(uint8_t *Buf, uint16_t Len);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  dbgprintf("==========HELLO WORLD==========\n");     
  /* USER CODE END Init */

  /* Create the mutex(es) */
  /* definition and creation of UartWriteMutex */
  osMutexDef(UartWriteMutex);
  UartWriteMutexHandle = osMutexCreate(osMutex(UartWriteMutex));

  /* definition and creation of IwdgWriteMutex */
  osMutexDef(IwdgWriteMutex);
  IwdgWriteMutexHandle = osMutexCreate(osMutex(IwdgWriteMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of uartQueue */
  osMessageQDef(uartQueue, 256, uint8_t);
  uartRxQueueHandle = osMessageCreate(osMessageQ(uartQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of HostDecodeTask */
  osThreadDef(HostDecodeTask, StartDefaultTask, osPriorityNormal, 0, 256);
  HostDecodeTaskHandle = osThreadCreate(osThread(HostDecodeTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  uint8_t fwdgId;
  uint8_t data;
    
  fwdgId = TaskFeedIwdgRegister();
  
  /*Init interface*/
  uartRxDecodeInfo.ProtocolDecode = HostCmdProcess;
  
  dbgprintf("Start HostDecode task\n");
  
  /* Infinite loop */
  for(;;)
  {
    /*Wait data from uart*/
    if(pdPASS == xQueueReceive(uartRxQueueHandle, &data, pdMS_TO_TICKS(2000)))
    {
        HostProtocolDecode(&uartRxDecodeInfo, data);
    }
    else
    {

    }
    
    MX_GPIO_Toggle(LED1_GPIO_Port, LED1_Pin);
    
    TaskFeedIwdg(fwdgId);
  }

  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
* @brief Register to feed iwdg.
* @param argument: None
* @retval This task's id
*/
uint8_t TaskFeedIwdgRegister(void)
{
    static uint8_t location = 0;
    uint8_t id;

    id = 1 << location++;

    IwdgTaskTable |= id;

    return id;
}
/**
* @brief Feed iwdg.
* @param argument: taskId task id
* @retval None
*/
void TaskFeedIwdg(uint8_t taskId)
{
    static uint8_t tasks = 0;

    if ( xSemaphoreTake(IwdgWriteMutexHandle, portMAX_DELAY) == pdTRUE)
    {
        tasks |= taskId;
        if ( tasks == IwdgTaskTable )
        {
          MX_IWDG_Refresh();
        }
      
        xSemaphoreGive( IwdgWriteMutexHandle ); 
    }
    else
    {

    }
}

/*Process host command*/
void HostCmdProcess(uint8_t *Buf, uint16_t Len)
{
    uint8_t *Cmd;
    Cmd = Buf;
    
    dbgprintf("Process CMD %#02X\n", Cmd[0]);
    
    switch(Cmd[0])
    {
        case 0x00:
        {

        }
        break;
        
        case 0x01:
        {
            if(Cmd[1] == 0xFF)
            {
                /*System reset*/
                uint8_t ack[] = {0x01, 0xFF, 0x00};
				uint32_t msTimer;
				
                dbgprintf("System reset...\n");
                UartTransmitDataToHost(ack, sizeof(ack));
                msTimer = Cmd[2] <<8 | Cmd[3];
				
				HAL_Delay(msTimer);
                HAL_NVIC_SystemReset();
            }
			
			if((Cmd[1] == 0x01) && (Cmd[2] == 0x01))
            {
                /*Got ping*/
                uint8_t ack[] = {0x01, 0x01, 0x00};
                dbgprintf("Got ping...\n");
                UartTransmitDataToHost(ack, sizeof(ack));
            }
        }
        break;
        
        case 0x0A:
        {
		  	//
		  	if(Cmd[1] == 0x01)
			{
			}
			
		  	//
		  	if(Cmd[1] == 0x0D)
			{
			}
			
		  	//
		  	if(Cmd[1] == 0xAD)
			{
			}			
        }
        break;
        
        case 0x0B:
        {
		  	//
		  	if(Cmd[1] == 0x02)
			{
			}
			
		  	//
		  	if(Cmd[1] == 0x03)
			{
			}
			
		  	//
		  	if(Cmd[1] == 0x04)
			{
			}
		}
        break;
    }
}

/*Decode host data and excute cmd process*/
void HostProtocolDecode(tHostUartProtocolInfo * Protocol, uint8_t data)
{
    switch(Protocol->FramStatus)
    {
        case 0:
            if(0xff == data)
            {
                Protocol->FramStatus = 1;
            }
            break;

        case 1:
            if(0x55 == data)
            {
                Protocol->FramStatus = 2;
                memset(&Protocol->FrameBuf, 0x00, sizeof(Protocol->FrameBuf));
            }
            else
            {
                Protocol->FramStatus = 0;
            }
            break;

        case 2:
            if(0xff != data)
            {
                Protocol->FrameCheckSum = Protocol->FrameLenMax = data;
                Protocol->FrameLen = 0;
                Protocol->FramStatus = 3;
            }
            else
            {
                Protocol->FramStatus = 0;
            }
            break;

        case 3:
            if(Protocol->FrameLen < Protocol->FrameLenMax - 1)
            {
                Protocol->FrameBuf[Protocol->FrameLen] = data;
                if(Protocol->FrameLen > 2)
                {		
                    if(0x81 == Protocol->FrameBuf[1])
                    {
                        Protocol->FrameBuf[Protocol->FrameLen - 1] = Protocol->FrameBuf[Protocol->FrameLen - 1] ^ Protocol->FrameBuf[Protocol->FrameLen];
                    }
                }
                Protocol->FrameCheckSum += data;
                Protocol->FrameLen++;
            }
            else
            {
                if(data == Protocol->FrameCheckSum)
                {                           
                    Protocol->FrameBuf[Protocol->FrameLen] = 0;
                    Protocol->FrameBuf[Protocol->FrameLen + 1] = 0;

                    /* Deal command at here */
                    if(Protocol->ProtocolDecode)
                    {
                        Protocol->ProtocolDecode(Protocol->FrameBuf, Protocol->FrameLen);
                    }
                    else
                    {
                        /*Print debug information*/
                    }
                }
                else
                {
                    /* debug start*/

                    /* debug end*/
                }
                Protocol->FramStatus = 0;
                Protocol->FrameLen = 0;
                Protocol->FrameCheckSum = 0;
                Protocol->FrameLenMax = 0;
            }
            break;
        
        default:
                Protocol->FramStatus = 0;
                break;
    }
}

/*Uart tx data to host*/
BaseType_t UartTransmitDataToHost(uint8_t * Buf, uint16_t Len)
{
    BaseType_t res = pdFAIL;
    
    uint16_t size, index;
    uint8_t checkSum;
    
    size = Len + 4;
    
    uint8_t *pData = pvPortMalloc(size * sizeof(uint8_t));
    
    if(pData)
    {
        uint16_t i;
        
        pData[0] = 0xff;
        pData[1] = 0x55;
        pData[2] = checkSum = Len + 1;
        index = 3;
        i = 0;
        
        while(i < Len)
        {
            pData[index] = Buf[i];
            checkSum += Buf[i];

            i++;
            index++;
        }
        
        pData[index] = checkSum;
        
        // Transmit to usb host
        res = UsbCdcTransmitDataToHost(pData, size);
        vPortFree( pData );
    }
    else
    {
        res = pdFAIL; 
    }
    
    return res;
}

/*Transmit data to host*/
BaseType_t UsbCdcTransmitDataToHost(uint8_t * Buf, uint16_t Len)
{
    BaseType_t res = pdFAIL;
    
    if(pdPASS == xSemaphoreTake(UartWriteMutexHandle,  portMAX_DELAY ))
    {
        uint16_t size, remain, index;
        
        remain = size = Len;
        index = 0;
        
        while(remain > 0)
        {
            if(size > sizeof(UartTxBuf))
            {
                remain = size - sizeof(UartTxBuf);
                size = sizeof(UartTxBuf);
            }
            else
            {
                size = remain;
                remain -= size;
            }
            
            memcpy(UartTxBuf, Buf + index, size);

            if(USBD_OK == CDC_Transmit_FS(UartTxBuf, size))
            {
                /*Transmit successfully*/
                res = pdPASS;
            }
            else
            {
                /*Transmit failed*/
                res = pdFAIL;
                break;
            }
            
            index += size;
        }
        
        xSemaphoreGive(UartWriteMutexHandle);
    }
    else
    {
        /*Mutex get failed*/
        res = pdFAIL;
    }
    
    return res;
}

/*Rx data from usb_cdc driver*/
void USBD_CDC_ReceiveData(uint8_t * Buf, uint32_t Len)
{
  uint32_t i;
  BaseType_t xHigherPriorityTaskWoken; 
  
  xHigherPriorityTaskWoken = pdFALSE; 
   
  for(i = 0; i < Len; i++)
  {
    if(pdTRUE == xQueueSendToBackFromISR(uartRxQueueHandle, &Buf[i], &xHigherPriorityTaskWoken))
    {
      /*Put successfully*/
    }
    else
    {
      /*Put failed*/
    }
  }
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
