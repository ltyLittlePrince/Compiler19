#include "ir.h"
#include <stdio.h>

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
    int nchild;    
};

extern int cur_level;    // Denote levels of '{'.
extern int cur_struct;   // Denote 结构体 or 函数体
extern struct HashNode** hashtable;// add by bxr
extern struct SymStack* symStack;
extern struct FieldList** fieldListTable;// add by bxr
extern struct Type* modelInt;
extern struct Type* modelFloat;
extern struct InterCodes* interCodes;
extern struct InterCode** interCodesList;
extern struct InterCode** iCList; //优化过的三地址码
// utilities 

void createFieldList(){
    fieldListTable = malloc(sizeof(struct FieldList*) * STRUCT_TABLE_SIZE);
 
    for(int i = 0;i < STRUCT_TABLE_SIZE;i++){
        fieldListTable[i] = malloc(sizeof(struct FieldList));
        fieldListTable[i]->n_field = -1;
        fieldListTable[i]->next = NULL;
    }
}
Bool varIsInFieldList(char* struct_name){
    struct FieldList* temp = searchFieldListTable(struct_name);
    if(temp == NULL) return FALSE; else return TRUE;
}
Bool insertintoFieldListtable(struct FieldList* fl){
    int ind = hashFunction(fl->name, STRUCT_TABLE_SIZE);
    struct FieldList* mh = fieldListTable[ind];
    fl->next = mh->next;
    mh->next = fl;
    return TRUE;
}
struct FieldList* searchFieldListTable(char* struct_name) {
    int ind = hashFunction(struct_name, STRUCT_TABLE_SIZE);    
    struct FieldList* temp = fieldListTable[ind]; 
    while(temp->next != NULL){
        if(eq(temp->next->name, struct_name))return temp->next;
        temp = temp->next;
    }
    return NULL;
}

int typeSize(struct Type* tp) {
    if (tp->kind == BASIC) return 4;
    if (tp->kind == ARRAY) return typeSize(tp->v.array.elem) * tp->v.array.size;
    int ret = 0;
    struct FieldList* fl = tp->v.structure;
    for (int i = 0; i < fl->n_field; i++) 
        ret += typeSize(fl->types[i]);
    return ret;
}
int relDistance(struct FieldList* fl, char* member_name) {
    int ret = 0;
    for (int i = 0; i < fl->n_field; i++) {
        if (eq(fl->fieldnames[i], member_name)) return ret;
        else ret += typeSize(fl->types[i]);
    }
    myassert(0);
}

struct SymNode* searchHashtableForUse(char* name, Bool isFunc){//直接检查对应符号，和域没有关系。
    int ind = hashFunction(name, STRUCT_TABLE_SIZE);                  
    struct HashNode* temp = hashtable[ind];
    while(temp->next){
        if(eq(temp->next->name, name) && isFunc == temp->next->entry->kind) return temp->next->entry;
        temp = temp->next;
    }
    return NULL;
}

// struct SymNode* searchHashtableForDefine(char* name, Bool isFunc){//给检查变量是否重复定义用
//     int ind = hashFunction(name, STRUCT_TABLE_SIZE);                  
//     struct HashNode* temp = hashtable[ind];
//     while(temp->next){
//         if(eq(temp->next->name, name) && isFunc == VAR && cur_level == temp->next->entry->lv) return temp->next->entry;
//         temp = temp->next;
//     }
//     return NULL;
// }

Bool insertHashtable(struct SymNode* sym, Bool isFunc){
    // if(searchHashtableForDefine(name(sym), isFunc)) return FALSE;
    int ind = hashFunction(name(sym), STRUCT_TABLE_SIZE);                  
    struct HashNode* temp = hashtable[ind];
    struct HashNode* insert = malloc(sizeof(struct HashNode));
    insert->entry = sym;
    strcpy(insert->name, name(sym));
    insert->del = 0;
    insert->next = temp->next;
    temp->next = insert;
    return TRUE;
}
void delHashtable(struct SymNode* sym){
    int ind = hashFunction(name(sym), SYMBLE_TABLE_SIZE);                  
    struct HashNode* temp = hashtable[ind];
    while(temp->next){
        if(sym == temp->next->entry){
            struct HashNode* del = temp->next;
            temp->next = temp->next->next;
            free(del);
            return;
        }
        temp = temp->next;
    }
    assert(0);
}
void createHashtable(){
    hashtable = malloc(sizeof(struct HashNode*) * STRUCT_TABLE_SIZE);
    for(int i = 0;i < STRUCT_TABLE_SIZE;i++){
        hashtable[i] = malloc(sizeof(struct HashNode));
        hashtable[i]->del = -1;
        hashtable[i]->next = NULL;
        hashtable[i]->entry = NULL;
    }
}


struct SymStack* createSymStack() {
    struct SymStack* ss = malloc(sizeof(struct SymStack));
    ss->stackTop = malloc(sizeof(struct SymNode));
    ss->stackTop->lv = -1;
    ss->stackTop->last = NULL;
    
    SymNode* rd = malloc(sizeof(SymNode));
    rd->entry = NULL; rd->kind = FUNC; rd->lv = 0;
    rd->v.func.n_param = 0;
    strcpy(rd->v.func.name, "read");
    rd->v.func.paramsType = NULL;
    rd->v.func.rtn_type = modelInt;
    rd->last = ss->stackTop;
    ss->stackTop = rd; 
    insertHashtable(rd, TRUE);

    SymNode* wt = malloc(sizeof(SymNode));
    wt->entry = NULL; wt->kind = FUNC; wt->lv = 0;
    wt->v.func.n_param = 1;
    strcpy(wt->v.func.name, "write");
    wt->v.func.paramsType = malloc(1*sizeof(struct Type*));
    wt->v.func.paramsType[0] = modelInt;
    wt->v.func.rtn_type = modelInt;
    wt->last = ss->stackTop;
    ss->stackTop = wt; 
    insertHashtable(wt, TRUE);

    return ss;
}
void insertIntoSymStack2(struct Node* semantic_node) {
    //一次性插入很多。
    struct SymNode* syms;
    for(int i = semantic_node->util2 - 1;i >= 0 ;i--){
        syms = malloc(sizeof(struct SymNode));

        syms->lv = cur_level;
        syms->kind = VAR;
        strcpy(syms->v.var.name, semantic_node->util[i]);
        syms->v.var.type = semantic_node->tps[i]; // 并没有重新开辟空间
        // syms->addr = start_addr;      // NEW
        // start_addr += sz;             // NEW
  
        if(!insertHashtable(syms, VAR)) assert(0);
        syms->last = symStack->stackTop->last;
        symStack->stackTop->last = syms;
    }
    return;
}

int equalsType(struct Type* tp1, struct Type* tp2) {
    // if(tp1 == NULL || tp2 == NULL)assert(0);
    if (tp1 == tp2) return TRUE;
    if (tp1->kind != tp2->kind) return FALSE;
    switch(tp1->kind) {
        case BASIC: return (tp1->v.basic == tp2->v.basic) ? TRUE : FALSE;
        case ARRAY: return equalsType(tp1->v.array.elem, tp2->v.array.elem);
        case STRUCTURE: { // Name Equivalence, In actual implementation, we will give name == '' random name
            if (eq(tp1->v.structure->name, "") || !eq(tp1->v.structure->name, tp2->v.structure->name)) return FALSE;
            else return TRUE;
        }
    }
    assert(0); /* DEBUG: cannot reach here. Remove this finally. */
}


char* genRandomString(char *string, int length){
	int flag, i;
	string = malloc(sizeof(char) * length);
	srand((unsigned)time(NULL));
 
	for (i = 0; i < length - 1; i++)
	{
		flag = rand() % 2;
		switch (flag)
		{
			case 0:
				string[i] = 'A' + rand() % 26;
				break;
			case 1:
				string[i] = 'a' + rand() % 26;
				break;
			default:
				string[i] = 'x';
				break;
		}
	}
	string[length - 1] = '\0';
	return string;
}
int hashFunction(char* name, int n_list) {                // very simple
    int res = 0; 
    for (char* p = name; *p != '\0'; p++) res += (int)(*p);
    return ((res % n_list) + n_list) % n_list;            // In case bug occurs
}
// DEBUG
void pri(){
    printf("\n");
    struct SymNode* pre = symStack->stackTop;
    while(pre->last != NULL){
        if(pre->last->kind == VAR)
            printf("type: %s, name: %s, paraType: %d， curlv: %d\n", "var ",pre->last->v.var.name, pre->last->v.var.type->kind, pre->last->lv);
        if(pre->last->kind == FUNC)
            printf("type: %s, name: %s\n", "func", pre->last->v.func.name);
        pre = pre->last;
    }
    printf("\n");
    return;
}

void pr(int i){
    printf("------------------%d\n", i);
}

void prs(const char* a, char* b){
    printf("%s--%s\n", a, b);
}

void fpr(const char* a, int i){
    FILE *fp = fopen("debug", "a");
    fprintf(fp, "%s%d\n", a, i);
    fclose(fp);
}

void fprs(const char* a, char* b){
    FILE *fp = fopen("debug", "a");
    fprintf(fp, "%s%s\n", a, b);
    fclose(fp);
}

void prStruct(struct FieldList* fl){
    printf("----Struct %s:\n", fl->name);
    for(int i = 0;i < fl->n_field;i++){
        printf("---fieldName: %10s; ", fl->fieldnames[i]);
        prType(fl->types[i]);
    }
    return;
}

void prType(struct Type* tp){
    if(tp == NULL){
        printf("No Type!\n");return;
    }
    switch (tp->kind){
    case BASIC:
        switch (tp->v.basic){
        case 0:
            printf("---------Type: int\n");
            break;
        case 1:
            printf("---------Type: float\n");
            break;
        case 2:
            printf("---------Type: undefine\n");
            break;
        default:assert(0);
            break;
        }
        break;
    case ARRAY:
        printf("[]");
        prType(tp->v.array.elem);
        break;
    case STRUCTURE:
        printf("--><><--Type: ");
        prStruct(tp->v.structure);
        break;
    default:
        break;
    }
    return;
}

struct Node* kth(struct Node* cur, int k) { 
    cur = cur->left;
    for (int i = 1; cur != NULL && i < k; i++) cur = cur->right;
    assert(cur != NULL); /* DEBUG */
    return cur;
} 

Bool initial(){
    createHashtable();
    symStack = createSymStack();
    createFieldList();
    modelInt = createType();
    modelFloat = createType();
    modelFloat->v.basic = 1;
    cur_struct = 0;
    // Lab3
    interCodes = createInterCodeList();
    interCodesList = malloc(sizeof(struct InterCode*) * 100000);//3500行代码就有50000+了
    //iCList = malloc(sizeof(struct InterCode*) * 100000);
    return TRUE;
}
