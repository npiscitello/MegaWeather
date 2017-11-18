CC 				:= xtensa-lx106-elf-gcc
CFLAGS 		:= -I. -mlongcalls -DICACHE_FLASH
LDLIBS 		:= -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -lc -Wl,--end-group -lgcc -ldriver
LDFLAGS 	:= -Teagle.app.v6.ld

MAIN 			:= blinky

CSRCS 		:= blinky.c
OBJS 			:= $(patsubst %.c, %.o, $(CSRCS))

# pack
$(MAIN)-0x00000.bin: $(MAIN)
	esptool.py elf2image $^

# link
$(MAIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# compile
$(OBJS): $(CSRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^

flash: $(BIN)
	esptool.py write_flash 0 $(MAIN)-0x00000.bin 0x10000 $(MAIN)-0x10000.bin

clean:
	rm -f $(MAIN) $(OBJS) $(MAIN)-0x00000.bin $(MAIN)-0x10000.bin
