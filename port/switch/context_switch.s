/**
 * context_switch — AArch64 cooperative context switch.
 *
 * C prototype: void context_switch(SwitchContext *from, SwitchContext *to);
 *
 * Saves callee-saved registers into *from, restores from *to.
 * SwitchContext layout (offsets must match coroutine_switch.cpp):
 *   +0  .. +72 : x19..x28  (10 registers)
 *   +80 .. +88 : x29 (fp), x30 (lr)
 *   +96        : sp
 *   +104       : x0
 *   +112..+160 : d8..d15   (8 registers)
 */
    .text
    .align 2
    .globl context_switch
context_switch:
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x2, sp
    str x2,      [x0, #96]
    str x0,      [x0, #104]
    stp d8,  d9,  [x0, #112]
    stp d10, d11, [x0, #128]
    stp d12, d13, [x0, #144]
    stp d14, d15, [x0, #160]

    /* Restore from target */
    ldp d8,  d9,  [x1, #112]
    ldp d10, d11, [x1, #128]
    ldp d12, d13, [x1, #144]
    ldp d14, d15, [x1, #160]
    ldr x2,      [x1, #96]
    mov sp, x2
    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldp x29, x30, [x1, #80]
    ldr x0,      [x1, #104]
    ret
