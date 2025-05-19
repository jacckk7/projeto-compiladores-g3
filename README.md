#Para compilar o analisador léxico

```
>> flex analisador.l                 # Gera lex.yy.c
>> gcc lex.yy.c -o analisador -lfl   # Compila com a biblioteca do Flex
>> ./analisador < entrada.txt        # Roda passando a entrada (se não tiver entrada retire)
```
