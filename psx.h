void ReadPSX(report_t *reportBuffer);
char PSXWaitACK();
uchar PSXCommand(uchar cmd);

#define	PSX_ID_DIGITAL	0x41
#define PSX_ID_A_RED	0x73
#define	PSX_ID_A_GREEN	0x53
#define	PSX_ID_NEGCON	0x23

#define	PSX_NOT_CON		0	// not connected
#define	PSX_CON_NORMAL	1	// normal style controller
#define	PSX_CON_DDG_A	2	// Densha De Go! controller in analogue axis mode
#define	PSX_CON_DDG_S	3	// Densha De Go! controller in digital shift mode

#define DAT	(1<<5)
#define	CMD	(1<<4)
#define	ATT	(1<<3)
#define	CLK	(1<<2)
#define	ACK	(1<<1)
