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
%token <str> INT ID
%type <str> exp

// precedências dos operadores aritmeticos
%left '+' '-'
%left '*' '/' '%'

%token FLOAT
%token STRING
%token CHAR
%token OPERADOR
%token AUTO_KW BREAK_KW CASE_KW CHAR_KW CONST_KW CONTINUE_KW DEFAULT_KW DO_KW
%token DOUBLE_KW ELSE_KW ENUM_KW EXTERN_KW FLOAT_KW FOR_KW GOTO_KW IF_KW
%token INLINE_KW INT_KW LONG_KW REGISTER_KW RESTRICT_KW RETURN_KW SHORT_KW
%token SIGNED_KW SIZEOF_KW STATIC_KW STRUCT_KW SWITCH_KW TYPEDEF_KW UNION_KW
%token UNSIGNED_KW VOID_KW VOLATILE_KW WHILE_KW BOOL_KW COMPLEX_KW IMAGINARY_KW

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
;
lista_cmds:	cmd	';'			{;}
		| cmd ';' lista_cmds	{;}
;
cmd:		ID '=' exp		{printf("Expressao analisada: %s\n", $3);free($3);}
;
exp:		INT				{ $$ = $1; }
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
