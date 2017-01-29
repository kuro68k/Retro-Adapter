#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"

#undef DEBUG

#define MAX_TRANSFER 491
#define MAX_WRITE 9

#define READ_DELAY (10000u) /* 10 ms */

static BYTE bcnt;
static BYTE senddata[MAX_WRITE];

static enum {
  STATE_PROBE,
  STATE_GETCTRLR,
  STATE_GETKBD,
  STATE_GETMOUSE,
} maple_state = STATE_PROBE;

#define FUNC_CTRLR 1
#define FUNC_KBD   2
#define FUNC_MOUSE 4

static BYTE device_funcs=0;

#ifdef DEBUG
static void tohex(BYTE p, BYTE n)
{
  EP2FIFOBUF[p] = "0123456789ABCDEF"[n>>4];
  EP2FIFOBUF[p+1] = "0123456789ABCDEF"[n&15];
}
#endif

static void bits_to_bytes()
{
  _asm
    mov r3,_bcnt
    mov dptr,#_EP6FIFOBUF
    inc _DPS
    mov dptr,#_EP6FIFOBUF+5
   btb_loop$:
    movx a,@dptr
    orl a,#0xfd
    mov r4,a
    inc dptr
    inc dptr
    movx a,@dptr
    orl a,#0xfe
    anl a,r4
    rl a
    rl a
    mov r4,a
    inc dptr
    inc dptr
    movx a,@dptr
    orl a,#0xfd
    anl ar4,a
    inc dptr
    inc dptr
    movx a,@dptr
    orl a,#0xfe
    anl a,r4
    rl a
    rl a
    mov r4,a
    inc dptr
    inc dptr
    movx a,@dptr
    orl a,#0xfd
    anl ar4,a
    inc dptr
    inc dptr
    movx a,@dptr
    orl a,#0xfe
    anl a,r4
    rl a
    rl a
    mov r4,a
    inc dptr
    inc dptr
    movx a,@dptr
    orl a,#0xfd
    anl ar4,a
    inc dptr
    inc dptr
    movx a,@dptr
    orl a,#0xfe
    anl a,r4
    dec _DPS
    movx @dptr,a
    inc dptr
    inc _DPS
    inc dptr
    inc dptr
    djnz r3,btb_loop$
    dec _DPS
    ret
  _endasm;
}

static void maple_read(void)
{
  // Start timer and trigger GPIF read
  SYNCDELAY;
  GPIFIRQ = 0x02;
  SYNCDELAY;
  EP6GPIFTCH = MAX_TRANSFER>>8;
  SYNCDELAY;
  EP6GPIFTCL = MAX_TRANSFER&255;
  SYNCDELAY;

  T2CON = 0x00;
  TL2 = (0xffff-READ_DELAY)&0xff;
  TH2 = (0xffff-READ_DELAY)>>8;
  T2CON = 0x04;

  GPIFTRIG = 0x06;
  SYNCDELAY;
  GPIFIDLECTL = 0x0f;

  // Wait for waveform done, GINT, or timeout
  while(!((GPIFTRIG & 0x80) ||
	  (GPIFIRQ & 0x02) ||
	  (T2CON & 0x80)))
    ;

  // Stop timer and abort waveform if not done
  T2CON &= ~0x04;
  if(!(GPIFTRIG & 0x80)) {
    GPIFABORT=0xff;
    SYNCDELAY;
  }

  // Get FIFO count
  bcnt = ((MAX_TRANSFER-2)-((EP6GPIFTCH<<8)|EP6GPIFTCL))>>3;

  // Reset FIFO
  SYNCDELAY;
  FIFORESET = 6;
  SYNCDELAY;
  FIFORESET = 0;
  SYNCDELAY;

  // Process data
  if(bcnt > 0 && bcnt < 127 &&
     (EP6FIFOBUF[1]&3) == 2 &&
     (EP6FIFOBUF[3]&3) == 0) {
    bits_to_bytes();
  } else {
    bcnt = 0;
  }
}

static void maple_delay()
{
_asm
  mov r0,#25
zloop$:
  nop
  djnz r0,zloop$ 
_endasm; 
}

static void maple_write()
{
  /* This gives us ~50kbps, not 2Mbps, but hey... */
_asm
    mov r1,#_senddata
    mov r3,#0xfe
    mov r4,#0xfd
    mov a,_bcnt
    mov r5,a
    mov dptr,#_GPIFIDLECTL
    clr _EA

    ; Leadin
    mov a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    anl a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    anl a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    anl a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    anl a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    orl a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay

 send_loop$:
    mov a,@r1     ; 1cy
    mov r7,#4     ; 2cy

 send_loop_inner$:

    ; Phase 1
    rlc a         ; 1cy
    mov r2,a	  ; 1cy
    mov a,#0x0f   ; 2cy
    mov acc.0,c   ; 2cy
    mov acc.1,c   ; 2cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay

    mov a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay

    ; Phase 2
    mov a,r2      ; 1cy
    rlc a         ; 1cy
    mov r2,a	  ; 1cy
    mov a,#0x0f   ; 2cy
    mov acc.0,c   ; 2cy
    mov acc.1,c   ; 2cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay

    mov a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r2      ; 1cy

    djnz r7,send_loop_inner$  ; 3cy

    inc r1        ; 1cy
    djnz r5,send_loop$  ; 3cy

    ; Leadout
    orl a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    anl a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    anl a,r3      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay
    mov a,r4      ; 1cy
    movx @dptr,a  ; 2cy
    lcall _maple_delay

    setb _EA
    ret

  _endasm;
}

static BOOL checksum_ok(void)
{
  BYTE i, c=0;
  for(i=0; i<bcnt; i++)
    c ^= EP6FIFOBUF[i];
  return c == 0;
}

static void next_state(BYTE funcs)
{
  if(!funcs)
    funcs = device_funcs;
  if(funcs & FUNC_CTRLR)
    maple_state = STATE_GETCTRLR;
  else if(funcs & FUNC_KBD)
    maple_state = STATE_GETKBD;
  else if(funcs & FUNC_MOUSE)
    maple_state = STATE_GETMOUSE;
  else
    maple_state = STATE_PROBE;
}

static void found_device(void)
{
#ifdef DEBUG
  if(!(EP2CS & 0x08)) {
    BYTE i;
    EP2FIFOBUF[0] = 'F';
    EP2FIFOBUF[1] = ' ';
    tohex(2, EP6FIFOBUF[7]);
    tohex(4, EP6FIFOBUF[6]);
    tohex(6, EP6FIFOBUF[5]);
    tohex(8, EP6FIFOBUF[4]);
    EP2FIFOBUF[10] = ' ';
    for(i=0; i<30; i++)
      EP2FIFOBUF[11+i] = EP6FIFOBUF[(22+i)^3];
    EP2FIFOBUF[41] = '\n';
    SYNCDELAY;
    EP2BCH = 0;
    SYNCDELAY;
    EP2BCL = 42;
    SYNCDELAY;
  }
#endif
  device_funcs = 0;
  if(EP6FIFOBUF[4]&0x01) device_funcs |= FUNC_CTRLR;
  if(EP6FIFOBUF[4]&0x40) device_funcs |= FUNC_KBD;
  if(EP6FIFOBUF[5]&0x02) device_funcs |= FUNC_MOUSE;
  next_state(device_funcs);
}

static void process_controller(void)
{
    if(EP6FIFOBUF[0] != 3 || EP6FIFOBUF[4] != 0x01)
      return;
#ifdef DEBUG
    if(!(EP2CS & 0x08)) {
      EP2FIFOBUF[0] = 'C';
      EP2FIFOBUF[1] = ' ';
      tohex(2, EP6FIFOBUF[10]);
      tohex(4, EP6FIFOBUF[11]);
      EP2FIFOBUF[6] = ' ';
      tohex(7, EP6FIFOBUF[9]);
      EP2FIFOBUF[9] = ' ';
      tohex(10, EP6FIFOBUF[8]);
      EP2FIFOBUF[12] = ' ';
      tohex(13, EP6FIFOBUF[15]);
      EP2FIFOBUF[15] = ' ';
      tohex(16, EP6FIFOBUF[14]);
      EP2FIFOBUF[18] = ' ';
      tohex(19, EP6FIFOBUF[13]);
      EP2FIFOBUF[21] = ' ';
      tohex(22, EP6FIFOBUF[12]);
      EP2FIFOBUF[24] = '\n';
      SYNCDELAY;
      EP2BCH = 0;
      SYNCDELAY;
      EP2BCL = 25;
      SYNCDELAY;
    }
#endif
    if( !(EP1INCS & 0x02) ) {
      BYTE i;
      EP1INBUF[0] = 1;
      for(i=0; i<8; i++)
	EP1INBUF[i+1] = EP6FIFOBUF[8+(i^3)];
      // Invert button mask
      EP1INBUF[1] = ~EP1INBUF[1];
      EP1INBUF[2] = ~EP1INBUF[2];
      // Invert sign bit of joystick axes
      EP1INBUF[5] ^= 0x80;
      EP1INBUF[6] ^= 0x80;
      EP1INBUF[7] ^= 0x80;
      EP1INBUF[8] ^= 0x80;
      EP1INBC = 9;
    }
}

static void process_keyboard(void)
{
    if(EP6FIFOBUF[0] != 3 || EP6FIFOBUF[4] != 0x40)
      return;
#ifdef DEBUG
    if(!(EP2CS & 0x08)) {
      EP2FIFOBUF[0] = 'K';
      EP2FIFOBUF[1] = ' ';
      tohex(2, EP6FIFOBUF[11]);
      EP2FIFOBUF[4] = ' ';
      tohex(5, EP6FIFOBUF[10]);
      EP2FIFOBUF[7] = ' ';
      tohex(8, EP6FIFOBUF[9]);
      EP2FIFOBUF[10] = ' ';
      tohex(11, EP6FIFOBUF[8]);
      EP2FIFOBUF[13] = ' ';
      tohex(14, EP6FIFOBUF[15]);
      EP2FIFOBUF[16] = ' ';
      tohex(17, EP6FIFOBUF[14]);
      EP2FIFOBUF[19] = ' ';
      tohex(20, EP6FIFOBUF[13]);
      EP2FIFOBUF[22] = ' ';
      tohex(23, EP6FIFOBUF[12]);
      EP2FIFOBUF[25] = '\n';
      SYNCDELAY;
      EP2BCH = 0;
      SYNCDELAY;
      EP2BCL = 26;
      SYNCDELAY;
    }
#endif
    if( !(EP1INCS & 0x02) ) {
      BYTE i;
      EP1INBUF[0] = 2;
      for(i=0; i<8; i++)
	EP1INBUF[i+1] = EP6FIFOBUF[8+(i^3)];
      EP1INBC = 9;
    }
}

static void process_mouse(void)
{
    if(EP6FIFOBUF[0] != 6 || EP6FIFOBUF[5] != 0x02)
      return;
#ifdef DEBUG
    if(!(EP2CS & 0x08)) {
      EP2FIFOBUF[0] = 'M';
      EP2FIFOBUF[1] = ' ';
      tohex(2, EP6FIFOBUF[8]);
      tohex(4, EP6FIFOBUF[9]);
      tohex(6, EP6FIFOBUF[10]);
      tohex(8, EP6FIFOBUF[11]);
      EP2FIFOBUF[10] = ' ';
      tohex(11, EP6FIFOBUF[14]);
      tohex(13, EP6FIFOBUF[15]);
      EP2FIFOBUF[15] = ' ';
      tohex(16, EP6FIFOBUF[12]);
      tohex(18, EP6FIFOBUF[13]);
      EP2FIFOBUF[20] = ' ';
      tohex(21, EP6FIFOBUF[18]);
      tohex(23, EP6FIFOBUF[19]);
      EP2FIFOBUF[25] = ' ';
      tohex(26, EP6FIFOBUF[16]);
      tohex(28, EP6FIFOBUF[17]);
      EP2FIFOBUF[30] = ' ';
      tohex(31, EP6FIFOBUF[22]);
      tohex(33, EP6FIFOBUF[23]);
      EP2FIFOBUF[35] = ' ';
      tohex(36, EP6FIFOBUF[20]);
      tohex(38, EP6FIFOBUF[21]);
      EP2FIFOBUF[40] = ' ';
      tohex(41, EP6FIFOBUF[26]);
      tohex(43, EP6FIFOBUF[27]);
      EP2FIFOBUF[45] = ' ';
      tohex(46, EP6FIFOBUF[24]);
      tohex(48, EP6FIFOBUF[25]);
      EP2FIFOBUF[50] = '\n';
      SYNCDELAY;
      EP2BCH = 0;
      SYNCDELAY;
      EP2BCL = 51;
      SYNCDELAY;
    }
#endif
    if( !(EP1INCS & 0x02) ) {
      BYTE i;
      EP1INBUF[0] = 3;
      for(i=0; i<20; i++)
	EP1INBUF[i+1] = EP6FIFOBUF[8+(i^3)];
      // Invert low byte of button mask
      EP1INBUF[1] = ~EP1INBUF[1];
      {
	// Negate wheel value
	WORD wheel = 0x200-(EP1INBUF[9]|(EP1INBUF[10]<<8));
	EP1INBUF[9] = wheel&0xff;
	EP1INBUF[10] = wheel>>8;
      }
      EP1INBC = 21;
    }
}

static BOOL get_condition(WORD func)
{
  senddata[0] = 0x01;		// additional words
  senddata[1] = 0x00;		// sender
  senddata[2] = 0x20;		// recipient
  senddata[3] = 0x09;		// command
  
  senddata[4] = func&0xff;	// function
  senddata[5] = func>>8;	// function
  senddata[6] = 0x00;
  senddata[7] = 0x00;
  
  senddata[8] = 0x28^(func>>8)^(func&0xff);	// checksum
  
  bcnt = 9;
  maple_write();
  maple_read();
  if(bcnt >= 9 && bcnt == (EP6FIFOBUF[0]<<2)+5 && checksum_ok() &&
     EP6FIFOBUF[1] == 0x20 && EP6FIFOBUF[2] == 0x00 && EP6FIFOBUF[3] == 0x08)
    return TRUE;
  else {
#ifdef DEBUG
    if(!(EP2CS & 0x08)) {
      EP2FIFOBUF[0] = 'X';
      EP2FIFOBUF[1] = '\n';
      SYNCDELAY;
      EP2BCH = 0;
      SYNCDELAY;
      EP2BCL = 2;
      SYNCDELAY;
    }
#endif
    maple_state = STATE_PROBE;
    return FALSE;
  }
}

void maple_work(void)
{
  switch(maple_state) {
  case STATE_PROBE:
    senddata[0] = 0x00;
    senddata[1] = 0x00;
    senddata[2] = 0x20;
    senddata[3] = 0x01;
    senddata[4] = 0x21;
    bcnt = 5;
    maple_write();
    maple_read();
    if(bcnt >= 52 && EP6FIFOBUF[0] == 0x1c &&
       EP6FIFOBUF[1] == 0x20 && EP6FIFOBUF[2] == 0x00 && EP6FIFOBUF[3] == 0x05)
      found_device();
    break;
  case STATE_GETCTRLR:
    if(get_condition(0x001)) {
      next_state(device_funcs & ~FUNC_CTRLR);
      process_controller();
    }
    break;
  case STATE_GETKBD:
    if(get_condition(0x040)) {
      next_state(device_funcs & ~(FUNC_CTRLR|FUNC_KBD));
      process_keyboard();
    }
    break;
  case STATE_GETMOUSE:
    if(get_condition(0x200)) {
      next_state(device_funcs & ~(FUNC_CTRLR|FUNC_KBD|FUNC_MOUSE));
      process_mouse();
    }
    break;
  }
}
