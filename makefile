# Makefile para o compilador C simplificado para RISC-V

CC = gcc
FLEX = flex
BISON = bison
RM = rm -f

# Nomes dos executáveis
LEXICO = lexico.exe
SINTATICO = sintatico.exe
RISC_GEN = riscv_gen2_otimizado.exe

# Arquivos de teste
TEST_INPUT = aritmetica.txt
TEST_OUTPUT = output_otimizado.s

# Alvo padrão
all: $(LEXICO) $(SINTATICO) $(RISC_GEN)

# Regra para o analisador léxico
$(LEXICO): lexico_c.l
	$(FLEX) lexico_c.l
	$(CC) lex.yy.c -o $(LEXICO)

# Regra para o analisador sintático
$(SINTATICO): sintatico_v3.y lexico_c_v2.l
	$(BISON) -dv sintatico_v3.y
	$(FLEX) lexico_c_v2.l
	$(CC) sintatico_v3.tab.c lex.yy.c -o $(SINTATICO)

# Regra para o gerador de código RISC-V
$(RISC_GEN): riscv_gen3.c
	$(CC) riscv_gen3.c -o $(RISC_GEN)

# Essa parte é com o otimizador, contudo ele não possui as últimas partes implementadas no gerador de código
# Para testar o otimizador só comentar as duas linhas de cima e descomentar as duas linhas abaixo
# $(RISC_GEN): riscv_gen2_otimizado.c
# 	$(CC) riscv_gen2_otimizado.c -o $(RISC_GEN)

# Regra para testar todo o pipeline
test: all
	@echo "Testando o pipeline completo..."
	@echo "1. Executando analisador léxico..."
	./$(LEXICO) < $(TEST_INPUT)
	@echo "\n2. Executando analisador sintático..."
	./$(SINTATICO) < $(TEST_INPUT)
	@echo "\n3. Gerando código RISC-V..."
	./$(SINTATICO) < $(TEST_INPUT) > sintatico_output.txt
	./$(RISC_GEN) sintatico_output.txt $(TEST_OUTPUT)
	@echo "\nCódigo RISC-V gerado:"
	@cat $(TEST_OUTPUT)

# Limpeza
clean:
	$(RM) *.exe *.tab.* *.yy.c *.output *.o $(TEST_OUTPUT) sintatico_output.txt

.PHONY: all test clean