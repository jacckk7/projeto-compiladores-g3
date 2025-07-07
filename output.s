.text
.globl main
main:
    addi sp, sp, -16

    # x = 5
    li t0, 5
    sw t0, 12(sp)

    # y = 3
    li t0, 3
    sw t0, 8(sp)

    # w = 2
    li t0, 2
    sw t0, 0(sp)

    # z = (((x + y) * (z - (w / 2))) % 3)
    lw t0, 12(sp)
    sw t0, 16(sp)
    lw t0, 8(sp)
    sw t0, 20(sp)
    lw t0, 16(sp)
    lw t1, 20(sp)
    add t2, t0, t1
    sw t2, 24(sp)
    lw t0, 4(sp)
    sw t0, 28(sp)
    lw t0, 0(sp)
    sw t0, 32(sp)
    li t0, 2
    sw t0, 36(sp)
    lw t0, 32(sp)
    lw t1, 36(sp)
    div t2, t0, t1
    sw t2, 40(sp)
    lw t0, 28(sp)
    lw t1, 40(sp)
    sub t2, t0, t1
    sw t2, 44(sp)
    lw t0, 24(sp)
    lw t1, 44(sp)
    mul t2, t0, t1
    sw t2, 48(sp)
    li t0, 3
    sw t0, 52(sp)
    lw t0, 48(sp)
    lw t1, 52(sp)
    rem t2, t0, t1
    sw t2, 56(sp)
    lw t0, 16(sp)
    sw t0, 4(sp)

    li a7, 10
    ecall
