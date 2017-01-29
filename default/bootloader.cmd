avrdude -p atmega168 -F -U flash:w:combined.hex
avrdude -p atmega168 -F -U lfuse:w:0xef:m -U hfuse:w:0xdf:m -U efuse:w:0xf8:m
