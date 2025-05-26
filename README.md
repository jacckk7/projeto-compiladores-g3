# Para compilar o analisador léxico

```
>> flex lexico.l                 # Gera lex.yy.c
>> gcc lex.yy.c -o lexico        # Compila com a biblioteca do Flex
>> ./lexico < entrada.txt        # Roda passando a entrada (se não tiver entrada retire)
```

```
>> bison -dv sintatico.y                            # Gera sintatico.tab.c e sintatico.tab.h
>> flex lexico.l                                    # Gera lex.yy.c
>> gcc sintatico.tab.c lex.yy.c -o sintatico        # Compila com a biblioteca do Flex
>> ./sintatico < entrada.txt                        # Roda passando a entrada (se não tiver entrada retire)
```