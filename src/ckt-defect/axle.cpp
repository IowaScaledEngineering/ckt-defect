/*************************************************************************
Title:    Axle Counter Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     axle.cpp
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2026 Michael Petersen & Nathan Holmes

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*************************************************************************/

#include <Arduino.h>

#include "common.h"
#include "io.h"
#include "axle.h"

QueueHandle_t axle_A1_Queue;
QueueHandle_t axle_A2_Queue;
QueueHandle_t axle_B1_Queue;
QueueHandle_t axle_B2_Queue;

void IRAM_ATTR axle_A1_isr(void *arg)
{
	BaseType_t xHigherPriorityTaskWoken;
	unsigned long time = millis();
	xQueueSendFromISR(axle_A1_Queue, &time, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void IRAM_ATTR axle_A2_isr(void *arg)
{
	BaseType_t xHigherPriorityTaskWoken;
	unsigned long time = millis();
	xQueueSendFromISR(axle_A2_Queue, &time, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void IRAM_ATTR axle_B1_isr(void *arg)
{
	BaseType_t xHigherPriorityTaskWoken;
	unsigned long time = millis();
	xQueueSendFromISR(axle_B1_Queue, &time, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void IRAM_ATTR axle_B2_isr(void *arg)
{
	BaseType_t xHigherPriorityTaskWoken;
	unsigned long time = millis();
	xQueueSendFromISR(axle_B2_Queue, &time, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void axleInit()
{
	axle_A1_Queue = xQueueCreate(4, sizeof(unsigned long));
	axle_A2_Queue = xQueueCreate(4, sizeof(unsigned long));
	axle_B1_Queue = xQueueCreate(4, sizeof(unsigned long));
	axle_A2_Queue = xQueueCreate(4, sizeof(unsigned long));

	gpio_install_isr_service(0);
	gpio_isr_handler_add(AXLE_A1, axle_A1_isr, NULL);
	gpio_isr_handler_add(AXLE_A2, axle_A2_isr, NULL);
	gpio_isr_handler_add(AXLE_B1, axle_B1_isr, NULL);
	gpio_isr_handler_add(AXLE_B2, axle_B2_isr, NULL);
}

size_t axleGetNumTimes(gpio_num_t pin)
{
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wswitch"
	switch(pin)
	{
		case AXLE_A1:
			return uxQueueMessagesWaiting(axle_A1_Queue);
			break;
		case AXLE_A2:
			return uxQueueMessagesWaiting(axle_A2_Queue);
			break;
		case AXLE_B1:
			return uxQueueMessagesWaiting(axle_B1_Queue);
			break;
		case AXLE_B2:
			return uxQueueMessagesWaiting(axle_B2_Queue);
			break;
	}
	#pragma GCC diagnostic pop
	return 0;
}

void axleGetTime(gpio_num_t pin, unsigned long *time)
{
	// Should only call this when you know something is sitting in the queue, otherwise it will block
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wswitch"
	switch(pin)
	{
		case AXLE_A1:
			xQueueReceive(axle_A1_Queue, time, portMAX_DELAY);
			break;
		case AXLE_A2:
			xQueueReceive(axle_A2_Queue, time, portMAX_DELAY);
			break;
		case AXLE_B1:
			xQueueReceive(axle_B1_Queue, time, portMAX_DELAY);
			break;
		case AXLE_B2:
			xQueueReceive(axle_B2_Queue, time, portMAX_DELAY);
			break;
	}
	#pragma GCC diagnostic pop
}
