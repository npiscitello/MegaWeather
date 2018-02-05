CC 				:= 	xtensa-lx106-elf-gcc
CFLAGS 		:= 	-Iinclude \
							-DICACHE_FLASH \
							-mlongcalls \
							-std=c99 \
							-Wno-implicit-function-declaration

LDLIBS 		:= 	-nostdlib -Wl,--start-group -ldriver -lmain -lnet80211 -lwpa -llwip -lpp -lphy -ldriver -lc -Wl,--end-group -lgcc
LDFLAGS 	:= 	-Teagle.app.v6.ld

SRC_DIR 	:=	src
BUILD_DIR := 	build

MAIN  		:=	$(BUILD_DIR)/megaweather

CSRCS 		:= 	$(wildcard $(SRC_DIR)/*.c)

OBJS 			:= 	$(notdir $(CSRCS))
OBJS 			:= 	$(patsubst %.c, $(BUILD_DIR)/%.o, $(OBJS))

.PHONY: all
all: images

# make the build folder
$(BUILD_DIR):
	mkdir -p $@

# compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# link
$(MAIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# pack
.PHONY: images
images: $(MAIN)
	esptool.py elf2image $<

# flashing probs? try a lower baudrate, like 921600, 460800, 230400, or remove the arg alltogether
flash: images
	esptool.py -b 1500000 write_flash 0 $(MAIN)-0x00000.bin 0x10000 $(MAIN)-0x10000.bin

clean:
	rm -rf $(BUILD_DIR)
