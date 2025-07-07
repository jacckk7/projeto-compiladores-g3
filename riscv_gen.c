#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 256
#define MAX_VARIABLES 50
#define MAX_TEMPORARIES 100

// Estrutura para rastrear variáveis
typedef struct {
    char name[50];
    char type[20];
    int offset;
    int size;
} Variable;

typedef struct {
    char op;
    int precedence;
} Operator;

typedef struct {
    int line_number;
    char code[MAX_LINE_LENGTH];
} CodeLine;

// Variáveis globais
Variable variables[MAX_VARIABLES];
int var_count = 0;
int current_offset = 0;
int temp_count = 0;
int label_count = 0;

Operator op_stack[100];
int op_stack_top = -1;

CodeLine output_code[1000];
int code_line_count = 0;

// Funções auxiliares para a pilha de operadores
void push_op(char op, int precedence) {
    op_stack[++op_stack_top].op = op;
    op_stack[op_stack_top].precedence = precedence;
}

Operator pop_op() {
    return op_stack[op_stack_top--];
}

Operator peek_op() {
    return op_stack[op_stack_top];
}

bool is_op_stack_empty() {
    return op_stack_top == -1;
}

// Adiciona uma linha de código à saída
void add_code_line(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    if (code_line_count < 1000) {
        output_code[code_line_count].line_number = code_line_count + 1;
        vsnprintf(output_code[code_line_count].code, MAX_LINE_LENGTH, format, args);
        code_line_count++;
    }
    
    va_end(args);
}

// Encontra uma variável na tabela de símbolos
Variable* find_variable(const char *var_name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, var_name) == 0) {
            return &variables[i];
        }
    }
    return NULL;
}

// Adiciona uma variável à tabela de símbolos
void add_variable(const char *var_name, const char *var_type) {
    if (find_variable(var_name)) return; // Variável já existe
    
    if (var_count < MAX_VARIABLES) {
        strcpy(variables[var_count].name, var_name);
        strcpy(variables[var_count].type, var_type);
        
        // Determina o tamanho baseado no tipo
        if (strcmp(var_type, "INT") == 0 || strcmp(var_type, "FLOAT") == 0) {
            variables[var_count].size = 4;
        } else if (strcmp(var_type, "DOUBLE") == 0 || strcmp(var_type, "LONG") == 0) {
            variables[var_count].size = 8;
        } else if (strcmp(var_type, "CHAR") == 0) {
            variables[var_count].size = 1;
        } else {
            variables[var_count].size = 4; // Padrão para 4 bytes
        }
        
        variables[var_count].offset = current_offset;
        current_offset += variables[var_count].size;
        var_count++;
    }
}

// Gera o cabeçalho do código RISC-V
void generate_riscv_header() {
    add_code_line(".text");
    add_code_line(".globl main");
    add_code_line("main:");
    add_code_line("    addi sp, sp, -%d  # Aloca espaço na pilha", current_offset);
}

// Gera o rodapé do código RISC-V
void generate_riscv_footer() {
    add_code_line("");
    add_code_line("    # Finalização do programa");
    add_code_line("    li a7, 10");
    add_code_line("    ecall");
}

// Gera código para carregar um operando
void generate_load_operand(const char *operand, const char *reg) {
    if (isdigit(operand[0])) {
        add_code_line("    li %s, %s  # Carrega constante", reg, operand);
    } else {
        Variable *var = find_variable(operand);
        if (var) {
            if (var->size == 4) {
                add_code_line("    lw %s, %d(sp)  # Carrega variável '%s'", reg, var->offset, operand);
            } else if (var->size == 8) {
                add_code_line("    ld %s, %d(sp)  # Carrega variável double/long '%s'", reg, var->offset, operand);
            } else if (var->size == 1) {
                add_code_line("    lb %s, %d(sp)  # Carrega variável char '%s'", reg, var->offset, operand);
            }
        } else {
            add_code_line("    # ERRO: Variável '%s' não declarada", operand);
        }
    }
}

// Gera código para uma operação aritmética
void generate_operation(char op, const char *reg1, const char *reg2, const char *reg_dest) {
    switch (op) {
        case '+':
            add_code_line("    add %s, %s, %s  # Soma", reg_dest, reg1, reg2);
            break;
        case '-':
            add_code_line("    sub %s, %s, %s  # Subtração", reg_dest, reg1, reg2);
            break;
        case '*':
            add_code_line("    mul %s, %s, %s  # Multiplicação", reg_dest, reg1, reg2);
            break;
        case '/':
            add_code_line("    div %s, %s, %s  # Divisão", reg_dest, reg1, reg2);
            break;
        case '%':
            add_code_line("    rem %s, %s, %s  # Resto", reg_dest, reg1, reg2);
            break;
    }
}

// Gera código para armazenar um valor temporário
void generate_temp_store(int temp_num, const char *reg) {
    add_code_line("    sw %s, %d(sp)  # Armazena temporário t%d", reg, current_offset + temp_num * 4, temp_num);
}

// Gera código para carregar um valor temporário
void generate_temp_load(int temp_num, const char *reg) {
    add_code_line("    lw %s, %d(sp)  # Carrega temporário t%d", reg, current_offset + temp_num * 4, temp_num);
}

// Retorna a precedência de um operador
int get_precedence(char op) {
    switch (op) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
        case '%':
            return 2;
        case '(':
            return 0;
        default:
            return -1;
    }
}

// Processa uma expressão e gera o código correspondente
void process_expression(const char *expr) {
    char token[50];
    int token_pos = 0;
    int expr_len = strlen(expr);
    int temp_stack[100];
    int temp_stack_top = -1;
    op_stack_top = -1;
    
    add_code_line("    # Processando expressão: %s", expr);
    
    for (int i = 0; i < expr_len; i++) {
        if (isspace(expr[i])) continue;
        
        if (isalnum(expr[i]) || expr[i] == '_') {
            // Coleta o token (número ou variável)
            token_pos = 0;
            while (i < expr_len && (isalnum(expr[i]) || expr[i] == '_')) {
                token[token_pos++] = expr[i++];
            }
            token[token_pos] = '\0';
            i--; // Volta um caractere
            
            // Empilha o operando
            temp_stack[++temp_stack_top] = temp_count++;
            
            // Gera código para carregar o operando
            generate_load_operand(token, "t0");
            generate_temp_store(temp_stack[temp_stack_top], "t0");
        } else if (expr[i] == '(') {
            // Empilha o parêntese
            push_op('(', 0);
        } else if (expr[i] == ')') {
            // Processa operadores até encontrar '('
            while (!is_op_stack_empty() && peek_op().op != '(') {
                Operator op = pop_op();
                
                // Gera código para a operação
                int op2 = temp_stack[temp_stack_top--];
                int op1 = temp_stack[temp_stack_top--];
                int result = temp_count++;
                
                generate_temp_load(op1, "t0");
                generate_temp_load(op2, "t1");
                generate_operation(op.op, "t0", "t1", "t2");
                generate_temp_store(result, "t2");
                
                temp_stack[++temp_stack_top] = result;
            }
            
            // Remove o '(' da pilha
            if (!is_op_stack_empty() && peek_op().op == '(') {
                pop_op();
            }
        } else {
            // É um operador
            char current_op = expr[i];
            int current_prec = get_precedence(current_op);
            
            // Processa operadores com maior ou igual precedência
            while (!is_op_stack_empty() && peek_op().precedence >= current_prec) {
                Operator op = pop_op();
                
                // Gera código para a operação
                int op2 = temp_stack[temp_stack_top--];
                int op1 = temp_stack[temp_stack_top--];
                int result = temp_count++;
                
                generate_temp_load(op1, "t0");
                generate_temp_load(op2, "t1");
                generate_operation(op.op, "t0", "t1", "t2");
                generate_temp_store(result, "t2");
                
                temp_stack[++temp_stack_top] = result;
            }
            
            // Empilha o operador atual
            push_op(current_op, current_prec);
        }
    }
    
    // Processa operadores restantes
    while (!is_op_stack_empty()) {
        Operator op = pop_op();
        
        // Gera código para a operação
        int op2 = temp_stack[temp_stack_top--];
        int op1 = temp_stack[temp_stack_top--];
        int result = temp_count++;
        
        generate_temp_load(op1, "t0");
        generate_temp_load(op2, "t1");
        generate_operation(op.op, "t0", "t1", "t2");
        generate_temp_store(result, "t2");
        
        temp_stack[++temp_stack_top] = result;
    }
    
    temp_count = 0; // Reseta o contador de temporários
}

// Gera código para uma atribuição
void generate_riscv_assignment(const char *var_name, const char *expr) {
    Variable *var = find_variable(var_name);
    if (!var) {
        add_code_line("    # ERRO: Variável '%s' não declarada!", var_name);
        return;
    }

    add_code_line("");
    add_code_line("    # %s = %s", var_name, expr);
    
    // Casos simples: constante ou variável única
    if (isdigit(expr[0]) && strspn(expr, "0123456789") == strlen(expr)) {
        add_code_line("    li t0, %s", expr);
        add_code_line("    sw t0, %d(sp)  # Armazena em %s", var->offset, var_name);
        return;
    }
    
    if (find_variable(expr) && strcspn(expr, "+-*/%()") == strlen(expr)) {
        Variable *src = find_variable(expr);
        if (src->size == 4) {
            add_code_line("    lw t0, %d(sp)  # Carrega %s", src->offset, expr);
            add_code_line("    sw t0, %d(sp)  # Armazena em %s", var->offset, var_name);
        } else if (src->size == 8) {
            add_code_line("    ld t0, %d(sp)  # Carrega %s (double/long)", src->offset, expr);
            add_code_line("    sd t0, %d(sp)  # Armazena em %s", var->offset, var_name);
        }
        return;
    }
    
    // Expressão complexa
    process_expression(expr);
    
    // Armazena o resultado (último temporário) na variável
    add_code_line("    lw t0, %d(sp)  # Carrega resultado", current_offset);
    add_code_line("    sw t0, %d(sp)  # Armazena em %s", var->offset, var_name);
}

// Processa o arquivo de entrada e gera o código RISC-V
void generate_riscv_code(FILE *input) {
    char line[MAX_LINE_LENGTH];
    char var_name[50];
    char var_type[20];
    
    // Primeira passada: declarações de variáveis
    while (fgets(line, sizeof(line), input) != NULL) {
        if (sscanf(line, "Variavel %s %s criada!", var_type, var_name) == 2) {
            add_variable(var_name, var_type); // Tipo padrão - precisa ser ajustado
        }
    }
    
    rewind(input);
    generate_riscv_header();
    
    // Segunda passada: processar atribuições
    while (fgets(line, sizeof(line), input) != NULL) {
        if (strstr(line, "=") != NULL) {
            char *equal_pos = strchr(line, '=');
            if (equal_pos) {
                *equal_pos = '\0';
                char *lhs = line;
                char *rhs = equal_pos + 1;
                
                // Remove espaços em branco
                while(isspace(*lhs)) lhs++;
                char* end = lhs + strlen(lhs) - 1;
                while(end > lhs && isspace(*end)) end--;
                *(end+1) = '\0';
                
                while(isspace(*rhs)) rhs++;
                end = rhs + strlen(rhs) - 1;
                while(end > rhs && isspace(*end)) end--;
                *(end+1) = '\0';
                
                generate_riscv_assignment(lhs, rhs);
            }
        }
    }
    
    generate_riscv_footer();
}

// Escreve o código gerado no arquivo de saída com numeração de linhas
void write_output_with_line_numbers(FILE *output) {
    // Primeiro calcula o maior número de linha para alinhamento
    int max_line_num = code_line_count;
    int num_digits = 1;
    while (max_line_num >= 10) {
        max_line_num /= 10;
        num_digits++;
    }
    
    // Escreve cada linha com numeração
    for (int i = 0; i < code_line_count; i++) {
        fprintf(output, "%*d: %s\n", num_digits, output_code[i].line_number, output_code[i].code);
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Uso: %s entrada.txt saida.s\n", argv[0]);
        return 1;
    }
    
    FILE *input = fopen(argv[1], "r");
    if (!input) {
        perror("Erro ao abrir arquivo de entrada");
        return 1;
    }
    
    FILE *output = fopen(argv[2], "w");
    if (!output) {
        perror("Erro ao criar arquivo de saída");
        fclose(input);
        return 1;
    }
    
    generate_riscv_code(input);
    write_output_with_line_numbers(output);
    
    fclose(input);
    fclose(output);
    
    printf("Código RISC-V gerado em %s com numeração de linhas\n", argv[2]);
    return 0;
}