# Para compilar, faça:

```
>> make                                                                    # Irá compilar todos os arquivos necessários
>> ./sintatico.exe < (nome_do_arquivo_teste.txt) > sintatico_output.txt    # Roda o sintatico e já gera o output para colocar no gerador
>> ./riscv_gen.exe sintatico_output.txt output.s                           # Roda o gerador com o output do sintatico e gera o código objeto em RISC-V (.s)
```

