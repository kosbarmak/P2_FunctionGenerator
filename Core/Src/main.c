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

#include "stdbool.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_gpio_ex.h"
#include <stddef.h>
#include <stdint.h>


int main()
{
	HAL_Init();

	while (true) {
		// SPI_send_data(0b111111111111);
		// HAL_Delay(1000);
		// SPI_send_data(0b000000000000);
		// HAL_Delay(1000);

		// SPI_send_data(0b)
	}
}
