; dscr.asm
; Contains the Device Descriptor, Configuration(Interface, HID and Endpoint) Descriptor,
; and String descriptors.
;

debugep = 0
	

DSCR_DEVICE   	 =   1   			;; Descriptor type: Device
DSCR_CONFIG  	 =   2   			;; Descriptor type: Configuration
DSCR_STRING   	 =   3  		 	;; Descriptor type: String
DSCR_INTRFC   	 =   4   			;; Descriptor type: Interface
DSCR_ENDPNT   	 =   5   			;; Descriptor type: Endpoint
DSCR_DEVQUAL  	 =   6   			;; Descriptor type: Device Qualifier
DSCR_OTHERSPEED  =   7 


ET_CONTROL   =   0   				;; Endpoint type: Control
ET_ISO       =   1   				;; Endpoint type: Isochronous
ET_BULK      =   2   				;; Endpoint type: Bulk
ET_INT       =   3   				;; Endpoint type: Interrupt

.globl	_DeviceDscr,_ConfigDscr,_StringDscr,_HIDDscr,_ReportDscr,_ReportDscrEnd,_StringDscr0, _StringDscr1, _StringDscr2
.globl  _HighSpeedConfigDscr,_FullSpeedConfigDscr,_DeviceQualDscr, _UserDscr
 
VID	=	0x0547
PID	=	0x1005
DID 	=	0x0000

;cseg at 0x90
.area DSCR  (CODE)
	
;;-----------------------------------------------------------------------------
;; Global Variables
;;-----------------------------------------------------------------------------
	.area DSCR      ;; locate the descriptor table in on-part memory.
	
_DeviceDscr:	
	.db	DeviceDscrEnd - _DeviceDscr	; Descriptor length
	.db	DSCR_DEVICE			; Descriptor type = DEVICE
	.db	0x00,0x02			; spec version (BCD) is 2.00               
	.db	0,0,0				; HID class is defined in the interface descriptor
	.db	64				; maxPacketSize
	.db	<(VID),>(VID)
	.db	<(PID),>(PID)
	.db	<(DID),>(DID)
	.db  	 1        			; Manufacturer string index
      	.db  	 2         			; Product string index
      	.db  	 0         			; Serial number string index
      	.db  	 1         			; Number of configurations
DeviceDscrEnd:

_DeviceQualDscr:
    .db  DeviceQualDscrEnd - _DeviceQualDscr    ;Descriptor Length
    .db  DSCR_DEVQUAL        			;Descriptor Type
    .db  0x00,0x02            			;spec version (BCD) is 2.00
    .db  0,0,0               			;Device class, sub-class, and sub-sub-class
    .db  0x04                 			;Max Packet Size
    .db  1                  		 	;Number of configurations
    .db  0                   			;Reserved
DeviceQualDscrEnd:

_HighSpeedConfigDscr:
_ConfigDscr:
	.db	ConfigDscrEnd - _ConfigDscr	; Descriptor length
        .db	DSCR_CONFIG			; Descriptor type = CONFIG
	.db	<(HS_End-_ConfigDscr)		; total length (conf+interface+HID+EP's)
	.db	>(HS_End-_ConfigDscr)
.if debugep
	.db	0x02				; number of interfaces
.else
	.db	0x01				; number of interfaces
.endif
	.db	0x01				; value to select this interface
	.db	0x03				; string index to describe this config
	.db	0b10000000			; b7=1; b6=self-powered; b5=Remote WU
	.db	0d40				; bus power = 80 ma
ConfigDscrEnd:

_IntrfcDscr:		; Interface Descriptor
	.db	IntrfcDscrEnd -  _IntrfcDscr	; Descriptor length
	.db	DSCR_INTRFC			; Descriptor type = INTERFACE
	.db	0,0				; Interface 0, Alternate setting 0
	.db	0x02				; number of endpoints
	.db	0x03,0,0			; class(03)HID, no subclass or protocol
	.db	0x0				; string index for this interface
IntrfcDscrEnd:

_HIDDscr:
	.db	HIDDscrEnd - _HIDDscr		; Descriptor length
	.db	0x21				; Descriptor type - HID
	.db	0x10,0x01			; HID Spec version 1.10
	.db	0				; country code(none)
	.db	0x01				; number of HID class descriptors
	.db	0x22				; class descriptor type: REPORT
	.db	<(_ReportDscrEnd - _ReportDscr)
        .db	>(_ReportDscrEnd - _ReportDscr)
HIDDscrEnd:

_EpInDscr:	; I-0, AS-0 first endpoint descriptor (EP1IN)
	.db	EpInDscrEnd - _EpInDscr		; Descriptor length
	.db	DSCR_ENDPNT			; Descriptor type = ENDPOINT
	.db	0x81				; IN-1
	.db	0x03				; Type: INTERRUPT
	.db	0d64,0				; MaxPacketSize = 64
	.db	0x05				; polling interval is 2^(5-1) = 16 mSec
EpInDscrEnd:

_EpOutDscr:
	.db	EpOutDscrEnd - _EpOutDscr	; Descriptor length
	.db	DSCR_ENDPNT			; Descriptor type = ENDPOINT
	.db	0x01				; OUT-1
	.db	0x03				; Type ; INTERRUPT
	.db	0d64,0				; MaxPacketSize = 64
	.db	0x05				; polling interval is 2^(5-1) = 16 mSec
EpOutDscrEnd:	

.if debugep
_IntrfcDscr2:		; Interface Descriptor
	.db	IntrfcDscrEnd2 -  _IntrfcDscr2	; Descriptor length
	.db	DSCR_INTRFC			; Descriptor type = INTERFACE
	.db	1,0				; Interface 1, Alternate setting 0
	.db	0x01				; number of endpoints
	.db	0xff,0,0			; class(03)HID, no subclass or protocol
	.db	0x0				; string index for this interface
IntrfcDscrEnd2:

_EpOutDscr2:
	.db	EpOutDscrEnd2 - _EpOutDscr2	; Descriptor length
	.db	DSCR_ENDPNT			; Descriptor type = ENDPOINT
	.db	0x82				; IN-2
	.db	0x02				; Type ; BULK
	.db	0d64,0				; MaxPacketSize = 64
	.db	0x00				; polling interval
EpOutDscrEnd2:	
.endif

HS_End:

    .db  0x00         ; Word alignment

_FullSpeedConfigDscr:
	.db	FullSpeedConfigDscrEnd - _FullSpeedConfigDscr		; Descriptor length
	.db	DSCR_OTHERSPEED			; Descriptor type = OTHER SPEED CONFIG
	.db	<(FS_End-_FullSpeedConfigDscr)	; Total length (conf+interface+HID+EP's)
	.db	>(FS_End-_FullSpeedConfigDscr)
	.db	0x01				; Number of interfaces
	.db	0x01				; Value to select this interface
	.db	0x03				; String index to describe this config
	.db	0b10100000			; b7=1; b6=self-powered; b5=Remote WU
	.db	0d0				; bus power = 80 ma
FullSpeedConfigDscrEnd:
 
_FullSpeedIntrfcDscr:				; Interface Descriptor
	.db	FullSpeedIntrfcDscrEnd -  _FullSpeedIntrfcDscr	; Descriptor length
	.db	DSCR_INTRFC			; Descriptor type: INTERFACE
	.db	0,0				; Interface 0, Alternate setting 0
	.db	0x02				; Number of endpoints
	.db	0x03,0,0			; Class(03)HID, no subclass or protocol
	.db	0x0				; string index for this interface
FullSpeedIntrfcDscrEnd:


_FullSpeedHIDDscr:
	.db	FullSpeedHIDDscrEnd -_FullSpeedHIDDscr	; Descriptor length
	.db	0x21				; Descriptor type - HID
	.db	0x10,0x01			; HID Spec version 1.10
	.db	0				; country code(none)
	.db	0x01				; number of HID class descriptors
	.db	0x22				; class descriptor type: REPORT
	.db	<(_ReportDscrEnd - _ReportDscr)
	.db	>(_ReportDscrEnd - _ReportDscr)
FullSpeedHIDDscrEnd:

_FSEpInDscr:	
	.db	FSEpInDscrEnd - _FSEpInDscr	; Descriptor length
	.db	DSCR_ENDPNT			; Descriptor type : ENDPOINT
	.db	0x81				; IN-1
	.db	ET_INT				; Type: INTERRUPT
	.db	0x40,0				; maxPacketSize = 64
	.db	0x01				; polling interval is 50 msec
FSEpInDscrEnd:

_FSEpOutDscr:	
	.db	FSEpOutDscrEnd - _FSEpOutDscr	; Descriptor length
	.db	DSCR_ENDPNT			; Descriptor type = ENDPOINT
	.db	0x01				; OUT-1
	.db	ET_INT				; type - INTERRUPT
	.db	0x40,0				; maxPacketSize = 12
	.db	0x01				; polling interval is 50 msec
FSEpOutDscrEnd:
FS_End:

    	.db  0x0         ;Word alignment


; HID report descriptor
_ReportDscr:
	.db 0x85, 0x01  ; Report ID (1)
	.db 0x05, 0x01  ; Usage Page (Generic Desktop)
	.db 0x09, 0x05  ; Usage (Game Pad)
	.db 0x0A1, 0x01 ; Collection (Application)
	.db 0x05, 0x05  ;   Usage Page (Game Controls)
	.db 0x09, 0x37  ;   Usage (Game Pad Fire/Jump)
	.db 0xa1, 0x02  ;   Collection (Logical)	
	.db 0x15, 0x00	;	Logical minimum (0)
	.db 0x25, 0x01	;	Logical maximum (1)
	.db 0x05, 0x09  ;       Usage Page (Buttons)
	.db 0x09, 0x03  ;       Usage (3)
	.db 0x09, 0x02  ;       Usage (2)
	.db 0x09, 0x01  ;       Usage (1)
	.db 0x09, 0x0c  ;       Usage (12)
	.db 0x09, 0x09  ;       Usage (9)
	.db 0x09, 0x0a  ;       Usage (10)
	.db 0x09, 0x0e  ;       Usage (14)
	.db 0x09, 0x0f  ;       Usage (15)
	.db 0x09, 0x06  ;       Usage (6)
	.db 0x09, 0x05  ;       Usage (5)
	.db 0x09, 0x04  ;       Usage (4)
	.db 0x09, 0x0b  ;       Usage (11)
	.db 0x09, 0x19  ;       Usage (25)
	.db 0x09, 0x1a  ;       Usage (26)
	.db 0x09, 0x1e  ;       Usage (30)
	.db 0x09, 0x1f  ;       Usage (31)
	.db 0x75, 0x01	;	Report size (1)
	.db 0x95, 0x10	;	Report count (16)
	.db 0x81, 0x02	;	Input (data, variable, absolute)
	.db 0x0C0       ;   End Collection
	.db 0x05, 0x05  ;   Usage Page (Game Controls)
	.db 0x09, 0x39  ;   Usage (Game Pad Trigger)
	.db 0xa1, 0x02  ;   Collection (Logical)	
	.db 0x05, 0x09  ;       Usage Page (Buttons)
	.db 0x09, 0x08  ;       Usage (8)
	.db 0x09, 0x07  ;       Usage (7)
	.db 0x15, 0x00	;	Logical minimum (0)
	.db 0x26, 0xff,0x00;	Logical maximum (255)
	.db 0x75, 0x08	;	Report size (8)
	.db 0x95, 0x02	;	Report count (2)
	.db 0x81, 0x02	;	Input (data, variable, absolute)
	.db 0x0C0       ;   End Collection
	.db 0x05, 0x01  ;   Usage Page (Generic Desktop)
	.db 0x09, 0x01  ;   Usage (Pointer)
	.db 0xA1, 0x00	;   Collection (Physical)
	.db 0x09, 0x30  ;       Usage (X)
	.db 0x09, 0x31  ;       Usage (Y)
	.db 0x09, 0x32  ;       Usage (Z)
	.db 0x09, 0x35  ;       Usage (RotZ)
	.db 0x15, 0x81	;	Logical minimum (-127)
	.db 0x25, 0x7f  ;	Logical maximum (127)
	.db 0x75, 0x08	;	Report size (8)
	.db 0x95, 0x04	;	Report count (4)
	.db 0x81, 0x02	;	Input (data, variable, absolute)
	.db 0x0C0       ;   End Collection
	.db 0x0C0       ; End Collection
	
	.db 0x85, 0x02  ; Report ID (2)
	.db 0x05, 0x01  ; Usage Page (Generic Desktop)
	.db 0x09, 0x06  ; Usage (Keyboard)
	.db 0x0A1, 0x01 ; Collection (Application)
	.db 0x05, 0x07  ;       Usage Page (Key codes)
	.db 0x19, 0x0E0	;	Usage minimum (224)
	.db 0x29, 0x0E7	;	Usage maximum (231)
	.db 0x15, 0x00	;	Logical minimum (0)
	.db 0x25, 0x01	;	Logical maximum (1)
	.db 0x75, 0x01	;	Report size (1)
	.db 0x95, 0x08	;	Report count (8)
	.db 0x81, 0x02	;	Input (data, variable, absolute)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x75, 0x08	;	Report size (8)
	.db 0x81, 0x01	;	Input (constant)
	.db 0x95, 0x05	;	Report count (5)
	.db 0x75, 0x01	;	Report size (1)
	.db 0x05, 0x08	;	Usage Page (LED)
	.db 0x19, 0x01	;	Usage minimum (1)
	.db 0x29, 0x05	;	Usage maximum (5)
	.db 0x91, 0x02	;	Output (data, variable, absolute)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x75, 0x03	;	Report size (3)
	.db 0x91, 0x01	;	Output (constant)
	.db 0x95, 0x06	;	Report count (6)
	.db 0x75, 0x08	;	Report size (8)
	.db 0x15, 0x00	;	Logical minimum (0)
	.db 0x25, 0xff	;	Logical maximum (255)
	.db 0x05, 0x07	;	Usage page (key codes)
	.db 0x19, 0x00	;	Usage minimum (0)
	.db 0x29, 0xff	;	Usage maximum (255)
 	.db 0x81, 0x00	;	Input (data, array)
	.db 0x0C0          ; End Collection

	.db 0x85, 0x03  ; Report ID (3)
	.db 0x05, 0x01  ; Usage Page (Generic Desktop)
	.db 0x09, 0x02  ; Usage (Mouse)
	.db 0x0A1, 0x01 ; Collection (Application)
	.db 0x09, 0x01  ;   Usage (Pointer)
	.db 0xA1, 0x00	;   Collection (Physical)
	.db 0x05, 0x09  ;       Usage Page (Buttons)
	.db 0x09, 0x03  ;       Usage (3)
	.db 0x09, 0x02  ;       Usage (2)
	.db 0x09, 0x01  ;       Usage (1)
	.db 0x09, 0x04  ;       Usage (4)
	.db 0x15, 0x00	;	Logical minimum (0)
	.db 0x25, 0x01	;	Logical maximum (1)
	.db 0x95, 0x04	;	Report count (4)
	.db 0x75, 0x01	;	Report size (1)
	.db 0x81, 0x02	;	Input (data, variable, absolute)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x75, 0x1c	;	Report size (28)
	.db 0x81, 0x01	;	Input (constant)
	.db 0x05, 0x01  ;       Usage Page (Generic Desktop)
	.db 0x16, 0x01, 0xff ;	Logical minimum (-255)
	.db 0x26, 0xff, 0x00 ;	Logical maximum (255)
	.db 0x09, 0x30  ;       Usage (X)
	.db 0x75, 0x08	;	Report size (9)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x81, 0x06  ;       Input (data, variable, relative)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x75, 0x07	;	Report size (7)
	.db 0x81, 0x01	;	Input (constant)
	.db 0x09, 0x31  ;       Usage (Y)
	.db 0x75, 0x08	;	Report size (8)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x81, 0x06  ;       Input (data, variable, relative)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x75, 0x07	;	Report size (7)
	.db 0x81, 0x01	;	Input (constant)
	.db 0x09, 0x38  ;       Usage (Wheel)
	.db 0x75, 0x08	;	Report size (8)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x81, 0x06  ;       Input (data, variable, relative)
	.db 0x95, 0x01	;	Report count (1)
	.db 0x75, 0x07	;	Report size (7)
	.db 0x81, 0x01	;	Input (constant)
	.db 0x95, 0x06	;	Report count (5)
	.db 0x75, 0x10	;	Report size (16)
	.db 0x81, 0x01	;	Input (constant)
	.db 0x0C0       ;   End Collection
	.db 0x0C0       ; End Collection
_ReportDscrEnd:

ReportEnd_word_allignment:
;   	.db  0x00         ;Force word alignment

	
_StringDscr:
_StringDscr0:
		.db	StringDscr0End-_StringDscr0		;; String descriptor length
		.db	DSCR_STRING
		.db	0x09,0x04
StringDscr0End:

_StringDscr1:	
		.db	StringDscr1End-_StringDscr1		;; String descriptor length
		.db	DSCR_STRING
		.db	'm,00
		.db	'c,00

StringDscr1End:
_StringDscr2:	
		.db	StringDscr2End-_StringDscr2		;; Descriptor length
		.db	DSCR_STRING
		.db	'M,00
		.db	'a,00
		.db	'p,00
		.db	'l,00
		.db	'e,00
		.db	' ,00
		.db	'B,00
		.db	'u,00
		.db	's,00
		.db	' ,00
		.db	'A,00
		.db	'd,00
		.db	'a,00
		.db	'p,00
		.db	't,00
		.db	'e,00
		.db	'r,00
StringDscr2End:
	
_UserDscr:		
		.dw	0x0000
;		.end


