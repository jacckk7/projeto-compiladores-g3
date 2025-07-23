#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

#define MAX_LINE_LENGTH 256
#define MAX_VARIABLES 50
#define MAX_TEMPORARIES 100
#define TEMP_RESULT_OFFSET 16  

void write_output_with_line_numbers(FILE *output);

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

Variable variables[MAX_VARIABLES];
int var_count = 0;
int current_offset = 0;
int temp_count = 0;

Operator op_stack[100];
int op_stack_top = -1;

CodeLine output_code[1000];
int code_line_count = 0;

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

Variable* find_variable(const char *var_name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, var_name) == 0) {
            return &variables[i];
        }
    }
    return NULL;
}

int get_size_from_type(const char *type) {
    if (strcmp(type, "CHAR") == 0) return 1;
    if (strcmp(type, "SHORT") == 0) return 2;
    if (strcmp(type, "INT") == 0) return 4;
    if (strcmp(type, "FLOAT") == 0) return 4;
    if (strcmp(type, "DOUBLE") == 0) return 8;
    if (strcmp(type, "LONG") == 0) return 8;
    return 4; // padrão
}

void add_variable(const char *var_name, const char *var_type) {
    if (find_variable(var_name)) return;
    
    if (var_count < MAX_VARIABLES) {
        strcpy(variables[var_count].name, var_name);
        strcpy(variables[var_count].type, var_type);
        variables[var_count].size = get_size_from_type(var_type);
        variables[var_count].offset = current_offset;
        current_offset += variables[var_count].size;
        var_count++;
    }
}

void generate_riscv_header() {
    add_code_line(".text\n");
    add_code_line(".globl main\n");
    add_code_line("main:\n");
    add_code_line("    addi sp, sp, -%d\n", current_offset + MAX_TEMPORARIES*4);
}

void generate_riscv_footer() {
    add_code_line("\n    li a7, 10\n");
    add_code_line("    ecall\n");
}

void generate_load_operand(const char *operand, const char *reg) {
    if (isdigit(operand[0])) {
        add_code_line("    li %s, %s\n", reg, operand);
    } else {
        Variable *var = find_variable(operand);
        if (var) {
            if (strcmp(var->type, "FLOAT") == 0) {
                add_code_line("    flw %s, %d(sp)\n", reg, var->offset);
            } else if (strcmp(var->type, "DOUBLE") == 0) {
                add_code_line("    fld %s, %d(sp)\n", reg, var->offset);
            } else {
                add_code_line("    lw %s, %d(sp)\n", reg, var->offset);
            }
        } else {
            add_code_line("    # ERRO: Variável '%s' não declarada\n", operand);
        }
    }
}

void generate_operation(char op, const char *reg1, const char *reg2, const char *reg_dest) {
    switch (op) {
        case '+':
            add_code_line("    add %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '-':
            add_code_line("    sub %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '*':
            add_code_line("    mul %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '/':
            add_code_line("    div %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '%':
            add_code_line("    rem %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
    }
}

void generate_temp_store(int temp_num, const char *reg) {
    add_code_line("    sw %s, %d(sp)\n", reg, current_offset + temp_num * 4);
}

void generate_temp_load(int temp_num, const char *reg) {
    add_code_line("    lw %s, %d(sp)\n", reg, current_offset + temp_num * 4);
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

void process_expression(const char *expr) {
    char token[50];
    int token_pos = 0;
    int expr_len = strlen(expr);
    int temp_stack[100];
    int temp_stack_top = -1;
    op_stack_top = -1;
    
    for (int i = 0; i < expr_len; i++) {
        if (isspace(expr[i])) continue;
        
        if (isalnum(expr[i]) || expr[i] == '_') {
            token_pos = 0;
            while (i < expr_len && (isalnum(expr[i]) || expr[i] == '_')) {
                token[token_pos++] = expr[i++];
            }
            token[token_pos] = '\0';
            i--;
            
            temp_stack[++temp_stack_top] = temp_count++;
            generate_load_operand(token, "t0");
            generate_temp_store(temp_stack[temp_stack_top], "t0");
        } else if (expr[i] == '(') {
            push_op('(', 0);
        } else if (expr[i] == ')') {
            while (!is_op_stack_empty() && peek_op().op != '(') {
                Operator op = pop_op();
                
                int op2 = temp_stack[temp_stack_top--];
                int op1 = temp_stack[temp_stack_top--];
                int result = temp_count++;
                
                generate_temp_load(op1, "t0");
                generate_temp_load(op2, "t1");
                generate_operation(op.op, "t0", "t1", "t2");
                generate_temp_store(result, "t2");
                
                temp_stack[++temp_stack_top] = result;
            }
            
            if (!is_op_stack_empty() && peek_op().op == '(') {
                pop_op();
            }
        } else {
            char current_op = expr[i];
            int current_prec = get_precedence(current_op);
            
            while (!is_op_stack_empty() && peek_op().precedence >= current_prec) {
                Operator op = pop_op();
                
                int op2 = temp_stack[temp_stack_top--];
                int op1 = temp_stack[temp_stack_top--];
                int result = temp_count++;
                
                generate_temp_load(op1, "t0");
                generate_temp_load(op2, "t1");
                generate_operation(op.op, "t0", "t1", "t2");
                generate_temp_store(result, "t2");
                
                temp_stack[++temp_stack_top] = result;
            }
            
            push_op(current_op, current_prec);
        }
    }
    
    while (!is_op_stack_empty()) {
        Operator op = pop_op();
        
        int op2 = temp_stack[temp_stack_top--];
        int op1 = temp_stack[temp_stack_top--];
        int result = temp_count++;
        
        generate_temp_load(op1, "t0");
        generate_temp_load(op2, "t1");
        generate_operation(op.op, "t0", "t1", "t2");
        generate_temp_store(result, "t2");
        
        temp_stack[++temp_stack_top] = result;
    }
    
    temp_count = 0;
}

bool is_numeric_constant(const char *str) {
    if (*str == '-') str++;  // Permite negativos
    while (*str) {
        if (!isdigit(*str)) return false;
        str++;
    }
    return true;
}

void generate_riscv_assignment(const char *var_name, const char *expr) {
    Variable *var = find_variable(var_name);
    if (!var) {
        add_code_line("    # ERRO: Variável '%s' não declarada!", var_name);
        return;
    }

    // Atribuição simples (constante numérica)
    if (is_numeric_constant(expr)) {
        add_code_line("    li t0, %s", expr);
        add_code_line("    sw t0, %d(sp)  # %s = %s", var->offset, var_name, expr);
        return;
    }
    
    // Atribuição de outra variável (cópia direta)
    Variable *src_var = find_variable(expr);
    if (src_var) {
        add_code_line("    lw t0, %d(sp)  # Carrega %s", src_var->offset, expr);
        add_code_line("    sw t0, %d(sp)  # %s = %s", var->offset, var_name, expr);
        return;
    }

    // Expressão aritmética mais complexa
    add_code_line("    # Calculando %s = %s", var_name, expr);
    process_expression(expr);
    
    // Otimização: usar posição temporária fixa para resultados
    add_code_line("    lw t0, %d(sp)  # Carrega resultado", TEMP_RESULT_OFFSET);
    add_code_line("    sw t0, %d(sp)  # Armazena em %s", var->offset, var_name);
    
    // Limpeza do temporário (opcional)
    add_code_line("    sw zero, %d(sp)  # Limpa temporário", TEMP_RESULT_OFFSET);
}

void generate_riscv_code(FILE *input, FILE *output) {
    char line[MAX_LINE_LENGTH];
    char var_name[50];
    char var_type[20];
    char expr[100];
    
    // Primeira passada: declarações
    while (fgets(line, sizeof(line), input)) {
        if (sscanf(line, "Variavel %s %s criada!", var_type, var_name) == 2) {
            add_variable(var_name, var_type);
        }
    }
    
    rewind(input);
    generate_riscv_header();
    
    // Segunda passada: atribuições
    while (fgets(line, sizeof(line), input)) {
        if (strstr(line, "Atribuicao:") != NULL) {
            char* equal_pos = strchr(line, '=');
            if (equal_pos) {
                // Extrai nome da variável
                char* start = line + strlen("Atribuicao: ");
                char* end = strchr(start, '=');
                if (end) {
                    strncpy(var_name, start, end - start);
                    var_name[end - start] = '\0';
                    
                    // Remove espaços do nome
                    char* trimmed = var_name;
                    while(isspace(*trimmed)) trimmed++;
                    char* end_trim = trimmed + strlen(trimmed) - 1;
                    while(end_trim > trimmed && isspace(*end_trim)) end_trim--;
                    *(end_trim + 1) = '\0';
                    
                    // Extrai expressão
                    strcpy(expr, equal_pos + 1);
                    char* expr_end = expr + strlen(expr) - 1;
                    while(expr_end > expr && isspace(*expr_end)) expr_end--;
                    *(expr_end + 1) = '\0';
                    
                    if (strlen(trimmed) > 0 && strlen(expr) > 0) {
                        printf("Processando atribuição: %s = %s\n", trimmed, expr); // Debug
                        generate_riscv_assignment(trimmed, expr);
                    }
                }
            }
        }
    }
    
    generate_riscv_footer();
    write_output_with_line_numbers(output);
}

void write_output_with_line_numbers(FILE *output) {
    int max_line_num = code_line_count;
    int num_digits = 1;
    while (max_line_num >= 10) {
        max_line_num /= 10;
        num_digits++;
    }
    
    for (int i = 0; i < code_line_count; i++) {
        fprintf(output, "%*d: %s", num_digits, output_code[i].line_number, output_code[i].code);
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
    
    generate_riscv_code(input, output);
    
    fclose(input);
    fclose(output);
    
    printf("Código RISC-V gerado em %s\n", argv[2]);
    return 0;
}