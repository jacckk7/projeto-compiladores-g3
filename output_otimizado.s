 1: .text
 2: .globl main
 3: main:
 4:     addi sp, sp, -12  # Aloca espaço para variáveis
 5:     # Otimização: usando tmpOffset para registradores t0-t4
 6:     li t0,  5  # x =  5
 7:     li t1,  10  # y =  10
 8:     add t2, t0, t1  # z =  (x + y)
 9: 
    li a7, 10
10:     ecall
