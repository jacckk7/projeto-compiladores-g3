# Para compilar, faça:

```
>> make                                                   # Compilar tudo
>> ./sintatico.exe < (teste).txt > sintatico_output.txt   # Passa o arquivo de teste para o sintatico verficar se ta tudo ok
>> ./riscv_gen.exe sintatico_output.txt output.s          # Passa para o gerador para gerar código obj.s
>> make clean                                             # Para apagar a compilação do make
```

