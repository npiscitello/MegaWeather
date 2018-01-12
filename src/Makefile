CC 				:= 	xtensa-lx106-elf-gcc
CFLAGS 		:= 	-I. \
							-DICACHE_FLASH \
							-mlongcalls \
							-std=c99 \
							-Wno-implicit-function-declaration \
							-I../lib/ESP8266_SPI_Driver/include

LDLIBS 		:= 	-nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -ldriver -lc -Wl,--end-group -lgcc
LDFLAGS 	:= 	-Teagle.app.v6.ld

MAIN 			:= 	main
MAIN			:=	$(MAIN)_compiled

CSRCS 		:= 	main.c \
							../lib/ESP8266_SPI_Driver/driver/spi.c

OBJS 			:= 	$(patsubst %.c, %.o, $(notdir $(CSRCS)))

# pack
$(MAIN)-0x00000.bin: $(MAIN)
	esptool.py elf2image $^

# link
$(MAIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# compile
$(OBJS): $(CSRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^

# flashing probs? try a lower baudrate, like 921600, 460800, 230400, or remove the arg alltogether
flash: $(BIN)
	esptool.py -b 1500000 write_flash 0 $(MAIN)-0x00000.bin 0x10000 $(MAIN)-0x10000.bin

clean:
	rm -f $(MAIN) $(OBJS) $(MAIN)-0x00000.bin $(MAIN)-0x10000.bin
