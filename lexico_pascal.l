%{
#include <stdio.h>
%}

ope_ari         ("+"|"-"|"*"|"/"|"div"|"mod")\n
ope_rel         ("="|"<>"|"<="|">="|"<"|">")\n
coment_uma	"//".*\n
coment_multi1   \(\*([^*]|\*+[^*)])*\*+\)\n
coment_multi2	\{[^\}]*\}\n
id              [a-zA-Z_][a-zA-Z0-9_]*\n
outro           .*\n

%%
{ope_ari}       {printf("Operador Aritmetico: %s\n", yytext);}
{ope_rel}       {printf("Operador Relacional: %s\n", yytext);}
{coment_uma}    {printf("Comentario de uma linha\n");}
{coment_multi1} {printf("Comentario de multiplas linhas\n");}
{coment_multi2} {printf("Comentario de multiplas linhas\n");}
{id}            {printf("Identificador\n");}
{outro}         {printf(":-(\n"); }
%%
/* recomendavel declarar sempre
   funcao yywrap() */
int yywrap ();

/* programa principal */
int main() {
  yylex(); /* scanner gerado por Flex */
}

int yywrap() {
 return 1;
}
