%option yylineno

%{
#include "syntax.tab.h"

int yycolumn = 1;

int isRD(char c1, char c2);
extern int treeflag;
char *remStr[100];
char* remRelop[100];
int indRelop = -1;
int ind = -1;
int op_kind = -1;

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
0                          { remStr[++ind] = strdup(yytext); return INT; } 
{nonZeroDigit}{digits}     { remStr[++ind] = strdup(yytext); return INT; }
([0-9]*\.?[0-9]+|[0-9]+\.) { remStr[++ind] = strdup(yytext); return FLOAT; }
struct                     { return STRUCT; }
return                     { return RETURN; }
if                         { return IF; }
else                       { return ELSE; }
while                      { return WHILE; }
int|float                  { remStr[++ind] = strdup(yytext); return TYPE; }
{digit}+{_alpha}+{_alnum}* { treeflag = 0; }
{_alpha}{_alnum}*          { remStr[++ind] = strdup(yytext); return ID; }
";"                        { return SEMI; } 
","                        { return COMMA; } 
"="                        { return ASSIGNOP; } 
">"|"<"|">="|"<="|"=="|"!=" { remRelop[++indRelop] = strdup(yytext); return RELOP; } 
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
                                if (!isRD(c1,c2)) {  treeflag = 0; } }
"*/"                       { treeflag = 0; }
[ \t]+                     { }
\n|\r                      { yycolumn = 1; }
.                          {  treeflag = 0; }

%%

int isRD(char c1, char c2) { return (c1=='*') && (c2=='/'); }
