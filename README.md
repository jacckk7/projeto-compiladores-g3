#Para compilar o analisador l�xico

>> flex analisador.l                 # Gera lex.yy.c
>> gcc lex.yy.c -o analisador -lfl   # Compila com a biblioteca do Flex
>> ./analisador < entrada.txt        # Roda passando a entrada (se n�o tiver entrada retire)