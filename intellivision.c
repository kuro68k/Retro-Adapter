#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "report.h"
#include "intellivision.h"

const uchar iv_disc_bitmap[] PROGMEM  = {	0b00100,	// 1
											0b01100,	// 2
											0b11100,	// 3
											0b11000,	// 4
											0b10000,	// 5
											0b01001,	// 6
											0b11001,	// 7
											0b10001,	// 8
											0b00001,	// 9
											0b00011,	// 10
											0b10011,	// 11
											0b10010,	// 12
											0b00010,	// 13
											0b01100,	// 14
											0b10110,	// 15
											0b10100 };	// 16

const uchar iv_disc_x[] PROGMEM  = {	0,		// 1
										-64,	// 2
										-127,	// 3
										-127,	// 4
										-127,	// 5
										-127,	// 6
										-127,	// 7
										-64,	// 8
										0,		// 9
										64,		// 10
										127,	// 11
										127,	// 12
										127,	// 13
										127,	// 14
										127,	// 15
										64 };	// 16

const uchar iv_disc_y[] PROGMEM  = {	127,	// 1
										127,	// 2
										127,	// 3
										64,		// 4
										0,		// 5
										-64,	// 6
										-127,	// 7
										-127, 	// 8
										-127,	// 9
										-127,	// 10
										-127,	// 11
										-64,	// 12
										0,		// 13
										64,		// 14
										127,	// 15
										127 };	// 16



void ReadIntellivision(report_t *reportBuffer)
{
	uchar	temp, i, index;

	DDRB	&= 0b11100001;
	PORTB	|= 0b00111110;

	DDRC	&= 0b11000111;
	DDRC	|= 0b00000110;
	PORTC	|= 0b00111110;
	
	temp = (PINB & 0b00111110) >> 1;

	PORTC &= ~(1<<2);
	reportBuffer->b1 = PINC;

	PORTC |= (1<<2);
	PORTC &= ~(1<<1);
	reportBuffer->b2 = PINB;

	index = 0xff;
	for (i = 0; i < 16; i++) {
		if (pgm_read_byte(&iv_disc_bitmap[i]) == temp) {
			index = i;
			i = 99;
		}
	}

	if (index != 0xff) {
		reportBuffer->x = pgm_read_byte(&iv_disc_x[index]);
		reportBuffer->y = pgm_read_byte(&iv_disc_y[index]);
	}
}

