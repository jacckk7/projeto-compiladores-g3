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
%token <str> NUM ID
%type <str> exp

// precedências dos operadores aritmeticos
%left '+' '-'
%left '*' '/' '%'

%token NUM
%token ID
%token OPE_REL
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
exp:		NUM				{ $$ = $1; }
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


