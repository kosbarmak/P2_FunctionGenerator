/* CPE 3160
 * John Oliver
 *
 * A5 - SPI Digital Analog Converter
 * Chris Amey and Alan Kusparmakov
 *
 * Analog Discovery Reading:
 * https://drive.google.com/file/d/1ridWuwsIxUjhIY48B9w1fNhTV8Lpmowz/view?usp=sharing

 * SPI data transmission example:
 * https://drive.google.com/file/d/1ScAAHVJ-xHwXpT-e0M0DIUzHLanU-KGV/view?usp=sharing
 *
 * The header 0x3 contains dac information.
 * Then there is this message 0b111000111000. No meaning.
 *
 *
 * We then sent over the codes corresponding to 1 and 2V
 * 25% duty cycle between these values:
 * https://drive.google.com/file/d/1OmJZAfnXotZHQ4mua-e7QsWWCk10DQcZ/view?usp=sharing


 And of course the video!!!!!!!1
 https://drive.google.com/file/d/1OmJZAfnXotZHQ4mua-e7QsWWCk10DQcZ/view?usp=sharing
       
 */

#include "dac.h"

#include "stdbool.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_gpio_ex.h"
#include <stddef.h>
#include <stdint.h>
#include "init.h"
#include "keypad.h"
#include "dac.h"

// Function Prototypes
int main(void);
void handleInput(int i);

// Global Variables
int dutyPercent = 50;
WaveType currentMode = SAWTOOTH;
int currentFrequency = 100;

void dacUpdate() {
    dac_start(currentMode, currentFrequency, dutyPercent);
}


void changeToSawtooth() {
    dutyPercent = 50;
    currentMode = SAWTOOTH;
    currentFrequency = 100;

    dacUpdate();   
}

void handleInput(int i)
{
	//Set frequency (1-5)
	if (i >= 1 && i <= 5) {
		currentFrequency = i * 100;
		dac_start(currentMode, currentFrequency, dutyPercent);
	}

	//Change mode (6-9)
	//Note: what if we did range 6-9 then convert to enum?
	if (i == 6)
		dac_start(SINE, currentFrequency, dutyPercent);

	if (i == 7)
		dac_start(TRIANGLE, currentFrequency, dutyPercent);

	if (i == 8)
		dac_start(SAWTOOTH, currentFrequency, dutyPercent);

	if (i == 9)
		dac_start(SQUARE, currentFrequency, dutyPercent);

	//Special characters
	if (currentMode != SQUARE)
		return;

	if (i == zeroValue) {
		dutyPercent = 50;
		dac_start(currentMode, currentFrequency, dutyPercent);
	}

	if (i == starValue && dutyPercent > 10) {
		dutyPercent -= 10;
		dac_start(currentMode, currentFrequency, dutyPercent);
	}

	if (i == poundValue && dutyPercent < 90) {
		dutyPercent += 10;
		dac_start(currentMode, currentFrequency, dutyPercent);
	}
}

int main(void)
{
	HAL_Init();
	dac_init();
    gpio_init();
	dac_start(currentMode, currentFrequency, dutyPercent);

	int keyInput = 0;
    HAL_Delay(1000);
	while (1) {
		keyInput = get_pressed_key();

		while (keyInput == INVALID_VALUE) {
			keyInput = get_pressed_key();
			HAL_Delay(25);
			continue;
		}

        handleInput(keyInput);
	}

	while (true) {
		// SPI_send_data(0b111111111111);
		// HAL_Delay(1000);
		// SPI_send_data(0b000000000000);
		// HAL_Delay(1000);

		// SPI_send_data(0b)
	}
}
