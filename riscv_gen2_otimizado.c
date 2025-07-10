#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

#define MAX_LINE_LENGTH 256
#define MAX_VARIABLES 50
#define MAX_TEMPORARIES 100
#define MAX_REGISTERS 5  // t0-t4 disponíveis para otimização
#define FIRST_TEMP_REG 0 // Índice do primeiro registrador temporário (t0)

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
int tmpOffset = 0;  // Controla alocação de registradores

Operator op_stack[100];
int op_stack_top = -1;

CodeLine output_code[1000];
int code_line_count = 0;

// Protótipos de funções
void push_op(char op, int precedence);
Operator pop_op();
Operator peek_op();
bool is_op_stack_empty();
void add_code_line(const char *format, ...);
Variable* find_variable(const char *var_name);
int get_size_from_type(const char *type);
void add_variable(const char *var_name, const char *var_type);
void generate_riscv_header();
void generate_riscv_footer();
void generate_load_operand(const char *operand, const char *reg);
void generate_operation(char op, const char *reg1, const char *reg2, const char *reg_dest);
int get_precedence(char op);
void process_expression(const char *expr);
bool is_numeric_constant(const char *str);
void generate_riscv_assignment(const char *var_name, const char *expr);
void generate_riscv_code(FILE *input, FILE *output);
void write_output_with_line_numbers(FILE *output);
const char* get_reg_name(int index);
bool is_in_register(int temp_num);
void store_if_needed(int temp_num, const char *reg);
void load_if_needed(int temp_num, const char *reg);

// Variável global para controlar os registradores temporários
int current_temp_reg = 0;  // 0=t0, 1=t1, ..., 4=t4

// Implementação das funções

const char* get_reg_name(int index) {
    static const char* reg_names[] = {"t0", "t1", "t2", "t3", "t4"};
    if (index >= 0 && index < MAX_REGISTERS) {
        return reg_names[index];
    }
    return "t0";
}

bool is_in_register(int temp_num) {
    // tmpOffset de 0 a -4 -> registradores
    // tmpOffset <= -5 -> memória
    return (tmpOffset + temp_num) >= -4;
}

int get_register_index(int tmpOffset) {
    // Converte tmpOffset de -4 a 0 para índices 0 a 4
    return (-tmpOffset) - 1;
}

void store_if_needed(int temp_num, const char *reg) {
    if (!is_in_register(temp_num)) {
        add_code_line("    sw %s, %d(sp)  # Temp%d -> memória\n", 
                     reg, current_offset + temp_num * 4, temp_num);
    }
}

void load_if_needed(int temp_num, const char *reg) {
    if (is_in_register(temp_num)) {
        int reg_index = get_register_index(temp_num);
        if (strcmp(reg, get_reg_name(reg_index)) != 0) {
            add_code_line("    mv %s, %s  # Temp%d já em registrador\n",
                         reg, get_reg_name(reg_index), temp_num);
        }
    } else {
        add_code_line("    lw %s, %d(sp)  # Carrega Temp%d\n", 
                     reg, current_offset + temp_num * 4, temp_num);
    }
}

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
    add_code_line("    addi sp, sp, -%d  # Aloca espaço para variáveis\n", current_offset);
    add_code_line("    # Otimização: usando tmpOffset para registradores t0-t4\n");
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
            add_code_line("    lw %s, %d(sp)  # Carrega %s\n", reg, var->offset, operand);
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
    tmpOffset = 0;  // Resetar o offset para nova expressão

    for (int i = 0; i < expr_len; i++) {
        if (isspace(expr[i])) continue;
        
        if (isalnum(expr[i]) || expr[i] == '_') {
            token_pos = 0;
            while (i < expr_len && (isalnum(expr[i]) || expr[i] == '_')) {
                token[token_pos++] = expr[i++];
            }
            token[token_pos] = '\0';
            i--;
            
            int current_temp = temp_count++;
            if (is_in_register(current_temp)) {
                int reg_index = get_register_index(current_temp);
                add_code_line("    # Temp%d -> %s (tmpOffset=%d)\n", 
                             current_temp, get_reg_name(reg_index), tmpOffset);
                generate_load_operand(token, get_reg_name(reg_index));
            } else {
                add_code_line("    # Temp%d -> memória (tmpOffset=%d)\n", current_temp, tmpOffset);
                generate_load_operand(token, "t0");
                add_code_line("    sw t0, %d(sp)  # Armazena Temp%d\n", 
                            current_offset + current_temp * 4, current_temp);
            }
            
            temp_stack[++temp_stack_top] = current_temp;
            tmpOffset--;  // Decrementa para o próximo temporário
        } else if (expr[i] == '(') {
            push_op('(', 0);
        } else if (expr[i] == ')') {
            while (!is_op_stack_empty() && peek_op().op != '(') {
                Operator op = pop_op();
                
                int op2_temp = temp_stack[temp_stack_top--];
                int op1_temp = temp_stack[temp_stack_top--];
                int result_temp = temp_count++;
                
                load_if_needed(op1_temp, "t0");
                load_if_needed(op2_temp, "t1");
                generate_operation(op.op, "t0", "t1", "t2");
                
                if (is_in_register(result_temp)) {
                    int reg_index = get_register_index(result_temp);
                    add_code_line("    mv %s, t2  # Temp%d -> %s\n", 
                                get_reg_name(reg_index), result_temp, get_reg_name(reg_index));
                } else {
                    add_code_line("    sw t2, %d(sp)  # Temp%d -> memória\n", 
                                current_offset + result_temp * 4, result_temp);
                }
                
                temp_stack[++temp_stack_top] = result_temp;
                tmpOffset--;  // Decrementa para o próximo temporário
            }
            
            if (!is_op_stack_empty() && peek_op().op == '(') {
                pop_op();
            }
        } else {
            char current_op = expr[i];
            int current_prec = get_precedence(current_op);
            
            while (!is_op_stack_empty() && peek_op().precedence >= current_prec) {
                Operator op = pop_op();
                
                int op2_temp = temp_stack[temp_stack_top--];
                int op1_temp = temp_stack[temp_stack_top--];
                int result_temp = temp_count++;
                
                load_if_needed(op1_temp, "t0");
                load_if_needed(op2_temp, "t1");
                generate_operation(op.op, "t0", "t1", "t2");
                
                if (is_in_register(result_temp)) {
                    int reg_index = get_register_index(result_temp);
                    add_code_line("    mv %s, t2  # Temp%d -> %s\n", 
                                get_reg_name(reg_index), result_temp, get_reg_name(reg_index));
                } else {
                    add_code_line("    sw t2, %d(sp)  # Temp%d -> memória\n", 
                                current_offset + result_temp * 4, result_temp);
                }
                
                temp_stack[++temp_stack_top] = result_temp;
                tmpOffset--;  // Decrementa para o próximo temporário
            }
            
            push_op(current_op, current_prec);
        }
    }
    
    while (!is_op_stack_empty()) {
        Operator op = pop_op();
        
        int op2_temp = temp_stack[temp_stack_top--];
        int op1_temp = temp_stack[temp_stack_top--];
        int result_temp = temp_count++;
        
        load_if_needed(op1_temp, "t0");
        load_if_needed(op2_temp, "t1");
        generate_operation(op.op, "t0", "t1", "t2");
        
        if (is_in_register(result_temp)) {
            int reg_index = get_register_index(result_temp);
            add_code_line("    mv %s, t2  # Temp%d -> %s\n", 
                        get_reg_name(reg_index), result_temp, get_reg_name(reg_index));
        } else {
            add_code_line("    sw t2, %d(sp)  # Temp%d -> memória\n", 
                        current_offset + result_temp * 4, result_temp);
        }
        
        temp_stack[++temp_stack_top] = result_temp;
        tmpOffset--;  // Decrementa para o próximo temporário
    }
    
    // O último temporário é o resultado da expressão
    int final_temp = temp_stack[temp_stack_top];
    if (is_in_register(final_temp)) {
        int reg_index = get_register_index(final_temp);
        add_code_line("    # Resultado final em %s\n", get_reg_name(reg_index));
    } else {
        add_code_line("    lw t0, %d(sp)  # Carrega resultado final\n", 
                    current_offset + final_temp * 4);
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

void generate_arithmetic_operation(const char *var_name, const char *expr);

void generate_riscv_assignment(const char *var_name, const char *expr) {
    Variable *var = find_variable(var_name);
    if (var == NULL) return;

    // Verifica se é uma expressão aritmética
    char *op_pos = strpbrk(expr, "+-*/");
    if (op_pos != NULL) {
        char op = *op_pos;
        char left[20] = {0};
        char right[20] = {0};
        
        // Extrai operandos
        strncpy(left, expr, op_pos - expr);
        strcpy(right, op_pos + 1);
        
        // Remove espaços
        //remove_spaces(left);
        //remove_spaces(right);

        // Verifica se temos registradores suficientes (precisa de 3 livres: 2 operandos + resultado)
        if (current_temp_reg <= 2) {
            // Processa o operando esquerdo
            generate_riscv_assignment("_temp_left", left);
            const char *left_reg = get_reg_name(current_temp_reg-2);
            
            // Processa o operando direito (usa próximo registrador)
            int right_reg_index = current_temp_reg;
            generate_riscv_assignment("_temp_right", right);
            const char *right_reg = get_reg_name(right_reg_index-1);
            
            // Usa o próximo registrador para o resultado
            const char *dest_reg = get_reg_name(current_temp_reg);
            
            // Gera a operação
            switch(op) {
                case '+':
                    add_code_line("    add %s, %s, %s  # %s = %s\n",
                                dest_reg, left_reg, right_reg, var_name, expr);
                    break;
                case '-':
                    add_code_line("    sub %s, %s, %s  # %s = %s\n",
                                dest_reg, left_reg, right_reg, var_name, expr);
                    break;
                // Adicione outros operadores conforme necessário
            }
            
            current_temp_reg += 2;  // Avança 2 posições (1 para cada operando + resultado)
            return;
        }
    }

    // Código para atribuições simples
    if (current_temp_reg <= 4) {
        const char *reg_name = get_reg_name(current_temp_reg);
        
        if (expr[0] == 't' && isdigit(expr[1])) {
            add_code_line("    mv %s, %s  # %s = %s\n",
                         reg_name, expr, var_name, expr);
        } else {
            add_code_line("    li %s, %s  # %s = %s\n",
                         reg_name, expr, var_name, expr);
        }
        
        current_temp_reg++;
    } else {
        // Fallback para memória...
    }
}

// Função auxiliar para remover espaços
void remove_spaces(char *str) {
    char *dst = str;
    while (*str) {
        if (*str != ' ') {
            *dst++ = *str;
        }
        str++;
    }
    *dst = '\0';
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
                char* start = line + strlen("Atribuicao: ");
                char* end = strchr(start, '=');
                if (end) {
                    strncpy(var_name, start, end - start);
                    var_name[end - start] = '\0';
                    
                    // Remove espaços
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
    
    printf("Código RISC-V otimizado gerado em %s\n", argv[2]);
    return 0;
}