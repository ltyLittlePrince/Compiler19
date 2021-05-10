
#include "semantic.h"


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
    int* lineNum;//DefList, for Var's definition line Number. errorReport   
    struct Type* tp;  // Used by a few kind of Nodes. 
    struct Type** tps;
    char** util;      // For convenience, used occasionally (for passing list of names)
    int   util2;      // used together with 'util' and 'tps' (indicator of their size)
    float util3;    
};

extern int mydebug;
extern int cur_level;    // Denote levels of '{'.
extern int cur_struct;   // Denote 结构体 or 函数体
extern struct HashTable* hashTable;
extern struct HashNode** hashtable;// add by bxr
extern struct SymStack* symStack;
extern struct FieldListHashTable* fieldListHashTable;
extern struct FieldList** fieldListTable;// add by bxr
extern struct Type* modleInt;
extern struct Type* modleFloat;
extern char **forError;
// utilities 

void createFieldList(){
    fieldListTable = malloc(sizeof(struct FieldList*) * STRUCT_TABLE_SIZE);
 
    for(int i = 0;i < STRUCT_TABLE_SIZE;i++){
        fieldListTable[i] = malloc(sizeof(struct FieldList));
        fieldListTable[i]->n_field = -1;//附加头节点
        fieldListTable[i]->next = NULL;//天哪，这都错。
    }
    //by bxr
}
Bool varIsInFieldList(char* struct_name){
    struct FieldList*temp = searchFieldListTable(struct_name);
    if(temp == NULL)return FALSE; else return TRUE;
}
Bool insertintoFieldListtable(struct FieldList* fl){
    // prStruct(fl);
    int ind = hashFunction(fl->name, STRUCT_TABLE_SIZE);
    struct FieldList* morehead = fieldListTable[ind];
    fl->next = morehead->next;
    morehead->next = fl;
    return TRUE;
}
struct FieldList* searchFieldListTable(char* struct_name) {
    int ind = hashFunction(struct_name, STRUCT_TABLE_SIZE);    
    struct FieldList* temp = fieldListTable[ind]; 
    while(temp->next != NULL){
        if(eq(temp->next->name, struct_name))return temp->next;
        temp = temp->next;
    }
    return NULL;//by bxr
}


// struct SymNode* searchHashtable(char* name, Bool isFunc){
//     int ind = hashFunction(name, STRUCT_TABLE_SIZE);                  
//     struct HashNode* temp = hashtable[ind];
//     while(temp->next){
//         if(eq(temp->next->name, name) && isFunc)return temp->next->entry;//函数名重复
//         if(eq(temp->next->name, name) && isFunc == VAR && cur_level == temp->next->entry->lv)return temp->next->entry;
//         temp = temp->next;
//     }
//     return NULL;//by bxr
// }
struct SymNode* searchHashtableForUse(char* name, Bool isFunc){//直接检查对应符号，和域没有关系。
    int ind = hashFunction(name, STRUCT_TABLE_SIZE);                  
    struct HashNode* temp = hashtable[ind];
    while(temp->next){
        if(eq(temp->next->name, name) && isFunc == temp->next->entry->kind)return temp->next->entry;
        temp = temp->next;
    }
    return NULL;//by bxr
}
struct SymNode* searchHashtableForDefine(char* name, Bool isFunc){//给检查变量是否重复定义用
    int ind = hashFunction(name, STRUCT_TABLE_SIZE);                  
    struct HashNode* temp = hashtable[ind];
    while(temp->next){
        if(eq(temp->next->name, name) && isFunc == VAR && cur_level == temp->next->entry->lv)return temp->next->entry;
        temp = temp->next;
    }
    return NULL;//by bxr
}
Bool insertHashtable(struct SymNode* sym, Bool isFunc){
    if(searchHashtableForDefine(name(sym), isFunc))return FALSE;
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
        hashtable[i]->del = -1;//附加头节点
        hashtable[i]->next = NULL;
        hashtable[i]->entry = NULL;
    }
}


struct SymStack* createSymStack() {
    struct SymStack* ss = malloc(sizeof(struct SymStack));
    ss->stackTop = malloc(sizeof(struct SymNode));
    ss->stackTop->lv = -1;
    ss->stackTop->last = NULL;//less
    // ss->size = 1024; //modifier by bxr
    // ss->stackTop = malloc(sizeof(struct SymNode) * ss->size);
    return ss;
}
void insertIntoSymStack2(struct Node* semantic_node) {
    //一次性插入很多。
    struct SymNode* syms;
    for(int i = semantic_node->util2 - 1;i >= 0 ;i--){
        // printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~%s\n", semantic_node->util[i]);
        if(varIsInFieldList(semantic_node->util[i])){
            strcpy(forError[0], semantic_node->util[i]);
            errorReport(3, semantic_node->lineNum[i], forError);
            continue;
        }
        syms = malloc(sizeof(struct SymNode));
        // syms->lv = semantic_node->lineNum[i];

        // syms->last = symStack->stackTop->last;
        // symStack->stackTop->last = syms;//head insert //位置放错
        syms->lv = cur_level;
        syms->kind = VAR;
        strcpy(syms->v.var.name, semantic_node->util[i]);
        syms->v.var.type = semantic_node->tps[i];//并没有重新开辟空间
        // syms->v.var.type = semantic_node->tp;//并没有重新开辟空间//wrong, fuck.
        // printf("=====================\n");pr(semantic_node->tps[i]->kind);
        if(!insertHashtable(syms, VAR)){
            strcpy(forError[0], semantic_node->util[i]);
            errorReport(3, semantic_node->lineNum[i], forError);
            free(syms);
            continue;
        }
        syms->last = symStack->stackTop->last;
        symStack->stackTop->last = syms;//head insert
        if(mydebug)
            pri();
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
	srand((unsigned) time(NULL ));
	// if ((string = (char*) myMalloc(length)) == NULL )
	// {
	// 	myLog("Malloc failed!flag:14\n");
    //     assert(0);
	// 	return NULL ;
	// }
 
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

// Error Report
void errorReport(int tp, int lin, char** util) { // Use 'util' as you like. -- I change it, heihei. --bxr
    printf("Error type %d at Line %d: ", tp, lin);
    switch(tp) {
        case 1: printf("Undefined variable \"%s\"\n", util[0]); /* ... */ break; 
        case 2: printf("Undefined function \"%s\"\n", util[0]); /* ... */ break; 
        case 3: printf("Redefined variable \"%s\"\n", util[0]); /* ... */ break;  
        case 4: printf("Redefined function \"%s\"\n", util[0]); /* ... */ break;
        case 5: printf("Type mismatched for assignment.\n"); break;
        case 6: printf("The left-hand side of an assignment must be a variable.\n"); break;
        case 7: printf("Type mismatched for operands.\n"); break;
        case 8: printf("Type mismatched for return.\n"); break;
        case 9: /* ... */ printf("Function \"%s\" is not applicable for arguments.\n", util[0]); break;
        case 10: /* ... */ printf("\"%s\" is not an array.\n", util[0]); break;
        case 11: /* ... */ printf("\"%s\" is not a function.\n", util[0]); break;
        case 12: /* ... */ printf("\"%s\" is not an integer.\n", util[0]); break;
        case 13: printf("Illegal use of \".\".\n"); break;
        case 14: /* ... */ printf("Non-existent field: \"%s\"\n", util[0]); break;
        case 15: /* ... */ printf("Redefined or initialized-in-definition field \"%s\".\n", util[0]); break;
        case 16: /* ... */ printf("Duplicated name \"%s\".\n", util[0]); break;
        case 17: /* ... */ printf("Undefined structure \"%s\".\n", util[0]); break;
        default: break;
    }
} 

struct Node* kth(struct Node* cur, int k) { 
    cur = cur->left;
    for (int i = 1;cur != NULL && i < k; i++) cur = cur->right;
    assert(cur != NULL); /* DEBUG */
    return cur;
} 

Bool initial(){
    createHashtable();
    symStack = createSymStack();
    // createFieldListHashTable(NR_FILED_LIST_LIST);
    createFieldList();

    modleInt = createType();
    modleFloat = createType();
    modleFloat->v.basic = 1;
    cur_struct = 0;
    forError = malloc(sizeof(char*) * 3);
    for(int i = 0;i < 3;i++)forError[i] = malloc(sizeof(char) * 37);
    return TRUE;
}

//delete by bex
    // void cleanHashList(struct HashList* hl) {                 // O(n) clean del=1 operation
    //     struct HashNode* p = hl->head->next;                  // A complete iteration over 'hl' is needed!
    //     struct HashNode* pp = hl->head;
    //     while (p != NULL) {                                   // After DEBUGing, condition can be changed to "p != NULL && cleaned < NR_CLEAN"
    //         if (p->del == 1) {
    //             hl->n_del--;
    //             hl->n_clean++;
    //             pp->next = p->next;
    //             free(p); p = pp->next;
    //         }
    //         else { pp = p; p = p->next; }
    //     }
    //     assert(hl->tail == pp && hl->n_del == 0);             // DEBUG: After condition changed, remove "hl->tail == pp && ".
    // }
    // void delFromHashTable(struct SymNode* sym) {  // Called when popping symbols from SymStack
    //     int group = hashFunction(name(sym), hashTable->n_list);
    //     struct HashList* hl = hashTable->hashLists[group];
    //     struct HashNode* h = hl->head->next;
    //     for (; h != NULL; h = h->next) {
    //         if (h->entry == sym) { 
    //             h->del = 1;
    //             hl->n_del++;
    //             hl->len--;
    //             if (hl->n_del == NR_CLEAN) cleanHashList(hl);
    //             break;
    //         }
    //     }
    //     assert(h != NULL); // DEBUG: Something must have been changed
    // }

    // struct HashTable* createHashTable(int nHashList)  {
    //     struct HashTable* ht = malloc(sizeof(struct HashTable));
    //     ht->n_list = nHashList;
    //     ht->hashLists = malloc((unsigned)nHashList * sizeof(struct HashList*));
    //     for (int i = 0; i < nHashList; i++) ht->hashLists[i] = createHashList();
    //     return ht;
    // }
    // struct HashList* createHashList() {
    //     struct HashList* hl = malloc(sizeof(struct HashList)); 
    //     hl->len = 0; hl->n_del = 0; hl->n_clean = 0;
    //     hl->head = createHashNode(); hl->tail = hl->head;     // IDLE head
    //     return hl;
    // }
    // struct HashNode* createHashNode() { 
    //     struct HashNode* h = malloc(sizeof(struct HashNode)); 
    //     h->del = 0; h->entry = NULL; h->next = NULL;
    //     strcpy(h->name, "\0");
    //     return h; 
    // }

    // struct SymNode* searchHashTable(char* name, Bool isFunc) {              // if searchResult != NULL, 'maxLevelOne->lv < cur_level' is also possible
    //     int id = hashFunction(name, hashTable->n_list);                  
    //     struct HashList* hl = hashTable->hashLists[id];
    //     for (struct HashNode* h = hl->head->next; h != NULL; h = h->next) { // Find the innermost-defined variable 
    //         if (h->del == 0 && strcmp(h->name, name) == 0) {                // h->del==1, then this hashNode has been removed (obsolete)
    //             if (isFunc && (h->entry->kind == FUNC) || 
    //                 !isFunc && (h->entry->kind == VAR)){
    //                     // printf("----------win\n");
    //                     return h->entry;    // The returned *SymNode must be non-obsolete, innermost(highest lv) definition.
    //                 }
    //         }
    //     }
    //     // printf("----------fail\n");
    //     return NULL;// add by bxr, when search fail.
    // }
    // Bool insertIntoHashTable(struct SymNode* sym) {  // Prerequisite: no redefintion
    //     //modifile by bxr
    //     struct HashNode* has = createHashNode();
    //     has->entry = sym;
    //     strcpy(has->name, name(sym)); 
    //     int group = hashFunction(has->name, hashTable->n_list);
    //     struct HashList* hl = hashTable->hashLists[group];
    //     struct HashNode* temp = hl->head->next;
    //     // pr(group
    //     while(temp){
    //         // prs(temp->name, sym->v.var.name);pr(sym->lv);pr(temp->entry->lv);pr(temp->entry->kind);
    //         if(temp->entry->kind == FUNC && eq(temp->name, sym->v.func.name)){
    //             return FALSE;
    //         }else if(temp->entry->kind == VAR && eq(temp->name, sym->v.var.name) && sym->lv == temp->entry->lv){
    //             // pr(group);
    //             return FALSE;
    //         } 
    //         temp = temp->next;
    //     }
    //     has->next = hl->head->next; hl->head->next = has; hl->len++;
    //     return TRUE;
    // }