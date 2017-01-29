
CC = sdcc -mmcs51
CFLAGS = -I. -DREPORTDSCR=
LDFLAGS = --out-fmt-ihx --code-size 8192 --xram-loc 0xe000 --xram-size 512 \
		-Wl"-b DSCR = 0x1800" -Wl"-b USBJT = 0x1A00"

DISTFILES = delay.c delayms.asm discon.c dscr.asm fw.c fx2.h fx2hid.c \
	fx2regs.h fx2sdly.h Makefile maple.c resume.c susp.asm USBJmpTb.asm


.SUFFIXES: .c .asm .rel .ihx

.c.rel:
	$(CC) $(CFLAGS) -c $<

.asm.rel:
	asx8051 -losg $<

dchid.ihx: fx2hid.rel fw.rel discon.rel delay.rel resume.rel \
		susp.rel delayms.rel USBJmpTb.rel dscr.rel maple.rel
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.rel *.lst *.rst *.sym *.lnk *.map *.mem *.ihx
	rm -f fx2hid.asm fw.asm discon.asm delay.asm resume.asm maple.asm

spotless: clean
	rm -f *~

dist : $(DISTFILES)
	@rm -f dchid.tar.gz
	tar czf dchid.tar.gz $(DISTFILES)

fx2hid.rel : fx2hid.c fx2.h fx2regs.h fx2sdly.h

fw.rel : fw.c fx2.h fx2regs.h fx2sdly.h

discon.rel : discon.c fx2.h fx2regs.h

delay.rel : delay.c fx2.h fx2regs.h

maple.rel : maple.c fx2.h fx2regs.h

resume.rel : resume.c fx2.h fx2regs.h

susp.rel : susp.asm

delayms.rel : delayms.asm

USBJmpTb.rel : USBJmpTb.asm

dscr.rel : dscr.asm


