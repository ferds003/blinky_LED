# Bare-Metal Morse Code Encoder / Decoder — STM32F401RE

A bare-metal project built from the ground up on the STM32F401RE Nucleo board — no HAL, no RTOS, no STM32CubeIDE. Just pure C, a linker script, and a startup file.

The end goal is a fully working **Morse code encoder and decoder** running entirely on bare-metal: input a message, watch it flash, and read it back — all driven by hardware we configure ourselves, register by register.

---

## 🎯 End Goal

Build a self-contained Morse code encoder / decoder on the STM32F401RE where:
- A **user input** (button or serial) encodes a message into Morse code
- An **LED flashes** the encoded Morse code signal
- An **LCD display** decodes and prints the message back in plain text

No libraries. No shortcuts. Every peripheral driven from scratch.

---

## 📍 Progress

| Task | Status | Description |
|------|--------|-------------|
| **Task 1** | ✅ Done | Blink onboard LD2 (PA5) to flash "I LOVE YOU" in Morse code. Bare-metal build system, Makefile, and `.bin` flashing working. |
| **Task 2** | 🔲 Next | Add a separate external LED wired to a GPIO pin to flash Morse code independently of the onboard LED. |
| **Task 3** | 🔲 Upcoming | Add an LCD display to decode and print the Morse code as readable text in real time. |
| **Task 4** | 🔲 Upcoming | Add onboard button input to encode custom messages into Morse code. |

---

## Hardware

- STM32F401RE Nucleo Board
- USB Cable (Mini-B) — power and flashing
- *(Upcoming)* External LED + resistor
- *(Upcoming)* LCD display (I2C or parallel)

---

## Morse Code Reference

Timing follows the ITU Morse code standard — all durations are multiples of one base `UNIT`:

| Symbol | Duration |
|--------|----------|
| Dot | 1 unit |
| Dash | 3 units |
| Gap between symbols | 1 unit |
| Gap between letters | 3 units |
| Gap between words | 7 units |

---

## Task 1: "I LOVE YOU" in Morse Code

The LD2 LED on PA5 blinks "I LOVE YOU" then pauses and repeats:

```
I  =  · ·
L  =  · − · ·
O  =  − − −
V  =  · · · −
E  =  ·
        (word gap)
Y  =  − · − −
O  =  − − −
U  =  · · −
```

To tune the speed, adjust `UNIT` in `main.c` — higher is slower, lower is faster:
```c
#define UNIT  200000
```

![**Demo of Morse Code:**](assets/iloveu_blinky.gif)

---

## Task 2: TBC

---

## Task 3: TBC

---

## Task 4: TBC

---

## Building & Flashing

```bash
mkdir -p build
make
```

Copy `build/blinky.bin` to the `NODE_F401RE` USB drive that appears when the Nucleo is plugged in, or flash via terminal:

```bash
cp build/blinky.bin /e/    # replace 'e' with your Nucleo drive letter
```

---