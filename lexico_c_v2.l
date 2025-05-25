/*Analizador Lexico*/
%{
  # include "sintatico.tab.h"
%}
NUM             [0-9]+
id		          [a-zA-Z_][a-zA-Z0-9_]*
ope_rel		      ("=="|"!="|"<="|">="|"<"|">")
coment_uma  	  "//".*
coment_multi  	"/*"([^*]|\*+[^*/])*\*+"/"
esp_tab         [ \t]+
outro		        .
%%
{NUM}           {yylval.str = strdup(yytext); return NUM;}
{id}		        {yylval.str = strdup(yytext); return ID;}
{ope_rel}	      {return OPE_REL;}
{coment_uma}	  {/* ignora */}
{coment_multi}  {/* ignora */}
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"%"             { return '%'; }
"{"             { return '{'; }
"}"             { return '}'; }
"="             { return '='; }
"\n"            { return '\n'; }
";"             { return ';';}
"("             { return '(';}
")"             { return ')';}
{esp_tab}       {;}
{outro}		      {printf(":-(\n");}
%%

int yywrap() {
 return 1;
}
