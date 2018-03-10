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
#include "queue.h"
#include "semphr.h"

#define EVENT_60_SECONDS (1<<0)
#define EVENT_60_MINUTES (1<<1)

#define EVENT_TARGET_SECONDS (1<<2)
#define EVENT_TARGET_MINUTES (1<<3)
#define EVENT_TARGET_HOURS   (1<<4)

#define EVENT_NEW_TIME (1<<5)

typedef struct
{
	uint8_t alarmSeconds;
	uint8_t alarmMinutes;
	uint8_t alarmHours;
} alarm_type_t;

typedef enum
{
	seconds_type, minutes_type, hours_type
} time_types_t;

typedef struct
{
	time_types_t time_type;
	uint8_t value;
} time_msg_t;

#define QUEUE_LENGTH 5
#define MESSAGE_SIZE sizeof( time_msg_t )

EventGroupHandle_t g_time_events;
QueueHandle_t timeQueue;
SemaphoreHandle_t printMutex;

//vQueueAddToRegistry(timeQueue,"TIME");

void seconds_task ( void *arg )
{
	TickType_t xLastWakeTime;
	alarm_type_t * receivedAlarm = ( alarm_type_t * ) arg;
	const TickType_t xPeriod = pdMS_TO_TICKS( 1000 );
	xLastWakeTime = xTaskGetTickCount ();
	uint8_t seconds = 58;
	time_msg_t sc_msg;
	sc_msg.time_type = seconds_type;
	sc_msg.value = seconds;
	xQueueSendToBack( timeQueue, &sc_msg, portMAX_DELAY );
	xEventGroupSetBits ( g_time_events, EVENT_NEW_TIME );

	for ( ;; )
	{
		//receivedAlarm = (alarm_type_t *) arg;
		vTaskDelayUntil ( &xLastWakeTime, xPeriod );
		seconds++;
		if (receivedAlarm->alarmSeconds == seconds)
		{
			xEventGroupSetBits ( g_time_events, EVENT_TARGET_SECONDS );
		}
		if (60 == seconds)
		{
			seconds = 0;
			xEventGroupSetBits ( g_time_events, EVENT_60_SECONDS );
		}
		sc_msg.value = seconds;
		xQueueSendToBack( timeQueue, &sc_msg, portMAX_DELAY );
		xEventGroupSetBits ( g_time_events, EVENT_NEW_TIME );
	}
}
void minutes_task ( void *arg )
{
	alarm_type_t * receivedAlarm = ( alarm_type_t * ) arg;
	uint8_t minutes = 59;
	time_msg_t mn_msg;
	mn_msg.time_type = minutes_type;
	mn_msg.value = minutes;
	xQueueSendToBack( timeQueue, &mn_msg, portMAX_DELAY );
	xEventGroupSetBits ( g_time_events, EVENT_NEW_TIME );

	for ( ;; )
	{
		//receivedAlarm = (alarm_type_t *) arg;
		if (receivedAlarm->alarmMinutes == minutes)
		{
			xEventGroupSetBits ( g_time_events, EVENT_TARGET_MINUTES );
		}
		xEventGroupWaitBits ( g_time_events, EVENT_60_SECONDS, pdTRUE, pdTRUE,
		portMAX_DELAY );
		minutes++;
		if (60 == minutes)
		{
			minutes = 0;
			xEventGroupSetBits ( g_time_events, EVENT_60_MINUTES );
		}
		mn_msg.value = minutes;
		xQueueSendToBack( timeQueue, &mn_msg, portMAX_DELAY );
		//xEventGroupSetBits ( g_time_events, EVENT_NEW_TIME );
	}
}
void hours_task ( void *arg )
{
	alarm_type_t * receivedAlarm = ( alarm_type_t * ) arg;
	uint8_t hours = 23;
	time_msg_t hr_msg;
	hr_msg.time_type = hours_type;
	hr_msg.value = hours;
	xQueueSendToBack( timeQueue, &hr_msg, portMAX_DELAY );
	xEventGroupSetBits ( g_time_events, EVENT_NEW_TIME );

	for ( ;; )
	{
		//receivedAlarm = (alarm_type_t *) arg;
		if (receivedAlarm->alarmHours == hours)
		{
			xEventGroupSetBits ( g_time_events, EVENT_TARGET_HOURS );
		}
		xEventGroupWaitBits ( g_time_events, EVENT_60_MINUTES, pdTRUE, pdTRUE,
		portMAX_DELAY );
		hours++;
		if (24 == hours)
		{
			hours = 0;
		}
		hr_msg.value = hours;
		xQueueSendToBack( timeQueue, &hr_msg, portMAX_DELAY );
		//xEventGroupSetBits ( g_time_events, EVENT_NEW_TIME );
	}
}

void alarm_task ( void *arg )
{
	for ( ;; )
	{
		xEventGroupWaitBits ( g_time_events,
		EVENT_TARGET_SECONDS | EVENT_TARGET_MINUTES | EVENT_TARGET_HOURS,
		pdTRUE, pdTRUE, portMAX_DELAY );
		xSemaphoreTake( printMutex, portMAX_DELAY );
		PRINTF ( "ALARM\n" );
		xSemaphoreGive( printMutex );
	}
}

void print_task ( void *arg )
{
	uint8_t sg_to_print = 0;
	uint8_t mn_to_print = 0;
	uint8_t hr_to_print = 0;
	for ( ;; )
	{
		xEventGroupWaitBits ( g_time_events, EVENT_NEW_TIME, pdTRUE, pdTRUE,
		portMAX_DELAY );
		while ( uxQueueMessagesWaiting ( timeQueue ) )
		{
			time_msg_t rx_msg;
			xQueueReceive( timeQueue, &rx_msg, portMAX_DELAY );
			switch ( rx_msg.time_type )
			{
				case seconds_type :
					sg_to_print = rx_msg.value;
					break;
				case minutes_type :
					mn_to_print = rx_msg.value;
					break;
				case hours_type :
					hr_to_print = rx_msg.value;
					break;
				default :
					break;
			}
		}
		xSemaphoreTake( printMutex, portMAX_DELAY );
		PRINTF ( "%d:%d:%d\n", hr_to_print, mn_to_print, sg_to_print );
		xSemaphoreGive( printMutex );
	}
}
int main ( void )
{

	/* Init board hardware. */
	BOARD_InitBootPins ();
	BOARD_InitBootClocks ();
	BOARD_InitBootPeripherals ();
	/* Init FSL debug console. */
	BOARD_InitDebugConsole ();

	g_time_events = xEventGroupCreate ();
	timeQueue = xQueueCreate( QUEUE_LENGTH, MESSAGE_SIZE );
	printMutex = xSemaphoreCreateMutex ();

	static alarm_type_t targetAlarm;
	targetAlarm.alarmHours = 0;
	targetAlarm.alarmMinutes = 0;
	targetAlarm.alarmSeconds = 1;

	xTaskCreate ( seconds_task, "Seconds", 300, ( void* ) &targetAlarm,
	configMAX_PRIORITIES - 3, NULL );
	xTaskCreate ( minutes_task, "Minutes", 300, ( void* ) &targetAlarm,
	configMAX_PRIORITIES - 2, NULL );
	xTaskCreate ( hours_task, "Hours", 300, ( void* ) &targetAlarm,
	configMAX_PRIORITIES - 2, NULL );
	xTaskCreate ( alarm_task, "Alarm", 300, NULL, configMAX_PRIORITIES - 1,
	NULL );
	xTaskCreate ( print_task, "Printer", 300, NULL, configMAX_PRIORITIES - 4,
	NULL );
	vTaskStartScheduler ();

	for ( ;; )
	{

	}

	return 0;
}
