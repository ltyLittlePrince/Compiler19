#ifndef _SEMANTIC_H__
#define _SEMANTIC_H__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>//头文件包含rand和srand函数
#include <assert.h>
#include <time.h>
#include "syntax.tab.h"

typedef int Bool;
#define TRUE 1
#define FALSE 0

#define __LAB3_DEBUG__ TRUE 
#define myassert(expr) assert((__LAB3_DEBUG__==FALSE)||(expr)) 

#define STRUCT_TABLE_SIZE 50
#define SYMBLE_TABLE_SIZE 50

// COMPROMISE_CONSTANT: can be properly set larger 
#define COMPROMISE_CONSTANT 10
#define CC COMPROMISE_CONSTANT
#define CC2 (CC*CC)
#define name(symbol)      ((symbol->kind==VAR)?(symbol->v.var.name):(symbol->v.func.name))
#define eq(str1,str2)     (strcmp((str1),(str2))==0)
#define make_process(nT)  Bool nT##_process(struct Node* root, int nch, char typeArr[][16])
#define call_process(nT)  nT##_process(root, nch, typeArr)
#define tuple2(rootType, errorMorphemeType) (eq(root->type, (rootType)) && eq(p->type,(errorMorphemeType)))

// Lab 3
#define eq2(op1,exp1,exp2) (((op1)==(exp1))||((op1)==(exp2)))
#define eq3(op1,exp1,exp2,exp3) ((eq2(op1,exp1,exp2))||((op1)==(exp3)))
#define eq4(op1,exp1,exp2,exp3,exp4) ((eq3(op1,exp1,exp2,exp3))||((op1)==(exp4)))
#define eq5(op1,exp1,exp2,exp3,exp4,exp5) ((eq4(op1,exp1,exp2,exp3,exp4))||((op1)==(exp5)))
#define eq6(op1,exp1,exp2,exp3,exp4,exp5,exp6) ((eq5(op1,exp1,exp2,exp3,exp4,exp5))||((op1)==(exp6)))


int cur_level;
struct SymStack* symStack;

// MAIN
Bool traverseTree(struct Node* root);



// STRUCTURE-INFORMATION CENTER
struct FieldList { // Defined structure info.
    char name[40];
    int n_field;
    struct Type** types;
    char** fieldnames;
    // others ...
    struct FieldList* next;
};
struct FieldList;
void createFieldList();
int typeSize(struct Type* tp);
int relDistance(struct FieldList* fl, char* member_name);

struct FieldList* searchFieldListHashTable (char* struct_name);
Bool insertintoFLhashtable(struct FieldList* fl);

// TYPE SYSTEM
struct Type {
    enum { BASIC, ARRAY, STRUCTURE } kind;
    union {
        int basic;                                      // int: 0, float: 1, undef: 2
        struct { struct Type* elem; int size; } array;
        struct FieldList* structure;                    // tree-like, FieldList* points to a token in one list of fieldListHashList.
    } v;
};

// Utilities

Bool equalsType(struct Type* tp1, struct Type* tp2);

// --- SYMBOL TABLE ---
typedef struct SymNode SymNode;
struct SymNode {
    int lv;       // Extension-function: for judging levels of '{'
    // int addr;     // address starting from 0 in created order
    enum { VAR, FUNC } kind;
    union {
        struct {
            char name[40];
            struct Type* type;
        } var;
        struct {
            char name[40];
            struct Type* rtn_type;
            int n_param;
            struct Type** paramsType;
        } func;
    } v;
    struct Node* entry;
    // content-unrelated
    struct SymNode* last;
    // others...
};
// int start_addr = 0;

struct HashNode {
    int del;             
    char name[40];
    struct SymNode* entry;
    struct HashNode* next;
};


int  hashFunction(char* name, int n_list);

struct SymNode* searchHashtableForUse(char* name, Bool isFunc);
//struct SymNode* searchHashtableForDefine(char* name, Bool isFunc);
Bool insertHashtable(struct SymNode* sym, Bool isFunc);
void delHashtable(struct SymNode* sym);
void createHashtable();

struct SymStack {
    struct SymNode* stackTop;
    int size;
};
struct SymStack* createSymStack();
void insertIntoSymStack2(struct Node* semantic_node);



// Others
struct Node* kth(struct Node* cur, int k);

Bool initial();
struct Type* createType();
int equalsType(struct Type* tp1, struct Type* tp2);
Bool varIsInFieldList (char* struct_name);
char* genRandomString(char *string, int length);
struct FieldList* searchFieldListTable(char* struct_name);
Bool insertintoFieldListtable(struct FieldList* fl);

void pri();//输出符号表
void pr(int i);//输出一个数字
void prs(const char* a, char* b);//输出字符串
void fpr(const char* a, int i);
void fprs(const char* a, char* b);//输出字符串
void prStruct(struct FieldList* fl);//输出结构体
void prType(struct Type* tp);//输出类型

//------------------------------High-level Definitions-------------------------------//
make_process(ExtDefList);

make_process(ExtDef);

make_process(ExtDecList);

//------------------------------High-level Definitions-------------------------------//

//------------------------------Specifiers-------------------------------//
make_process(Specifier);

make_process(StructSpecifier);

make_process(OptTag);

make_process(Tag);
//------------------------------Specifiers-------------------------------//

//------------------------------Declarators-------------------------------//

make_process(VarDec);

make_process(FunDec);

make_process(VarList);

make_process(ParamDec);
//------------------------------Declarators-------------------------------//

//------------------------------Statements-------------------------------//
make_process(CompSt);

make_process(StmtList);

make_process(Stmt);
//------------------------------Statements-------------------------------//

//------------------------------Local Definitions-------------------------------//
make_process(DefList);

make_process(Def);

make_process(DecList);

make_process(Dec);

//------------------------------Local Definitions-------------------------------//

//------------------------------Expressions-------------------------------//
make_process(Exp);

make_process(Args);
//------------------------------Expressions-------------------------------//

// MAIN
Bool traverseTree(struct Node* root);

#endif