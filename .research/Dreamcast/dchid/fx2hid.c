#ifndef SDCC
#pragma NOIV               // Do not generate interrupt vectors
#endif
/*
-----------------------------------------------------------------------------
   File:      usbhidio_fx2.c
   Contents:   Hooks required to implement USB peripheral function.

   This is an adaptation of the Cypress example FX2_Hid_Keyboard.c,
   which was in turn adapted from Cypress's bulkloop.c 
   (and is available on request from Cypress).

   The application communicates with the usbhdio host applications available from:
 
   www.Lvr.com/hidpage.htm

   This code requires the full (not evaluation) version of the Keil compiler.
   Additions to the keyboard code are labeled with "usbhidio start" and "usbhidio end"
   Unneeded keyboard code is commented out.
  
   Copyright (c) 2000 Cypress Semiconductor All rights reserved
   with some changes by Jan Axelson (jan@Lvr.com) 
-----------------------------------------------------------------------------
*/

#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"            // SYNCDELAY macro

extern BOOL GotSUD;             // Received setup data flag
extern BOOL Sleep;
extern BOOL Rwuen;
extern BOOL Selfpwr;

#define	min(a,b) (((a)<(b))?(a):(b))

#define GD_HID	0x21
#define GD_REPORT	0x22
#define CR_SET_REPORT 0x09
#define HID_OUTPUT_REPORT 2

#define BTN_ADDR		0x41
#define LED_ADDR		0x42

#define PF_IDLE			0
#define PF_GETKEYS		1

#define KEY_WAKEUP		0
#define KEY_F1			1
#define KEY_F2			2
#define KEY_F3			3

WORD	pHIDDscr;
WORD	pReportDscr;
WORD	pReportDscrEnd;
extern code HIDDscr;
extern code REPORTDSCR ReportDscr; 
extern code ReportDscrEnd;

BYTE Configuration;             // Current configuration
BYTE AlternateSetting;          // Alternate settings

BYTE	Configuration;		// Current configuration
BYTE	AlternateSetting;	// Alternate settings

BYTE buttons;
BYTE oldbuttons;
BYTE leds = 0xFF;
BYTE timestamp;

BYTE read_buttons (void);
void write_leds (BYTE d);

#define VR_NAKALL_ON    0xD0
#define VR_NAKALL_OFF   0xD1
//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------

// read_buttons and write_leds are unused by usbhidio
/*
BYTE read_buttons (void)
{
	BYTE d;

	while (I2CS & 0x40);	//Wait for stop to be done
	I2CS = 0x80;			//Set start condition
	I2DAT = BTN_ADDR;		//Write button address
	while (!(I2CS & 0x01));	//Wait for done
	I2CS = 0x20;			//Set last read
	d = I2DAT;				//Dummy read
	while (!(I2CS & 0x01));	//Wait for done
	I2CS = 0x40;			//Set stop bit
	return(I2DAT);			//Read the data
}


void write_leds (BYTE d)
{
	while (I2CS & 0x40);	//Wait for stop to be done
	I2CS = 0x80;			//Set start condition
	I2DAT = LED_ADDR;		//Write led address
	while (!(I2CS & 0x01));	//Wait for done
	I2DAT = d;				//Write data
	while (!(I2CS & 0x01));	//Wait for done
	I2CS = 0x40;			//Set stop bit
}
*/

static const BYTE wfdata[] = {
  0x01, /* L/B[0] */ /* 1:->0, 0:->1 */
  0x11, /* L/B[1] */ /* 1:->2, 0:->1 */
  0x2b, /* L/B[2] */ /* 1:->5, 0:->3 */
  0x1c, /* L/B[3] */ /* 1:->3, 0:->4 */
  0x3c, /* L/B[4] */ /* 1:->7, 0:->4 */
  0x2d, /* L/B[5] */
  0x00, /* L/B[6] */
  0x00, /* Resrvd */
  0x01, /* OPC[0] */
  0x03, /* OPC[1] */ /* sample */
  0x01, /* OPC[2] */
  0x01, /* OPC[3] */
  0x03, /* OPC[4] */ /* sample */
  0x11, /* OPC[5] */ /* GINT */
  0x00, /* OPC[6] */
  0x00, /* Resrvd */
  0x0f, /* OUT[0] */
  0x0f, /* OUT[1] */
  0x0f, /* OUT[2] */
  0x0f, /* OUT[3] */
  0x0f, /* OUT[4] */
  0x0f, /* OUT[5] */
  0x0f, /* OUT[6] */
  0x00, /* Resrvd */
  0x00, /* LOG[0] */ /* RDY0 & RDY0 */
  0x09, /* LOG[1] */ /* RDY1 & RDY1 */
  0x00, /* LOG[2] */ /* RDY0 & RDY0 */
  0x09, /* LOG[3] */ /* RDY1 & RDY1 */
  0x00, /* LOG[4] */ /* RDY0 & RDY0 */
  0x00, /* LOG[5] */
  0x00, /* LOG[6] */
  0x00, /* Resrvd */
};

void TD_Init(void)             // Called once at startup
{
  BYTE i;

   // set the CPU clock to 48MHz
   CPUCS = ((CPUCS & ~bmCLKSPD) | bmCLKSPD1) ;

   // set the slave FIFO interface to 48MHz
   IFCONFIG |= 0x40;

   // Set port B and D to general purpose input, for now.
   OEB = 0;
   OED = 0;
   IFCONFIG &= ~0x03;

   // Configure open drain for GPIF
   GPIFIDLECTL = 0x0f;
   GPIFCTLCFG = 0x0f;

  // Registers which require a synchronization delay, see section 15.14
  // FIFORESET        FIFOPINPOLAR
  // INPKTEND         OUTPKTEND
  // EPxBCH:L         REVCTL
  // GPIFTCB3         GPIFTCB2
  // GPIFTCB1         GPIFTCB0
  // EPxFIFOPFH:L     EPxAUTOINLENH:L
  // EPxFIFOCFG       EPxGPIFFLGSEL
  // PINFLAGSxx       EPxFIFOIRQ
  // EPxFIFOIE        GPIFIRQ
  // GPIFIE           GPIFADRH:L
  // UDMACRCH:L       EPxGPIFTRIG
  // GPIFTRIG
  
  // Note: The pre-REVE EPxGPIFTCH/L register are affected, as well...
  //      ...these have been replaced by GPIFTC[B3:B0] registers

  // default: all endpoints have their VALID bit set
  // default: TYPE1 = 1 and TYPE0 = 0 --> BULK  
  // default: EP2 and EP4 DIR bits are 0 (OUT direction)
  // default: EP6 and EP8 DIR bits are 1 (IN direction)
  // default: EP2, EP4, EP6, and EP8 are double buffered

  // we are just using the default values, yes this is not necessary...
  EP1OUTCFG = 0xA0;
  EP1INCFG = 0xA0;
  SYNCDELAY;                    // see TRM section 15.14
  EP2CFG = 0xe2;
  SYNCDELAY;                    
  EP4CFG = 0x20;
  SYNCDELAY;                    
  EP6CFG = 0x6a;
  SYNCDELAY;                    
  EP8CFG = 0x60;

  SYNCDELAY;
  EP2FIFOCFG = 0x04;
  SYNCDELAY;
  EP6FIFOCFG = 0x05;
  SYNCDELAY;                    

  // out endpoints do not come up armed
  
  // enable dual autopointer feature
  AUTOPTRSETUP |= 0x01;

  // Configure GPIF
  GPIFREADYCFG = 0x20;

  for(i=0; i!=sizeof(wfdata); i++)
    (&GPIF_WAVE_DATA)[i] = wfdata[i];

  IFCONFIG = 0xc2;

  Rwuen = TRUE;                 // Enable remote-wakeup

}


void TD_Poll(void)              // Called repeatedly while the device is idle
{
	if( !(EP1OUTCS & 0x02) )	// Is there something available in EP1OUTBUF
	{
	  EP1OUTBC = 0;				//Rearm the OUT endpoint buffer to enable receiving a report.	  
	}

	if( ((BYTE)(USBFRAMEL-timestamp))>10 && !(EP1INCS & 0x02) )
	  {
	    maple_work();
	    timestamp = USBFRAMEL;
	  }
}

BOOL TD_Suspend(void)          // Called before the device goes into suspend mode
{
   return(TRUE);
}

BOOL TD_Resume(void)          // Called after the device resumes
{
   return(TRUE);
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------

BOOL DR_GetDescriptor(void)
{
		return(TRUE);
}

BOOL DR_SetConfiguration(void)   // Called when a Set Configuration command is received
{
   Configuration = SETUPDAT[2];
   return(TRUE);            // Handled by user code
}

BOOL DR_GetConfiguration(void)   // Called when a Get Configuration command is received
{
   EP0BUF[0] = Configuration;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);            // Handled by user code
}

BOOL DR_SetInterface(void)       // Called when a Set Interface command is received
{
   AlternateSetting = SETUPDAT[2];
   return(TRUE);            // Handled by user code
}

BOOL DR_GetInterface(void)       // Called when a Set Interface command is received
{
   EP0BUF[0] = AlternateSetting;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);            // Handled by user code
}

BOOL DR_GetStatus(void)
{
   return(TRUE);
}

BOOL DR_ClearFeature(void)
{
   return(TRUE);
}

BOOL DR_SetFeature(void)
{
   return(TRUE);
}

BOOL DR_VendorCmnd(void)
{
  BYTE tmp;
  
  switch (SETUPDAT[1])
  {
     case VR_NAKALL_ON:
        tmp = FIFORESET;
        tmp |= bmNAKALL;      
        SYNCDELAY;                    
        FIFORESET = tmp;
        break;
     case VR_NAKALL_OFF:
        tmp = FIFORESET;
        tmp &= ~bmNAKALL;      
        SYNCDELAY;                    
        FIFORESET = tmp;
        break;
     default:
        return(TRUE);
  }

  return(FALSE);
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 13
#endif
{
   GotSUD = TRUE;            // Set flag
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUDAV;         // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 15
#endif
{
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUTOK;         // Clear SUTOK IRQ
}

void ISR_Sof(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 14
#endif
{
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSOF;            // Clear SOF IRQ
}

void ISR_Ures(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 17
#endif
{
   // whenever we get a USB reset, we should revert to full speed mode
   pConfigDscr = pFullSpeedConfigDscr;
   ((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
   pOtherConfigDscr = pHighSpeedConfigDscr;
   ((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;

   EZUSB_IRQ_CLEAR();
   USBIRQ = bmURES;         // Clear URES IRQ
}

void ISR_Susp(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 16
#endif
{
   Sleep = TRUE;
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUSP;
}

void ISR_Highspeed(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 18
#endif
{
   if (EZUSB_HIGHSPEED())
   {
      pConfigDscr = pHighSpeedConfigDscr;
      ((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
      pOtherConfigDscr = pFullSpeedConfigDscr;
      ((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;
   }

   EZUSB_IRQ_CLEAR();
   USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 19
#endif
{
}
void ISR_Stub(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 20
#endif
{
}
void ISR_Ep0in(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 21
#endif
{
}
void ISR_Ep0out(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 22
#endif
{
}
void ISR_Ep1in(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 23
#endif
{
}
void ISR_Ep1out(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 24
#endif
{
}
void ISR_Ep2inout(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 25
#endif
{
}
void ISR_Ep4inout(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 26
#endif
{
}
void ISR_Ep6inout(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 27
#endif
{
}
void ISR_Ep8inout(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 28
#endif
{
}
void ISR_Ibn(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 29
#endif
{
}
void ISR_Ep0pingnak(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 30
#endif
{
}
void ISR_Ep1pingnak(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 31
#endif
{
}
void ISR_Ep2pingnak(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 32
#endif
{
}
void ISR_Ep4pingnak(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 33
#endif
{
}
void ISR_Ep6pingnak(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 34
#endif
{
}
void ISR_Ep8pingnak(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 35
#endif
{
}
void ISR_Errorlimit(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 36
#endif
{
}
void ISR_Ep2piderror(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 37
#endif
{
}
void ISR_Ep4piderror(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 38
#endif
{
}
void ISR_Ep6piderror(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 39
#endif
{
}
void ISR_Ep8piderror(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 40
#endif
{
}
void ISR_Ep2pflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 41
#endif
{
}
void ISR_Ep4pflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 42
#endif
{
}
void ISR_Ep6pflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 43
#endif
{
}
void ISR_Ep8pflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 44
#endif
{
}
void ISR_Ep2eflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 45
#endif
{
}
void ISR_Ep4eflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 46
#endif
{
}
void ISR_Ep6eflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 47
#endif
{
}
void ISR_Ep8eflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 48
#endif
{
}
void ISR_Ep2fflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 49
#endif
{
}
void ISR_Ep4fflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 50
#endif
{
}
void ISR_Ep6fflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 51
#endif
{
}
void ISR_Ep8fflag(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 52
#endif
{
}
void ISR_GpifComplete(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 53
#endif
{
}
void ISR_GpifWaveform(void)
#ifndef SDCC
 interrupt 0
#else
 interrupt 54
#endif
{
}

