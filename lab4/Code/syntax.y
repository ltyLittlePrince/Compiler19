%locations
%{

#define YYDEBUG 1
#include "lex.yy.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct Node* Root;
int treeflag = 1;

int nerrors = 0;

extern int yyleng;
extern char* yytext;
extern int yycolumn;
extern int yylineno;
extern char* remStr[100];
extern char* remRelop[100];
extern int ind;
extern int indRelop;
extern int op_kind;

struct Type;

struct Node {
	int flag;        // non-terminal: 0, int: 1, float: 2, ID: 3, TYPE: 4, STRUCT: 5, terminal: 6
	int line;        // position (of first child morpheme)
	char type[16];   // morpheme
	char text[40];   // ID
	struct Node* left; 
	struct Node* right;
    struct Node* parent;
    struct Node* structDef;
    // Useful types
    int* lineNum;     // DefList, for Var's definition line Number. errorReport   
    struct Type* tp;  // Used by a few kind of Nodes. 
    struct Type** tps;
    char** util;      // For convenience, used occasionally (for passing list of names)
    int   util2;      // used together with 'util' and 'tps' (indicator of their size)
    float util3;    
    int nchild;       // number of children
};

void yyerror(char *str);
struct Node* GenerateANode(const char str[20], int flag);
void connect (struct Node* node1, struct Node* node2, int flag);
void printTree(struct Node* head, int counts);
int getLineNum(struct Node* head);

%}


%union {
    char *type_char;
    struct Node* nodes;
}

%token <nodes> COMMA LD RD 
%token <nodes> IF WHILE TYPE ID SEMI
%token <nodes> LC RC RETURN STRUCT
%token <nodes> ASSIGNOP OR AND RELOP PLUS 
%token <nodes> MINUS STAR DIV NOT 
%token <nodes> DOT LB RB LP RP
%token <nodes> INT ELSE
%token <nodes> FLOAT 

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT 
%left DOT LC RC LB RB LP RP

%type <nodes> Program ExtDefList ExtDef ExtDecList
%type <nodes> Specifier StructSpecifier OptTag Tag
%type <nodes> VarDec FunDec VarList ParamDec
%type <nodes> CompSt StmtList Stmt DefList 
%type <nodes> Def DecList Dec Exp Args 
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

Program : ExtDefList  {
    $$ = GenerateANode( "Program", 0);
    connect($$, $1, 1);
    $$->line = getLineNum($$->left); $1->parent = $$;
	Root = $$;
    $$->parent = NULL;
    $$->nchild = 1;
    //printTree($$, 0);
}
        ;
ExtDefList : ExtDef ExtDefList  {
    $$ = GenerateANode( "ExtDefList", 0);
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
}
           |  {
    $$ = GenerateANode( "ExtDefList", 0);
    $$->line = getLineNum($$->left);
    $$->nchild = 0;
}
           ;
ExtDef : Specifier ExtDecList SEMI  {
    $$ = GenerateANode( "ExtDef", 0);
    $3 = GenerateANode("SEMI", 6);
    $3->line = @3.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
       | Specifier SEMI  {
    $$ = GenerateANode( "ExtDef", 0);
    $2 = GenerateANode( "SEMI", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
}
       | Specifier FunDec CompSt  {
    $$ = GenerateANode( "ExtDef", 0);
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}      
       ;

ExtDecList : VarDec  {
    $$ = GenerateANode( "ExtDecList", 0);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
           | VarDec COMMA ExtDecList  {
    $$ = GenerateANode( "ExtDecList", 0);
    $2 = GenerateANode( "COMMA", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
           ;

Specifier : TYPE  {
    $$ = GenerateANode( "Specifier", 0);
    $1 = GenerateANode( "TYPE", 4);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
          | StructSpecifier  {
    $$ = GenerateANode( "Specifier", 0);
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
          ;
StructSpecifier : STRUCT OptTag LC DefList RC  {
    $$ = GenerateANode( "StructSpecifier", 0);
    $1 = GenerateANode( "STRUCT", 5);
    $1->line = @1.first_line;
    $3 = GenerateANode( "LC", 6);
    $3->line = @3.first_line;
    $5 = GenerateANode( "RC", 6);
    $5->line = @5.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    connect($4, $5, 0); $5->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 5;
}
                | STRUCT Tag  {
    $$ = GenerateANode( "StructSpecifier", 0);
    $1 = GenerateANode( "STRUCT", 5);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
}
                ;
OptTag : ID  {
    $$ = GenerateANode( "OptTag", 0);
    $1 = GenerateANode( "ID", 3);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
       | {
    $$ = GenerateANode( "OptTag", 0);
    $$->line = getLineNum($$->left);
    $$->nchild = 0;
}
       ;
Tag : ID  {
    ind--;
    $$ = GenerateANode( "Tag", 0);
    $1 = GenerateANode( "ID", 3);
    strcpy(remStr[ind + 1], remStr[ind + 2]);
    ind++;
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
    ;

VarDec : ID  {
    $$ = GenerateANode( "VarDec", 0);
    $1 = GenerateANode( "ID", 3);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
       | VarDec LB INT RB  {
    $$ = GenerateANode( "VarDec", 0);
    $2 = GenerateANode( "LB", 6);
    $2->line = @2.first_line;
    $3 = GenerateANode( "INT", 1);
    $3->line = @3.first_line;
    $4 = GenerateANode( "RB", 6);
    $4->line = @4.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 4;
}
       ;
FunDec : ID LP VarList RP  {
    $$ = GenerateANode( "FunDec", 0);
    $1 = GenerateANode( "ID", 3);
    $1->line = @1.first_line;
    $2 = GenerateANode( "LP", 6);
    $2->line = @2.first_line;
    $4 = GenerateANode( "RP", 6);
    $4->line = @4.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 4;
}
       | ID LP RP  {
    $$ = GenerateANode( "FunDec", 0);
    $1 = GenerateANode( "ID", 3);
    $1->line = @1.first_line;
    $2 = GenerateANode( "LP", 6);
    $2->line = @2.first_line;
    $3 = GenerateANode( "RP", 6);
    $3->line = @3.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}      
       ;
VarList : ParamDec COMMA VarList  {
    $$ = GenerateANode( "VarList", 0);
    $2 = GenerateANode( "COMMA", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
        | ParamDec {
    $$ = GenerateANode( "VarList", 0);
    connect($$, $1, 1);
    $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}       
        ;
ParamDec : Specifier VarDec  {
    $$ = GenerateANode( "ParamDec", 0);
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
} 
        ;

CompSt : LC DefList StmtList RC  {
    $$ = GenerateANode( "CompSt", 0);
    $1 = GenerateANode( "LC", 6);
    $1->line = @1.first_line;
    $4 = GenerateANode( "RC", 6);
    $4->line = @4.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 4;
}
       ;

StmtList : Stmt StmtList  {
    $$ = GenerateANode( "StmtList", 0);
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
}
         | {
    $$ = GenerateANode( "StmtList", 0);
    $$->line = getLineNum($$->left);
    $$->nchild = 0;
}
         ;

Stmt : Exp SEMI  {
    $$ = GenerateANode( "Stmt", 0);
    $2 = GenerateANode( "SEMI", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
}
     | CompSt  {
    $$ = GenerateANode( "Stmt", 0);
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
     | RETURN Exp SEMI  {
    $$ = GenerateANode( "Stmt", 0);
    $1 = GenerateANode( "RETURN", 6);
    $1->line = @1.first_line;
    $3 = GenerateANode( "SEMI", 6);
    $3->line = @3.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
     | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
    $$ = GenerateANode( "Stmt", 0);
    $1 = GenerateANode( "IF", 6);
    $1->line = @1.first_line;
    $2 = GenerateANode( "LP", 6);
    $2->line = @2.first_line;
    $4 = GenerateANode( "RP", 6);
    $4->line = @4.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    connect($4, $5, 0); $5->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 5;
}
     | IF LP Exp RP Stmt ELSE Stmt  {
    $$ = GenerateANode( "Stmt", 0);
    $1 = GenerateANode( "IF", 6);
    $1->line = @1.first_line;
    $2 = GenerateANode( "LP", 6);
    $2->line = @2.first_line;
    $4 = GenerateANode( "RP", 6);
    $4->line = @4.first_line;
    $6 = GenerateANode( "ELSE", 6);
    $6->line = @6.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    connect($4, $5, 0); $5->parent = $$;
    connect($5, $6, 0); $6->parent = $$;
    connect($6, $7, 0); $7->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 7;
}
     | WHILE LP Exp RP Stmt  {
    $$ = GenerateANode( "Stmt", 0);
    $1 = GenerateANode( "WHILE", 6);
    $1->line = @1.first_line;
    $2 = GenerateANode( "LP", 6);
    $2->line = @2.first_line;
    $4 = GenerateANode( "RP", 6);
    $4->line = @4.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    connect($4, $5, 0); $5->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 5;
}
     ;

DefList : Def DefList  {  // No "Def error DefList" Here: s/r conflict
    $$ = GenerateANode( "DefList", 0);
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
}
        | {
    $$ = GenerateANode( "DefList", 0);
    $$->line = getLineNum($$->left);
    $$->nchild = 0;
}
        ;
Def : Specifier DecList SEMI  {
    $$ = GenerateANode( "Def", 0);
    $3 = GenerateANode( "SEMI", 6);
    $3->line = @3.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    ;
DecList : Dec {
    $$ = GenerateANode( "DecList", 0);
    connect($$, $1, 1);  $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
        | Dec COMMA DecList  { // No "Dec error DecList" Here: s/r conflict
    $$ = GenerateANode( "DecList", 0);
    $2 = GenerateANode( "COMMA", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left); 
    $$->nchild = 3;
}        
        ;
Dec : VarDec  {
    $$ = GenerateANode( "Dec", 0);
    connect($$, $1, 1);  $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
    | VarDec ASSIGNOP Exp  {
    $$ = GenerateANode( "Dec", 0);
    $2 = GenerateANode( "ASSIGNOP", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    ;

Exp : Exp ASSIGNOP Exp  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "ASSIGNOP", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | Exp AND Exp  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "AND", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | Exp OR Exp  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "OR", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | Exp RELOP Exp  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "RELOP", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | Exp PLUS Exp  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "PLUS", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | Exp MINUS Exp  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "MINUS", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | Exp STAR Exp  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "STAR", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | Exp DIV Exp  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "DIV", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | LP Exp RP  {
    $$ = GenerateANode( "Exp", 0);
    $1 = GenerateANode( "LP", 6);
    $1->line = @1.first_line;      // XXX
    $3 = GenerateANode( "RP", 6);
    $3->line = @3.first_line;      // XXX
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | MINUS Exp  {
    $$ = GenerateANode( "Exp", 0);
    $1 = GenerateANode( "MINUS", 6);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
}
    | NOT Exp  {
    $$ = GenerateANode( "Exp", 0);
    $1 = GenerateANode( "NOT", 6);                // XXX
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 2;
}
    | ID LP Args RP  {
    $$ = GenerateANode( "Exp", 0);
    $1 = GenerateANode( "ID", 3);
    $1->line = @1.first_line;
    $2 = GenerateANode( "LP", 6);
    $2->line = @2.first_line;
    $4 = GenerateANode( "RP", 6);
    $4->line = @4.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 4;
}
    | ID LP RP  {
    $$ = GenerateANode( "Exp", 0);
    $1 = GenerateANode( "ID", 3);
    $1->line = @1.first_line;
    $2 = GenerateANode( "LP", 6);
    $2->line = @2.first_line;
    $3 = GenerateANode( "RP", 6);
    $3->line = @3.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | Exp LB Exp RB  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "LB", 6);
    $2->line = @2.first_line;
    $4 = GenerateANode( "RB", 6);
    $4->line = @4.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    connect($3, $4, 0); $4->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 4;
}
    | Exp DOT ID  {
    $$ = GenerateANode( "Exp", 0);
    $2 = GenerateANode( "DOT", 6);
    $2->line = @2.first_line;
    $3 = GenerateANode( "ID", 3);
    $3->line = @3.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
    | ID  {
    $$ = GenerateANode( "Exp", 0);
    $1 = GenerateANode( "ID", 3);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
    | INT  {
    $$ = GenerateANode( "Exp", 0);
    $1 = GenerateANode( "INT", 1);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
    | FLOAT  {
    $$ = GenerateANode( "Exp", 0);
    $1 = GenerateANode( "FLOAT", 2);
    $1->line = @1.first_line;
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
    ;
Args : Exp COMMA Args  {
    $$ = GenerateANode( "Args", 0);
    $2 = GenerateANode("COMMA", 6);
    $2->line = @2.first_line;
    connect($$, $1, 1); $1->parent = $$;
    connect($1, $2, 0); $2->parent = $$;
    connect($2, $3, 0); $3->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 3;
}
     | Exp  {
    $$ = GenerateANode( "Args", 0);
    connect($$, $1, 1); $1->parent = $$;
    $$->line = getLineNum($$->left);
    $$->nchild = 1;
}
     ;

%%




void yyerror(char *str){
	treeflag = 0;
}

struct Node* GenerateANode(const char str[20], int flag){
    struct Node* node;
    node = malloc(sizeof(struct Node));
    node->left = NULL;
    node->right = NULL;
    node->flag = flag;
    // printf("******************%s %d\n", str, node->line);
    if(ind >= 0 && (flag == 1 || flag == 2 || flag == 3 || flag == 4)) //2019-11-01
        strcpy(node->text, remStr[ind--]); // something wrong

    if (strcmp(str,"RELOP") == 0) { strcpy(node->text, remRelop[indRelop--]); }
    // printf("%s\n", str);
    strcpy(node->type, str);
    // assert(0);
    // printf("%s\n", node->type);
    return node;
}

void connect(struct Node* node1, struct Node* node2, int flag) {
    if (flag) node1->left  = node2;
    else      node1->right = node2;
}

void printTree(struct Node* head, int counts){
    if(head == NULL)return;
    if(!(head->flag == 0 && head->left == NULL))
    for(int i = 0;i < counts * 2;i++){
        printf(" ");
    }
    switch (head->flag){
        case 0: if(head->left != NULL) { printf("%s", head->type); printf(" (%d)\n", head->line); } break;
        case 1: case 3: case 4: printf("%s", head->type); printf(": %s\n", head->text); break;
	    case 2:                 printf("%s", head->type); float ff; sscanf(head->text, "%f", &ff); printf(": %f\n", ff); break; 
        case 5: case 6:         printf("%s", head->type); if (strcmp(head->type, "RELOP") == 0) printf(": %s\n", head->text); else printf("\n"); break;
        default: printf("%s", head->type); printf("\n"); break;
    }
    printTree(head->left, counts + 1);
    printTree(head->right, counts);
}

int getLineNum(struct Node* head){
    if (head == NULL) return yylineno;
    else return head->line;
}
