 1: .text
 2: .globl main
 3: main:
 4:     addi sp, sp, -412
 5:     # Calculando x =  5 6:     li t0, 5
 7:     sw t0, 12(sp)
 8:     lw t0, 16(sp)  # Carrega resultado 9:     sw t0, 0(sp)  # Armazena em x10:     sw zero, 16(sp)  # Limpa temporário11:     # Calculando y =  1012:     li t0, 10
13:     sw t0, 12(sp)
14:     lw t0, 16(sp)  # Carrega resultado15:     sw t0, 4(sp)  # Armazena em y16:     sw zero, 16(sp)  # Limpa temporário17:     # Calculando z =  (x + y)18:     lw t0, 0(sp)
19:     sw t0, 12(sp)
20:     lw t0, 4(sp)
21:     sw t0, 16(sp)
22:     lw t0, 12(sp)
23:     lw t1, 16(sp)
24:     add t2, t0, t1
25:     sw t2, 20(sp)
26:     lw t0, 16(sp)  # Carrega resultado27:     sw t0, 8(sp)  # Armazena em z28:     sw zero, 16(sp)  # Limpa temporário29: 
    li a7, 10
30:     ecall
