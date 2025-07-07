/*Analizador Lexico*/
%{
  # include "sintatico_v1.tab.h"
%}

%x COMENTARIO_M

FLOAT           [0-9]+\.[0-9]*([eE][-+]?[0-9]+)?|[0-9]+[eE][-+]?[0-9]+
INT             [0-9]+
ID		          [a-zA-Z_][a-zA-Z0-9_]*
STRING          \"([^\\\n]|(\\.))*?\"
CHAR            \'([^\\\n]|(\\.))*?\'
OPERADOR        ("=="|"!="|"<="|">="|"->"|"++"|"--"|"||"|"&&"|"+="|"-="|"*="|"/="|"%=")
coment_uma  	  "//".*
esp_tab         [ \t]+
outro		        .
%%
{FLOAT}         {yylval.str = strdup(yytext); return FLOAT;}
{INT}           {yylval.str = strdup(yytext); return INT;}
{STRING}        {yylval.str = strdup(yytext); return STRING;}
{CHAR}          {yylval.str = strdup(yytext); return CHAR;}
{OPERADOR}	    {return OPERADOR;}

{ID} {
  if      (strcmp(yytext, "auto") == 0)         return AUTO_KW;
  else if (strcmp(yytext, "break") == 0)        return BREAK_KW;
  else if (strcmp(yytext, "case") == 0)         return CASE_KW;
  else if (strcmp(yytext, "char") == 0)         return CHAR_KW;
  else if (strcmp(yytext, "const") == 0)        return CONST_KW;
  else if (strcmp(yytext, "continue") == 0)     return CONTINUE_KW;
  else if (strcmp(yytext, "default") == 0)      return DEFAULT_KW;
  else if (strcmp(yytext, "do") == 0)           return DO_KW;
  else if (strcmp(yytext, "double") == 0)       return DOUBLE_KW;
  else if (strcmp(yytext, "else") == 0)         return ELSE_KW;
  else if (strcmp(yytext, "enum") == 0)         return ENUM_KW;
  else if (strcmp(yytext, "extern") == 0)       return EXTERN_KW;
  else if (strcmp(yytext, "float") == 0)        return FLOAT_KW;
  else if (strcmp(yytext, "for") == 0)          return FOR_KW;
  else if (strcmp(yytext, "goto") == 0)         return GOTO_KW;
  else if (strcmp(yytext, "if") == 0)           return IF_KW;
  else if (strcmp(yytext, "inline") == 0)       return INLINE_KW;
  else if (strcmp(yytext, "int") == 0)          return INT_KW;
  else if (strcmp(yytext, "long") == 0)         return LONG_KW;
  else if (strcmp(yytext, "register") == 0)     return REGISTER_KW;
  else if (strcmp(yytext, "restrict") == 0)     return RESTRICT_KW;
  else if (strcmp(yytext, "return") == 0)       return RETURN_KW;
  else if (strcmp(yytext, "short") == 0)        return SHORT_KW;
  else if (strcmp(yytext, "signed") == 0)       return SIGNED_KW;
  else if (strcmp(yytext, "sizeof") == 0)       return SIZEOF_KW;
  else if (strcmp(yytext, "static") == 0)       return STATIC_KW;
  else if (strcmp(yytext, "struct") == 0)       return STRUCT_KW;
  else if (strcmp(yytext, "switch") == 0)       return SWITCH_KW;
  else if (strcmp(yytext, "typedef") == 0)      return TYPEDEF_KW;
  else if (strcmp(yytext, "union") == 0)        return UNION_KW;
  else if (strcmp(yytext, "unsigned") == 0)     return UNSIGNED_KW;
  else if (strcmp(yytext, "void") == 0)         return VOID_KW;
  else if (strcmp(yytext, "volatile") == 0)     return VOLATILE_KW;
  else if (strcmp(yytext, "while") == 0)        return WHILE_KW;
  else if (strcmp(yytext, "_Bool") == 0)        return BOOL_KW;
  else if (strcmp(yytext, "_Complex") == 0)     return COMPLEX_KW;
  else if (strcmp(yytext, "_Imaginary") == 0)   return IMAGINARY_KW;
  else {
    yylval.str = strdup(yytext);
    return ID;
  }
}

{coment_uma}	  {/* ignora */}
"/*"            {BEGIN(COMENTARIO_M);}

"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"%"             { return '%'; }
"="             { return '='; }
"&"             { return '&'; }
"|"             { return '|'; }
"!"             { return '!'; }
"<"             { return '<'; }
">"             { return '>'; }
"^"             { return '^'; }
"~"             { return '~'; }
"?"             { return '?'; }
":"             { return ':'; }
";"             { return ';'; }
","             { return ','; }
"."             { return '.'; }
"("             { return '(';}
")"             { return ')';}
"{"             { return '{'; }
"}"             { return '}'; }
"["             { return '[';}
"]"             { return ']';}
"\n"            { return '\n'; }

{esp_tab}       {;}

<COMENTARIO_M>{
  "*/"        {/* ignora */; BEGIN(INITIAL);}
  \n          {;}
  .           {;}
  <<EOF>>     {
    fprintf(stderr, "Erro léxico: comentário de múltiplas linhas não terminado\n");
    exit(1);
  }
}

{outro}		      {printf(":-(\n");}
%%

int yywrap() {
 return 1;
}