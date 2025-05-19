%{
#include <stdio.h>
%}
id		[a-zA-Z_][a-zA-Z0-9_]*\n
ope_rel		("=="|"!="|"<="|">="|"<"|">")\n
ope_ari		("+"|"-"|"*"|"/"|"%")\n
coment_uma  	"//".*\n
coment_multi  	"/*"([^*]|\*+[^*/])*\*+"/"\n
outro		.*\n
%%
{id}		{printf("Identificador\n");}
{ope_rel}	{printf("Operador Relacional: %s\n", yytext);}
{ope_ari}	{printf("Operador Aritmetico: %s\n", yytext);}
{coment_uma}	{printf("Comentario de uma linha\n");}
{coment_multi}  {printf("Comentario de multiplas linhas\n");}
{outro}		{printf(":-(\n");}
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
