/* Verificando a sintaxe de programas segundo nossa GLC-exemplo */
/* considerando notacao polonesa para expressoes */
%{
#include <stdio.h> 
int yylex(void);
void yyerror(const char *s);
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
        | programa '\n'  { printf ("Programa sintaticamente correto!\n"); }
;
programa:	'{' lista_cmds '}'	{;}
		|   lista_declaracoes '{' lista_cmds '}' {;}
;
lista_declaracoes: declaracao 
		|		   declaracao lista_declaracoes
;
declaracao:         CHAR_KW lista_ids	{printf ("declaracao CHAR\n");} 
		|           DOUBLE_KW lista_ids	{printf ("declaracao DOUBLE\n");} 
		|           FLOAT_KW lista_ids	{printf ("declaracao FLOAT\n");} 
		|           INT_KW lista_ids	{printf ("declaracao INT\n");} 
		|           LONG_KW lista_ids	{printf ("declaracao LONG\n");} 
		|           SHORT_KW lista_ids	{printf ("declaracao SHORT\n");} 
;
lista_ids:			ID ';'				{;}				
		|			ID ',' lista_ids	{;}							

lista_cmds:	cmd	';'			{;}
		| cmd ';' lista_cmds	{;}
;
cmd:		ID '=' exp		{printf("Expressao analisada: %s\n", $3);free($3);}
;
exp:		INT				{ $$ = $1; }
		| FLOAT				{ $$ = $1; }
		| ID				{ $$ = $1; }
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
int main(void) {
    yyparse();
    return 0;
}
void yyerror(const char *s) {
    printf("Problema com a analise sintatica: %s\n", s);
}


