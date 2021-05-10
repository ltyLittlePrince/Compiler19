#include "semantic.h"


#define OPTIMIZE //打开这个才开启了所有的优化
#define FIRST  //优化了一点点和便签以及跳转有关的
#define SECOND //优化了：t25 := #0 RETURN t25，类似这种。
#define ARRAY_ADDRESS
// #define THIRD //优化了：t1 := #0 /*中间没有使用过t1*/ t1 := #1，类似这种。 //没意义，白写了。
#define FOURTH //删除所有只定义却从来没有用过的变量，同一个变量的连续自增。
// #define BLOCKDIVISION //将三地址码分块



extern int n_labels;
struct Label {
    int no;
};
struct Label* createLabel();       
char* labelString(struct Label* l);
void printALabel(struct Label* l);

extern int n_temps;
struct Temp {
    int no;
    Bool isAddress;
};
struct Temp* createTemp();          
char* tempString(struct Temp* t);

struct Operand {// 普通变量、
    // VARIABLE 0, TEMP 1, INT_CONST 2, FLOAT_CONST 3, GET_TEMP_ADDRESS 4,  
    // GET_VAR_ADDRESS 5, GET_TEMP_VALUE 6, GET_VAR_VALUE 7, FUNCTION 8
    enum { VARIABLE, TEMP, INT_CONST, FLOAT_CONST, GET_TEMP_ADDRESS, \
        GET_VAR_ADDRESS, GET_TEMP_VALUE, GET_VAR_VALUE, FUNCTION } kind;
    union {
        char va_name[40];  // for kind = 0, 5, 7, 8
        struct Temp* tmp;  // for kind = 1, 4, 6
        int int_const;     // for kind = 2
        float float_const; // for kind = 3 
    } u;
};

enum Relop { EQ, NE, GE, G, LE, L }; // ==, !=, >=, >, <=, <
struct InterCode {
    int kind;             // P64, 1-19
    union {
        struct { struct Label* l; } kind_1_11;
        struct { struct Operand* op; } kind_2_13_15_17_18_19;
        struct { struct Operand* op1, *result; } kind_389_10_16;
        struct { struct Operand* op1, *op2, *result; } kind_4567;
        struct { struct Operand* op1, *op2; struct Label* l; enum Relop r; } kind_12;
        struct { struct Operand* op; int sz; } kind_14;
    } u;
};
struct InterCode* createInterCode(int i);

//-------------------------------------by bxr-------------------------------------//
void optimizeX();
int optimizeBlock(int l, int r);
void opReplace(struct Operand* op1, struct Operand* op2, struct InterCode* ic);
Bool eqOperand(struct Operand* op1, struct Operand* op2);
Bool isExist(struct Operand* op, struct InterCode* ic);
struct Operand* createOp(struct Operand* op);
void interCodesListDelete(int ind);
void reverseRelop(int ind);
//-------------------------------------by bxr-------------------------------------//

struct InterCodes { struct InterCode* code; struct InterCodes* prev, *next; };
struct InterCodes* createInterCodeList();
void insertInterCode(struct InterCode* ic);
void printInterCodes();
void printInterCode(struct InterCode* ic);
void printAnOperand(struct Operand* op);
void printARelop(enum Relop r);

void icCheck(struct InterCode* ic);
void executeCheck();

struct U {char** va_name; struct Temp* tmp; }; 
struct Place {
    int kind;   // isVariable: 1, isTemp: 2
    struct U u; // kind == 1: uses va_name; 
                // kind == 2: uses tmp
};
struct Place* createPlace(int kind, char** va_name, struct Temp* t);

void translate_Program(struct Node* root);
void translate_ExtDefList(struct Node* root);
void translate_ExtDef(struct Node* root);
void translate_VarDec(struct Node* spec, struct Node* root, Bool inFunc);
void translate_FunDec(struct Node* spec, struct Node* root);
void translate_VarList(struct Node* root);
void translate_ParamDec(struct Node* root);
void translate_CompSt(struct Node* root);
void translate_StmtList(struct Node* root);
void translate_Stmt(struct Node* root);
void translate_DefList(struct Node* root);
void translate_Def(struct Node* root);
void translate_DecList(struct Node* spec, struct Node* root);
void translate_Dec(struct Node* spec, struct Node* root);
void translate_Exp(struct Node* root, struct Place* place);
void translate_Cond(struct Node* root, struct Label* l_true, struct Label* l_false);
struct ArgList {
    struct Temp* t;
    Bool isStruct; // in case structure type parameters
    struct ArgList* next;
    struct ArgList* last;
};
void translate_Args(struct Node* root, struct ArgList** args);

struct StructVar_Temp {
    char name[40];  // Function's structure parameter's name
    struct Temp* t; // According temp;
};
struct StructVar_Temp svt[10]; // cheat 
void svt_clear();
struct Temp* svt_search(char* name);
void svt_write(struct Node* root, int index);