/**
  ******************************************************************************
  * @file          : app_tof.c
  * @author        : IMG SW Application Team
  * @brief         : This file provides code for the configuration
  *                  of the STMicroelectronics.X-CUBE-TOF1.3.4.1 instances.
  ******************************************************************************
  *
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_tof.h"
#include "main.h"
#include <stdio.h>

#include "53l5a1_ranging_sensor.h"
#include "app_tof_pin_conf.h"
////#include "stm32l4xx_nucleo.h"

#include "cmsis_os.h"
#include "VCP_UART.h"

extern osSemaphoreId xSemaphoreTOF;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TIMING_BUDGET (30U) /* 5 ms < TimingBudget < 100 ms */
#define RANGING_FREQUENCY (5U) /* Ranging frequency Hz (shall be consistent with TimingBudget value) */
#define POLLING_PERIOD (1000U/RANGING_FREQUENCY) /* refresh rate for polling mode (milliseconds) */

/* Private variables ---------------------------------------------------------*/
static RANGING_SENSOR_Capabilities_t Cap;
static RANGING_SENSOR_ProfileConfig_t Profile;
static RANGING_SENSOR_Result_t Result;

static int32_t status = 0;
#if 0
static volatile uint8_t PushButtonDetected = 0;
volatile uint8_t ToF_EventDetected = 0;
#endif

/* Private function prototypes -----------------------------------------------*/
static void MX_53L5A1_SimpleRanging_Init(void);
static void MX_53L5A1_SimpleRanging_Process(void);
static void print_result(RANGING_SENSOR_Result_t *Result);
void toggle_resolution(void);
void toggle_signal_and_ambient(void);
static void clear_screen(void);
static void display_commands_banner(void);
static int  handle_cmd(uint8_t cmd);
static uint8_t get_key(void);
static uint32_t com_has_data(void);

void MX_TOF_Init(void)
{
  MX_53L5A1_SimpleRanging_Init();
}

/*
 * LM background task
 */
void MX_TOF_Process(void)
{
  MX_53L5A1_SimpleRanging_Process();
}

static volatile int sTOFTaskRunning = 0;

void ReleaseTOFTask(void)
{
	sTOFTaskRunning = 1;
	osSemaphoreRelease(xSemaphoreTOF);
}

void PauseTOFTask(void)
{
	sTOFTaskRunning = 0;
}

unsigned char TOFdataBuffer[100];

void StartTOFTask(void *argument)
{
	extern int SendTOFUDP(const unsigned char *b, unsigned int len);
	(void)argument;

	uint32_t Id;

	for(;;)
	{
		/* wait for release */
		if (osSemaphoreAcquire(xSemaphoreTOF, portMAX_DELAY) == osOK)
		{
			VL53L5A1_RANGING_SENSOR_ReadID(VL53L5A1_DEV_CENTER, &Id);
			VL53L5A1_RANGING_SENSOR_GetCapabilities(VL53L5A1_DEV_CENTER, &Cap);

			Profile.RangingProfile = RS_PROFILE_4x4_CONTINUOUS;
			Profile.TimingBudget = TIMING_BUDGET;
			Profile.Frequency = RANGING_FREQUENCY; /* Ranging frequency Hz (shall be consistent with TimingBudget value) */
			Profile.EnableAmbient = 0; /* Enable: 1, Disable: 0 */
			Profile.EnableSignal = 0; /* Enable: 1, Disable: 0 */

			/* set the profile if different from default one */
			VL53L5A1_RANGING_SENSOR_ConfigProfile(VL53L5A1_DEV_CENTER, &Profile);

			status = VL53L5A1_RANGING_SENSOR_Start(VL53L5A1_DEV_CENTER, RS_MODE_BLOCKING_CONTINUOUS);

			if (status != BSP_ERROR_NONE)
			{
				print_log(UART_OUT, "VL53L5A1_RANGING_SENSOR_Start failed\r\n");
				osThreadTerminate(osThreadGetId());
				return;
			}

			/* Infinite loop */
			for(;;)
			{
				if ( ! sTOFTaskRunning)
				{
					VL53L5A1_RANGING_SENSOR_Stop(VL53L5A1_DEV_CENTER);
					break;
				}

			    status = VL53L5A1_RANGING_SENSOR_GetDistance(VL53L5A1_DEV_CENTER, &Result);

			    if (status == BSP_ERROR_NONE)
			    {
			    	/* do the data processing and send via network */
			    	SendTOFUDP((unsigned char *)&Result, sizeof(Result));
			    }

				osDelay(POLLING_PERIOD);
			}
		}
	}
}

/* ----------------------------- static ---------------------------------------*/

static void MX_53L5A1_SimpleRanging_Init(void)
{
#if 0
  /* Initialize Virtual COM Port */
  BSP_COM_Init(COM1);

  /* Initialize button */
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
#endif

  status = VL53L5A1_RANGING_SENSOR_Init(VL53L5A1_DEV_CENTER);

  if (status != BSP_ERROR_NONE)
  {
    print_log(UART_OUT, "VL53L5A1_RANGING_SENSOR_Init failed\r\n");
    print_log(UART_OUT, "Check you're using ONLY the center device soldered on the shield, NO satellite shall be connected !\r\n");
    while (1);
  }
}

static void MX_53L5A1_SimpleRanging_Process(void)
{
  uint32_t Id;

  VL53L5A1_RANGING_SENSOR_ReadID(VL53L5A1_DEV_CENTER, &Id);
  VL53L5A1_RANGING_SENSOR_GetCapabilities(VL53L5A1_DEV_CENTER, &Cap);

  Profile.RangingProfile = RS_PROFILE_4x4_CONTINUOUS;
  Profile.TimingBudget = TIMING_BUDGET;
  Profile.Frequency = RANGING_FREQUENCY; /* Ranging frequency Hz (shall be consistent with TimingBudget value) */
  Profile.EnableAmbient = 0; /* Enable: 1, Disable: 0 */
  Profile.EnableSignal = 0; /* Enable: 1, Disable: 0 */

  /* set the profile if different from default one */
  VL53L5A1_RANGING_SENSOR_ConfigProfile(VL53L5A1_DEV_CENTER, &Profile);

  status = VL53L5A1_RANGING_SENSOR_Start(VL53L5A1_DEV_CENTER, RS_MODE_BLOCKING_CONTINUOUS);

  if (status != BSP_ERROR_NONE)
  {
    print_log(UART_OUT, "VL53L5A1_RANGING_SENSOR_Start failed\r\n");
    ////while (1);
    return;
  }

  while (1)
  {
    /* polling mode */
    status = VL53L5A1_RANGING_SENSOR_GetDistance(VL53L5A1_DEV_CENTER, &Result);

    if (status == BSP_ERROR_NONE)
    {
      print_result(&Result);
    }

    if (com_has_data())
    {
      if (handle_cmd(get_key()))
    	  break;
    }

    HAL_Delay(POLLING_PERIOD);
  }
}

static void print_result(RANGING_SENSOR_Result_t *Result)
{
  int8_t i;
  int8_t j;
  int8_t k;
  int8_t l;
  uint8_t zones_per_line;

  zones_per_line = ((Profile.RangingProfile == RS_PROFILE_8x8_AUTONOMOUS) ||
                    (Profile.RangingProfile == RS_PROFILE_8x8_CONTINUOUS)) ? 8 : 4;

  display_commands_banner();

  print_log(UART_OUT, "Cell Format :\r\n\n");
  for (l = 0; l < RANGING_SENSOR_NB_TARGET_PER_ZONE; l++)
  {
    print_log(UART_OUT, " \033[38;5;10m%20s\033[0m : %20s\r\n", "Distance [mm]", "Status");
    if ((Profile.EnableAmbient != 0) || (Profile.EnableSignal != 0))
    {
      print_log(UART_OUT, " %20s : %20s\r\n", "Signal [kcps/spad]", "Ambient [kcps/spad]");
    }
  }

  print_log(UART_OUT, "\r\n\n");

  for (j = 0; j < Result->NumberOfZones; j += zones_per_line)
  {
    for (i = 0; i < zones_per_line; i++) /* number of zones per line */
    {
      print_log(UART_OUT, " -----------------");
    }
    print_log(UART_OUT, "\r\n");

    for (i = 0; i < zones_per_line; i++)
    {
      print_log(UART_OUT, "|                 ");
    }
    print_log(UART_OUT, "|\r\n");

    for (l = 0; l < RANGING_SENSOR_NB_TARGET_PER_ZONE; l++)
    {
      /* Print distance and status */
      for (k = (zones_per_line - 1); k >= 0; k--)
      {
        if (Result->ZoneResult[j + k].NumberOfTargets > 0)
          print_log(UART_OUT, "| \033[38;5;10m%5ld\033[0m  :  %5ld ",
                 (long)Result->ZoneResult[j + k].Distance[l],
                 (long)Result->ZoneResult[j + k].Status[l]);
        else
          print_log(UART_OUT, "| %5s  :  %5s ", "X", "X");
      }
      print_log(UART_OUT, "|\r\n");

      if ((Profile.EnableAmbient != 0) || (Profile.EnableSignal != 0))
      {
        /* Print Signal and Ambient */
        for (k = (zones_per_line - 1); k >= 0; k--)
        {
          if (Result->ZoneResult[j + k].NumberOfTargets > 0)
          {
            if (Profile.EnableSignal != 0)
            {
              print_log(UART_OUT, "| %5ld  :  ", (long)Result->ZoneResult[j + k].Signal[l]);
            }
            else
              print_log(UART_OUT, "| %5s  :  ", "X");

            if (Profile.EnableAmbient != 0)
            {
              print_log(UART_OUT, "%5ld ", (long)Result->ZoneResult[j + k].Ambient[l]);
            }
            else
              print_log(UART_OUT, "%5s ", "X");
          }
          else
            print_log(UART_OUT, "| %5s  :  %5s ", "X", "X");
        }
        print_log(UART_OUT, "|\r\n");
      }
    }
  }

  for (i = 0; i < zones_per_line; i++)
  {
    print_log(UART_OUT, " -----------------");
  }
  print_log(UART_OUT, "\r\n");
}

void toggle_resolution(void)
{
  VL53L5A1_RANGING_SENSOR_Stop(VL53L5A1_DEV_CENTER);

  switch (Profile.RangingProfile)
  {
    case RS_PROFILE_4x4_AUTONOMOUS:
      Profile.RangingProfile = RS_PROFILE_8x8_AUTONOMOUS;
      break;

    case RS_PROFILE_4x4_CONTINUOUS:
      Profile.RangingProfile = RS_PROFILE_8x8_CONTINUOUS;
      break;

    case RS_PROFILE_8x8_AUTONOMOUS:
      Profile.RangingProfile = RS_PROFILE_4x4_AUTONOMOUS;
      break;

    case RS_PROFILE_8x8_CONTINUOUS:
      Profile.RangingProfile = RS_PROFILE_4x4_CONTINUOUS;
      break;

    default:
      break;
  }

  VL53L5A1_RANGING_SENSOR_ConfigProfile(VL53L5A1_DEV_CENTER, &Profile);
  VL53L5A1_RANGING_SENSOR_Start(VL53L5A1_DEV_CENTER, RS_MODE_BLOCKING_CONTINUOUS);
}

void toggle_signal_and_ambient(void)
{
  VL53L5A1_RANGING_SENSOR_Stop(VL53L5A1_DEV_CENTER);

  Profile.EnableAmbient = (Profile.EnableAmbient) ? 0U : 1U;
  Profile.EnableSignal = (Profile.EnableSignal) ? 0U : 1U;

  VL53L5A1_RANGING_SENSOR_ConfigProfile(VL53L5A1_DEV_CENTER, &Profile);
  VL53L5A1_RANGING_SENSOR_Start(VL53L5A1_DEV_CENTER, RS_MODE_BLOCKING_CONTINUOUS);
}

static void clear_screen(void)
{
  print_log(UART_OUT, "\033[2J\033[H"); /* 0x1B = \033 (octal) is ESC command */
}

static void display_commands_banner(void)
{
  /* clear screen */
  print_log(UART_OUT, "\033[2J\033[H");

  print_log(UART_OUT, "53L5A1 Simple Ranging\r\n");
  print_log(UART_OUT, "---------------------\r\n\n");

  print_log(UART_OUT, "control keys:\r\n");
  print_log(UART_OUT, " 'r' : toggle resolution\r\n");
  print_log(UART_OUT, " 's' : enable signal and ambient\r\n");
  print_log(UART_OUT, " 'c' : clear screen\r\n");
  print_log(UART_OUT, " 'x' : exit from endless loop\r\n");
  print_log(UART_OUT, "\r\n");
}

static int handle_cmd(uint8_t cmd)
{
  switch (cmd)
  {
    case 'r':
      toggle_resolution();
      clear_screen();
      break;

    case 's':
      toggle_signal_and_ambient();
      clear_screen();
      break;

    case 'c':
      clear_screen();
      break;

    case 'x':
    	return 1;	//stop endless loop
    	break;

    default:
      break;
  }
  return 0;
}

static uint8_t get_key(void)
{
  uint8_t cmd = 0;

#if 0
  HAL_UART_Receive(&hcom_uart[COM1], &cmd, 1, HAL_MAX_DELAY);
#else
  cmd = (uint8_t)UART_getChar();
#endif

  return cmd;
}

static uint32_t com_has_data(void)
{
#if 0
  return __HAL_UART_GET_FLAG(&hcom_uart[COM1], UART_FLAG_RXNE);
#else
  return (uint32_t)UART_WaitForChar();
#endif
}

#if 0
void BSP_PB_Callback(Button_TypeDef Button)
{
  PushButtonDetected = 1;
}
#endif

#ifdef __cplusplus
}
#endif
