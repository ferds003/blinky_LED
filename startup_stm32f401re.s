.syntax unified
.cpu cortex-m4
.thumb

.global _start
.global Reset_Handler

.section .isr_vector
.word _estack
.word Reset_Handler

.section .text
Reset_Handler:
    bl main
loop:
    b loop