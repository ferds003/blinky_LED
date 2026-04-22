CC=arm-none-eabi-gcc
CFLAGS=-mcpu=cortex-m4 -mthumb -O0 -g

all:
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o
	$(CC) $(CFLAGS) -T linker.ld startup_stm32f401re.s build/main.o -o build/blinky.elf

clean:
	rm -rf build/*