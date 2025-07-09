#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

#define MAX_LINE_LENGTH 256
#define MAX_VARIABLES 50
#define MAX_TEMPORARIES 100
#define MAX_REGISTERS 7  // t0-t6 disponíveis para temporários
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

typedef struct {
    bool used;
    int temp_number;  // Temporário associado (-1 se não associado)
} RegisterStatus;

typedef struct {
    char name[50];
    int reg; // -1 se não estiver em registrador
} VarRegMap;

Variable variables[MAX_VARIABLES];
int var_count = 0;
int current_offset = 0;
int temp_count = 0;

VarRegMap var_reg_map[MAX_VARIABLES];
int var_reg_count = 0;

Operator op_stack[100];
int op_stack_top = -1;

CodeLine output_code[1000];
int code_line_count = 0;

RegisterStatus reg_status[MAX_REGISTERS];  // Status dos registradores t0-t6

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
int allocate_register_for_temp(int temp_num);
void free_register(int reg_index);
void free_temp(int temp_num);
const char* get_reg_name(int index);
int get_var_reg(const char *var_name);
void set_var_reg(const char *var_name, int reg);
void flush_vars_to_memory();

// Implementação das funções

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

int get_var_reg(const char *var_name) {
    for (int i = 0; i < var_reg_count; i++) {
        if (strcmp(var_reg_map[i].name, var_name) == 0) {
            return var_reg_map[i].reg;
        }
    }
    return -1;
}

void set_var_reg(const char *var_name, int reg) {
    for (int i = 0; i < var_reg_count; i++) {
        if (strcmp(var_reg_map[i].name, var_name) == 0) {
            var_reg_map[i].reg = reg;
            return;
        }
    }
    if (var_reg_count < MAX_VARIABLES) {
        strcpy(var_reg_map[var_reg_count].name, var_name);
        var_reg_map[var_reg_count].reg = reg;
        var_reg_count++;
    }
}

void flush_vars_to_memory() {
    for (int i = 0; i < var_reg_count; i++) {
        if (var_reg_map[i].reg != -1) {
            Variable *var = find_variable(var_reg_map[i].name);
            if (var) {
                add_code_line("    sw %s, %d(sp)  # Armazena %s na memória\n",
                            get_reg_name(var_reg_map[i].reg), var->offset, var_reg_map[i].name);
            }
            var_reg_map[i].reg = -1;
        }
    }
}

void generate_riscv_header() {
    add_code_line(".text\n");
    add_code_line(".globl main\n");
    add_code_line("main:\n");
    add_code_line("    # Aloca espaço apenas para variáveis (%d bytes)\n", current_offset);
    add_code_line("    addi sp, sp, -%d\n", current_offset);
    add_code_line("    # Otimização: Variáveis serão mantidas em registradores quando possível\n");
}

void generate_riscv_footer() {
    // Garante que todas as variáveis estão na memória antes de sair
    flush_vars_to_memory();
    add_code_line("\n    li a7, 10\n");
    add_code_line("    ecall\n");
}

void generate_load_operand(const char *operand, const char *reg) {
    if (isdigit(operand[0])) {
        add_code_line("    li %s, %s\n", reg, operand);
    } else {
        // Primeiro verifica se a variável está em algum registrador
        int reg_index = get_var_reg(operand);
        if (reg_index != -1) {
            if (strcmp(reg, get_reg_name(reg_index)) != 0) {
                add_code_line("    mv %s, %s  # %s já está em %s\n",
                            reg, get_reg_name(reg_index), operand, get_reg_name(reg_index));
            }
            return;
        }
        
        // Se não estiver em registrador, carrega da memória
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

int allocate_register_for_temp(int temp_num) {
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (!reg_status[i].used) {
            reg_status[i].used = true;
            reg_status[i].temp_number = temp_num;
            return i;
        }
    }
    return -1;  // Nenhum registrador disponível
}

void free_register(int reg_index) {
    if (reg_index >= 0 && reg_index < MAX_REGISTERS) {
        reg_status[reg_index].used = false;
        reg_status[reg_index].temp_number = -1;
    }
}

void free_temp(int temp_num) {
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (reg_status[i].temp_number == temp_num) {
            free_register(i);
            break;
        }
    }
}

const char* get_reg_name(int index) {
    static const char* reg_names[] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6"};
    if (index >= 0 && index < MAX_REGISTERS) {
        return reg_names[index];
    }
    return "t0";
}

void process_expression(const char *expr) {
    char token[50];
    int token_pos = 0;
    int expr_len = strlen(expr);
    int temp_stack[100];
    int temp_stack_top = -1;
    op_stack_top = -1;

    for (int i = 0; i < MAX_REGISTERS; i++) {
        reg_status[i].used = false;
        reg_status[i].temp_number = -1;
    }

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
            int reg_index = allocate_register_for_temp(current_temp);
            
            if (reg_index != -1) {
                add_code_line("    # Temp%d -> %s\n", current_temp, get_reg_name(reg_index));
                
                // Verifica se é uma variável já em registrador
                int var_reg = get_var_reg(token);
                if (var_reg != -1) {
                    if (var_reg != reg_index) {
                        add_code_line("    mv %s, %s  # Usa %s já em registrador\n",
                                    get_reg_name(reg_index), get_reg_name(var_reg), token);
                    }
                } else {
                    generate_load_operand(token, get_reg_name(reg_index));
                }
            } else {
                add_code_line("    # Temp%d -> memória (sp+%d)\n", 
                            current_temp, current_offset + current_temp * 4);
                generate_load_operand(token, "t0");
                add_code_line("    sw t0, %d(sp)  # Armazena temporário\n", 
                            current_offset + current_temp * 4);
            }
            
            temp_stack[++temp_stack_top] = current_temp;
        } else if (expr[i] == '(') {
            push_op('(', 0);
        } else if (expr[i] == ')') {
            while (!is_op_stack_empty() && peek_op().op != '(') {
                Operator op = pop_op();
                
                int op2_temp = temp_stack[temp_stack_top--];
                int op1_temp = temp_stack[temp_stack_top--];
                int result_temp = temp_count++;
                
                bool op1_in_reg = false, op2_in_reg = false;
                int op1_reg = -1, op2_reg = -1;
                
                for (int i = 0; i < MAX_REGISTERS; i++) {
                    if (reg_status[i].temp_number == op1_temp) {
                        op1_in_reg = true;
                        op1_reg = i;
                    }
                    if (reg_status[i].temp_number == op2_temp) {
                        op2_in_reg = true;
                        op2_reg = i;
                    }
                }
                
                int result_reg = allocate_register_for_temp(result_temp);
                if (result_reg != -1) {
                    if (op1_in_reg && op2_in_reg) {
                        generate_operation(op.op, get_reg_name(op1_reg), get_reg_name(op2_reg), get_reg_name(result_reg));
                    } else if (op1_in_reg) {
                        add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                                    current_offset + op2_temp * 4, op2_temp);
                        generate_operation(op.op, get_reg_name(op1_reg), "t0", get_reg_name(result_reg));
                    } else if (op2_in_reg) {
                        add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                                    current_offset + op1_temp * 4, op1_temp);
                        generate_operation(op.op, "t0", get_reg_name(op2_reg), get_reg_name(result_reg));
                    } else {
                        add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                                    current_offset + op1_temp * 4, op1_temp);
                        add_code_line("    lw t1, %d(sp)  # Carrega Temp%d\n", 
                                    current_offset + op2_temp * 4, op2_temp);
                        generate_operation(op.op, "t0", "t1", get_reg_name(result_reg));
                    }
                    
                    if (op1_in_reg) free_register(op1_reg);
                    if (op2_in_reg) free_register(op2_reg);
                } else {
                    add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                                current_offset + op1_temp * 4, op1_temp);
                    add_code_line("    lw t1, %d(sp)  # Carrega Temp%d\n", 
                                current_offset + op2_temp * 4, op2_temp);
                    generate_operation(op.op, "t0", "t1", "t2");
                    add_code_line("    sw t2, %d(sp)  # Armazena Temp%d\n", 
                                current_offset + result_temp * 4, result_temp);
                }
                
                temp_stack[++temp_stack_top] = result_temp;
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
                
                bool op1_in_reg = false, op2_in_reg = false;
                int op1_reg = -1, op2_reg = -1;
                
                for (int i = 0; i < MAX_REGISTERS; i++) {
                    if (reg_status[i].temp_number == op1_temp) {
                        op1_in_reg = true;
                        op1_reg = i;
                    }
                    if (reg_status[i].temp_number == op2_temp) {
                        op2_in_reg = true;
                        op2_reg = i;
                    }
                }
                
                int result_reg = allocate_register_for_temp(result_temp);
                if (result_reg != -1) {
                    if (op1_in_reg && op2_in_reg) {
                        generate_operation(op.op, get_reg_name(op1_reg), get_reg_name(op2_reg), get_reg_name(result_reg));
                    } else if (op1_in_reg) {
                        add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                                    current_offset + op2_temp * 4, op2_temp);
                        generate_operation(op.op, get_reg_name(op1_reg), "t0", get_reg_name(result_reg));
                    } else if (op2_in_reg) {
                        add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                                    current_offset + op1_temp * 4, op1_temp);
                        generate_operation(op.op, "t0", get_reg_name(op2_reg), get_reg_name(result_reg));
                    } else {
                        add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                                    current_offset + op1_temp * 4, op1_temp);
                        add_code_line("    lw t1, %d(sp)  # Carrega Temp%d\n", 
                                    current_offset + op2_temp * 4, op2_temp);
                        generate_operation(op.op, "t0", "t1", get_reg_name(result_reg));
                    }
                    
                    if (op1_in_reg) free_register(op1_reg);
                    if (op2_in_reg) free_register(op2_reg);
                } else {
                    add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                                current_offset + op1_temp * 4, op1_temp);
                    add_code_line("    lw t1, %d(sp)  # Carrega Temp%d\n", 
                                current_offset + op2_temp * 4, op2_temp);
                    generate_operation(op.op, "t0", "t1", "t2");
                    add_code_line("    sw t2, %d(sp)  # Armazena Temp%d\n", 
                                current_offset + result_temp * 4, result_temp);
                }
                
                temp_stack[++temp_stack_top] = result_temp;
            }
            
            push_op(current_op, current_prec);
        }
    }
    
    while (!is_op_stack_empty()) {
        Operator op = pop_op();
        
        int op2_temp = temp_stack[temp_stack_top--];
        int op1_temp = temp_stack[temp_stack_top--];
        int result_temp = temp_count++;
        
        bool op1_in_reg = false, op2_in_reg = false;
        int op1_reg = -1, op2_reg = -1;
        
        for (int i = 0; i < MAX_REGISTERS; i++) {
            if (reg_status[i].temp_number == op1_temp) {
                op1_in_reg = true;
                op1_reg = i;
            }
            if (reg_status[i].temp_number == op2_temp) {
                op2_in_reg = true;
                op2_reg = i;
            }
        }
        
        int result_reg = allocate_register_for_temp(result_temp);
        if (result_reg != -1) {
            if (op1_in_reg && op2_in_reg) {
                generate_operation(op.op, get_reg_name(op1_reg), get_reg_name(op2_reg), get_reg_name(result_reg));
            } else if (op1_in_reg) {
                add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                            current_offset + op2_temp * 4, op2_temp);
                generate_operation(op.op, get_reg_name(op1_reg), "t0", get_reg_name(result_reg));
            } else if (op2_in_reg) {
                add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                            current_offset + op1_temp * 4, op1_temp);
                generate_operation(op.op, "t0", get_reg_name(op2_reg), get_reg_name(result_reg));
            } else {
                add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                            current_offset + op1_temp * 4, op1_temp);
                add_code_line("    lw t1, %d(sp)  # Carrega Temp%d\n", 
                            current_offset + op2_temp * 4, op2_temp);
                generate_operation(op.op, "t0", "t1", get_reg_name(result_reg));
            }
            
            if (op1_in_reg) free_register(op1_reg);
            if (op2_in_reg) free_register(op2_reg);
        } else {
            add_code_line("    lw t0, %d(sp)  # Carrega Temp%d\n", 
                        current_offset + op1_temp * 4, op1_temp);
            add_code_line("    lw t1, %d(sp)  # Carrega Temp%d\n", 
                        current_offset + op2_temp * 4, op2_temp);
            generate_operation(op.op, "t0", "t1", "t2");
            add_code_line("    sw t2, %d(sp)  # Armazena Temp%d\n", 
                        current_offset + result_temp * 4, result_temp);
        }
        
        temp_stack[++temp_stack_top] = result_temp;
    }
    
    // O último temporário é o resultado da expressão
    int final_temp = temp_stack[temp_stack_top];
    bool final_in_reg = false;
    int final_reg = -1;
    
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (reg_status[i].temp_number == final_temp) {
            final_in_reg = true;
            final_reg = i;
            break;
        }
    }
    
    if (final_in_reg) {
        add_code_line("    # Resultado final em %s\n", get_reg_name(final_reg));
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

void generate_riscv_assignment(const char *var_name, const char *expr) {
    Variable *var = find_variable(var_name);
    if (!var) {
        add_code_line("    # ERRO: Variável '%s' não declarada!\n", var_name);
        return;
    }

    // Atribuição de constante
    if (is_numeric_constant(expr)) {
        int reg = allocate_register_for_temp(-1); // Aloca qualquer registrador
        if (reg != -1) {
            add_code_line("    li %s, %s  # %s = %s (em registrador)\n",
                         get_reg_name(reg), expr, var_name, expr);
            set_var_reg(var_name, reg);
            return;
        }
    }
    
    // Atribuição de outra variável
    Variable *src_var = find_variable(expr);
    if (src_var) {
        int src_reg = get_var_reg(expr);
        if (src_reg != -1) {
            // Variável fonte está em registrador
            int dest_reg = allocate_register_for_temp(-1);
            if (dest_reg != -1) {
                add_code_line("    mv %s, %s  # Copia %s para %s\n",
                             get_reg_name(dest_reg), get_reg_name(src_reg), expr, var_name);
                set_var_reg(var_name, dest_reg);
                return;
            }
        }
        
        // Caso geral - carrega e armazena
        add_code_line("    lw t0, %d(sp)  # Carrega %s\n", src_var->offset, expr);
        add_code_line("    sw t0, %d(sp)  # Armazena em %s\n", var->offset, var_name);
        return;
    }

    // Expressão complexa
    process_expression(expr);
    
    // Verifica se o resultado está em registrador
    bool result_in_reg = false;
    int result_reg = -1;
    
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (reg_status[i].temp_number == temp_count - 1) {
            result_in_reg = true;
            result_reg = i;
            break;
        }
    }
    
    if (result_in_reg) {
        // Mantém no registrador se possível
        set_var_reg(var_name, result_reg);
        add_code_line("    # %s mantido em %s\n", var_name, get_reg_name(result_reg));
    } else {
        // Armazena na memória
        add_code_line("    lw t0, %d(sp)  # Carrega resultado\n", current_offset + (temp_count-1)*4);
        add_code_line("    sw t0, %d(sp)  # Armazena em %s\n", var->offset, var_name);
    }
}

void generate_riscv_code(FILE *input, FILE *output) {
    char line[MAX_LINE_LENGTH];
    char var_name[50];
    char var_type[20];
    char expr[100];
    
    // Inicializa registradores
    for (int i = 0; i < MAX_REGISTERS; i++) {
        reg_status[i].used = false;
        reg_status[i].temp_number = -1;
    }
    var_reg_count = 0;
    
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