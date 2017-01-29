/* Retro Adapter V2
 * 
 * Target: ATmega168 @ 15MHz
 *
 * Parts (c) 2009 MoJo aka Paul Qureshi
 * Parts (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v3
 */

#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */

#include "usbdrv.h"
#include "report.h"
#include "descriptors.h"
#include "hid_modes.h"

#include "direct.h"
#include "saturn.h"
#include "psx.h"
#include "famicom.h"
#include "pc-engine.h"
#include "dreamcast.h"
#include "n64gc.h"
#include "atari_driving.h"
#include "analogue.h"
#include "dualdb9.h"
#include "timers.h"
#include "amiga_mouse.h"
#include "sega_paddle.h"
#include "3do.h"
#include "cd32.h"
#include "pc-fx.h"
#include "dreamcast.h"
#include "jaguar.h"
#include "intellivision.h"

static	void*			usbDeviceDescriptorAddress;
static	int				usbDeviceDescriptorLength;
static	report_t		reportBuffer;
static	reportMouse_t	reportBufferMouse;
void*	reportBufferAddress;
uchar	reportBufferLength;
uchar	hidMode;
void*	hidReportDescriptorAddress;
int		hidReportDescriptorLength;
uchar	hidNumReports;
uchar	idleRate;
uchar	interface;

const uchar hat_lut[] PROGMEM  = { -1, 0, 4, -1, 6, 7, 5, -1, 2, 1, 3, -1, -1, -1, -1, -1 };

/* ------------------------------------------------------------------------- */

void HardwareInit()
{
	// See schmatic for connections

	DDRD	= 0b00000000;
	PORTD	= 0b11111001;	// All inputs with pull-ups except USB D+/D-

	DDRB	= 0b00000000;
	PORTB	= 0b00111111;	// All inputs with pull-ups except xtal

	DDRC	= 0b00000000;
	PORTC	= 0b00111111;	// All inputs except unused bits
}

/* ------------------------------------------------------------------------- */

// Detection of interface works by watching D+ line. If it is held high then
// serial mode is entered. USB ports hold D+ low during reset.

void DetectInterface()
{
	uchar i = 0;
	uchar detect = 0;

	DDRD &= ~(1<<2);

	while(--i){             	// USB disconnect for >250ms
		_delay_ms(1);
		detect |= PIND;
	}

	if (detect & (1<<2)) interface = IF_SERIAL;
	else interface = IF_USB;
}

/* ------------------------------------------------------------------------- */

void ReadController(uchar id)
{
	uchar	skipdb9flag = 0;	// don't read DB9 when shared lines are in use by DB15
	uchar	pcinton	= 0;
	uchar	key;

	reportBuffer.y = reportBuffer.x = reportBuffer.b1 = reportBuffer.b2 = 0;
	reportBuffer.rx = reportBuffer.ry = 0;
	reportBuffer.hat = -1;
	reportBuffer.reportid = id;

	reportBufferMouse.x = reportBufferMouse.y = reportBufferMouse.w = 0;
	reportBufferMouse.b1 = 0;
	reportBufferMouse.reportid = id;

	hidMode = HIDM_1P;
	//ReadIntellivision(&reportBuffer);

	// Check 15 pin connector
	// 2-9-10-11-12
	key = (PINC & 0b00111110) >> 1;
	switch (key)
	{
		case 0b00000:
			ReadSaturn(&reportBuffer);
			skipdb9flag = 1;
			break;

		case 0b00001:
			ReadPSX(&reportBuffer);
			skipdb9flag = 1;
			break;

		case 0b00010:
			// PC-FX
			break;

		case 0b00011:
			ReadPCE(&reportBuffer);
			skipdb9flag = 1;
			break;

		case 0b00100:
			switch (PINB & 0b110)
			{
				case 0b110:				// HH
				ReadAtariDriving(&reportBuffer);
				break;

				case 0b000:				// LL
				Read3DO(&reportBuffer);
				break;

				case 0b010:				// LH
				ReadSegaPaddle(&reportBuffer);
				break;
			}
			skipdb9flag = 1;
			break;

		case 0b00101:
			ReadCD32(&reportBuffer);
			skipdb9flag = 1;
			break;

		case 0b00110:
		case 0b00111:
			hidMode = HIDM_MOUSE;
			pcinton = 1;
			if (id == 1)
			{
				ReadAmigaMouse(&reportBufferMouse);
				skipdb9flag = 1;
			}
			break;

		case 0b01000:
		case 0b01001:
		case 0b01010:
		case 0b01011:
			ReadTimers(&reportBuffer);
			skipdb9flag = 1;
			break;

		case 0b01100:
		case 0b01101:
		case 0b01110:
		case 0b01111:
			if (id == 2)
			{
				ReadDualDB9(&reportBuffer);
				skipdb9flag = 1;
			}
			hidMode = HIDM_2P;
			break;

		case 0b10000:
		case 0b10001:
		case 0b10010:
		case 0b10011:
		case 0b11000:
		case 0b11001:
		case 0b11010:
		case 0b11011:
			ReadAnalogue(&reportBuffer, id);
			skipdb9flag = 1;
			break;
		
		case 0b10100:
		case 0b10101:
		case 0b10110:
		case 0b10111:
			ReadJaguar(&reportBuffer);
			skipdb9flag = 1;
			break;

		default:
			ReadDB15(&reportBuffer);
	}

	// Check 9 pin connector
	// 3-6
	if (!skipdb9flag)
	{
		if ((PIND & 0b1001) == 0) {
			key = 0;
			if (PIND & (1<<4)) key = 0b10;
			if (PIND & (1<<6)) key |= 0b01;
			switch (key)
			{
				case 0b00:
					break;
				
				case 0b01:
					ReadFamicom(&reportBuffer, &reportBufferMouse);
					break;

				case 0b10:
					break;

				case 0b11:
					ReadN64GC(&reportBuffer);
					break;
			}
		} else {
			ReadDB9(&reportBuffer);
		}
	}

	// Mouse mode joystick support
	if ((id == 2) & (hidMode == HIDM_MOUSE))
	{
		reportBufferMouse.b1 = reportBuffer.x;
		reportBufferMouse.x = reportBuffer.y;
		reportBufferMouse.y = reportBuffer.b1;
		reportBufferMouse.w = reportBuffer.b2;
	}

	if (!pcinton) PCICR	&= ~(1<<PCIE0);
}

/* ------------------------------------------------------------------------- */

void SetHIDMode()
{
	//uchar	i;

	switch(hidMode)
	{
		case HIDM_1P:
			usbDeviceDescriptorAddress = usbDescriptorDeviceJoystick;
			usbDeviceDescriptorLength = sizeof(usbDescriptorDeviceJoystick);
			hidReportDescriptorAddress = usbHidReportDescriptor1P;
			hidReportDescriptorLength = usbHidReportDescriptor1PLength;
			hidNumReports = 1;
			reportBufferAddress = &reportBuffer;
			reportBufferLength = sizeof(reportBuffer);
			break;
		case HIDM_2P:
			usbDeviceDescriptorAddress = usbDescriptorDeviceJoystick;
			usbDeviceDescriptorLength = sizeof(usbDescriptorDeviceJoystick);
			hidReportDescriptorAddress = usbHidReportDescriptor2P;
			hidReportDescriptorLength = usbHidReportDescriptor2PLength;
			hidNumReports = 2;
			reportBufferAddress = &reportBuffer;
			reportBufferLength = sizeof(reportBuffer);
			break;
		case HIDM_MOUSE:
			usbDeviceDescriptorAddress = usbDescriptorDeviceMouse;
			usbDeviceDescriptorLength = sizeof(usbDescriptorDeviceMouse);
			hidReportDescriptorAddress = usbHidReportDescriptorMouse;
			hidReportDescriptorLength = usbHidReportDescriptorMouseLength;
			hidNumReports = 2;
			reportBufferAddress = &reportBufferMouse;
			reportBufferLength = sizeof(reportBufferMouse);
			break;
	}
	usbDescriptorConfiguration[25] = hidReportDescriptorLength;

	cli();						// disable interrupts
    usbDeviceDisconnect();
	DDRD |= (1<<1) | (1<<2);	// USB reset
	//DDRD |= (1<<1);				// USB reset (RS232)

	_delay_ms(255);				// disconnect for >250ms
	//DetectInterface();			// USB reset / RS232 detect

    usbDeviceConnect();
	DDRD &= ~((1<<1) | (1<<2));	// clear reset
	//DDRD &= ~(1<<1);			// clear reset (RS232)
	sei();						// restart interrupts
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    /* The following requests are never used. But since they are required by
     * the specification, we implement them in this example.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){
		/* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){
			/* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
			//ReadJoystick(rq->wValue.bytes[0]);
            usbMsgPtr = reportBufferAddress; //(void *)&reportBuffer;
            return reportBufferLength; //sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];

        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

uchar	usbFunctionDescriptor(struct usbRequest *rq)
{
	if (rq->bRequest == USBRQ_GET_DESCRIPTOR)
	{
		// USB spec 9.4.3, high byte is descriptor type
		switch (rq->wValue.bytes[1])
		{
			case USBDESCR_HID_REPORT:
				usbMsgPtr = (void*)hidReportDescriptorAddress;
				return hidReportDescriptorLength;
			case USBDESCR_CONFIG:
				usbMsgPtr = (void*)usbDescriptorConfiguration;
				return sizeof(usbDescriptorConfiguration);
			case USBDESCR_DEVICE:
				usbMsgPtr = usbDeviceDescriptorAddress;
				return usbDeviceDescriptorLength;
		}
	}

	return 0;
}

/* ------------------------------------------------------------------------- */

void serialtx(uchar byte)
{
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = byte;
}

void SerialReport(void)
{
	serialtx(10);		// newline
	serialtx(hidMode);	// type of report
	serialtx(reportBuffer.reportid);
	serialtx(reportBuffer.x);
	serialtx(reportBuffer.y);
	serialtx(reportBuffer.rx);
	serialtx(reportBuffer.ry);
	serialtx(reportBuffer.hat);
	serialtx(reportBuffer.b1);
	serialtx(reportBuffer.b2);
	serialtx(27);		// esc
}

/* ------------------------------------------------------------------------- */

int main(void)
{
	uchar   i = 1;
	uchar	hidCurrentMode = 255;

	interface = IF_NONE;

	HardwareInit();
	usbInit();

	// Set up descriptor
	hidMode = HIDM_1P;
	ReadController(1);
	SetHIDMode();

	// test
	interface = IF_USB;

	if (interface == IF_USB) {
		for(;;) {                /* main event loop */
		    usbPoll();
		    if(usbInterruptIsReady()){
		        /* called after every poll of the interrupt endpoint */
				ReadController(i);
		        usbSetInterrupt(reportBufferAddress, reportBufferLength);
				i++;
				if (i > hidNumReports) i = 1;
				if (hidCurrentMode != hidMode)
				{
					SetHIDMode();
					hidCurrentMode = hidMode;
				}
		    }
		}
	}

	if (interface == IF_SERIAL) {
		// enable USART transmit
		UBRR0 = 0x000f;						// 57600 baud, 8n1
		UCSR0A = 0;
		UCSR0B = (1<<TXEN0);
		UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);

		for (;;) {
			ReadController(i);
			SerialReport();
			i++;
			if (i > hidNumReports) i = 1;
		}
	}

    return 0;
}

/* ------------------------------------------------------------------------- */
