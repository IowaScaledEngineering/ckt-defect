/*************************************************************************
Title:    I2S Audio Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     audio.cpp
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
#include <SPI.h>
#include <SD.h>
#include <stdlib.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"

#include "common.h"
#include "io.h"
#include "sound.h"
#include "audio.h"

// Frames per I2S DMA buffer
#define I2S_NFRAMES   240
// Number of I2S DMA buffers
#define I2S_NBUFFERS    6

static i2s_chan_handle_t i2s_tx_handle;

static QueueHandle_t wavSoundQueue;

typedef enum
{
	PLAYER_IDLE,
	PLAYER_PTT_DELAY,
	PLAYER_INIT,
	PLAYER_RECONFIGURE,
	PLAYER_PLAY,
	PLAYER_FLUSH,
	PLAYER_FLUSHING,
	PLAYER_RESET,
} PlayerState;

static PlayerState playerState;
static volatile bool stopPlayer;
static volatile bool killPlayer = false;

static bool unmute;
static uint8_t audioVolumeStep = VOL_STEP_NOM;
static uint16_t audioVolume = 0;
static uint8_t audioVolumeUpCoef = 0;
static uint8_t audioVolumeDownCoef = 0;
static uint16_t volumeLevels[] = {
		0,      // 0
		100,
		200,
		300,
		400,
		500,
		600,
		700,
		800,
		900,
		1000,   // 10
		1900,
		2800,
		3700,
		4600,
		5500,
		6400,
		7300,
		8200,
		9100,
		10000,  // 20
		11000,
		12000,
		13000,
		14000,
		15000,
		16000,
		17000,
		18000,
		19000,
		20000,  // 30
};

// --- Programmable Noise Levels & Step Control ---
static uint8_t audioNoiseStep = 0;
static uint8_t audioPopcornStep = 0;
static uint16_t noiseLevels[] = {
		0,      // 0
		40,
		80,
		270,
		640,
		1250,
		2160,
		3430,
		5120,
		7290,
		10000    // 10
};

// --- 4th Order High-Pass Filter (375 Hz @ 16 kHz) ---
struct Biquad {
	float b0, b1, b2, a1, a2;
	float x1 = 0.0f, x2 = 0.0f;
	float y1 = 0.0f, y2 = 0.0f;

	float process(float input) {
		float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
		x2 = x1;
		x1 = input;
		y2 = y1;
		y1 = output;
		return output;
	}
};

struct HighPass4thOrder {
	// Coefficients calculated for 4th-order High-Pass Butterworth (Fc=375Hz, Fs=16000Hz)
	Biquad stage1 = { 0.832960f, -1.665920f, 0.832960f, -1.570583f, 0.761259f };
	Biquad stage2 = { 0.895584f, -1.791168f, 0.895584f, -1.688663f, 0.893674f };

	float process(float input) {
		return stage2.process(stage1.process(input));
	}
};

static HighPass4thOrder noiseFilter;

static uint32_t audioPttDelayMillis = 0;
void (*pttEnable)(void) = NULL;
void (*pttDisable)(void) = NULL;

static uint32_t dmaBufferSize;

static TaskHandle_t audioPumpHandle = NULL;


void audioStopPlaying(void)
{
	stopPlayer = true;
}


bool audioIsPlaying(void)
{
	return (PLAYER_IDLE != playerState);
}


void audioSetVolumeStep(uint8_t newVolumeStep)
{
	audioVolumeStep = newVolumeStep;
}


uint8_t audioGetVolumeStep(void)
{
	return(audioVolumeStep);
}


void audioSetNoiseStep(uint8_t newNoiseStep)
{
	if(newNoiseStep <= 10)
	{
		audioNoiseStep = newNoiseStep;
	}
}


uint8_t audioGetNoiseStep(void)
{
	return(audioNoiseStep);
}


void audioSetPopcornStep(uint8_t newPopcornStep)
{
	if(newPopcornStep <= 10)
	{
		audioPopcornStep = newPopcornStep;
	}
}


uint8_t audioGetPopcornStep(void)
{
	return(audioPopcornStep);
}


void audioSetVolumeUpCoef(uint8_t value)
{
	audioVolumeUpCoef = value;
}


uint8_t audioGetVolumeUpCoef(void)
{
	return(audioVolumeUpCoef);
}


void audioSetVolumeDownCoef(uint8_t value)
{
	audioVolumeDownCoef = value;
}


uint8_t audioGetVolumeDownCoef(void)
{
	return(audioVolumeDownCoef);
}


void audioMute(void)
{
	unmute = false;
}


void audioUnmute(void)
{
	unmute = true;
}


void audioSetPttDelay(uint32_t milliseconds)
{
	audioPttDelayMillis = milliseconds;
}

void audioSetPttEnableCallback(void (*callback)(void))
{
	pttEnable = callback;
}

void audioSetPttDisableCallback(void (*callback)(void))
{
	pttDisable = callback;
}



bool audioIsMuted(void)
{
	return( (unmute == false) && (0 == audioVolume) );
}


bool audioQueueEmpty(void)
{
	if(0 == uxQueueMessagesWaiting(wavSoundQueue))
		return true;
	else
		return false;
}


void audioQueuePush(WavSound* wavSound)
{
	xQueueSend(wavSoundQueue, wavSound, portMAX_DELAY);
}


BaseType_t audioQueuePop(WavSound* wavSound)
{
	return xQueueReceive(wavSoundQueue, wavSound, portMAX_DELAY);
}


void audioProcessVolume(void)
{
	uint16_t deltaVolume;
	uint16_t volumeTarget;
	volumeTarget = volumeLevels[audioVolumeStep] * unmute;

	if(audioVolume < volumeTarget)
	{
		deltaVolume = (volumeTarget - audioVolume);
		if((deltaVolume > 0) && (deltaVolume < audioVolumeUpCoef))
			deltaVolume = audioVolumeUpCoef;  // Make sure it goes all the way to min or max
		audioVolume += deltaVolume / audioVolumeUpCoef;
	}
	else if(audioVolume > volumeTarget)
	{
		deltaVolume = (audioVolume - volumeTarget);
		if((deltaVolume > 0) && (deltaVolume < audioVolumeDownCoef))
			deltaVolume = audioVolumeDownCoef;  // Make sure it goes all the way to min or max
		audioVolume -= deltaVolume / audioVolumeDownCoef;
	}
}


static void audioPump(void *args)
{
	int16_t sampleValue;
	size_t bytesWritten;
	i2s_std_clk_config_t clk_cfg;
	uint32_t outputValue;
	WavSound wavSound = { nullptr, false };
	uint32_t oldSampleRate = 0;
	uint32_t flushCount = 0;
	unsigned long timeoutStartTime;
	unsigned long pttStartTime;

	playerState = PLAYER_RESET;

	while(1)
	{
		switch(playerState)
		{
			case PLAYER_IDLE:
				if(!audioQueueEmpty())
				{
					// Queue not empty
					pttStartTime = millis();
					if(NULL != pttEnable)
					{
						pttEnable();
					}
					playerState = PLAYER_PTT_DELAY;
				}
				break;

			case PLAYER_PTT_DELAY:
				if((millis() - pttStartTime) >= audioPttDelayMillis)
				{
					playerState = PLAYER_INIT;
				}
				else
				{
					vTaskDelay(10 / portTICK_PERIOD_MS);
				}
				break;

			case PLAYER_INIT:
				if(audioQueuePop(&wavSound))  // Should only get here when there is something in the queue, so portMAX_DELAY is fine
				{
					wavSound.wav->open();         // Open the sound
					if(wavSound.wav->getSampleRate() == oldSampleRate)
						playerState = PLAYER_PLAY;
					else
						playerState = PLAYER_RECONFIGURE;
					stopPlayer = false;  // Needed here in case we run out of samples before muting is complete
				}
				break;

			case PLAYER_RECONFIGURE:
				clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(wavSound.wav->getSampleRate());
				gpio_set_level(I2S_SD, 0);             // Disable amplifier
				i2s_channel_disable(i2s_tx_handle);  // Disable I2S
				i2s_channel_reconfig_std_clock(i2s_tx_handle, &clk_cfg);  // Reset sample rate
				i2s_channel_enable(i2s_tx_handle);  // Enable I2S
				gpio_set_level(I2S_SD, 1);             // Enable amplifier
				oldSampleRate = wavSound.wav->getSampleRate();
				playerState = PLAYER_PLAY;
				break;

			case PLAYER_PLAY:
				if(stopPlayer)
				{
					stopPlayer = false;
					if(nullptr != wavSound.wav)
					{
						wavSound.wav->close();
					}
					playerState = PLAYER_FLUSH;
				}
				else if(wavSound.wav->available())
				{
					// Sound not done, more samples available
					timeoutStartTime = millis();
					sampleValue = wavSound.wav->getNextSample();
					if((millis() - timeoutStartTime) > 150)
					{
						// Getting the next sample took longer than 150ms (5760 bytes @ 44.1kHz = 130ms) so something is wrong (mostly likely the card was removed)
						// Delay task for 1s to allow main loop to clean up
						vTaskDelay(1000 / portTICK_PERIOD_MS);
					}

					// --- Noise Generation & Pre-Volume Mix ---
					int32_t mixedValue = sampleValue;
					if((audioNoiseStep > 0) || (audioPopcornStep > 0))
					{
						// Raw white noise scaled by audioNoiseStep level
						float rawNoise = (float)((rand() % 65536) - 32768) * 0.2f;
						float scaledWhiteNoise = rawNoise * (noiseLevels[audioNoiseStep] / (float)volumeLevels[VOL_STEP_NOM]);

						// RTS (Random Telegraph Signal) Noise Generation scaled by audioPopcornStep level
						static float rtsState = 1.0f;

						// Target ~20 pops/transitions per second adaptive to active sample rate
						uint32_t currentFs = (wavSound.wav && wavSound.wav->getSampleRate()) ? wavSound.wav->getSampleRate() : 16000;
						float pTransition = 20.0f / (float)currentFs;

						if (((float)rand() / (float)RAND_MAX) < pTransition)
						{
							rtsState = -rtsState; // Discrete state toggle (+1.0 / -1.0)
						}

						float scaledRtsNoise = (rtsState * 32768.0f) * (noiseLevels[audioPopcornStep] / (float)volumeLevels[VOL_STEP_NOM]);

						// Add scaled noise sources together and pass through High-Pass Filter
						float filteredNoise = noiseFilter.process(scaledWhiteNoise + scaledRtsNoise);

						mixedValue += (int32_t)filteredNoise;
					}

					// Apply main audio volume calculation on the mixed sample
					int32_t adjustedValue = mixedValue * audioVolume / volumeLevels[VOL_STEP_NOM];
					if(adjustedValue > 32767)
						sampleValue = 32767;
					else if(adjustedValue < -32768)
						sampleValue = -32768;
					else
						sampleValue = (int16_t)adjustedValue;

					// Combine into 32 bit word (left & right)
					outputValue = (sampleValue<<16) | (sampleValue & 0xffff);
setTestPoint(TP0);
					i2s_channel_write(i2s_tx_handle, &outputValue, 4, &bytesWritten, portMAX_DELAY);
clrTestPoint(TP0);
				}
				else
				{
					// Sound done, no samples available
					wavSound.wav->close();
					if(!audioQueueEmpty() && wavSound.seamlessPlay)
					{
						// Queue not empty and seamless playing, so grab next
						playerState = PLAYER_INIT;
					}
					else
					{
						// Otherwise, flush
						playerState = PLAYER_FLUSH;
					}
				}
				break;

			case PLAYER_FLUSH:
				flushCount = 0;
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
				playerState = PLAYER_FLUSHING;
				// Intentional fall through so we attempt one flush
			case PLAYER_FLUSHING:
				#pragma GCC diagnostic pop
				outputValue = 0;
				while(flushCount < dmaBufferSize)
				{
					i2s_channel_write(i2s_tx_handle, &outputValue, 4, &bytesWritten, portMAX_DELAY);
					flushCount++;
				}
				playerState = PLAYER_RESET;
				break;
			
			case PLAYER_RESET:
				if(NULL != pttDisable)
				{
					pttDisable();
				}
				gpio_set_level(I2S_SD, 0);             // Disable amplifier
				i2s_channel_disable(i2s_tx_handle);  // Disable I2S
				oldSampleRate = 0;
				playerState = PLAYER_IDLE;
				break;
		}

		if(killPlayer)
		{
			if(NULL != wavSound.wav)
				wavSound.wav->close();
			killPlayer = false;
			break;  // Escape the while loop
		}

		if(PLAYER_IDLE == playerState)
		{
			vTaskDelay(10 / portTICK_PERIOD_MS);  // Block execution of this task for 10ms since we're not doing anything useful at the moment
		}
		
	}
	vTaskDelete(NULL);
}


void audioInit(void)
{
	gpio_config_t io_conf = {};

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = (1ULL << I2S_SD);
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf); // Apply configuration

	gpio_set_level(I2S_SD, 0);	// Disable amplifier

	wavSoundQueue = xQueueCreate(1, sizeof(WavSound));

	// Default dma_frame_num = 240, dma_desc_num = 6 (i2s_common.h)
	//    Total DMA size = 240 * 6 * 2 * 16 / 8 = 5760 bytes
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
	chan_cfg.dma_frame_num = I2S_NFRAMES;
	chan_cfg.dma_desc_num = I2S_NBUFFERS;
	Serial.print("dma_frame_num: ");
	Serial.println(chan_cfg.dma_frame_num);
	Serial.print("dma_desc_num: ");
	Serial.println(chan_cfg.dma_desc_num);
	i2s_new_channel(&chan_cfg, &i2s_tx_handle, NULL);

	i2s_std_config_t std_cfg = {
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
		.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
		.gpio_cfg = {
			.mclk = I2S_GPIO_UNUSED,
			.bclk = I2S_BCLK,
			.ws = I2S_LRCLK,
			.dout = I2S_DOUT,
			.din = I2S_GPIO_UNUSED,
			.invert_flags = {
				.mclk_inv = false,
				.bclk_inv = false,
				.ws_inv = false,
			},
		},
	};

	i2s_channel_init_std_mode(i2s_tx_handle, &std_cfg);

	i2s_channel_enable(i2s_tx_handle);

	i2s_chan_info_t chan_info;
	i2s_channel_get_info(i2s_tx_handle, &chan_info);
	dmaBufferSize = chan_info.total_dma_buf_size;
	Serial.print("DMA buffer size: ");
	Serial.println(dmaBufferSize);

	gpio_set_level(I2S_SD, 1);	// Enable amplifier

	xTaskCreate(audioPump, "audioPump", 8192, NULL, AUDIO_TASK_PRIORITY, &audioPumpHandle);
}


void audioTerminate(void)
{
	gpio_set_level(I2S_SD, 0);	// Disable amplifier
	killPlayer = true;
	while(killPlayer)
	{
		delay(10);  // Wait for the task to terminate
	}
	xQueueReset(wavSoundQueue);  // Empty the queue
	vQueueDelete(wavSoundQueue);  // Delete the queue
	i2s_channel_disable(i2s_tx_handle);
	i2s_del_channel(i2s_tx_handle);
}
