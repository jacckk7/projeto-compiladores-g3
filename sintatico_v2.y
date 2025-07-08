/* Verificando a sintaxe de programas segundo nossa GLC-exemplo */
/* considerando notacao polonesa para expressoes */
%{
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

int yylex(void);
void yyerror(const char *s);

extern FILE *yyin;
extern FILE *yyout;

char* currentType;
int semanticError1 = 0;
int semanticError2 = 0;

struct node {
	char* name; 
	char* type; 
	int used;
	int address;
	struct node* next; 
};
typedef struct node node;

struct symbolTable {
	int size;
	node* head;
};
typedef struct symbolTable symbolTable;

node* firstNode;
symbolTable ST;

void insert(symbolTable* table, char* name);

int search(symbolTable* table, char* name);

void print_table(symbolTable* table);

int isNotUsedVariable(symbolTable* table);

%}
%union {
    char *str;
}
%token <str> INT FLOAT ID
%type <str> exp

// precedências dos operadores aritmeticos
%left '+' '-'
%left '*' '/' '%'

//%token INT
//%token FLOAT
//%token ID
%token STRING
%token CHAR
%token OPERADOR
%token AUTO_KW
%token BREAK_KW
%token CASE_KW
%token CHAR_KW
%token CONST_KW
%token CONTINUE_KW
%token DEFAULT_KW
%token DO_KW
%token DOUBLE_KW
%token ELSE_KW
%token ENUM_KW
%token EXTERN_KW
%token FLOAT_KW
%token FOR_KW
%token GOTO_KW
%token IF_KW
%token INLINE_KW
%token INT_KW
%token LONG_KW
%token REGISTER_KW
%token RESTRICT_KW
%token RETURN_KW
%token SHORT_KW
%token SIGNED_KW
%token SIZEOF_KW
%token STATIC_KW
%token STRUCT_KW
%token SWITCH_KW
%token TYPEDEF_KW
%token UNION_KW
%token UNSIGNED_KW
%token VOID_KW
%token VOLATILE_KW
%token WHILE_KW
%token BOOL_KW
%token COMPLEX_KW
%token IMAGINARY_KW
%%
/* Regras definindo a GLC e acoes correspondentes */
/* neste nosso exemplo quase todas as acoes estao vazias */
input:    /* empty */
        | programa {;}
;
programa:	'{' lista_cmds '}'	{;}
		|   lista_declaracoes  {;} 
		|   lista_declaracoes '{' lista_cmds '}'
{
	printf("Sintaxe correta!\n");
	if(semanticError1) {
		printf("ERROR: Variavel nao declarada!\n");
	} else if(semanticError2) {
		printf("ERROR: Variavel ja declarada!\n");
	} else if(isNotUsedVariable(&ST)) {
		printf("WARNING: Variavel declarada nao usada!\n");
	} else {
		printf("Semantica correta!\n");
	}
};

lista_declaracoes: declaracao 
		|		   declaracao lista_declaracoes
;
declaracao:         CHAR_KW {currentType = "CHAR";} lista_ids {;} 
		|           DOUBLE_KW {currentType = "DOUBLE";} lista_ids {;} 
		|           FLOAT_KW {currentType = "FLOAT";} lista_ids	{;} 
		|           INT_KW {currentType = "INT";} lista_ids	{;} 
		|           LONG_KW {currentType = "LONG";} lista_ids {;} 
		|           SHORT_KW {currentType = "SHORT";} lista_ids {;} 
;
lista_ids:			ID ';'	
{
	if(!search(&ST, $1)) {
		insert(&ST, $1);
	} else {
		semanticError2 = 1;
	}
}

		|			ID ',' lista_ids	
{
	if(!search(&ST, $1)) {
		insert(&ST, $1);
	} else {
		semanticError2 = 1;
	}
}

lista_cmds:	cmd	';'			{;}
		| cmd ';' lista_cmds	{;}
;
cmd:		ID '=' exp		
{ //printf("Expressao analisada: %s\n", $3);free($3);
//printf("%s\n", $1);
	if(!search(&ST, $1)) {
		semanticError1 = 1;
	}
    printf("Atribuicao: %s = %s\n", $1, $3);
    free($3);
}
;
exp:		INT				{ $$ = $1; }
		| FLOAT				{ $$ = $1; }
		| ID				
{ 
	if(!search(&ST, $1)) {
		semanticError1 = 1;
	}		
					
	$$ = $1;
}
    	| exp '+' exp		{ int size = snprintf(NULL, 0, "(%s + %s)", $1, $3) + 1;
          					$$ = malloc(size);
          					snprintf($$, size, "(%s + %s)", $1, $3);
          					free($1); free($3);}
    	| exp '-' exp		{ int size = snprintf(NULL, 0, "(%s - %s)", $1, $3) + 1;
          					$$ = malloc(size);
          					snprintf($$, size, "(%s - %s)", $1, $3);
          					free($1); free($3);}
    	| exp '*' exp		{ int size = snprintf(NULL, 0, "(%s * %s)", $1, $3) + 1;
          					$$ = malloc(size);
          					snprintf($$, size, "(%s * %s)", $1, $3);
          					free($1); free($3);}
    	| exp '/' exp		{ int size = snprintf(NULL, 0, "(%s / %s)", $1, $3) + 1;
          					$$ = malloc(size);
          					snprintf($$, size, "(%s / %s)", $1, $3);
          					free($1); free($3);}
    	| exp '%' exp		{ int size = snprintf(NULL, 0, "(%s %% %s)", $1, $3) + 1;
          					$$ = malloc(size);
          					snprintf($$, size, "(%s %% %s)", $1, $3);
          					free($1); free($3);}
		| '(' exp ')'     	{ $$ = $2; }
;
%%

void initSymbolTable(symbolTable* table) {
	firstNode = (node*) malloc(sizeof(node));
    
    firstNode->name = "-1";
    firstNode->type = "-1";
    firstNode->used = -1;
	firstNode->address = -1;
    firstNode->next = NULL;

    ST.size = 0;
    ST.head = firstNode;
}

// insere um simbolo na tabela
// Modifique a função insert() para:
void insert(symbolTable* table, char* name) {
    node* n = (node*) malloc(sizeof(node));
    n->name = name;
    n->type = strdup(currentType);
    n->used = 0;
    n->address = table->size; 
    n->next = table->head;

    table->head = n;
    table->size++;

    //printf("Atribuicao: %s = 0;\n", n->name);  // Adiciona linha de inicialização
    printf("Variavel %s %s criada!\n", n->type, n->name);
}


// retorna 1 se achar o simbolo
int search(symbolTable* table, char* symbolName) {
	for(node* n = table->head; n->name != "-1"; n = n->next) {
		if(strcmp(n->name, symbolName) == 0) {
			n->used = 1;
			return 1;
		}
	}
	return 0;
} 


// printa todos os simbolos da tabela
void print_table(symbolTable* table) {
    printf("\nTabela de Símbolos:\n");
    printf("Nome\tTipo\tUsado\tEndereço\n");
    printf("------------------------------------------------\n");
    for(node* n = table->head; n->name != "-1"; n = n->next) {
        printf("%s\t\t%s\t\t%d\t\t%d\n", 
               n->name, n->type, n->used, n->address * 4);  // Multiplica por 4 para mostrar o offset real
    }
}

// retorna 1 se alguma variavel declarada nao for usada e 0 caso todas as variaveis decleradas sao usadas
int isNotUsedVariable(symbolTable* table) {
	for(node* n = table->head; n->name != "-1"; n = n->next) {
		if(n->used == 0) {
			return 1;
		}
	}
	return 0;
}

// retorna o endereco de memória da variável ou -1, caso a variavel não tenha sido declarada
int getVariableAddress(symbolTable* table, char* symbolName) {
	for(node* n = table->head; n->name != "-1"; n = n->next) {
		if(strcmp(n->name, symbolName) == 0) {
			return n->address;
		}
	}
	return -1;
}


int main(int argc, char **argv) {
	currentType = "";
	
	if(argc > 0) {
		FILE *output = fopen("input_formatado.txt", "w");
		if (output == NULL) {
			perror("Erro ao abrir o arquivo de saída");
			return 1;
		}

		char line[MAX_LINE];
		int isFirstLine = 1;

		// Ler linha a linha da entrada padrão
		while (fgets(line, sizeof(line), stdin)) {
			// Remove o \n do final da linha, se existir
			line[strcspn(line, "\r\n")] = '\0';

			// Se não for a primeira linha, escreve um espaço antes
			if (!isFirstLine) {
				fputc(' ', output);
			}

			// Escreve a linha no arquivo de saída
			fputs(line, output);

			isFirstLine = 0;
		}

		fclose(output);
	}

	yyin = fopen("input_formatado.txt","rt");

	initSymbolTable(&ST);
	yyparse();
    fclose(yyin);
	print_table(&ST);
	return 0;
}

void yyerror(const char *s) {
    printf("Problema com a analise sintatica: %s\n", s);
}