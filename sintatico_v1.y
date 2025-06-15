/* Verificando a sintaxe de programas segundo nossa GLC-exemplo */
/* considerando notacao polonesa para expressoes */
%{
#include <stdio.h> 
int yylex(void);
void yyerror(const char *s);

extern FILE *yyin;
extern FILE *yyout;

char* currentType = "";
int semanticError = 0;

struct node {
	char* name; 
	char* type; 
	int used; 
	struct node* next; 
};
typedef struct node node;

struct symbolTable {
	int size;
	node* head;
};
typedef struct symbolTable symbolTable;

node* firstNode = (node*) malloc(sizeof(node));
firstNode->name = "-1";
firstNode->type = "-1";
firstNode->used = -1;
firstNode->next = NULL;
symbolTable ST = {0, firstNode};

void insert(symbolTable* table, char* name, char* type);

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

%token INT
%token FLOAT
%token ID
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
        | input line
;
line:     '\n'
        | programa '\n'  {;}
;
programa:	'{' lista_cmds '}'	{;}
		|   lista_declaracoes '{' lista_cmds '}' 
{
	if(semanticError) {
		print("Sintaxe correta!\n");
		printf("ERROR: Variavel não declarada!\n");
	} else {
		if(isNotUsedVariable(&ST)) {
			printf("WARNING: Variavel declarada não usada!\n");
		}
	}
};

lista_declaracoes: declaracao 
		|		   declaracao lista_declaracoes
;
declaracao:         CHAR_KW lista_ids	{currentType = "CHAR";printf ("declaracao CHAR\n");} 
		|           DOUBLE_KW lista_ids	{currentType = "DOUBLE";printf ("declaracao DOUBLE\n");} 
		|           FLOAT_KW lista_ids	{currentType = "FLOAT";printf ("declaracao FLOAT\n");} 
		|           INT_KW lista_ids	{currentType = "INT";printf ("declaracao INT\n");} 
		|           LONG_KW lista_ids	{currentType = "LONG";printf ("declaracao LONG\n");} 
		|           SHORT_KW lista_ids	{currentType = "SHORT";printf ("declaracao SHORT\n");} 
;
lista_ids:			ID ';'	{insert(&ST, $1, currentType);}				
		|			ID ',' lista_ids	{insert(&ST, $1, currentType);}							

lista_cmds:	cmd	';'			{;}
		| cmd ';' lista_cmds	{;}
;
cmd:		ID '=' exp		
{// printf("Expressao analisada: %s\n", $3);free($3);
	if(!search(&ST, $1)) {
		semanticError = 1;
	}
}
;
exp:		INT				{ $$ = $1; }
		| FLOAT				{ $$ = $1; }
		| ID				
{ 
	if(!search(&ST, $1)) {
		semanticError = 1;
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

void insert(symbolTable* table, char* name, char* type) {
	node* n = (node*) malloc(sizeof(node));
	n->name = name;
	n->type = type;
	n->used = 0;
	n->next = table->head;

	table->head = n;
	table->size++;

	printf("Variavel %s criada!\n", n->name);
}

int search(symbolTable* table, char* symbolName) {
	for(node* n = table->head; n->name != "-1"; n = n->next) {
		if(n->name == symbolName) {
			n->used = 1;
			return 1;
		}
	}
	return 0;
} 

void print_table(symbolTable* table) {
	printf("Name\t\tType\t\tUsed\n");
	for(node* n = table->head; n->name != "-1"; n = n->next) {
		printf("%s\t\t%s\t\t%d\n", n->name, n->type, n->used);
	}
}


int isNotUsedVariable(symbolTable* table) {
	for(node* n = table->head; n->name != "-1"; n = n->next) {
		if(n->used == 0) {
			return 1;
		}
	}
	return 0;
}


main(int argc, char **argv) {
	++argv; --argc; 	    /* abre arquivo de entrada se houver */
	if(argc > 0)
		yyin = fopen(argv[0],"rt");
	else
		yyin = stdin;    /* cria arquivo de saida se especificado */
	if(argc > 1)
		yyout = fopen(argv[1],"wt");
	else
		yyout = stdout;
	yyparse();
	fclose(yyin);
	fclose(yyout);
}

void yyerror(const char *s) {
    printf("Problema com a analise sintatica: %s\n", s);
}


