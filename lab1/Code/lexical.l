%option yylineno

%{
#include "syntax.tab.h"

int yycolumn = 1;

int isRD(char c1, char c2);
extern int treeflag;

#ifndef YY_USER_ACTION

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn; \
    yycolumn += yyleng;

#endif

%}

digit         [0-9]
digits        [0-9]*
nonZeroDigit  [1-9]
_alnum         [_0-9a-zA-Z]
_alpha        [_a-zA-Z]

%%
{nonZeroDigit}{digits}     { yylval.type_char = strdup(yytext); return INT; }
([0-9]*\.?[0-9]+|[0-9]+\.) { yylval.type_char = strdup(yytext);return FLOAT; }
struct                     { return STRUCT; }
return                     { return RETURN; }
if                         { return IF; }
else                       { yylval.type_char = strdup(yytext);return ELSE; }
while                      { return WHILE; }
int|float                  { yylval.type_char = strdup(yytext); return TYPE; }
{digit}+{_alpha}+{_alnum}* { printf("Error type A at Line %d: Invalid Identifier \'%s\'.\n", yylineno, yytext); 
                                treeflag = 0; yylval.type_char = strdup(yytext); return ID; }
{_alpha}{_alnum}*          { yylval.type_char = strdup(yytext); return ID; }
";"                        { return SEMI; } 
","                        { return COMMA; } 
"="                        { return ASSIGNOP; } 
">"|"<"|">="|"<="|"=="|"!=" { return RELOP; } 
"+"                        { return PLUS; }
"-"                        { return MINUS; }
"*"                        { return STAR; }
"/"                        { return DIV; }
"&&"                       { return AND; }
"||"                       { return OR; }
"."                        { return DOT; }
"!"                        { return NOT; }
"("                        { return LP; }
")"                        { return RP; }
"["                        { return LB; }
"]"                        { return RB; }
"{"                        { return LC; }
"}"                        { return RC; }
"//"                       { char c = input(); while (c && c != '\n') c = input(); }
"/*"                       { int lno = yylineno; char c1 = input(), c2 = input(); while (c1 && c2 && !isRD(c1,c2)) { c1 = c2; c2 = input(); } 
                                if (!isRD(c1,c2)) { printf("Error type B at Line %d: No match for symbol \'/*\'\n", lno);  treeflag = 0; } }
"*/"                       { treeflag = 0; printf("Error type B at Line %d: No match for symbol \'*/\'\n", yylineno); }
[ \t]+                     { }
\n|\r                      { yycolumn = 1; }
.                          {  treeflag = 0; printf("Error type A at Line %d: Mysterious characters \'%s\'.\n", yylineno, yytext); }

%%

int isRD(char c1, char c2) { return (c1=='*') && (c2=='/'); }
