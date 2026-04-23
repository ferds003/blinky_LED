CC      = arm-none-eabi-gcc
CFLAGS  = -mcpu=cortex-m4 -mthumb -O0 -g

all: build/blinky.bin

# Step 1: Compile main.c → main.o
build/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

# Step 2: Link everything → blinky.elf
build/blinky.elf: build/main.o
	$(CC) $(CFLAGS) -nostdlib -nostartfiles \
		-T linker.ld \
		startup_stm32f401re.s \
		build/main.o \
		-o build/blinky.elf

# Step 3: Convert ELF → blinky.bin
build/blinky.bin: build/blinky.elf
	arm-none-eabi-objcopy -O binary build/blinky.elf build/blinky.bin

clean:
	rm -rf build/*