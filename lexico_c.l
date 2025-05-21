%{
  #include <stdio.h>
%}
NUM             [0-9]+
id		          [a-zA-Z_][a-zA-Z0-9_]*
ope_rel		      ("=="|"!="|"<="|">="|"<"|">")
ope_ari		      ("+"|"-"|"*"|"/"|"%")
coment_uma  	  "//".*
coment_multi  	"/*"([^*]|\*+[^*/])*\*+"/"
esp_tab         [ \t\n]+
outro		        .
%%
{NUM}           {printf("Numero\n");}
{id}		        {printf("Identificador\n");}
{ope_rel}	      {printf("Operador Relacional: %s\n", yytext);}
{ope_ari}	      {printf("Operador Aritmetico: %s\n", yytext);}
{coment_uma}	  {printf("Comentario de uma linha\n");}
{coment_multi}  {printf("Comentario de multiplas linhas\n");}
{esp_tab}       {;}
{outro}		      {printf(":-(\n");}
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
