// Atari Jaguar

#include <avr/io.h>
#include <util/delay.h>
#include "report.h"
#include "jaguar.h"

void ReadJaguar(report_t *reportBuffer)
{
	DDRB	|= JAG_COL1|JAG_COL2|JAG_COL3|JAG_COL4;		// Column select outputs
	DDRB	&= ~(JAG_ROW1);								// Row input
	PORTB	|= JAG_COL2|JAG_COL3|JAG_COL4|JAG_ROW1;		// Start COL1 low, pull-up on row
	PORTB	&= ~(JAG_COL1);

	DDRC	&= ~(JAG_ROW2|JAG_ROW3|JAG_ROW4);			// Row inputs
	PORTC	|= JAG_ROW2|JAG_ROW3|JAG_ROW4;				// Row pull-ups

	DDRD	&= ~(JAG_ROW5|JAG_ROW6);					// Row inputs
	PORTD	|= JAG_ROW5|JAG_ROW6;						// Row pull-ups

	// Column 1
	if (!(PINC & JAG_ROW2)) reportBuffer->b2 |= (1<<1);		// Option
	if (!(PINC & JAG_ROW3)) reportBuffer->b1 |= (1<<5);		// 3
	if (!(PINC & JAG_ROW4)) reportBuffer->b2 |= (1<<2);		// 6
	if (!(PIND & JAG_ROW5)) reportBuffer->b2 |= (1<<5);		// 9
	if (!(PIND & JAG_ROW6)) reportBuffer->ry |= -128;		// #

	// Column 2
	PORTB |= JAG_COL1;
	PORTB &= ~(JAG_COL2);
	asm("nop");
	asm("nop");
	if (!(PINC & JAG_ROW2)) reportBuffer->b1 |= (1<<2);		// C
	if (!(PINC & JAG_ROW3)) reportBuffer->b1 |= (1<<4);		// 2
	if (!(PINC & JAG_ROW4)) reportBuffer->b1 |= (1<<7);		// 5
	if (!(PIND & JAG_ROW5)) reportBuffer->b2 |= (1<<4);		// 8
	if (!(PIND & JAG_ROW6)) reportBuffer->b2 |= (1<<7);		// 0

	// Column 3
	PORTB |= JAG_COL2;
	PORTB &= ~(JAG_COL3);
	asm("nop");
	asm("nop");
	if (!(PINC & JAG_ROW2)) reportBuffer->b1 |= (1<<1);		// B
	if (!(PINC & JAG_ROW3)) reportBuffer->b1 |= (1<<3);		// 1
	if (!(PINC & JAG_ROW4)) reportBuffer->b1 |= (1<<6);		// 4
	if (!(PIND & JAG_ROW5)) reportBuffer->b2 |= (1<<3);		// 7
	if (!(PIND & JAG_ROW6)) reportBuffer->b2 |= (1<<6);		// *

	// Column 4
	PORTB |= JAG_COL3;
	PORTB &= ~(JAG_COL4);
	asm("nop");
	asm("nop");
	if (!(PINB & JAG_ROW1)) reportBuffer->b2 |= (1<<0);		// Pause
	if (!(PINC & JAG_ROW2)) reportBuffer->b1 |= (1<<0);		// A
	if (!(PINC & JAG_ROW3)) reportBuffer->x = 127;			// Right
	if (!(PINC & JAG_ROW4)) reportBuffer->x = -128;			// Left
	if (!(PIND & JAG_ROW5)) reportBuffer->y = 127;			// Down
	if (!(PIND & JAG_ROW6)) reportBuffer->y = -128;			// Up
}
