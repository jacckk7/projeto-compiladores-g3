# Para compilar o analisador léxico

```
>> flex lexico.l                 # Gera lex.yy.c
>> gcc lex.yy.c -o lexico.exe    # Compila com a biblioteca do Flex
>> ./lexico < entrada.txt        # Roda passando a entrada (se não tiver entrada retire)
```

# Para compilar o analisador sintático

```
>> bison -dv sintatico_v1.y                            # Gera sintatico.tab.c e sintatico.tab.h
>> flex lexico_c_v2.l                                    # Gera lex.yy.c
>> gcc sintatico_v1.tab.c lex.yy.c -o sintatico        # Compila com a biblioteca do Flex
>> ./sintatico < entrada.txt                        # Roda passando a entrada (se não tiver entrada retire)
```
