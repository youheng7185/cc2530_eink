TARGET  = firmware
OUTDIR  = out

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

SRCS = main.c uart.c wdt.c spi.c DEV_Config.c EPD_2in13b_V3_test.c EPD_2in13b_V3.c GxGDEW0213Z16.c
OBJS = $(addprefix $(OUTDIR)/,$(SRCS:.c=.rel))

.PHONY: all clean

all: $(OUTDIR)/$(TARGET).bin
	@echo "Size: $$(wc -c < $(OUTDIR)/$(TARGET).bin) bytes"

# Create output directory if not exists
$(OUTDIR):
	mkdir -p $(OUTDIR)

# Compile .c -> out/*.rel
$(OUTDIR)/%.rel: %.c | $(OUTDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link to .ihx inside out/
$(OUTDIR)/$(TARGET).ihx: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

# Convert to .bin inside out/
$(OUTDIR)/$(TARGET).bin: $(OUTDIR)/$(TARGET).ihx
	$(MAKEBIN) -p $< $@

clean:
	rm -rf $(OUTDIR)