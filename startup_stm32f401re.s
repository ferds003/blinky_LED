.thumb
.syntax unified

    .section .isr_vector, "a", %progbits
    .word _estack               /* 0:  Initial Stack Pointer */
    .word Reset_Handler         /* 1:  Reset Handler */
    .word 0                     /* 2:  NMI */
    .word 0                     /* 3:  HardFault */
    .word 0                     /* 4:  MemManage */
    .word 0                     /* 5:  BusFault */
    .word 0                     /* 6:  UsageFault */
    .word 0                     /* 7:  Reserved */
    .word 0                     /* 8:  Reserved */
    .word 0                     /* 9:  Reserved */
    .word 0                     /* 10: Reserved */
    .word 0                     /* 11: SVCall */
    .word 0                     /* 12: Debug Monitor */
    .word 0                     /* 13: Reserved */
    .word 0                     /* 14: PendSV */
    .word 0                     /* 15: SysTick */

    .section .text.Reset_Handler
    .global Reset_Handler
    .type Reset_Handler, %function

Reset_Handler:

    /* -------------------------------------------------------
       Step 1: Copy .data from Flash to RAM
    ------------------------------------------------------- */
    ldr r0, =_sdata             /* r0 = RAM destination start */
    ldr r1, =_edata             /* r1 = RAM destination end */
    ldr r2, =_la_data           /* r2 = Flash source */
    b   copy_check

copy_loop:
    ldr r3, [r2]                /* read word from Flash */
    str r3, [r0]                /* write word to RAM */
    add r0, r0, #4              /* advance destination */
    add r2, r2, #4              /* advance source */

copy_check:
    cmp r0, r1
    blt copy_loop

    /* -------------------------------------------------------
       Step 2: Zero out .bss in RAM
    ------------------------------------------------------- */
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0
    b   bss_check

bss_loop:
    str r2, [r0]                /* write 0 to address */
    add r0, r0, #4              /* advance pointer */

bss_check:
    cmp r0, r1
    blt bss_loop

    /* -------------------------------------------------------
       Step 3: Jump to main()
    ------------------------------------------------------- */
    bl main

    /* -------------------------------------------------------
       Step 4: Hang if main() returns
    ------------------------------------------------------- */
hang:
    b hang

    .size Reset_Handler, .-Reset_Handler