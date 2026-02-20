TARGET  = firmware
CC      = sdcc
MAKEBIN = makebin

CFLAGS  = -mmcs51            \
           --model-large      \
           --xram-size 8192   \
           --xram-loc 0x0000  \
           --code-size 0x7F00 \
           --iram-size 256    \
           --stack-size 64    \
           --opt-code-size

PORT      = COM5
FLASHTOOL = py -3 cc_debugger.py

.PHONY: all flash clean

all: $(TARGET).bin
	@echo "Size: $$(wc -c < $(TARGET).bin) bytes"

$(TARGET).ihx: main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET).ihx

$(TARGET).bin: $(TARGET).ihx
	$(MAKEBIN) -p $(TARGET).ihx $(TARGET).bin

flash: $(TARGET).bin
	$(FLASHTOOL) -p $(PORT) erase
	$(FLASHTOOL) -p $(PORT) write $(TARGET).bin

clean:
	rm -f *.asm *.lst *.rel *.rst *.sym *.map *.mem *.lnk *.ihx *.bin