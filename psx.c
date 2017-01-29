#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "report.h"
#include "psx.h"

//#define PSXDELAY	5

#define	PSX_CLK_SHORT	4	// nominally 4
#define PSX_CLK_LONG	14	// nominally 14
#define PSX_INTERFRAME	32	// nominally 32
#define DDG_SLOWNESS	10	// speed at which shifts are sent

/*	LDRU
	0000	-
	0001	0
	0010	2
	0011	1
	0100	4
	0101	-
	0110	3
	0111	-
	1000	6
	1001	7
	1010	-
	1011	-
	1100	5
*/

const uchar psx_hat_lut[] PROGMEM  = { -1, 0, 2, 1, 4, -1, 3, -1, 6, 7, -1, -1, 5 };

/*
PB5		DAT		In
PB4		CMD		Out
PB3		ATT		Out
PB2		CLK		Out
PB1		ACK		In
*/

void ReadPSX(report_t *reportBuffer)
{
	uchar	data, id, temp = 0;
	static uchar	lastthrottle = 0;
	static uchar	lastbreak = 0;
	static uchar	connected = 0;
	static uchar	targetthrottle = 0;
	static uchar	targetbreak = 0;
	static signed char		throttlewait = 0;
	static signed char		breakwait = 0;

	DDRB	&= 0b11011101;			// See table above for I/O directions
	DDRB	|= 0b00011100;
	PORTB	|= 0b00111110;			// Pull-ups prevent floating inputs, all outputs start high

	PORTB &= ~ATT;					// Attention line low for duration of comms
	_delay_us(100); //flb

	data = PSXCommand(0x01);		// Issue start command, data is discarded
	/*
	if (!(PSXWaitACK()))			// Wait for ACK, if not received then no pad connected
	{
		PORTB |= ATT;				// ATT high again
		_delay_us(100); //flb
		return;
	}
	*/

	_delay_us(PSX_INTERFRAME);
	id = PSXCommand(0x42);			// Request controller ID

	if ((id == PSX_ID_DIGITAL) | (id == PSX_ID_A_RED) | (id == PSX_ID_A_GREEN) | (id == PSX_ID_NEGCON))
	{
		_delay_us(PSX_INTERFRAME);
		data = PSXCommand(0x00);	// expect 0x5a from controller
		if (data == 0x5a)
		{
			_delay_us(PSX_INTERFRAME);
			data = PSXCommand(0x00);

			if (connected == PSX_NOT_CON) {

				// not connected before, detect DDG controller
				if (data == 0b00001111) {
					_delay_us(PSX_INTERFRAME);
					data = PSXCommand(0x00);
					if (data == 0b11110100) connected = PSX_CON_DDG_A;
					else if (data == 0xff) connected = PSX_CON_DDG_S;
					else connected = PSX_CON_NORMAL;
					lastthrottle = 0;
					lastbreak = 0;
					targetthrottle = 0;
					targetbreak = 0;
					throttlewait = 0;
					breakwait = 0;
				} else {
					connected = PSX_CON_NORMAL;
				}

			} else {

				// connected and identified before

				// Byte 04
				if (!(data & (1<<0))) reportBuffer->b2 |= (1<<1);	// Select
				if (!(data & (1<<3))) reportBuffer->b2 |= (1<<0);	// Start

				if (connected == PSX_CON_NORMAL) {
					if (id == PSX_ID_DIGITAL)
					{
						if (!(data & (1<<4))) reportBuffer->y = -127;		// Up
						if (!(data & (1<<6))) reportBuffer->y = 127;		// Down
						if (!(data & (1<<7))) reportBuffer->x = -127;		// Left
						if (!(data & (1<<5))) reportBuffer->x = 127;		// Right
					}
					else if (id == PSX_ID_A_RED)
					{
						if (!(data & (1<<1))) reportBuffer->b2 |= (1<<2);	// L3 Left joystick
						if (!(data & (1<<2))) reportBuffer->b2 |= (1<<3);	// R3 Right joystick
						reportBuffer->hat = pgm_read_byte(&psx_hat_lut[(~(data>>4)&0x0f)]);
					}
					else if (id == PSX_ID_NEGCON)
					{
						reportBuffer->hat = pgm_read_byte(&psx_hat_lut[(~(data>>4)&0x0f)]);
					}

				} else if (connected == PSX_CON_DDG_A) {
					temp = 0;
					if (data & (1<<7)) temp |= (1<<6);
					if (data & (1<<5)) temp |= (1<<7);
				} else if (connected == PSX_CON_DDG_S) {
					temp = 0;
					if (data & (1<<7)) temp |= (1<<1);
					if (data & (1<<5)) temp |= (1<<2);
				}

				// Byte 05
				_delay_us(PSX_INTERFRAME);
				data = PSXCommand(0x00);

				if (connected == PSX_CON_NORMAL) {
					if (id != PSX_ID_NEGCON)
					{
						if (!(data & (1<<0))) reportBuffer->b1 |= (1<<4);	// L2
						if (!(data & (1<<1))) reportBuffer->b1 |= (1<<5);	// R2
						if (!(data & (1<<2))) reportBuffer->b1 |= (1<<6);	// L1
						if (!(data & (1<<3))) reportBuffer->b1 |= (1<<7);	// R1
						if (!(data & (1<<4))) reportBuffer->b1 |= (1<<0);	// /\ Triangle
						if (!(data & (1<<5))) reportBuffer->b1 |= (1<<1);	// O  Circle
						if (!(data & (1<<6))) reportBuffer->b1 |= (1<<2);	// X  Cross
						if (!(data & (1<<7))) reportBuffer->b1 |= (1<<3);	// [] Square
					}
					else // NeGcon
					{
						if (!(data & (1<<3))) reportBuffer->b1 |= (1<<7);	// R1
						if (!(data & (1<<4))) reportBuffer->b1 |= (1<<0);	// A
						if (!(data & (1<<5))) reportBuffer->b1 |= (1<<1);	// B
					}

				} else if (connected == PSX_CON_DDG_A) {
					if (data & (1<<4)) temp |= (1<<5);
					if (temp != 0b11100000) {
						reportBuffer->y = 127 - temp;
						lastthrottle = reportBuffer->y;
					} else {
						reportBuffer->y = lastthrottle;
					}

					temp = 0;
					if (data & (1<<2)) temp |= (1<<4);
					if (data & (1<<0)) temp |= (1<<5);
					if (data & (1<<3)) temp |= (1<<6);
					if (data & (1<<1)) temp |= (1<<7);
					if (temp != 0) {
						reportBuffer->x = temp - 128;
						lastbreak = reportBuffer->x;
					} else {
						reportBuffer->x = lastbreak;
					}

					if (!(data & (1<<5))) reportBuffer->b1 |= (1<<0);	// O  Circle
					if (!(data & (1<<6))) reportBuffer->b1 |= (1<<1);	// X  Cross
					if (!(data & (1<<7))) reportBuffer->b1 |= (1<<2);	// [] Square

				} else if (connected == PSX_CON_DDG_S) {
					// throttle
					if (data & (1<<4)) temp |= (1<<0);
					if (temp != 0b00000111) targetthrottle = temp;
					// throttle up
					if (throttlewait > (DDG_SLOWNESS/2)) reportBuffer->y = 127;
					if (throttlewait > 0) throttlewait--;
					if ((lastthrottle < targetthrottle) && (throttlewait == 0)) {
						lastthrottle++;
						throttlewait = DDG_SLOWNESS;
					}
					// throttle down
					if (throttlewait < -(DDG_SLOWNESS/2)) reportBuffer->y = -128;
					if (throttlewait < 0) throttlewait++;
					if ((lastthrottle > targetthrottle) && (throttlewait == 0)) {
						lastthrottle--;
						throttlewait = -DDG_SLOWNESS;
					}

					// breaks
					temp = 0;
					if (data & (1<<2)) temp |= (1<<0);
					if (data & (1<<0)) temp |= (1<<1);
					if (data & (1<<3)) temp |= (1<<2);
					if (data & (1<<1)) temp |= (1<<3);
					if (temp != 0) targetbreak = temp;
					// breaks up
					if (breakwait > (DDG_SLOWNESS/2)) reportBuffer->b1 |= (1<<3); // []
					if (breakwait > 0) breakwait--;
					if ((lastbreak < targetbreak) && (breakwait == 0)) {
						lastbreak++;
						breakwait = DDG_SLOWNESS;
					}
					// breaks down
					if (breakwait < -(DDG_SLOWNESS/2)) reportBuffer->b1 |= (1<<1); // X
					if (breakwait < 0) breakwait++;
					if ((lastbreak > targetbreak) && (breakwait == 0)) {
						lastbreak--;
						breakwait = -DDG_SLOWNESS;
					}

					if (!(data & (1<<5))) reportBuffer->b1 |= (1<<0);	// O  Circle
					if (!(data & (1<<6))) reportBuffer->b1 |= (1<<4);	// L2
					if (!(data & (1<<7))) reportBuffer->b1 |= (1<<5);	// R2
				}

				// Remaining bytes
				if (connected == PSX_CON_NORMAL) {
					if ((id == PSX_ID_A_RED) | (id == PSX_ID_A_GREEN))
					{
						_delay_us(PSX_INTERFRAME);
						data = PSXCommand(0x00);
						reportBuffer->rx = -128+(char)data;
						_delay_us(PSX_INTERFRAME);
						data = PSXCommand(0x00);
						reportBuffer->ry = -128+(char)data;
						_delay_us(PSX_INTERFRAME);
						data = PSXCommand(0x00);
						reportBuffer->x = -128+(char)data;
						_delay_us(PSX_INTERFRAME);
						data = PSXCommand(0x00);
						reportBuffer->y = -128+(char)data;
					}
					else if (id == PSX_ID_NEGCON)
					{
						_delay_us(PSX_INTERFRAME);
						data = PSXCommand(0x00);				// Steering
						reportBuffer->x = -128+(char)data;
						_delay_us(PSX_INTERFRAME);
						data = PSXCommand(0x00);				// I button
						reportBuffer->rx = 127-(char)data;
						_delay_us(PSX_INTERFRAME);
						data = PSXCommand(0x00);				// II button
						reportBuffer->ry = 127-(char)data;
						_delay_us(PSX_INTERFRAME);
						data = PSXCommand(0x00);				// L1 button
						reportBuffer->y = 127-(char)data;
					}
				}

				_delay_us(PSX_INTERFRAME);
				data = PSXCommand(data);
			}
		}
	}
	else
	{
		PORTB |= ATT;
		data = PSXCommand(id);
		connected = PSX_NOT_CON;
	}

	PORTB |= ATT;				// ATT high again
	_delay_us(100);
}

char PSXWaitACK()
{
	uchar	count = 0;

	// wait approximately 100us for ACK.
	while (count < 100)
	{
		if (!(PINB & ACK)) return 1;
		count++;
		_delay_us(1);
	}
	return 0;
}

uchar PSXCommand(uchar command)
{
	uchar i = 0;
	uchar data = 0;

	// wait up to approx 100us for ACK line to go high
	while ((PINB & ACK) == 0)
	{
		i++;
		_delay_us(1);
		if (i > 100){ return 0;		// for some reason ACK is stuck high, should never happen
		}
	}								// but testing prevents an infinite loop

	_delay_us(2);

	for (i = 0; i < 8; i++)
	{
		// set command line
		if (command & 1) PORTB |= CMD;
		else PORTB &= ~CMD;
		command >>= 1;

		PORTB &= ~CLK;				// clock falling edge

		_delay_us(PSX_CLK_SHORT);
		
		data >>= 1;
		PORTB |= CLK;				// clock rising edge

		_delay_us(5);
		if (PINB & DAT) data |= (1<<7);
		_delay_us(PSX_CLK_LONG-5);

#ifdef DEBUG
	PORTA |= (1<<3);
	PORTA &= ~(1<<3);
	if (data & (1<<7)) PORTA |= (1<<3);
	else PORTA &= ~(1<<3);
#endif
	}
	
	return data;
}
