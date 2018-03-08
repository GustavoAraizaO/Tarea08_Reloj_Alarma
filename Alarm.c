#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#define EVENT_60_SECONDS (1<<0)
#define EVENT_60_MINUTES (1<<1)

#define EVENT_TARGET_SECONDS (1<<2)
#define EVENT_TARGET_MINUTES (1<<3)
#define EVENT_TARGET_HOURS   (1<<4)

EventGroupHandle_t g_time_events;

typedef struct {
	uint8_t alarmSeconds;
	uint8_t alarmMinutes;
	uint8_t alarmHours;
} alarm_type_t;

void seconds_task(void *arg)
{
	TickType_t xLastWakeTime;
	alarm_type_t * receivedAlarm = (alarm_type_t *) arg;
	const TickType_t xPeriod = pdMS_TO_TICKS( 1000 );
	xLastWakeTime = xTaskGetTickCount();

	uint8_t seconds = 58;

	for(;;)
	{
		vTaskDelayUntil( &xLastWakeTime, xPeriod );
		seconds++;
		if(receivedAlarm->alarmSeconds == seconds)
		{
			xEventGroupSetBits(g_time_events,EVENT_TARGET_SECONDS);
		}
		if(60 == seconds)
		{
			seconds = 0;
			xEventGroupSetBits(g_time_events,EVENT_60_SECONDS);
		}
	}
}
void minutes_task(void *arg)
{
	alarm_type_t * receivedAlarm = (alarm_type_t *) arg;
	uint8_t minutes = 59;
	for(;;)
	{
		xEventGroupWaitBits(g_time_events, EVENT_60_SECONDS, pdTRUE, pdTRUE, portMAX_DELAY);
		minutes++;
		if(receivedAlarm->alarmMinutes == minutes)
		{
			xEventGroupSetBits(g_time_events,EVENT_TARGET_MINUTES);
		}
		if(60 == minutes)
		{
			minutes = 0;
			xEventGroupSetBits(g_time_events,EVENT_60_MINUTES);

		}
	}
}
void hours_task(void *arg)
{
	alarm_type_t * receivedAlarm = (alarm_type_t *) arg;
	uint8_t hours = 0;
	for(;;)
	{
		xEventGroupWaitBits(g_time_events, EVENT_60_MINUTES, pdTRUE, pdTRUE, portMAX_DELAY);
		hours++;
		if(receivedAlarm->alarmHours == hours)
		{
			xEventGroupSetBits(g_time_events,EVENT_TARGET_HOURS);
		}
		if(24 == hours)
		{
			hours = 0;
		}
	}
}

void alarm_task(void *arg)
{
	for(;;)
	{
		xEventGroupWaitBits(g_time_events, EVENT_TARGET_SECONDS | EVENT_TARGET_MINUTES | EVENT_TARGET_HOURS, pdTRUE, pdTRUE, portMAX_DELAY);
		PRINTF("ALARM");
	}
}

void print_task(void *arg)
{
	while(1)
	{
		vTaskDelay(100);
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

    g_time_events = xEventGroupCreate();
    alarm_type_t targetAlarm = {0,0,1};

    xTaskCreate(seconds_task, "Seconds",300,(void*)&targetAlarm,configMAX_PRIORITIES-3 ,NULL);
    xTaskCreate(minutes_task, "Minutes",300,(void*)&targetAlarm,configMAX_PRIORITIES-2 ,NULL);
    xTaskCreate(hours_task, "Hours",300,(void*)&targetAlarm,configMAX_PRIORITIES-2 ,NULL);
    xTaskCreate(alarm_task,"Alarm",300,NULL,configMAX_PRIORITIES-1, NULL);
    xTaskCreate(print_task, "Printer",300,NULL,configMAX_PRIORITIES-2 ,NULL);
    vTaskStartScheduler();

    for(;;)
    {

    }

    return 0 ;
}
