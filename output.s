 1: .text
 2: .globl main
 3: main:
 4:     addi sp, sp, -416
 5:     # Calculando x =  5 6:     li t0, 5
 7:     sw t0, 16(sp)
 8:     lw t0, 16(sp)  # Carrega resultado 9:     sw t0, 12(sp)  # Armazena em x10:     sw zero, 16(sp)  # Limpa temporário11:     # Calculando y =  312:     li t0, 3
13:     sw t0, 16(sp)
14:     lw t0, 16(sp)  # Carrega resultado15:     sw t0, 8(sp)  # Armazena em y16:     sw zero, 16(sp)  # Limpa temporário17:     # Calculando w =  218:     li t0, 2
19:     sw t0, 16(sp)
20:     lw t0, 16(sp)  # Carrega resultado21:     sw t0, 0(sp)  # Armazena em w22:     sw zero, 16(sp)  # Limpa temporário23:     # Calculando z =  (((x + y) * (z - (w / 2))) % 3)24:     lw t0, 12(sp)
25:     sw t0, 16(sp)
26:     lw t0, 8(sp)
27:     sw t0, 20(sp)
28:     lw t0, 16(sp)
29:     lw t1, 20(sp)
30:     add t2, t0, t1
31:     sw t2, 24(sp)
32:     lw t0, 4(sp)
33:     sw t0, 28(sp)
34:     lw t0, 0(sp)
35:     sw t0, 32(sp)
36:     li t0, 2
37:     sw t0, 36(sp)
38:     lw t0, 32(sp)
39:     lw t1, 36(sp)
40:     div t2, t0, t1
41:     sw t2, 40(sp)
42:     lw t0, 28(sp)
43:     lw t1, 40(sp)
44:     sub t2, t0, t1
45:     sw t2, 44(sp)
46:     lw t0, 24(sp)
47:     lw t1, 44(sp)
48:     mul t2, t0, t1
49:     sw t2, 48(sp)
50:     li t0, 3
51:     sw t0, 52(sp)
52:     lw t0, 48(sp)
53:     lw t1, 52(sp)
54:     rem t2, t0, t1
55:     sw t2, 56(sp)
56:     lw t0, 16(sp)  # Carrega resultado57:     sw t0, 4(sp)  # Armazena em z58:     sw zero, 16(sp)  # Limpa temporário59: 
    li a7, 10
60:     ecall
