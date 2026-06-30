.syntax unified
.cpu cortex-m3
.thumb

.section .isr_vector, "a"
.word _estack          @ Initial stack pointer
.word Reset_Handler    @ Reset vector
.fill 14, 4, 0         @ Remaining exception vectors

.section .text
.thumb_func
.global Reset_Handler
Reset_Handler:
    ldr sp, =_estack

    @ Copy .data from Flash (LMA) to RAM (VMA)
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata       @ LMA start of .data in flash
copy_data:
    cmp r0, r1
    bge zero_bss
    ldr r3, [r2], #4
    str r3, [r0], #4
    b copy_data

    @ Zero out .bss
zero_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
bss_loop:
    cmp r0, r1
    bge call_main
    str r2, [r0], #4
    b bss_loop

call_main:
    bl main
    b .                 @ Loop forever after main returns
