
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "fsl_port.h"
#include "fsl_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define TIME_BASE_VALUE 0
#define MAX_SECONDS 60
#define MAX_MINUTES 60
#define MAX_HOURS 24
#define TIME_DELAY 500

SemaphoreHandle_t mn_semaphore;
SemaphoreHandle_t hr_semaphore;

void sg_task (void *arg)
{
	uint8_t sg_count = 0;
	const TickType_t xPeriod = 1000;
	TickType_t xLastWakeTime = xTaskGetTickCount();
	for(;;)
	{
		vTaskDelayUntil(&xLastWakeTime,xPeriod);
		xSemaphoreTake(mn_semaphore,TIME_DELAY);
		sg_count++;
		if(MAX_SECONDS == sg_count)
		{
			sg_count = TIME_BASE_VALUE;
			xSemaphoreGive(mn_semaphore);
		}
	}
}

void mn_task (void *arg)
{
	uint8_t mn_count = 0;
	for(;;)
	{
		xSemaphoreTake(mn_semaphore,TIME_DELAY);
		mn_count++;
		if(MAX_MINUTES == mn_count)
		{
			mn_count = TIME_BASE_VALUE;
			xSemaphoreGive(hr_semaphore);
		}
		xSemaphoreGive(mn_semaphore);
	}
}

void hr_task (void *arg)
{
	uint8_t hr_count = 0;
	for(;;)
	{
		xSemaphoreTake(hr_semaphore,TIME_DELAY);
		hr_count++;
		if(MAX_HOURS == hr_count){
			hr_count = TIME_BASE_VALUE;
		}
		xSemaphoreGive(hr_semaphore);
	}
}

int main(void)
{

	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();

	xTaskCreate(sg_task,"seconds",110,NULL,3,NULL);
	xTaskCreate(mn_task,"minutes",110,NULL,3,NULL);
	xTaskCreate(hr_task,"hours",110,NULL,3,NULL);

	vTaskStartScheduler();
	while (1)
	{

	}
	return 0;
}
