%{
  #include <stdio.h>
  #include <string.h>

  void print_token(const char *type, const char *text);
%}

%option noyywrap

%x COMENTARIO_M

NUM             [0-9]+
EXP             ([eE][-+]?[0-9]+)
ID		          [a-zA-Z_][a-zA-Z0-9_]*
COMENTARIO  	  "//".*
STRING          \"([^\\\n]|(\\.))*?\"
CHAR            \'([^\\\n]|(\\.))*?\'
ET              [ \t]+
QL              \r?\n
KEYWORDS        auto|break|case|char|const|continue|default|do|double|else|enum|extern|float|for|goto|if|inline|int|long|register|restrict|return|short|signed|sizeof|static|struct|switch|typedef|union|unsigned|void|volatile|while|_Bool|_Complex|_Imaginary
outro		        .

%%
{ET}        {;}
{QL}        {;}

{COMENTARIO}	  {print_token("Comentario de uma linha", yytext);}
"/*"            {BEGIN(COMENTARIO_M);}

{KEYWORDS}      {print_token("Palavra reservada", yytext);}
{ID}		        {print_token("Identificador", yytext);}

{NUM}"."{NUM}{EXP}?     {print_token("Float", yytext);}
{NUM}{EXP}              {print_token("Float", yytext);}
{NUM}                   {print_token("Inteiro", yytext);}

{STRING}        {print_token("String", yytext);}
{CHAR}          {print_token("Char", yytext);}

"=="|"!="|"<="|">="|"->"|"++"|"--"|"||"|"&&"|"+="|"-="|"*="|"/="|"%="   {print_token("Operador", yytext);}
[+\-*/%=&|!<>^~?:;,.(){}\[\]]                                           {print_token("Simbolo", yytext);}

<COMENTARIO_M>{
  "*/"        {print_token("Comentario de multiplas linhas", "/*...*/"); BEGIN(INITIAL);}
  \n          {;}
  .           {;}
  <<EOF>>     {
    fprintf(stderr, "Erro léxico: comentário de múltiplas linhas não terminado\n");
    exit(1);
  }
}

{outro}		      {print_token("Desconhecido", yytext);}
%%

void print_token(const char *type, const char *text) {
  printf("<%s, \"%s\">\n", type, text);
}

int main(void) {
    yylex();  // inicia a análise léxica
    return 0;
}