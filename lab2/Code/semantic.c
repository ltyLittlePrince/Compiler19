
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

unsigned __seed = 814931251;
unsigned __next_seed() { __seed = (__seed * 21840 + 106971187) % 2147483467; return __seed; }

int mydebug = 0;
int cur_level = 0;    // Denote levels of '{'.
int cur_struct = 0;   // Denote 结构体 or 函数体
struct HashTable* hashTable = NULL;
struct HashTable* hashtable = NULL;
struct SymStack* symStack = NULL;
struct FieldListHashTable* fieldListHashTable = NULL;
struct FieldList** fieldListTable = NULL;// add by bxr
struct Type* modleInt;
struct Type* modleFloat;
struct Type* defType;

char **forError;//输出错误信息

struct Type* createType() { 
    struct Type* tp = malloc(sizeof(struct Type));
    tp->kind = BASIC;
    tp->v.basic = 0;        // Default to int(0)
    return tp;
}

struct Type* getType(struct Node* nodes) {// edit by bxr
    return nodes->tp;
    // return nodes->type;// written by lty, haha
}

//#define make_process(nT)  Bool nT##_process(struct Node* root, int nch, char typeArr[][16])
//If make_process(...) is executed, then ...'s upper-layer morphemes are CORRECT.

//------------------------------High-level Definitions-------------------------------//
make_process(ExtDefList){
    if (nch == 2 && eq(typeArr[0], "ExtDef") && eq(typeArr[1], "ExtDefList")){
        return TRUE;
    }
    else if (nch == 0){
        return TRUE;
    }
    else return TRUE;//assert(0);
}

make_process(ExtDef) {
    if (nch == 3 && eq(typeArr[0], "Specifier") && eq(typeArr[1], "ExtDecList") && eq(typeArr[2], "SEMI")) {
        /* Steps: 
         * 1. Check the names' validity. (ExtDef->util, from upper layer)
         * 2. Insert them into hashTable and symTable.
         * 3. Error report and recovery.
        */
        insertIntoSymStack2(root->left->right);
        return TRUE;
    }
    
    else if (nch == 2 && eq(typeArr[0], "Specifier") && eq(typeArr[1], "SEMI")) {
        return TRUE;
    }  
   
    else if (nch == 3 && eq(typeArr[0], "Specifier") && eq(typeArr[1], "FunDec") && eq(typeArr[2], "CompSt")) {
        return TRUE;
    }
    else    assert(0); /* DEBUG */ 
    return FALSE;
}

make_process(ExtDecList){
    if (nch == 3 && eq(typeArr[0], "VarDec") && eq(typeArr[1], "COMMA") && eq(typeArr[2], "ExtDecList")){
        root->tps = root->left->right->right->tps;
        root->lineNum = root->left->right->right->lineNum;
        root->util = root->left->right->right->util;//每次都忘

        root->util[root->util2 - 1] = malloc(sizeof(char) * 40);
        strcpy(root->util[root->util2 - 1], root->left->text);
        root->lineNum[root->util2 - 1] = root->line;

        struct Node* sp = root->parent;
        while(eq(sp->type, "ExtDef") == 0)sp = sp->parent;
        sp = sp->left;

        if(eq(sp->type, "Specifier") == 0)assert(0);/*DEBUG*/
        //寻找到类型,Def中的Specifier.

        // strcpy(root->type, root->left->type);//?
        strcpy(root->text, root->left->text);//var name

        struct Type* typ = root->left->tp;
        if(typ->kind == ARRAY){
            while(typ->v.array.elem->kind == ARRAY){
                typ = typ->v.array.elem;
            }
            typ->v.array.elem = sp->tp;
            root->tp = root->left->tp;
        }
        else{
            root->tp = sp->tp;
        }
        root->tps[root->util2-1] = root->tp;

        if(eq(root->parent->type, "ExtDecList"))
            root->parent->util2 = root->util2 + 1;
            // pr(root->util2);
        return TRUE;
    }
    else if (nch == 1 && eq(typeArr[0], "VarDec")){
        root->tps = malloc(sizeof(struct Type*) * CC);
        root->util = malloc(sizeof(char*) * CC);
        root->lineNum = malloc(sizeof(int) * CC);
        root->util[0] = malloc(sizeof(char) * 40);
        strcpy(root->util[0], root->left->text);
        root->lineNum[0] = root->line;
        root->util2 = 1;

        struct Node* sp = root->parent;
        while(eq(sp->type, "ExtDef") == 0)sp = sp->parent;
        sp = sp->left;

        if(eq(sp->type, "Specifier") == 0)assert(0);/*DEBUG*/
        //寻找到类型,Def中的Specifier.

        // strcpy(root->type, root->left->type);//?
        strcpy(root->text, root->left->text);//var name

        struct Type* typ = root->left->tp;
        if(typ->kind == ARRAY){
            while(typ->v.array.elem->kind == ARRAY){
                typ = typ->v.array.elem;
            }
            typ->v.array.elem = sp->tp;
            root->tp = root->left->tp;
        }
        else{
            root->tp = sp->tp;
        }
        root->tps[0] = root->tp;
        if(eq(root->parent->type, "ExtDecList"))
            root->parent->util2 = root->util2 + 1;
        return TRUE;
    }
    else assert(0);
}

//------------------------------High-level Definitions-------------------------------//

//------------------------------Specifiers-------------------------------//
make_process(Specifier) { // parent can be ExtDef, Def, ParamDec, all the same
    root->tp = root->left->tp;
    strcpy(root->text, root->left->text);
    return TRUE;
}

make_process(StructSpecifier) { // parent can only be Specifier
    if (nch == 5 /* StructSpecifier : STRUCT OptTag LC DefList RC */ ) { 
        strcpy(root->text, root->left->right->text);// 之前漏了
        if (eq(root->text, "")){
            genRandomString(root->text, 24);
        }//默认生成的名字没有没用过，看运气，heihei
        // Structure has name, so search in hashTable/FLHashTable in case the name has been used.
        struct SymNode*   sr = searchHashtableForDefine(root->text, VAR);
        struct FieldList* fl = searchFieldListTable(root->text);
        if (sr != NULL || fl != NULL){
            char** t; mallocAll(t,1); 
            strcpy(t[0], root->text); 
            errorReport(16, root->line, t); // XXX: Error 16(1,2)
            freeAll(t,1); return FALSE;
        }
        // All the variables' information (should be)are stored in StructSpecifier->util, StructSpecifier->tps (See: DefList_process).
        // Insert this new Structure Type into FieldListHashList: (to be done)
        fl = malloc(sizeof(struct FieldList));
        strcpy(fl->name, root->text);//struct name
        struct Node* nodes = root->left;
        fl->next = NULL;
        fl->n_field = nodes->util2;
        fl->fieldnames = malloc(sizeof(char*) * fl->n_field);
        fl->types = malloc(sizeof(struct Type*) * fl->n_field);
        for(int i = 0;i < nodes->util2;i++){
            for(int j = 0;j < i;j++){//检查是否在结构体里面存在重复定义
                if(eq(nodes->util[j], nodes->util[i])){
                    fl->n_field--;
                    // strcpy(forError[0], fl->fieldnames[j]);
                    strcpy(forError[0], fl->fieldnames[i + fl->n_field - nodes->util2]);
                    errorReport(15, nodes->lineNum[i], forError);
                    break;
                }
                else if(i - 1 == j){//意味着结构体没有和该变量同名的变量
                    fl->fieldnames[i + fl->n_field - nodes->util2] = malloc(sizeof(char) * 40);
                    strcpy(fl->fieldnames[i + fl->n_field - nodes->util2], nodes->util[i]);
                    fl->types[i + fl->n_field - nodes->util2] = nodes->tps[i];
                }
            }
            if(i == 0){
                fl->fieldnames[i] = malloc(sizeof(char) * 40);
                strcpy(fl->fieldnames[i], nodes->util[i]);
                fl->types[i] = nodes->tps[i];   
            }
        }
        // prStruct(fl);
        root->tp = createType();
        root->tp->kind = STRUCTURE;
        root->tp->v.structure = fl;
        fl->next = NULL;
        insertintoFieldListtable(fl);
        return TRUE;
    }
    else if (nch == 2 /* StructSpecifier : STRUCT Tag */ ) {
        // Check validity of StructSpecifier's text, which is provided by Tag->text in make_process(Tag)
        struct FieldList* fl = searchFieldListTable(root->left->right->text);
        // struct FieldList* fl = searchFieldListTable(root->left->right->text);
        if(fl == NULL){
            strcpy(forError[0], root->left->right->text);
            errorReport(17, root->line, forError);
            return FALSE;
        }else{// 给StructSpecifier一个类型，然后它就可以去定义变量了。
            root->tp = malloc(sizeof(struct Type));
            root->tp->kind = STRUCTURE;
            root->tp->v.structure = fl;
        }
        return TRUE;
    }
    else assert(0); /* DEBUG */ 
    assert(0);
}

make_process(OptTag) { 
    if (nch == 1 && eq(typeArr[0], "ID"))  { 
        strcpy(root->text, root->left->text); 
        return TRUE; 
    }
    else if(nch == 0){
        strcpy(root->text, ""); 
        return TRUE;
    }
    else assert(0); /* DEBUG */  
    return FALSE;
}

make_process(Tag){
    if(nch == 1 && eq(typeArr[0], "ID")){
        strcpy(root->text, root->left->text); 
        return TRUE; 
    }
    else assert(0);
}
//------------------------------Specifiers-------------------------------//

//------------------------------Declarators-------------------------------//

make_process(VarDec) {  // Each VarDec's parent can be Dec, ParamDec, VarDec and ExtDecList.
    if ((nch == 4 && eq(typeArr[0], "VarDec") && eq(typeArr[1], "LB") && eq(typeArr[2], "INT") && eq(typeArr[3], "RB"))){
        root->tp = createType();
        root->tp->kind = ARRAY;
        root->tp->v.array.elem = root->left->tp;
        strcpy(root->text, root->left->text);//less
        return TRUE;
    }
    else if(nch == 1 && eq(typeArr[0], "ID")){
        strcpy(root->text, root->left->text);
        root->tp = createType();
        return TRUE;
    }
    else assert(0);
    return FALSE;
}

make_process(FunDec){
    if (nch == 4 && eq(typeArr[0], "ID") && eq(typeArr[1], "LP") && eq(typeArr[2], "VarList") && eq(typeArr[3], "RP")){
        if(searchHashtableForUse(root->left->text, FUNC) != NULL){
            strcpy(forError[0], root->left->text);
            errorReport(4, root->line, forError);
            return FALSE;
        }
        struct SymNode* symsfunc = malloc(sizeof(struct SymNode));
        struct Node* nodes = root->left->right->right;

        // insertIntoSymStack2(nodes);//后来加上的，下面手动插入，可以做错误的细节处理。

        symsfunc->v.func.paramsType = malloc(sizeof(struct Type*) * (nodes->util2 + 1));//忘记分配空间了

        struct SymNode* syms;
        for(int i = nodes->util2 - 1;i >= 0 ;i--){
            if(varIsInFieldList(nodes->util[i])){
                strcpy(forError[0], nodes->util[i]);
                // errorReport(3, nodes->lineNum[i], forError);
                errorReport(3, root->line, forError);
                assert(0);
                continue;
            }
            symsfunc->v.func.paramsType[nodes->util2 - 1 - i] = nodes->tps[i];//函数的symNode
            syms = malloc(sizeof(struct SymNode));
            syms->lv = cur_level + 1;//因为此时还没有进那个大括号，所以加一。
            syms->kind = VAR;
            strcpy(syms->v.var.name, nodes->util[i]);
            syms->v.var.type = nodes->tps[i];//并没有重新开辟空间
            if(!insertHashtable(syms, FUNC)){
                strcpy(forError[0], nodes->util[i]);
                errorReport(3, root->line, forError);
                free(syms);
                nodes->util2--;//和上面对应
                continue;
            }
            // prs("\n\n----------------------1--------------", "Before insert");
            // pri();
            syms->last = symStack->stackTop->last;
            symStack->stackTop->last = syms;//head insert
            // prs("-------------------------------------", "After insert");
            // pri();
        }
        symsfunc->kind = FUNC;
        symsfunc->v.func.n_param = nodes->util2;

        // for(int i = 0;i < nodes->util2;i++){
        //     symsfunc->v.func.paramsType[i] = nodes->tps[i];//参数的名字也记录下来了，但是似乎没派上用场。
        //     //还是派上用场了，这个应该也是要插入符号表中的。
        // }
        strcpy(symsfunc->v.func.name, root->left->text);//错啦?
        // printf("%s\n", root->left->text);
        // strcpy(syms->v.func.name, root->left->type);
        if(searchHashtableForUse(symsfunc->v.func.name, FUNC) != NULL){
            strcpy(forError[0], symsfunc->v.func.name);
            errorReport(4, root->line, forError);
            free(symsfunc);
            return FALSE;
        }
        symsfunc->v.func.rtn_type = root->parent->left->tp;//向上获取返回值类型
        //手动插入符号表
        // pri();printf("\n");
        // prs("--------------------------1----------", "Before insert");
        // pri();
        symsfunc->last = symStack->stackTop->last;
        symStack->stackTop->last = symsfunc;
        // prs("-------------------------------------", "After insert");
        // pri();
        // pri();printf("\n");
        // prType(symsfunc->v.func.paramsType[0]);
        // prType(symsfunc->v.func.paramsType[1]);assert(0);
        if(!insertHashtable(symsfunc, FUNC))assert(0);
        // insertIntoSymStack(syms);//不直接接受 struct SymNode*
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "ID") && eq(typeArr[1], "LP") && eq(typeArr[2], "RP")){
        struct SymNode* syms = malloc(sizeof(struct SymNode));
        syms->kind = FUNC;
        syms->v.func.n_param = 0;
        strcpy(syms->v.func.name, root->left->text);
        if(searchHashtableForUse(syms->v.func.name, FUNC) != NULL){
            strcpy(forError[0], syms->v.func.name);
            errorReport(4, root->line, forError);
            free(syms);
            return FALSE;
        }
        syms->v.func.rtn_type = root->parent->left->tp;//向上获取返回值类型
        //手动插入符号表
        syms->last = symStack->stackTop->last;
        symStack->stackTop->last = syms;
        if(!insertHashtable(syms, FUNC))assert(0);
        // insertIntoSymStack(syms);//不直接接受 struct SymNode*
        return TRUE;
    }
    else assert(0);
    return TRUE;
}

make_process(VarList){
    if(root->left->tp == NULL)return TRUE;
    if (nch == 3 && eq(typeArr[0], "ParamDec") && eq(typeArr[1], "COMMA") && eq(typeArr[2], "VarList")){
        root->tps = root->left->right->right->tps;
        root->util = root->left->right->right->util;//每次都忘
        
        if(root->left->tp == NULL){
            root->util2 = 0;
            if(eq(root->parent->type,"VarList")){
                root->parent->util2 = root->util2;
            }
            if(eq(root->parent->type,"FunDec")){
                root->util2--;//注意这个很容易错误
            }
            return TRUE;
        }
        root->tps[root->util2 - 1] = root->left->tp;
        root->util[root->util2 - 1] = root->left->text;//没有新分配空间，而是指向已有的。
        if(eq(root->parent->type,"VarList")){
            root->parent->util2 = root->util2 + 1;
        }
    }
    else if (nch == 1 && eq(typeArr[0], "ParamDec")){
        root->tps = malloc(sizeof(struct Type*) * CC);
        root->util = malloc(sizeof(char*) * CC);
        if(root->left->tp == NULL){
            root->util2 = 0;
            if(eq(root->parent->type,"VarList")){
                root->parent->util2 = root->util2 + 1;
            }
            return TRUE;
        }
        root->util2 = 1;
        root->tps[0] = root->left->tp;
        // root->util[0] = root->text;//没有新分配空间，而是指向已有的。 //wrong
        root->util[0] = root->left->text;//没有新分配空间，而是指向已有的。
        if(eq(root->parent->type,"VarList")){
            root->parent->util2 = root->util2 + 1;
        }
    }
    else assert(0);
}

make_process(ParamDec){
    if (nch == 2 && eq(typeArr[0], "Specifier") && eq(typeArr[1], "VarDec")){
        struct Type* typ = root->left->right->tp;
        // struct Type* typ = root->tp;
        if(typ->kind == ARRAY){
            while(typ->v.array.elem->kind == ARRAY){
                typ = typ->v.array.elem;
            }
            typ->v.array.elem = root->left->tp;
            root->tp = root->left->right->tp;
        }
        else{
            root->tp = root->left->tp;//后来加上的
        }
        strcpy(root->text, root->left->right->text);//var name
        return TRUE;
    }
    else assert(0);
}
//------------------------------Declarators-------------------------------//

//------------------------------Statements-------------------------------//
make_process(CompSt){
    if (nch == 4 && eq(typeArr[0], "LC") && eq(typeArr[1], "DefList") && eq(typeArr[2], "StmtList") && eq(typeArr[3], "RC")){
        // struct Node* nodes = root->left->right;
        // insertIntoSymStack2(nodes);//在下面就做了，否则变量使用之前来不及定义
        return TRUE;
    }else assert(0);
}

make_process(StmtList){
    if (nch == 2 && eq(typeArr[0], "Stmt") && eq(typeArr[1], "StmtList")){
        //Nothing should be done.
        return TRUE;
    }else if(nch == 0){
        //Nothing should be done.
        return TRUE;
    }
    else assert(0);
}

make_process(Stmt){
    if (nch == 2 && eq(typeArr[0], "Exp") && eq(typeArr[1], "SEMI")){
        //Nothing should be done.
        return TRUE;
    }
    else if (nch == 1 && eq(typeArr[0], "CompSt")){
        //Nothing should be done.
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "RETURN") && eq(typeArr[1], "Exp") && eq(typeArr[2], "SEMI")){
        //search the ret type from SymStack
        // if(root->left->right->tp == NULL){//Exp是错误的//这个和下面的居然是一样的，匪夷所思。
        //     errorReport(8, root->line, NULL);
        //     return FALSE;
        // }
        struct SymNode* symNode= symStack->stackTop;//note: symNode is also a morehead.
        while(symNode->last->kind != FUNC){
            symNode = symNode->last;
            if(symNode->last == NULL)assert(0); /*DEBUG*/
        }
        if(root->left->right->tp == NULL){
            errorReport(8, root->line, NULL);
            return FALSE;
        }
        if(equalsType(symNode->last->v.func.rtn_type, root->left->right->tp) == FALSE){
            errorReport(8, root->line, NULL);
            return FALSE; 
        }
        return TRUE;
    }
    else if (nch == 5 && eq(typeArr[0], "IF") && eq(typeArr[1], "LP") && \
    eq(typeArr[2], "Exp") && eq(typeArr[3], "RP") && eq(typeArr[4], "Stmt")){
        if(root->left->right->right->tp == NULL) return FALSE;
        else if(!equalsType(modleInt, root->left->right->right->tp)){//原来少了一个right
            errorReport(7, root->line, NULL);
            return FALSE;
        }
        return TRUE;
    }
    else if (nch == 7 && eq(typeArr[0], "IF") && eq(typeArr[1], "LP") && eq(typeArr[2], "Exp")\
     && eq(typeArr[3], "RP") && eq(typeArr[4], "Stmt") && eq(typeArr[5], "ELSE") && eq(typeArr[6], "Stmt")){
        if(root->left->right->right->tp == NULL) return FALSE;
        else if(!equalsType(modleInt, root->left->right->right->tp)){//原来少了一个right
            errorReport(7, root->line, NULL);
            return FALSE;
        }
        return TRUE;
    }
    else if (nch == 5 && eq(typeArr[0], "WHILE") && eq(typeArr[1], "LP") && \
    eq(typeArr[2], "Exp") && eq(typeArr[3], "RP") && eq(typeArr[4], "Stmt")){
        if(root->left->right->right->tp == NULL) return FALSE;
        else if(!equalsType(modleInt, root->left->right->right->tp)){//原来少了一个right
            errorReport(7, root->line, NULL);
            return FALSE;
        }
        return TRUE;
    }
    else assert(0);
}
//------------------------------Statements-------------------------------//

//------------------------------Local Definitions-------------------------------//
make_process(DefList) { // Its (non-DefList) parent can be of type "StructSpecifier" and "CompSt" in our grammar.
   return TRUE;
}

make_process(Def) { // Each "Def" may produce one or more structure member variables. Its only parent is of type "DefList".
    return TRUE;
}

make_process(DecList) { // Each DecList have one or more Dec. Its only parent (expept itself) is of type "Def".
    return TRUE;
}

make_process(Dec) {  // Each Dec's only parent is DecList.
    if(cur_struct == 0){ // 局部变量定义
        if(!eq(root->parent->parent->type, "Def")){
            root->tp = root->parent->parent->left->tp;
        }else {
            struct Node* sp = root->parent;
            while(eq(sp->type, "Def") == 0)sp = sp->parent;
            sp = sp->left;
            if(eq(sp->type, "Specifier") == 0)assert(0);/*DEBUG*/
            root->tp = sp->tp;
        }

        struct Type* typ = root->left->tp; //为数组传入类型
        if(typ->kind == ARRAY){
            while(typ->v.array.elem->kind == ARRAY){
                typ = typ->v.array.elem;
            }
            typ->v.array.elem = root->tp;
            root->tp = root->left->tp;
        }

        // if(nch == 3 && root->left->right->right->tp == NULL)return TRUE;
        if(nch == 3 && root->left->right->right->tp != NULL && \
        equalsType(root->tp, root->left->right->right->tp) == 0){//"="两边类型不匹配
            errorReport(5, root->line, NULL);
        }
        if(varIsInFieldList(root->left->text)){ //和已经定义的结构体类型同名
            strcpy(forError[0], root->left->text);
            errorReport(3, root->line, forError);return TRUE;
        }
        struct SymNode* syms = malloc(sizeof(struct SymNode));
        syms->lv = cur_level;
        syms->kind = VAR;
        strcpy(syms->v.var.name, root->left->text);
        
        syms->v.var.type = root->tp;//并没有重新开辟空间

        if(!insertHashtable(syms, VAR)){//和已经定义的变量同名
            strcpy(forError[0], root->left->text);
            errorReport(3, root->line, forError);
            free(syms);return TRUE;
        }
        else{
            // prs("----------------------2--------------", "Before insert");
            // pri();
            syms->last = symStack->stackTop->last;
            symStack->stackTop->last = syms;
            // prs("----------------------2--------------", "after  insert");
            // pri();
        }
        return TRUE;
    }
    else if(cur_struct > 0){//结构体定义列表
        if(!eq(root->parent->parent->type, "Def")){//不是最后一个Dec
            root->tp = root->parent->parent->left->tp;//向上一个Dec索要类型
            root->structDef = root->parent->parent->left->structDef;
        }else {//
            struct Node* sp = root->parent;
            while(eq(sp->type, "Def") == 0)sp = sp->parent;
            sp = sp->left;
            if(eq(sp->type, "Specifier") == 0)assert(0);/*DEBUG*/
            root->tp = sp->tp;
        
            sp = root->parent;//找到结构体定义的 STRUCT节点，把东西都存在这个节点里
            while(eq(sp->type, "Def") == 0)sp = sp->parent;
            if(eq(sp->type, "Def") == 0)assert(0);/*DEBUG*/
            while(eq(sp->type, "StructSpecifier") == 0)sp = sp->parent;
            if(eq(sp->type, "StructSpecifier") == 0)assert(0);/*DEBUG*/
            sp = sp->left;
            if(eq(sp->type, "STRUCT") == 0)assert(0);/*DEBUG*/
            root->structDef = sp;
            if(sp->util3 != 123){
                sp->tps = malloc(sizeof(struct Type*) * CC2);
                sp->util = malloc(sizeof(char*) * CC2);
                sp->lineNum = malloc(sizeof(int) * CC2);
                sp->util3 = 123;
                sp->util2 = 0;
            }
        }
        struct Type* typ = root->left->tp;//有可能是数组类型
        // if(typ->kind == ARRAY){
        //     while(typ->kind == ARRAY){
        //         typ = typ->v.array.elem;
        //     }
        //     typ = root->tp;
        //     root->tp = root->left->tp;
        // }

        if(typ->kind == ARRAY){
            while(typ->v.array.elem->kind == ARRAY){
                typ = typ->v.array.elem;
            }
            typ->v.array.elem = root->tp;
            root->tp = root->left->tp;
        }
        if(nch == 3){//结构体类型，定义变量不能赋值
            strcpy(forError[0], root->left->text);
            strcat(forError[0], " = ");
            strcat(forError[0], root->left->right->right->text);
            errorReport(15, root->line, forError);//return FALSE;这里并没有返回错误，而是记录信息，让上层去跳过这个。
            return TRUE;
        }
        struct Node* rem = root->structDef;//用来指向结构体定义的 STRUCT节点，把东西都存在这个节点里
        // for(int i = 0;i < rem->util2;i++){
        //     if(eq(rem->util[i], root->left->text)){
        //         strcpy(forError[0], root->left->text);
        //         errorReport(15, root->line, forError);return TRUE;
        //     }
        // }//在上层统一检查。
        rem->lineNum[rem->util2] = root->line;
        rem->util[rem->util2] = malloc(sizeof(char) * 40);
        strcpy(rem->util[rem->util2], root->left->text);
        rem->tps[rem->util2] = root->tp;
        rem->util2++;
        return TRUE;
    }
    assert(0);
}

//------------------------------Local Definitions-------------------------------//

//------------------------------Expressions-------------------------------//
make_process(Exp) {  // 
    if (nch == 4 && eq(typeArr[0], "ID") && eq(typeArr[1], "LP") && eq(typeArr[2], "Args") && eq(typeArr[3], "RP")){
        struct SymNode* syms = searchHashtableForUse(root->left->text, FUNC);
        // pri();prs("",syms->v.var.name);
        if(syms == NULL){
            if(searchHashtableForUse(root->left->text, VAR)){
                strcpy(forError[0], root->left->text);
                errorReport(11, root->line, forError);return FALSE;
            }
            else{
                strcpy(forError[0], root->left->text);
                errorReport(2, root->line, forError);return FALSE;
            }
        }
        struct Node* args = root->left->right->right;
        if(args->util2 != syms->v.func.n_param){//参数个数不匹配。
            strcpy(forError[0], syms->v.func.name);
            errorReport(9, args->line, forError);return FALSE;
        }
        for(int i = 0;i < syms->v.func.n_param;i++){
            if(!equalsType(syms->v.func.paramsType[i], args->tps[args->util2 - i - 1])){
                strcpy(forError[0], syms->v.func.name);
                errorReport(9, args->line, forError);return FALSE;//参数类型不匹配
            }
        }
        root->tp = syms->v.func.rtn_type;//
        return TRUE;
    }
    else if (nch == 4 && eq(typeArr[0], "Exp") && eq(typeArr[1], "LB") && eq(typeArr[2], "Exp") && eq(typeArr[3], "RB")){
        //
        if(!equalsType(modleInt, root->left->right->right->tp)){//a[b], b必须为int型
            strcpy(forError[0], root->left->right->right->text);
            errorReport(12, root->line, forError);
            return FALSE;
        }
        if(root->left->tp->kind != 1){//这玩意不是数组类型,不能用[]访问
            strcpy(forError[0], "Variable");
            errorReport(10, root->line, forError);
            return FALSE;
        }
        root->tp = root->left->tp->v.array.elem;
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "ASSIGNOP") && eq(typeArr[2], "Exp")){
        struct Node* node1 = root->left;
        struct Node* node2 = root->left->right->right;
        //判断左值
        if((eq(node1->left->type, "ID") && node1->left->right == NULL)|| (eq(node1->left->type, "Exp") && eq(node1->left->right->type, "DOT") && \
        eq(node1->left->right->right->type, "ID")) || (eq(node1->left->type, "Exp") && eq(node1->left->right->type, "LB") && \
        eq(node1->left->right->right->type, "Exp")  && eq(node1->left->right->right->right->type, "RB"))){
            if(equalsType(node1->tp, node2->tp)){
                root->tp = node1->tp;//赋值,返回的应该是个int
            }else{
                errorReport(5, root->line, NULL);return FALSE;//类型不同,不能赋值.
            }
            return TRUE;
        }else{
            errorReport(6, root->line, NULL);return FALSE;//错误类型本来是5
        }
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "AND") && eq(typeArr[2], "Exp")){
        struct Node* node1 = root->left;
        struct Node* node2 = root->left->right->right;
        if(equalsType(node1->tp, node2->tp) && (equalsType(node1->tp, modleFloat) || equalsType(node1->tp, modleInt))){
            //类型相同，并且都为INT。
            root->tp = node1->tp;
            return TRUE;
        }else{
            errorReport(7, root->line, NULL);return FALSE;
        }
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "OR") && eq(typeArr[2], "Exp")){
        struct Node* node1 = root->left;
        struct Node* node2 = root->left->right->right;
        if(equalsType(node1->tp, node2->tp) && equalsType(node1->tp, modleInt)){
            //类型相同，并且都为INT。
            root->tp = node1->tp;
            return TRUE;
        }else{
            errorReport(7, root->line, NULL);return FALSE;
        }
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "RELOP") && eq(typeArr[2], "Exp")){
        struct Node* node1 = root->left;
        struct Node* node2 = root->left->right->right;
        if(equalsType(node1->tp, node2->tp)){
            //类型相同
            root->tp = createType();
            return TRUE;
        }else{
            errorReport(7, root->line, NULL);return FALSE;
        }
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "PLUS") && eq(typeArr[2], "Exp")){
        struct Node* node1 = root->left;
        struct Node* node2 = root->left->right->right;
        if(equalsType(node1->tp, node2->tp) && (equalsType(node1->tp, modleFloat) || equalsType(node1->tp, modleInt))){
            //类型相同，并且都为INT或FLOAT。
            root->tp = node1->tp;
            return TRUE;
        }else{
            errorReport(7, root->line, NULL);return FALSE;
        }
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "MINUS") && eq(typeArr[2], "Exp")){
        struct Node* node1 = root->left;
        struct Node* node2 = root->left->right->right;
        if(equalsType(node1->tp, node2->tp) && (equalsType(node1->tp, modleFloat) || equalsType(node1->tp, modleInt))){
            //类型相同，并且都为INT或FLOAT。
            root->tp = node1->tp;
            return TRUE;
        }else{
            errorReport(7, root->line, NULL);return FALSE;
        }
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "STAR") && eq(typeArr[2], "Exp")){
        struct Node* node1 = root->left;
        struct Node* node2 = root->left->right->right;
        if(equalsType(node1->tp, node2->tp) && (equalsType(node1->tp, modleFloat) || equalsType(node1->tp, modleInt))){
            //类型相同，并且都为INT或FLOAT。
            root->tp = node1->tp;
            return TRUE;
        }else{
            errorReport(7, root->line, NULL);return FALSE;
        }
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "DIV") && eq(typeArr[2], "Exp")){
        struct Node* node1 = root->left;
        struct Node* node2 = root->left->right->right;
        if(equalsType(node1->tp, node2->tp) && (equalsType(node1->tp, modleFloat) || equalsType(node1->tp, modleInt))){
            //类型相同，并且都为INT或FLOAT。
            root->tp = node1->tp;
            return TRUE;
        }else{
            errorReport(7, root->line, NULL);return FALSE;
        }
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "DOT") && eq(typeArr[2], "ID")){
        if(root->left->tp->kind != STRUCTURE){
            strcpy(forError[0], root->left->text);
            errorReport(13, root->line, forError);return FALSE;
        }
        struct FieldList* struc = root->left->tp->v.structure;
        for(int i = 0;i < struc->n_field;i++){
            if(eq(root->left->right->right->text, struc->fieldnames[i])){
                root->tp = struc->types[i];//maybe wrong
                return TRUE;
            }
        }
        strcpy(forError[0], root->left->right->right->text);
        errorReport(14, root->line, forError);
        return FALSE;
    }
    else if (nch == 3 && eq(typeArr[0], "LP") && eq(typeArr[1], "Exp") && eq(typeArr[2], "RP")){
        root->tp = root->left->right->tp;
        return TRUE;
    }
    else if (nch == 3 && eq(typeArr[0], "ID") && eq(typeArr[1], "LP") && eq(typeArr[2], "RP")){
        struct SymNode* syms = searchHashtableForUse(root->left->text, FUNC);
        if(syms == NULL){
            if(searchHashtableForUse(root->left->text, VAR)){
                strcpy(forError[0], root->left->text);
                errorReport(11, root->line, forError);return FALSE;
            }
            else{
                strcpy(forError[0], root->left->text);
                errorReport(2, root->line, forError);return FALSE;
            }
        }
        if(syms->v.func.n_param != 0){
            errorReport(9, root->line, forError);return FALSE;
        }
        root->tp = syms->v.func.rtn_type;//
        return TRUE;
    }
    else if (nch == 2 && eq(typeArr[0], "MINUS") && eq(typeArr[1], "Exp")){
        root->tp = root->left->right->tp;
        return TRUE;
    }
    else if (nch == 2 && eq(typeArr[0], "NOT") && eq(typeArr[1], "Exp")){
        root->tp = root->left->right->tp;
        return TRUE;
    }
    else if (nch == 1 && eq(typeArr[0], "ID")){
        struct SymNode* syms = searchHashtableForUse(root->left->text, VAR);//?，咋知道这个是不是函数名
        if(syms == NULL){
            strcpy(forError[0], root->left->text);
            errorReport(1, root->line, forError);return FALSE;
        }
        else{
            strcpy(root->text, root->left->text);
            root->tp = syms->v.var.type;
        }
        return TRUE;
    }
    else if (nch == 1 && eq(typeArr[0], "INT")){
        strcpy(root->text, root->left->text);//2019-11-01 19:06:40
        root->tp = createType();
        return TRUE;
    }
    else if (nch == 1 && eq(typeArr[0], "FLOAT")){
        strcpy(root->text, root->left->text);//2019-11-01 19:06:40
        root->tp = createType();
        root->tp->v.basic = 1;
        return TRUE;
    }
}

make_process(Args){
    //把所有参数都记录在util，用util2作为util的大小
    if(nch == 3 && eq(typeArr[0], "Exp") && eq(typeArr[1], "COMMA") && eq(typeArr[2], "Args")){
        if(root->left->tp == NULL)return FALSE;
        root->tps = root->left->right->right->tps;
        root->tps[root->util2 - 1] = root->left->tp;
        if(eq(root->parent->type, "Args")){
            root->parent->util2 = root->util2 + 1;
        }
    }else if(nch == 1 && eq(typeArr[0], "Exp")){
        if(root->left->tp == NULL)return FALSE;
        root->tps = malloc(sizeof(struct Type*) * CC);//numbers of function's para
        // root->util[0] = malloc(sizeof(struct Type)); //?
        root->util2 = 1;
        root->tps[0] = root->left->tp;
        if(eq(root->parent->type, "Args")){
            root->parent->util2 = root->util2 + 1;
        }
    }else assert(0);
    return TRUE;
}
//------------------------------Expressions-------------------------------//

// MAIN
Bool traverseTree(struct Node* root) {
    if(mydebug)
        printf("%d--------%s\n", root->line, root->type);
    assert(root != NULL); // DEBUG
    char typeArr[8][16]; int nch = 0;        // (typeArr, nchildren) determines all children's morpheme  
    if (root->flag == 0 && root->left != NULL) { // CASE 0: Non-terminal, and ensure no " M -> \epsilon "*
        for (struct Node* p = root->left; p != NULL; p = p->right) {
            Bool b = traverseTree(p);
            if (b == FALSE) {

                if(eq(p->type, "Exp")){
                    p->tp = NULL;
                    /*return FALSE;*/
                }//对于Exp，我们关注的是类型，但是它其实不太影响上层的东西，所以只需要将其类型置空。

                if (eq(root->type, "Exp")){ root->tp = NULL; return FALSE;}
                else if (tuple2("Dec", "Exp"))  {
                    // return TRUE; 
                }//用未定义的结构体定义变量。
                else if (eq(root->type, "DecList"))   assert(0);
                else if (tuple2("Def", "Specifier"))  {
                    return TRUE;
                    //  root->util2 = 0;return FALSE; //告诉上层这里一个变量也没有
                }//用未定义的结构体定义变量。
                else if (tuple2("Stmt", "Exp") && nch == 3) ;// return FALSE;//返回值，可以直接跳过这句。Stmt其他的错了似乎对大局没有影响。
                else if (tuple2("StmtList", "Stmt")) ;
                else if (eq(root->type, "CompSt")) ;
                else if (tuple2("Specifier", "StructSpecifier")) return FALSE;
                else if (tuple2("ParamDec", "VarDec") && tuple2("ParamDec", "Specifier")) {
                    root->tp = NULL;//告诉上层，这个变量定义有问题。
                    return FALSE;//定义了重名变量 && 用了未定义的结构类型
                }
                else if (tuple2("VarList", "ParamDec")) ;//不用做，但是要在VarList里跳过。
                else if (tuple2("VarList", "VarList")) ;
                else if (tuple2("ExtDef", "FunDec")) return TRUE;//跳过该函数定义
                else if (tuple2("FunDec", "ID"))   return FALSE;//函数名重复，跳过函数体的定义??
                else if (tuple2("Specifier", "StructSpecifier"))   { root->tp = NULL; return FALSE; }//
                else if (tuple2("ExtDef", "Specifier"))  return FALSE;//
                else if (eq(root->type, "Args"))   return FALSE;
                // else if (eq(root->type, "DefList"))   assert(0);
                // return TRUE;// edit by bxr,诶呀， 这个根本不能打开。sb了。
            }
            strcpy(typeArr[nch++], p->type);
        }
        if      (eq(root->type, "ExtDefList"))      return call_process(ExtDefList);
        else if (eq(root->type, "ExtDef"))          return call_process(ExtDef);
        else if (eq(root->type, "ExtDecList"))      return call_process(ExtDecList);

        else if (eq(root->type, "Specifier"))       return call_process(Specifier);
        else if (eq(root->type, "StructSpecifier")) return call_process(StructSpecifier);
        else if (eq(root->type, "OptTag"))          return call_process(OptTag);
        else if (eq(root->type, "Tag"))             return call_process(Tag);
        
        else if (eq(root->type, "VarDec"))          return call_process(VarDec);
        else if (eq(root->type, "FunDec"))          return call_process(FunDec);
        else if (eq(root->type, "VarList"))         return call_process(VarList);
        else if (eq(root->type, "ParamDec"))        return call_process(ParamDec);

        else if (eq(root->type, "DefList"))         return call_process(DefList);
        else if (eq(root->type, "Def"))             return call_process(Def);
        else if (eq(root->type, "DecList"))         return call_process(DecList);
        else if (eq(root->type, "Dec"))             return call_process(Dec);

        else if (eq(root->type, "CompSt"))          return call_process(CompSt);
        else if (eq(root->type, "StmtList"))        return call_process(StmtList);
        else if (eq(root->type, "Stmt"))            return call_process(Stmt);

        else if (eq(root->type, "Exp"))             return call_process(Exp);
        else if (eq(root->type, "Args"))            return call_process(Args);
        else return TRUE;
    }
    else if (root->flag == 0 && root->left == NULL) { // A few special cases
        if       (eq(root->type, "ExtDefList"))     return call_process(ExtDefList);//strcpy(root->parent->text, "");//(?)
        else if  (eq(root->type, "OptTag"))         return call_process(OptTag);//strcpy(root->parent->text, "");
        else if  (eq(root->type, "StmtList"))       return call_process(StmtList);//strcpy(root->parent->text, "");
        else if  (eq(root->type, "DefList"))        return call_process(DefList);//strcpy(root->parent->text, "");
        else assert(0);//return TRUE;
    }
    else if (root->flag == 1) { // int, its parent can be Exp and VarDec
        sscanf(root->text, "%d", &root->parent->util2);
        if (eq(root->parent->text, "Exp")) root->parent->tp = createType(); // default to int
    }
    else if (root->flag == 2) { // float, its parent can be only be Exp
        sscanf(root->text, "%f", &root->parent->util3);
        root->parent->tp = createType(); root->parent->tp->v.basic = 1; // set to float
    }
    else if (root->flag == 3) { // ID
        if (!eq(root->parent->type, "Exp")) {
            strcpy(root->parent->text, root->text);
            root->parent->tp = createType();
            root->parent->tp->v.basic = 2;
        }
    }

    else if (root->flag == 4) { // TYPE
        root->tp = createType();
        if      (eq(root->text, "int"))   root->tp->v.basic = 0;   
        else if (eq(root->text, "float")) root->tp->v.basic = 1;
    }// note: STRUCT is just a symbol, unuseful. bxr: It's used by me.
    else if (root->flag == 6) {
        if(eq(root->type, "LC") && eq(root->parent->type, "StructSpecifier"))      cur_struct++;
        else if(eq(root->type, "RC") && eq(root->parent->type, "StructSpecifier")) cur_struct--;
        else if(eq(root->type, "LC") && eq(root->parent->type, "CompSt"))          cur_level++;
        else if(eq(root->type, "RC") && eq(root->parent->type, "CompSt")){
            
            if(mydebug){
                prs("-------------------------------------", "Before delete");
                pri();
            }
            // // 出了一层大括号嵌套,把该层的变量定义都删除.
            struct SymNode* symNode= symStack->stackTop;//note: symNode is also a morehead.
            struct SymNode* temp = NULL;
            while(symNode->last){
                if(symNode->last->kind == VAR && symNode->last->lv == cur_level){
                    temp = symNode->last;
                    delHashtable(symNode->last);
                    symNode->last = symNode->last->last;
                    free(temp);
                }else if(symNode->last->kind == VAR && symNode->last->lv < cur_level){
                    break;
                }
                else symNode = symNode->last;
            }
            cur_level--;
            if(mydebug){
                prs("-------------------------------------", "After delete");
                pri();
            }
        }
        return TRUE;
    }
    else if(root->flag == 5);
    else{
        pr(root->flag);
        assert(0);//return TRUE;
    }
    return TRUE;
}