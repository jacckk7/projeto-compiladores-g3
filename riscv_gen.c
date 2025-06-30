#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Estrutura para rastrear variáveis
typedef struct {
    char name[50];
    int offset;
} Variable;

Variable variables[50];
int var_count = 0;
int current_offset = 0;
int temp_count = 0;

// Pilha para avaliação de expressões
typedef struct {
    char op;
    int precedence;
} Operator;

Operator op_stack[100];
int op_stack_top = -1;

// Funções auxiliares para a pilha
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

int get_var_offset(const char *var_name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, var_name) == 0) {
            return variables[i].offset;
        }
    }
    return -1;
}

void add_variable(const char *var_name) {
    if (get_var_offset(var_name) == -1) {
        int offset = var_count * 4;  // 0, 4, 8, 12...
        strcpy(variables[var_count].name, var_name);
        variables[var_count].offset = offset;
        var_count++;
        current_offset += 4;
    }
}

void generate_riscv_header(FILE *output) {
    fprintf(output, ".text\n");
    fprintf(output, ".globl main\n");
    fprintf(output, "main:\n");
    fprintf(output, "    addi sp, sp, -%d\n", current_offset);
}

void generate_riscv_footer(FILE *output) {
    fprintf(output, "\n    li a7, 10\n");
    fprintf(output, "    ecall\n");
}

void generate_load_operand(FILE *output, const char *operand, const char *reg) {
    if (isdigit(operand[0])) {
        fprintf(output, "    li %s, %s\n", reg, operand);
    } else {
        int offset = get_var_offset(operand);
        if (offset != -1) {
            fprintf(output, "    lw %s, %d(sp)\n", reg, offset);
        } else {
            fprintf(stderr, "Erro: Variável '%s' não declarada\n", operand);
        }
    }
}

void generate_operation(FILE *output, char op, const char *reg1, const char *reg2, const char *reg_dest) {
    switch (op) {
        case '+':
            fprintf(output, "    add %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '-':
            fprintf(output, "    sub %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '*':
            fprintf(output, "    mul %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '/':
            fprintf(output, "    div %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '%':
            fprintf(output, "    rem %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
    }
}

void generate_temp_store(FILE *output, int temp_num, const char *reg) {
    fprintf(output, "    sw %s, %d(sp)\n", reg, current_offset + temp_num * 4);
}

void generate_temp_load(FILE *output, int temp_num, const char *reg) {
    fprintf(output, "    lw %s, %d(sp)\n", reg, current_offset + temp_num * 4);
}

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

void process_expression(FILE *output, const char *expr) {
    char token[50];
    int token_pos = 0;
    int expr_len = strlen(expr);
    int temp_stack[100];
    int temp_stack_top = -1;
    op_stack_top = -1;
    
    for (int i = 0; i < expr_len; i++) {
        if (isspace(expr[i])) continue;
        
        if (isalnum(expr[i])) {
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
            generate_load_operand(output, token, "t0");
            generate_temp_store(output, temp_stack[temp_stack_top], "t0");
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
                
                generate_temp_load(output, op1, "t0");
                generate_temp_load(output, op2, "t1");
                generate_operation(output, op.op, "t0", "t1", "t2");
                generate_temp_store(output, result, "t2");
                
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
                
                generate_temp_load(output, op1, "t0");
                generate_temp_load(output, op2, "t1");
                generate_operation(output, op.op, "t0", "t1", "t2");
                generate_temp_store(output, result, "t2");
                
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
        
        generate_temp_load(output, op1, "t0");
        generate_temp_load(output, op2, "t1");
        generate_operation(output, op.op, "t0", "t1", "t2");
        generate_temp_store(output, result, "t2");
        
        temp_stack[++temp_stack_top] = result;
    }
    
    // O resultado final está no topo da pilha
    temp_count = 0; // Reseta o contador de temporários para próxima expressão
}

void generate_riscv_assignment(FILE *output, const char *var_name, const char *expr) {
    int offset = get_var_offset(var_name);
    if (offset == -1) {
        fprintf(stderr, "Erro: Variável '%s' não declarada!\n", var_name);
        return;
    }

    fprintf(output, "\n    # %s = %s\n", var_name, expr);
    
    // Se for um número direto
    if (isdigit(expr[0]) && strspn(expr, "0123456789") == strlen(expr)) {
        fprintf(output, "    li t0, %s\n", expr);
        fprintf(output, "    sw t0, %d(sp)\n", offset);
        return;
    }
    
    // Se for uma variável simples
    if (get_var_offset(expr) != -1 && strcspn(expr, "+-*/%()") == strlen(expr)) {
        int src_offset = get_var_offset(expr);
        fprintf(output, "    lw t0, %d(sp)\n", src_offset);
        fprintf(output, "    sw t0, %d(sp)\n", offset);
        return;
    }
    
    // Processa expressão complexa
    process_expression(output, expr);
    
    // O resultado está no último temporário (0)
    fprintf(output, "    lw t0, %d(sp)\n", current_offset);
    fprintf(output, "    sw t0, %d(sp)\n", offset);
}

void generate_riscv_code(FILE *input, FILE *output) {
    char line[256];
    char var_name[50];
    char expr[100];
    
    // Primeira passada: declarações de variáveis
    while (fgets(line, sizeof(line), input) != NULL) {
        if (sscanf(line, "Variavel %s criada!", var_name) == 1) {
            add_variable(var_name);
        }
    }
    
    rewind(input);
    generate_riscv_header(output);
    
    // Segunda passada: processar atribuições
    while (fgets(line, sizeof(line), input) != NULL) {
        if (strstr(line, "Atribuicao:") != NULL) {
            char* equal_pos = strchr(line, '=');
            if (equal_pos) {
                *equal_pos = '\0';
                char* lhs = line + strlen("Atribuicao: ");
                char* rhs = equal_pos + 1;
                
                // Remove espaços em branco
                while(isspace(*lhs)) lhs++;
                char* end = lhs + strlen(lhs) - 1;
                while(end > lhs && isspace(*end)) end--;
                *(end+1) = '\0';
                
                while(isspace(*rhs)) rhs++;
                end = rhs + strlen(rhs) - 1;
                while(end > rhs && isspace(*end)) end--;
                *(end+1) = '\0';
                
                generate_riscv_assignment(output, lhs, rhs);
            }
        }
    }
    
    generate_riscv_footer(output);
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
    
    generate_riscv_code(input, output);
    
    fclose(input);
    fclose(output);
    
    printf("Código RISC-V gerado em %s\n", argv[2]);
    return 0;
}