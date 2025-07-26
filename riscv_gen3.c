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
#define MAX_STRINGS 100  

void write_output_with_line_numbers(FILE *output);

typedef struct {
    int label;
    char* value;
} StringEntry;

typedef struct {
    char name[50];
    char type[20];
    int offset;
    int size;
    bool is_const;
    bool is_static;
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
int label_count = 0;
int current_depth = 0;

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

void add_code_line(const char *format,...) {
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
    if (strcmp(type, "CHAR") == 0 || strcmp(type, "CHAR_KW") == 0) return 1;
    if (strcmp(type, "SHORT") == 0 || strcmp(type, "SHORT_KW") == 0) return 2;
    if (strcmp(type, "INT") == 0 || strcmp(type, "INT_KW") == 0) return 4;
    if (strcmp(type, "FLOAT") == 0 || strcmp(type, "FLOAT_KW") == 0) return 4;
    if (strcmp(type, "DOUBLE") == 0 || strcmp(type, "DOUBLE_KW") == 0) return 8;
    if (strcmp(type, "LONG") == 0 || strcmp(type, "LONG_KW") == 0) return 8;
    if (strcmp(type, "BOOL_KW") == 0) return 1;
    if (strcmp(type, "STRING") == 0) return 4; // Ponteiro para string
    return 4; // padrão
}

void add_variable(const char *var_name, const char *var_type, bool is_const, bool is_static) {
    if (find_variable(var_name)) return;
    
    if (var_count < MAX_VARIABLES) {
        strcpy(variables[var_count].name, var_name);
        strcpy(variables[var_count].type, var_type);
        variables[var_count].size = get_size_from_type(var_type);
        variables[var_count].offset = current_offset;
        variables[var_count].is_const = is_const;
        variables[var_count].is_static = is_static;
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
    add_code_line("    li a7, 10\n");
    add_code_line("    ecall\n");
}

void generate_load_operand(const char *operand, const char *reg) {
    if (isdigit(operand[0])) {
        add_code_line("    li %s, %s\n", reg, operand);
    } else if (operand[0] == '\'') { // Caractere
        add_code_line("    li %s, %d\n", reg, operand[1]);
    } else if (operand[0] == '"') { // String (tratada como ponteiro)
        add_code_line("    la %s, %s\n", reg, operand);
    } else {
        Variable *var = find_variable(operand);
        if (var) {
            if (strcmp(var->type, "FLOAT") == 0 || strcmp(var->type, "FLOAT_KW") == 0) {
                add_code_line("    flw %s, %d(sp)\n", reg, var->offset);
            } else if (strcmp(var->type, "DOUBLE") == 0 || strcmp(var->type, "DOUBLE_KW") == 0) {
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
        case '&':
            add_code_line("    and %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '|':
            add_code_line("    or %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '^':
            add_code_line("    xor %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '=': // ==
            add_code_line("    xor %s, %s, %s\n", reg_dest, reg1, reg2);
            add_code_line("    seqz %s, %s\n", reg_dest, reg_dest);
            break;
        case '!': // !=
            add_code_line("    xor %s, %s, %s\n", reg_dest, reg1, reg2);
            add_code_line("    snez %s, %s\n", reg_dest, reg_dest);
            break;
        case '<':
            add_code_line("    slt %s, %s, %s\n", reg_dest, reg1, reg2);
            break;
        case '>':
            add_code_line("    sgt %s, %s, %s\n", reg_dest, reg1, reg2);
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
        case '&':
        case '|':
        case '^':
            return 3;
        case '=':
        case '!':
        case '<':
        case '>':
            return 4;
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

void generate_if_statement(const char *condition, const char *true_block) {
    int current_label = label_count++;
    add_code_line("    # Início do if\n");
    process_expression(condition);
    add_code_line("    lw t0, %d(sp)\n", TEMP_RESULT_OFFSET);
    add_code_line("    beqz t0, L_else_%d\n", current_label);
    
    // Gera o bloco verdadeiro (simplificado)
    add_code_line("    # Bloco if\n");
    
    add_code_line("L_else_%d:\n", current_label);
}

void generate_while_loop(const char *condition, const char *body) {
    int current_label = label_count++;
    add_code_line("    # Início do while\n");
    add_code_line("L_while_start_%d:\n", current_label);
    
    process_expression(condition);
    add_code_line("    lw t0, %d(sp)\n", TEMP_RESULT_OFFSET);
    add_code_line("    beqz t0, L_while_end_%d\n", current_label);
    
    // Gera o corpo (simplificado)
    add_code_line("    # Corpo do while\n");
    
    add_code_line("    j L_while_start_%d\n", current_label);
    add_code_line("L_while_end_%d:\n", current_label);
}

void generate_return(const char *expr) {
    if (expr) {
        process_expression(expr);
        add_code_line("    lw a0, %d(sp)  # Valor de retorno\n", TEMP_RESULT_OFFSET);
    }
    add_code_line("    j main_end\n");
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

    if (var->is_const) {
        add_code_line("    # AVISO: Tentativa de modificar constante '%s'!\n", var_name);
        return;
    }

    // Atribuição simples (constante numérica)
    if (is_numeric_constant(expr)) {
        add_code_line("    li t0, %s\n", expr);
        add_code_line("    sw t0, %d(sp)  # %s = %s\n", var->offset, var_name, expr);
        return;
    }
    
    // Atribuição de outra variável (cópia direta)
    Variable *src_var = find_variable(expr);
    if (src_var) {
        add_code_line("    lw t0, %d(sp)  # Carrega %s\n", src_var->offset, expr);
        add_code_line("    sw t0, %d(sp)  # %s = %s\n", var->offset, var_name, expr);
        return;
    }

    // Expressão aritmética mais complexa
    add_code_line("    # Calculando %s = %s\n", var_name, expr);
    process_expression(expr);
    
    // Otimização: usar posição temporária fixa para resultados
    add_code_line("    lw t0, %d(sp)  # Carrega resultado\n", TEMP_RESULT_OFFSET);
    add_code_line("    sw t0, %d(sp)  # Armazena em %s\n", var->offset, var_name);
    
    add_code_line("    sw zero, %d(sp)  # Limpa temporário\n", TEMP_RESULT_OFFSET);
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
const char* get_expression_type(const char* expr) {
    // 1. Verifica se é uma constante char ('a')
    if (expr[0] == '\'') {
        return "CHAR";
    }
    
    // 2. Verifica se é uma string literal ("texto")
    if (expr[0] == '"') {
        return "STRING";
    }
    
    // 3. Verifica se é um número float (contém ponto ou notação científica)
    const char* p = expr;
    while (*p) {
        if (*p == '.' || *p == 'e' || *p == 'E') {
            return "FLOAT";
        }
        p++;
    }
    
    // 4. Verifica se é uma variável declarada
    Variable* var = find_variable(expr);
    if (var != NULL) {
        // Retorna o tipo da variável, removendo sufixo "_KW" se existir
        if (strstr(var->type, "_KW")) {
            static char clean_type[20];
            strncpy(clean_type, var->type, strlen(var->type) - 3);
            clean_type[strlen(var->type) - 3] = '\0';
            return clean_type;
        }
        return var->type;
    }
    
    // 5. Verifica operações com floats em expressões mais complexas
    if (strstr(expr, ".") || 
        strstr(expr, "/") ||  // Divisão pode resultar em float
        strstr(expr, "*") ||  // Multiplicação entre int e float
        strstr(expr, "+") ||  // Soma entre int e float
        strstr(expr, "-")) {   // Subtração entre int e float
        // Verifica se algum operando é float
        char* tokens = strdup(expr);
        char* token = strtok(tokens, " +-*/%");
        while (token != NULL) {
            if (strchr(token, '.')) {
                free(tokens);
                return "FLOAT";
            }
            Variable* v = find_variable(token);
            if (v && (strcmp(v->type, "FLOAT") == 0 || strcmp(v->type, "FLOAT_KW") == 0)) {
                free(tokens);
                return "FLOAT";
            }
            token = strtok(NULL, " +-*/%");
        }
        free(tokens);
    }
    
    // 6. Padrão: inteiro
    return "INT";
}

void process_condition(const char* condition, int label_base, bool is_while) {
    char left[50], op[3], right[50];
    char left_type[20], right_type[20];
    
    // Extrai os componentes da condição
    if (sscanf(condition, "%49s %2s %49s", left, op, right) != 3) {
        add_code_line("    # ERRO: Condição mal formada: %s\n", condition);
        return;
    }

    // Determina os tipos dos operandos
    strcpy(left_type, get_expression_type(left));
    strcpy(right_type, get_expression_type(right));

    // Gera código para carregar os operandos
    add_code_line("    # Avaliando condição: %s %s %s\n", left, op, right);
    
    // Tratamento especial para tipos mistos
    bool float_comp = (strcmp(left_type, "FLOAT")) == 0 || strcmp(right_type, "FLOAT") == 0;
    
    // Carrega operando esquerdo
    if (strcmp(left_type, "FLOAT") == 0) {
        if (find_variable(left)) {
            add_code_line("    flw ft0, %d(sp)  # %s\n", find_variable(left)->offset, left);
        } else {
            add_code_line("    li t0, %s\n", left);
            add_code_line("    fmv.w.x ft0, t0\n");
        }
    } else {
        generate_load_operand(left, "t0");
    }

    // Carrega operando direito
    if (strcmp(right_type, "FLOAT") == 0) {
        if (find_variable(right)) {
            add_code_line("    flw ft1, %d(sp)  # %s\n", find_variable(right)->offset, right);
        } else {
            add_code_line("    li t1, %s\n", right);
            add_code_line("    fmv.w.x ft1, t1\n");
        }
    } else {
        generate_load_operand(right, "t1");
    }

    // Gera a comparação apropriada
    if (float_comp) {
        // Comparação entre floats
        if (strcmp(op, "==") == 0) {
            add_code_line("    feq.s t2, ft0, ft1\n");
            add_code_line("    beqz t2, L_false_%d\n", label_base);
        } else if (strcmp(op, "!=") == 0) {
            add_code_line("    feq.s t2, ft0, ft1\n");
            add_code_line("    bnez t2, L_false_%d\n", label_base);
        } else if (strcmp(op, "<") == 0) {
            add_code_line("    flt.s t2, ft0, ft1\n");
            add_code_line("    beqz t2, L_false_%d\n", label_base);
        } else if (strcmp(op, ">") == 0) {
            add_code_line("    flt.s t2, ft1, ft0\n");
            add_code_line("    beqz t2, L_false_%d\n", label_base);
        } else if (strcmp(op, "<=") == 0) {
            add_code_line("    fle.s t2, ft0, ft1\n");
            add_code_line("    beqz t2, L_false_%d\n", label_base);
        } else if (strcmp(op, ">=") == 0) {
            add_code_line("    fle.s t2, ft1, ft0\n");
            add_code_line("    beqz t2, L_false_%d\n", label_base);
        }
    } else {
        // Comparação entre inteiros
        if (strcmp(op, "==") == 0) {
            add_code_line("    bne t0, t1, L_false_%d\n", label_base);
        } else if (strcmp(op, "!=") == 0) {
            add_code_line("    beq t0, t1, L_false_%d\n", label_base);
        } else if (strcmp(op, "<") == 0) {
            add_code_line("    bge t0, t1, L_false_%d\n", label_base);
        } else if (strcmp(op, ">") == 0) {
            add_code_line("    ble t0, t1, L_false_%d\n", label_base);
        } else if (strcmp(op, "<=") == 0) {
            add_code_line("    bgt t0, t1, L_false_%d\n", label_base);
        } else if (strcmp(op, ">=") == 0) {
            add_code_line("    blt t0, t1, L_false_%d\n", label_base);
        }
    }

    // Para loops while, adiciona um jump de volta ao início
    if (is_while) {
        add_code_line("    j L_loop_start_%d\n", label_base);
    }

    // Label para o caso falso
    add_code_line("L_false_%d:\n", label_base);
}

void generate_riscv_code(FILE *input, FILE *output) {
    char line[MAX_LINE_LENGTH];
    char var_name[50];
    char var_type[20];
    char expr[100];
    char format_str[100];
    int str_label_count = 0;
    int float_temp_count = 0;

    // Tabela para armazenar strings já definidas
    StringEntry string_table[MAX_STRINGS];
    int string_count = 0;

    // Primeira passada: coletar variáveis
    while (fgets(line, sizeof(line), input)) {
        if (sscanf(line, "Variavel %s %s criada!", var_type, var_name) == 2) {
            add_variable(var_name, var_type, false, false);
        }
    }
    
    rewind(input);
    generate_riscv_header();
    
    // Adiciona seção de dados para strings constantes
    add_code_line(".section .rodata\n");
    
 // Segunda passada: processamento
    while (fgets(line, sizeof(line), input)) {
        line[strcspn(line, "\n")] = 0;
        char *trimmed_line = line;
        while(isspace(*trimmed_line)) trimmed_line++;

        // Processa condicionais
        if (strstr(line, "Condicional if:")) {
            char *cond = strchr(line, ':') + 2;
            add_code_line("    # Condicional if\n");
            process_condition(cond, label_count, false);
            add_code_line("L_if_%d:\n", label_count);
            label_count++;
            current_depth++;
        }
        
        if (strstr(line, "Condicional if-else:")) {
            char *cond = strchr(line, ':') + 2;
            add_code_line("    # Condicional if-else\n");
            process_condition(cond, label_count, false);
            add_code_line("L_else_%d:\n", label_count);
            label_count++;
            current_depth++;
        }
        
        if (strstr(line, "Loop while:")) {
            char *cond = strchr(line, ':') + 2;
            add_code_line("    # Loop while\n");
            add_code_line("L_while_start_%d:\n", label_count);
            process_condition(cond, label_count, true);
            add_code_line("L_while_end_%d:\n", label_count);
            label_count++;
            current_depth++;
        }
        
        if (strlen(trimmed_line) == 0) continue;
        
        // Processa printf
        if (strstr(trimmed_line, "Comando printf:")) {
            char *start = strchr(trimmed_line, ':') + 1;
            while(isspace(*start)) start++;
            
            // Verifica se é string formatada
            if (start[0] == '"') {
                char *end_quote = strchr(start + 1, '"');
                if (end_quote) {
                    *end_quote = '\0';
                    strncpy(format_str, start + 1, end_quote - start - 1);
                    format_str[end_quote - start - 1] = '\0';
                    
                    // Adiciona string na seção .rodata
                    add_code_line("str_%d: .string \"%s\"\n", str_label_count, format_str);
                    
                    // Gera chamada para printf
                    add_code_line("    # Chamada printf\n");
                    add_code_line("    la a0, str_%d\n", str_label_count);
                    add_code_line("    li a7, 4\n");  // Código do sistema para print string
                    add_code_line("    ecall\n");
                    
                    str_label_count++;
                }
            } 
            // Se for uma expressão simples (sem string de formato)
            else {
                process_expression(start);
                add_code_line("    # Print de expressão\n");
                add_code_line("    lw a0, %d(sp)\n", TEMP_RESULT_OFFSET);
                
                add_code_line("    li a7, 1\n");  // Código para print_int
                add_code_line("    ecall\n");
            }
            continue;
        }
        
        // Processa scanf
        if (strstr(trimmed_line, "Comando scanf:")) {
            char *start = strchr(trimmed_line, ':') + 1;
            while(isspace(*start)) start++;
            
            // Extrai a variável (formato simplificado: "format", var)
            char *comma = strchr(start, ',');
            if (comma) {
                *comma = '\0';
                char *var_start = comma + 1;
                while(isspace(*var_start)) var_start++;
                
                Variable *var = find_variable(var_start);
                if (var) {
                    add_code_line("    # Chamada scanf\n");
                    add_code_line("    addi a0, sp, %d\n", var->offset);  // Endereço da variável
                    
                    // Determina o tipo de scanf com base no tipo da variável
                    if (strcmp(var->type, "INT") == 0 || strcmp(var->type, "INT_KW") == 0) {
                        add_code_line("    li a7, 5\n");  // Código para read_int
                    } 
                    else if (strcmp(var->type, "FLOAT") == 0 || strcmp(var->type, "FLOAT_KW") == 0) {
                        add_code_line("    li a7, 6\n");  // Código para read_float
                    }
                    else {
                        add_code_line("    # ERRO: Tipo não suportado no scanf\n");
                        continue;
                    }
                    
                    add_code_line("    ecall\n");
                    
                    // Para float, precisamos armazenar o resultado
                    if (strcmp(var->type, "FLOAT") == 0 || strcmp(var->type, "FLOAT_KW") == 0) {
                        add_code_line("    fsw fa0, %d(sp)\n", var->offset);
                    }
                } else {
                    add_code_line("    # ERRO: Variável '%s' não declarada\n", var_start);
                }
            }
            continue;
        }
        // Ignora linhas vazias e comentários/metadados
        if (strlen(trimmed_line) == 0 || 
            strstr(trimmed_line, "Sintaxe") || 
            strstr(trimmed_line, "Semantica") ||
            strstr(trimmed_line, "Name")) continue;
        
        // Processa declarações (já tratadas)
        if (strstr(trimmed_line, "Variavel")) continue;
        
        // Processa atribuições
        if (strstr(trimmed_line, "Atribuicao:")) {
            char* equal_pos = strchr(trimmed_line, '=');
            if (equal_pos) {
                // Extrai nome da variável
                char* start = trimmed_line + strlen("Atribuicao: ");
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
                        generate_riscv_assignment(trimmed, expr);
                    }
                }
            }
            continue;
        }
    }
    
    generate_riscv_footer();
    write_output_with_line_numbers(output);
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
