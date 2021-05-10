#include "ir.h"
int n_labels = 0; // GLOBAL Labels
int n_temps = 0;  // GLOBAL Temps

extern struct InterCodes* interCodes;
extern struct InterCode** interCodesList;
extern struct InterCode** iCList;

int endInterCodesList = 0;
int endICList = 0;

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
    int nchild;    
};

struct StructVar_Temp svt[10];


//-------------------------------------by bxr-------------------------------------//

void optimizeX(){
    int rs = 1, ls = 0;
    while (ls < endInterCodesList){
        if(rs == endInterCodesList || interCodesList[rs]->kind == 2){
            rs -= optimizeBlock(ls, rs);// r不是最右下标，倒2.
            ls = rs; 
        }
        rs++;
    }
    return ;
}

int countc = 0;
int optimizeBlock(int l, int r){
    int numDelete = 0;
#ifdef FIRST
    for(int i = l;i < r - 2;i++){//注意这里只到 r-1
        //连续的lable，其实这个没意义，因为lable不算执行指令。
        if(interCodesList[i]->kind == 1 && interCodesList[i + 1]->kind == 1){ 
            int numLable = interCodesList[i]->u.kind_1_11.l->no;
            for(int j = l;j < r;j++){ //将跳转到i上的改到跳转到i+1上。
                if(interCodesList[j]->kind == 11 && interCodesList[j]->u.kind_1_11.l->no == numLable){
                    interCodesList[j]->u.kind_1_11.l->no = interCodesList[i + 1]->u.kind_1_11.l->no;
                }
                else if(interCodesList[j]->kind == 12 && interCodesList[j]->u.kind_12.l->no == numLable){
                    interCodesList[j]->u.kind_12.l->no = interCodesList[i + 1]->u.kind_1_11.l->no;
                }
            }
            interCodesListDelete(i); r--; i--; numDelete++;//注意这个r--删掉了这个block，不仅总的要减一，这个也要减一。
        }
        //跳转到自己的下一条语句
        else if(interCodesList[i]->kind == 11 && interCodesList[i + 1]->kind == 1 && \
            interCodesList[i]->u.kind_1_11.l->no == interCodesList[i + 1]->u.kind_1_11.l->no){
            interCodesListDelete(i); r--; i--; numDelete++;//注意这个r--删掉了这个block，不仅总的要减一，这个也要减一。
        }
        //跳转到自己的下下一条语句，并且中间是个无条件跳转语句
        else if(interCodesList[i]->kind == 12 && interCodesList[i + 1]->kind == 11 && \
            interCodesList[i + 2]->kind == 1 && \
            interCodesList[i]->u.kind_12.l->no == interCodesList[i + 2]->u.kind_1_11.l->no){
            //还得判断 i + 2 是不是只出现过一次

            int lableCount = 0;
            for(int k = l;lableCount < 2 && k < r;k++){
                if(interCodesList[k]->kind == 12 && \
                    interCodesList[k]->u.kind_12.l->no == interCodesList[i + 2]->u.kind_1_11.l->no)
                    lableCount++;
                if(interCodesList[k]->kind == 11 && \
                    interCodesList[k]->u.kind_1_11.l->no == interCodesList[i + 2]->u.kind_1_11.l->no)
                    lableCount++;
            }
            if(lableCount >= 2) continue;

            reverseRelop(i);//将relop翻转
            interCodesList[i]->u.kind_12.l->no = interCodesList[i + 1]->u.kind_1_11.l->no;
            interCodesListDelete(i+1); r--; numDelete++; // i--; //注意这个r--删掉了这个block，不仅总的要减一，这个也要减一。
            interCodesListDelete(i+1); r--; numDelete++; // i--; //注意这个r--删掉了这个block，不仅总的要减一，这个也要减一。
        }
    }
#endif

#ifdef SECOND
    for(int i = l;i < r - 1;i++){//注意这里只到 r-1
        // t25 := #0 RETURN t25，类似这种。
        if(interCodesList[i]->kind == 3 && interCodesList[i]->u.kind_389_10_16.result->kind == TEMP){// && \
            interCodesList[i]->u.kind_389_10_16.op1->kind == INT_CONST){
            struct Operand *res = interCodesList[i]->u.kind_389_10_16.result;
            struct Operand *op1 = interCodesList[i]->u.kind_389_10_16.op1;
            int counts = 0; int rem = 0;
            for(int j = i + 1;counts < 2 && j < r;j++)
                if(isExist(res, interCodesList[j])){
                    counts++;
                    if(counts == 1) rem = j;
                }
                
            if(counts == 1 && interCodesList[rem]->kind != 12 && \
            interCodesList[rem]->kind != 8 && interCodesList[rem]->kind != 9 && \
            interCodesList[rem]->kind != 10 && interCodesList[rem]->kind != 9){
                opReplace(op1, res, interCodesList[rem]);
                interCodesListDelete(i); r--; i--; numDelete++;//注意这个r--删掉了这个block，不仅总的要减一，这个也要减一。
            }
            else if(counts == 1 && (interCodesList[rem]->kind == 8 || \
              interCodesList[rem]->kind == 9 || interCodesList[rem]->kind == 10) && \
              interCodesList[i]->u.kind_389_10_16.op1->kind != INT_CONST){
                opReplace(op1, res, interCodesList[rem]);
                interCodesListDelete(i); r--; i--; numDelete++;//注意这个r--删掉了这个block，不仅总的要减一，这个也要减一。
            }
        }
        if(interCodesList[i]->kind >= 4 && interCodesList[i]->kind <= 7 && interCodesList[i]->u.kind_4567.result->kind == TEMP){// && \
            interCodesList[i]->u.kind_389_10_16.op1->kind == INT_CONST){
            // fpr("t2 := c + #1: ", 123);
            struct Operand *res = interCodesList[i]->u.kind_4567.result;
            struct Operand *op1 = interCodesList[i]->u.kind_4567.op1;
            int counts = 0; int rem = 0;
            for(int j = i + 1;counts < 2 && j < r;j++)
                if(isExist(res, interCodesList[j])){
                    counts++;
                    if(counts == 1) rem = j;
                }
            // fpr("rem:", rem);
            if(counts == 1 && interCodesList[rem]->kind == 3){
                opReplace(interCodesList[rem]->u.kind_389_10_16.result, \
                    interCodesList[i]->u.kind_4567.result, interCodesList[i]);
                free(interCodesList[rem]);
                interCodesList[rem] = interCodesList[i];
                interCodesList[i] = NULL; // bug
                interCodesListDelete(i); r--; i--; numDelete++;//注意这个r--删掉了这个block，不仅总的要减一，这个也要减一。
            }
        }
    }
#endif

#ifdef SB
    #ifdef THIRD // 这个白写了，理解错了，哎呀。
    for(int i = l;i < r - 1;i++){//注意这里只到 r-1
        if(interCodesList[i]->kind == 3 && \
            interCodesList[i]->u.kind_389_10_16.result->kind == TEMP && \
            interCodesList[i]->u.kind_389_10_16.op1->kind == INT_CONST){
            for(int j = i + 1;j < r;j++){
                if(countc < 5 && interCodesList[j]->kind == 3 && \
                  interCodesList[j]->u.kind_389_10_16.result->kind == TEMP && \
                  interCodesList[j]->u.kind_389_10_16.op1->kind == INT_CONST && \
                  eqOperand(interCodesList[j]->u.kind_389_10_16.result, interCodesList[i]->u.kind_389_10_16.result)){
                        // fpr("THIRD: ", j + 1 - countc); //break;
                        
                        // printInterCode(interCodesList[i]);
                        interCodesList[i] = NULL; countc++;
                        interCodesListDelete(i); r--; 
                        i -= 2; numDelete++; break; 
                }
                else if(isExist(interCodesList[i]->u.kind_389_10_16.op1, \
                    interCodesList[j])) break;
            }
        }
    }
    #endif
#endif

#ifdef ARRAY_ADDRESS
    for(int i = l;i < r;i++){
        // if(i == 2)fpr("-=-",interCodesList[i]->u.kind_389_10_16.op1->kind);
        if(interCodesList[i]->kind == 8 && \
        interCodesList[i]->u.kind_389_10_16.op1->kind == GET_VAR_ADDRESS){
            struct Operand *op = interCodesList[i]->u.kind_389_10_16.op1;
            struct Operand *rep = interCodesList[i]->u.kind_389_10_16.result;
            struct Operand *berep = NULL;
            for(int j = i + 1;j < r;j++){
                // fpr("s r: ", j);
                if(interCodesList[j]->kind == 8 && \
                interCodesList[j]->u.kind_389_10_16.op1->kind == GET_VAR_ADDRESS && \
                eqOperand(op, interCodesList[j]->u.kind_389_10_16.op1)){
                    berep = createOp(interCodesList[j]->u.kind_389_10_16.result);
                    interCodesListDelete(j); j--; r--; numDelete++;// i--; 这个i不用减。
                    for(int k = j + 1;k < r;k++){
                        if(isExist(berep, interCodesList[k])){
                            // fpr("ARRAY_ADDRESS: ", k);
                            opReplace(rep, berep, interCodesList[k]);
                        }
                    }
                }
            }
        }
    }
#endif

#ifdef FOURTH
    for(int i = l;i < r - 1;i++){//注意这里只到 r-1
        // t25 := #0 RETURN t25，类似这种。
        if((interCodesList[i]->kind == 3 || interCodesList[i]->kind == 16)&& \
          interCodesList[i]->u.kind_389_10_16.result->kind == VARIABLE){// && \
            interCodesList[i]->u.kind_389_10_16.op1->kind == INT_CONST){
            struct Operand *res = interCodesList[i]->u.kind_389_10_16.result;
            struct Operand *op1 = interCodesList[i]->u.kind_389_10_16.op1;
            int counts = 0; int rem = 0;
            for(int j = l;counts < 2 && j < r;j++){
                if(isExist(res, interCodesList[j])){
                    counts++;
                }
            }
            if(counts == 1){
                interCodesListDelete(i); r--; i = l; numDelete++;
            }
        }
    }
    for(int i = l;i < r - 1;i++){//注意这里只到 r-1
        if((interCodesList[i]->kind == 4 || interCodesList[i]->kind == 5) && \
          (interCodesList[i+1]->kind == 4 || interCodesList[i+1]->kind == 5) && \
          eqOperand(interCodesList[i]->u.kind_4567.result, interCodesList[i]->u.kind_4567.op1) && \
          eqOperand(interCodesList[i+1]->u.kind_4567.result, interCodesList[i+1]->u.kind_4567.op1) && \
          eqOperand(interCodesList[i]->u.kind_4567.result, interCodesList[i+1]->u.kind_4567.result) && \
          interCodesList[i]->u.kind_4567.op2->kind == INT_CONST && \
          interCodesList[i+1]->u.kind_4567.op2->kind == INT_CONST
          ){
            long long int Min = -2147483648, Max = 2147483647;
            long long int a = interCodesList[i]->u.kind_4567.op2->u.int_const;
            long long int b = interCodesList[i+1]->u.kind_4567.op2->u.int_const;
            if((interCodesList[i]->kind == 4 && interCodesList[i+1]->kind == 4)){
                if(a + b < Min || a + b > Max) continue;
                interCodesList[i]->u.kind_4567.op2->u.int_const += b;
                interCodesListDelete(i+1); r--; i--; numDelete++;
            }
            else if((interCodesList[i]->kind == 4 && interCodesList[i+1]->kind == 5)){
                b = -b;
                if(a + b < Min || a + b > Max) continue;
                if(a + b > 0){    
                    interCodesList[i]->u.kind_4567.op2->u.int_const += b;
                    interCodesListDelete(i+1); r--; i--; numDelete++;
                }
                else if(a + b < 0){
                    interCodesList[i+1]->u.kind_4567.op2->u.int_const = - a - b;
                    interCodesListDelete(i); r--; i--; numDelete++;
                }
                else{
                    interCodesListDelete(i); r--; i--; numDelete++;
                    interCodesListDelete(i+1); r--; i--; numDelete++;
                }
            }
            else if((interCodesList[i]->kind == 5 && interCodesList[i+1]->kind == 4)){
                a = -a;
                if(a + b < Min || a + b > Max) continue;
                if(a + b > 0){    
                    interCodesList[i+1]->u.kind_4567.op2->u.int_const += a;
                    interCodesListDelete(i); r--; i--; numDelete++;
                }
                else if(a + b < 0){
                    interCodesList[i]->u.kind_4567.op2->u.int_const = - a - b;
                    interCodesListDelete(i+1); r--; i--; numDelete++;
                }
                else{
                    interCodesListDelete(i); r--; i--; numDelete++;
                    interCodesListDelete(i+1); r--; i--; numDelete++;
                }
            }
            else if((interCodesList[i]->kind == 5 && interCodesList[i+1]->kind == 5)){
                if(a + b < Min || a + b > Max) continue;
                interCodesList[i]->u.kind_4567.op2->u.int_const += b;
                interCodesListDelete(i+1); r--; i--; numDelete++;
            }
        }
    }
#endif

#ifdef BLOCKDIVISION
    int a[1000]; int aCount = 0; a[aCount++] = l;
    for(int i = l + 1;i < r;i++){
        switch (interCodesList[i]->kind){
            case 12: case 11: a[aCount++] = i + 1; break;
            case 1: a[aCount++] = i; break;
            default: break;
        }
    }
    // for(int i = 0;i < aCount;i++) fpr(a[i]);
#endif

    for(int i = l, j = 0;i < r;i++){
        //fpr("print: ", i);
#ifdef BLOCKDIVISION
        if(j < aCount && i == a[j]){ 
            printf("\n"); 
            if(a[j] == a[j + 1]) j+=2; 
            else j++; 
        }    
#endif
        printInterCode(interCodesList[i]);
    }
    return numDelete;
}


void interCodesListDelete(int ind){ //删除一条三地址指令
    if(interCodesList[ind] != NULL)//其实并没有真的free掉，因为里面有指针
        free(interCodesList[ind]); 
    for(;ind < endInterCodesList - 1;ind++)
        interCodesList[ind] = interCodesList[ind + 1];
    endInterCodesList--;
    return ;
}

void reverseRelop(int ind){
    if(interCodesList[ind]->kind != 12) assert(0);
    switch (interCodesList[ind]->u.kind_12.r){
        case EQ: interCodesList[ind]->u.kind_12.r = NE; break;
        case NE: interCodesList[ind]->u.kind_12.r = EQ; break;
        case GE: interCodesList[ind]->u.kind_12.r = L;  break;
        case L:  interCodesList[ind]->u.kind_12.r = GE; break;
        case LE: interCodesList[ind]->u.kind_12.r = G;  break;
        case G:  interCodesList[ind]->u.kind_12.r = LE; break;
        default: assert(0); break;
    } return ;
}

Bool eqOperand(struct Operand* op1, struct Operand* op2){
    int res = 0;
    // fpr("op1kind:", op1->kind);
    // fpr("op2kind:", op2->kind);
    if(op1->kind == op2->kind){
        // fpr("opkind:", op1->kind);
        switch (op2->kind){
            case VARIABLE:          res =  (strcmp(op1->u.va_name, op2->u.va_name) == 0);   break;
            case FUNCTION:          res =  (strcmp(op1->u.va_name, op2->u.va_name) == 0);   break;
            case GET_TEMP_VALUE:    res =  (strcmp(op1->u.va_name, op2->u.va_name) == 0);   break;
            case GET_VAR_ADDRESS:   res =  (strcmp(op1->u.va_name, op2->u.va_name) == 0);   break;
            case FLOAT_CONST:       res =  (op1->u.float_const == op2->u.float_const);      break;
            case INT_CONST:         res =  (op1->u.int_const == op2->u.int_const);          break;
            case TEMP:              res =  (op1->u.tmp->no == op2->u.tmp->no);              break;
            case GET_TEMP_ADDRESS:  res =  (op1->u.tmp->no == op2->u.tmp->no);              break;
            case GET_VAR_VALUE:     res =  (op1->u.tmp->no == op2->u.tmp->no);              break;
            default: myassert(0); break;
        }
    }
    return res;
}

Bool isExist(struct Operand* op, struct InterCode* ic){
    // fpr("ickind:", ic->kind);
    switch (ic->kind) {
        case 1:  return 0; break;
        case 2:  return 0; break;
        case 11: return 0; break;
        case 13: return eqOperand(op, ic->u.kind_2_13_15_17_18_19.op); break;
        case 14: return eqOperand(op, ic->u.kind_14.op);               break;
        case 15: return eqOperand(op, ic->u.kind_2_13_15_17_18_19.op); break;
        case 16: return eqOperand(op, ic->u.kind_389_10_16.result);    break;
        case 17: return eqOperand(op, ic->u.kind_2_13_15_17_18_19.op); break;
        case 18: return eqOperand(op, ic->u.kind_2_13_15_17_18_19.op); break;
        case 19: return eqOperand(op, ic->u.kind_2_13_15_17_18_19.op); break;
        case 3: case 8: case 9: case 10: 
            return (eqOperand(op, ic->u.kind_389_10_16.op1) || \
                    eqOperand(op, ic->u.kind_389_10_16.result)); break; 
        case 4: case 5: case 6: case 7:
            return (eqOperand(op, ic->u.kind_4567.op1) || \
                    eqOperand(op, ic->u.kind_4567.op2) || 
                    eqOperand(op, ic->u.kind_4567.result)); break; 
        case 12: return (eqOperand(op, ic->u.kind_12.op1) || \
                         eqOperand(op, ic->u.kind_12.op2)); break;
        default: myassert(0);
    }
    assert(0);
}

void opReplace(struct Operand* op1, struct Operand* op2, struct InterCode* ic){
    //用op1，替代ic中和op2一样的op。
    switch (ic->kind) {
        case 1:  return; break;
        case 2:  return; break;
        case 11: return; break;
        case 14: 
            free(ic->u.kind_14.op);
            ic->u.kind_14.op = createOp(op1);
        case 13: case 15: case 17: case 18: case 19:
            free(ic->u.kind_2_13_15_17_18_19.op);
            ic->u.kind_2_13_15_17_18_19.op = createOp(op1); break;
        case 16:
            free(ic->u.kind_389_10_16.result);
            ic->u.kind_389_10_16.result = createOp(op1); break;
        case 3: case 8: case 9: case 10: 
            if(eqOperand(op2, ic->u.kind_389_10_16.op1)){
                free(ic->u.kind_389_10_16.op1);
                ic->u.kind_389_10_16.op1 = createOp(op1); break;
            }
            else if(eqOperand(op2, ic->u.kind_389_10_16.result)){
                // assert(0);
                free(ic->u.kind_389_10_16.result);
                ic->u.kind_389_10_16.result = createOp(op1); break;
            }
        case 4: case 5: case 6: case 7:
            if(eqOperand(op2, ic->u.kind_4567.op1)){
                free(ic->u.kind_4567.op1);
                ic->u.kind_4567.op1 = createOp(op1); break;
            }
            else if(eqOperand(op2, ic->u.kind_4567.op2)){
                free(ic->u.kind_4567.op2);
                ic->u.kind_4567.op2 = createOp(op1); break;
            }
            else if(eqOperand(op2, ic->u.kind_4567.result)){
                // assert(0);
                free(ic->u.kind_4567.result);
                ic->u.kind_4567.result = createOp(op1); break;
            }
        case 12:
            if(eqOperand(op2, ic->u.kind_12.op1)){
                free(ic->u.kind_12.op1);
                ic->u.kind_12.op1 = createOp(op1); break;
            }
            else if(eqOperand(op2, ic->u.kind_12.op2)){
                free(ic->u.kind_12.op2);
                ic->u.kind_12.op2 = createOp(op1); break;
            }
        default: myassert(0);
    }
    return ;
}

struct Operand* createOp(struct Operand* op){
    // fpr("createOp: ", op->kind);
    // fprs("-------=--------", op->u.va_name);
    struct Operand* newOp = malloc(sizeof(struct Operand));
    newOp->kind = op->kind;
    switch(op->kind) {
        case INT_CONST:         newOp->u.int_const   = op->u.int_const;     break;
        case FLOAT_CONST:       newOp->u.float_const = op->u.float_const;   break;
        case GET_VAR_ADDRESS:   strcpy(newOp->u.va_name, op->u.va_name);    break;
        case GET_VAR_VALUE:     strcpy(newOp->u.va_name, op->u.va_name);    break;
        case VARIABLE:          strcpy(newOp->u.va_name, op->u.va_name);    break;
        case FUNCTION:          strcpy(newOp->u.va_name, op->u.va_name);    break;
        case TEMP: case GET_TEMP_ADDRESS:  case GET_TEMP_VALUE:
            newOp->u.tmp = malloc(sizeof(struct Temp));//居然没分配空间。。。
            newOp->u.tmp->isAddress = op->u.tmp->isAddress;
            newOp->u.tmp->no = op->u.tmp->no;                               break;
        default: assert(0);
     }
    // fprs("-------=--------", newOp->u.va_name);
    return newOp;
}
//-------------------------------------by bxr-------------------------------------//

int first_def = 1;
void printInterCode(struct InterCode* ic) {
    switch (ic->kind) {
        case 1: printf("LABEL "); printALabel(ic->u.kind_1_11.l); printf(" :\n"); break;
        case 2: if (first_def == 1) { first_def = 0; } else { printf("\n"); } // First function declaration doesn't need an interval blank.
                printf("FUNCTION "); printAnOperand(ic->u.kind_2_13_15_17_18_19.op); printf(" :\n"); break;
        case 3: case 8: case 9: case 10:
                printAnOperand(ic->u.kind_389_10_16.result); printf(" := "); 
                printAnOperand(ic->u.kind_389_10_16.op1); printf("\n"); break;
        case 4: case 5: case 6: case 7:
                printAnOperand(ic->u.kind_4567.result); printf(" := "); printAnOperand(ic->u.kind_4567.op1); 
                if (ic->kind == 4) printf(" + "); else if (ic->kind == 5) printf(" - "); else if (ic->kind == 6) printf(" * "); else printf(" / ");
                printAnOperand(ic->u.kind_4567.op2); printf("\n"); break;        
        case 11: printf("GOTO "); printALabel(ic->u.kind_1_11.l); printf("\n"); break;
        case 12: printf("IF "); printAnOperand(ic->u.kind_12.op1); printf(" "); printARelop(ic->u.kind_12.r); printf(" ");
                 printAnOperand(ic->u.kind_12.op2); printf(" GOTO "); printALabel(ic->u.kind_12.l); printf("\n"); break;
        case 13: printf("RETURN "); printAnOperand(ic->u.kind_2_13_15_17_18_19.op); printf("\n"); break;
        case 14: printf("DEC "); printAnOperand(ic->u.kind_14.op); printf(" %d\n", ic->u.kind_14.sz); break;
        case 15: printf("ARG "); printAnOperand(ic->u.kind_2_13_15_17_18_19.op); printf("\n"); break;
        case 16: printAnOperand(ic->u.kind_389_10_16.result); printf(" := CALL "); printAnOperand(ic->u.kind_389_10_16.op1); printf("\n"); break; 
        case 17: printf("PARAM "); printAnOperand(ic->u.kind_2_13_15_17_18_19.op); printf("\n"); break;
        case 18: printf("READ ");  printAnOperand(ic->u.kind_2_13_15_17_18_19.op); printf("\n"); break;
        case 19: printf("WRITE ");  printAnOperand(ic->u.kind_2_13_15_17_18_19.op); printf("\n"); break;
        default: myassert(0);
    }
}

void printInterCodes() {
    executeCheck();
#ifdef OPTIMIZE
    for (struct InterCodes* ic = interCodes->next; ic != interCodes;\
        ic = ic->next, endInterCodesList++)
        interCodesList[endInterCodesList] = ic->code;
    optimizeX();
#else
    for (struct InterCodes* ic = interCodes->next; ic != interCodes; ic = ic->next) 
        printInterCode(ic->code);
#endif
}

void printAnOperand(struct Operand* op) {
    char* t;
    switch(op->kind) {
        case VARIABLE: case FUNCTION: printf("%s", op->u.va_name); break;
        case TEMP: t = tempString(op->u.tmp); printf("%s",t); free(t); break;
        case INT_CONST: printf("#%d", op->u.int_const); break;
        case FLOAT_CONST: printf("#%f", op->u.float_const); break;
        case GET_TEMP_ADDRESS: t = tempString(op->u.tmp); printf("&%s",t); free(t); break;
        case GET_VAR_ADDRESS: printf("&%s", op->u.va_name); break;
        case GET_VAR_VALUE: printf("*%s", op->u.va_name); break;
        case GET_TEMP_VALUE: t = tempString(op->u.tmp); printf("*%s",t); free(t); break;
        default: myassert(0);
     }
}
void printALabel(struct Label* l) { char *t = labelString(l); printf("%s", t); free(t); }

void printARelop(enum Relop r) { 
    switch(r) {
        case EQ: printf("=="); break;
        case NE: printf("!="); break;
        case GE: printf(">="); break;
        case G: printf(">"); break;
        case LE: printf("<="); break;
        case L: printf("<"); break;
    }
}

void svt_clear() {
    for (int i = 0; i < 10; i++) {
        strcpy(svt[i].name, "");
        svt[i].t = NULL;
    }
}

struct Temp* svt_search(char* name) {
    assert(name != NULL);
     for (int i = 0; i < 10; i++) {
        if (eq(name, svt[i].name)) {
            assert(svt[i].t);
            return svt[i].t;
        }
    }
    return NULL;
}

void svt_write(struct Node* root, int i) { // root->type is VarList
    if (i == 10) assert(0);
    if (kth(kth(root,1),1)->tp->kind == STRUCTURE) { 
        char* n = kth(kth(kth(root,1),2),1)->text;
        struct Temp* t = createTemp(); t->isAddress = TRUE;
        strcpy(svt[i].name, n);
        svt[i].t = t;
        i = i + 1;
    }
    if (root->nchild == 3) svt_write(kth(root,3), i);
}

void svt_printCode() {
    for (int i = 0; i < 10; i++) {
        if (eq(svt[i].name,"") == FALSE && svt[i].t != NULL) {
            // tmp = struct_var
            struct InterCode* ic = createInterCode(3);
            ic->u.kind_389_10_16.op1->kind = VARIABLE;
            strcpy(ic->u.kind_389_10_16.op1->u.va_name, svt[i].name);
            ic->u.kind_389_10_16.result->kind = TEMP;
            ic->u.kind_389_10_16.result->u.tmp = svt[i].t;
            insertInterCode(ic);
        }
    }
}

struct Label* createLabel() { struct Label* l = malloc(sizeof(struct Label)); l->no = ++n_labels; }  // start from 1

char* labelString(struct Label* l) { 
    char* str = malloc(8*sizeof(char)); 
    str[0] = 'L'; int i = l->no, j = l->no, n = 0;
    while (i != 0) { i /= 10; n++; } myassert(n <= 6);
    for (int k = n; k > 0; k--) { str[k] = '0' + j % 10; j /= 10; }
    for (int k = n+1; k <= 7; k++) str[k] = '\0'; 
    return str;
}

struct Temp* createTemp() { struct Temp* t = malloc(sizeof(struct Temp)); t->isAddress = FALSE; t->no = ++n_temps; } // start from 1

char* tempString(struct Temp* t) {
    char* str = malloc(8*sizeof(char)); 
    str[0] = 't'; int i = t->no, j = t->no, n = 0;
    while (i != 0) { i /= 10; n++; } myassert(n <= 6);
    for (int k = n; k > 0; k--) { str[k] = '0' + j % 10; j /= 10; }
    for (int k = n+1; k <= 7; k++) str[k] = '\0'; 
    return str;
}

struct InterCodes* createInterCodeList() {
    struct InterCodes* list = malloc(sizeof(struct InterCodes));
    list->code = malloc(sizeof(struct InterCode));
    list->code->kind = 0; // IDLE
    list->next = list;
    list->prev = list;
    return list;
}

void insertInterCode(struct InterCode* ic) {
    struct InterCodes* ics = malloc(sizeof(struct InterCodes));
    ics->code = ic;
    ics->next = interCodes;
    ics->prev = interCodes->prev;
    interCodes->prev->next = ics;
    interCodes->prev = ics;
}

struct InterCode* createInterCode(int i) { 
    struct InterCode* ic = malloc(sizeof(struct InterCode));
    ic->kind = i;
    switch(i) {
        case 1: case 11: break;
        case 2: case 13: case 15: case 17: case 18: case 19:
        ic->u.kind_2_13_15_17_18_19.op = malloc(sizeof(struct Operand)); break;
        case 3: case 8: case 9: case 10: case 16:
        ic->u.kind_389_10_16.op1 = malloc(sizeof(struct Operand)); 
        ic->u.kind_389_10_16.result = malloc(sizeof(struct Operand)); break;
        case 4: case 5: case 6: case 7:
        ic->u.kind_4567.op1 = malloc(sizeof(struct Operand)); 
        ic->u.kind_4567.op2 = malloc(sizeof(struct Operand)); 
        ic->u.kind_4567.result = malloc(sizeof(struct Operand)); break;
        case 12:
        ic->u.kind_12.op1 = malloc(sizeof(struct Operand)); 
        ic->u.kind_12.op2 = malloc(sizeof(struct Operand)); break;
        case 14:
        ic->u.kind_14.op = malloc(sizeof(struct Operand));
        ic->u.kind_14.sz = 0; break;
    }
    return ic;
}

// enum Relop { EQ, NE, GE, G, LE, L }; // ==, !=, >=, >, <=, <
    // struct InterCode {
    //     int kind;             // P64, 1-19
    //     union {
    //         struct { struct Label* l; } kind_1_11;
    //         struct { struct Operand* op; } kind_2_13_15_17_18_19;
    //         struct { struct Operand* op1, *result; } kind_389_10_16;
    //         struct { struct Operand* op1, *op2, *result; } kind_4567;
    //         struct { struct Operand* op1, *op2, *result; enum Relop r; } kind_12;
    //         struct { struct Operand* op; int sz; } kind_14;
    //     } u;
    // };

    // struct InterCodes { struct InterCode code; struct InterCodes* prev, *next; };


/* DEBUG */

void icCheck(struct InterCode* ic) {
    if (!__LAB3_DEBUG__) return;
    switch(ic->kind) {
        case 1: case 11:
                break;
        case 2: myassert(ic->u.kind_2_13_15_17_18_19.op->kind == FUNCTION); 
                break;
        case 3: myassert(eq2(ic->u.kind_389_10_16.result->kind, TEMP, VARIABLE)); 
                myassert(eq4(ic->u.kind_389_10_16.op1->kind, TEMP, VARIABLE, INT_CONST, FLOAT_CONST)); 
                break;
        case 4: case 5: case 6: case 7:
                myassert(eq2(ic->u.kind_4567.result->kind, TEMP, VARIABLE));
                myassert(eq6(ic->u.kind_4567.op1->kind, TEMP, VARIABLE, INT_CONST, FLOAT_CONST, GET_TEMP_ADDRESS, GET_VAR_ADDRESS)); // exception mentioned on P65
                myassert(eq4(ic->u.kind_4567.op2->kind, TEMP, VARIABLE, INT_CONST, FLOAT_CONST)); 
                break;
        case 8: myassert(eq2(ic->u.kind_389_10_16.result->kind, TEMP, VARIABLE));
                myassert(eq2(ic->u.kind_389_10_16.op1->kind, GET_TEMP_ADDRESS, GET_VAR_ADDRESS)); 
                break;
        case 9: myassert(eq2(ic->u.kind_389_10_16.result->kind, TEMP, VARIABLE));
                myassert(eq2(ic->u.kind_389_10_16.op1->kind, GET_TEMP_VALUE, GET_VAR_VALUE)); 
                break;
        case 10: 
                myassert(eq2(ic->u.kind_389_10_16.result->kind, GET_TEMP_VALUE, GET_VAR_VALUE));
                myassert(eq4(ic->u.kind_389_10_16.op1->kind, TEMP, VARIABLE, INT_CONST, FLOAT_CONST));
                break;
        case 12: 
                myassert(eq4(ic->u.kind_12.op1->kind, TEMP, VARIABLE, INT_CONST, FLOAT_CONST)); 
                myassert(eq4(ic->u.kind_12.op2->kind, TEMP, VARIABLE, INT_CONST, FLOAT_CONST)); 
                break;
        case 13: case 15:
                myassert(eq6(ic->u.kind_2_13_15_17_18_19.op->kind, TEMP, VARIABLE, INT_CONST, FLOAT_CONST, GET_TEMP_ADDRESS, GET_VAR_ADDRESS)); 
                break;
        case 14: 
                myassert(ic->u.kind_14.op->kind == VARIABLE);
                myassert(ic->u.kind_14.sz % 4 == 0);
                break;
        case 16: 
                myassert(eq2(ic->u.kind_389_10_16.result->kind, TEMP, VARIABLE));
                myassert(ic->u.kind_389_10_16.op1->kind == FUNCTION);
                break;
        case 17: case 18: 
                myassert(eq2(ic->u.kind_2_13_15_17_18_19.op->kind, TEMP, VARIABLE));
                break;
        case 19:
                myassert(eq3(ic->u.kind_2_13_15_17_18_19.op->kind, TEMP, VARIABLE, INT_CONST));
                break;
        default: myassert(0);
    }
}

void executeCheck() { 
    for (struct InterCodes* ic = interCodes->next; ic != interCodes; ic = ic->next) 
        icCheck(ic->code); 
}

/* DEBUG */

// --------------------

void translate_Program(struct Node* root) {
    translate_ExtDefList(kth(root, 1));
}

void translate_ExtDefList(struct Node* root) {
    if (root->nchild == 0) return;
    translate_ExtDef(kth(root, 1));
    translate_ExtDefList(kth(root, 2));
}

void translate_ExtDef(struct Node* root) {
    if (root->nchild == 2) { /* Do nothing */ }
    else {
        myassert(eq(kth(root,2)->type, "FunDec")); // No global variables
        translate_FunDec(kth(root,1), kth(root,2));
        translate_CompSt(kth(root,3));
    }
}

void translate_FunDec(struct Node* spec, struct Node* root) {
    // Insertion
    struct InterCode* intercode = createInterCode(2);
    struct Operand* op = intercode->u.kind_2_13_15_17_18_19.op;
    op->kind = FUNCTION;
    strcpy(op->u.va_name, kth(root, 1)->text);
    insertInterCode(intercode);
    // Recursive calling
    if (root->nchild == 4) {
        translate_VarList(kth(root, 3));
        svt_clear();
        svt_write(kth(root,3), 0);
        svt_printCode();
    }
} 

void translate_VarList(struct Node* root) {
    translate_ParamDec(kth(root, 1));
    if (root->nchild == 3) translate_VarList(kth(root, 3));  
}

void translate_ParamDec(struct Node* root) {
    translate_VarDec(NULL, kth(root, 2), TRUE); // No check for specifier ( struct { xxx array[yyy]; }; )
}


void translate_VarDec(struct Node* spec, struct Node* root, Bool inFunc) {
    if (inFunc == TRUE) { // PARAM x
        struct InterCode* ic = createInterCode(17);
        struct Operand* op = ic->u.kind_2_13_15_17_18_19.op;
        op->kind = VARIABLE; strcpy(op->u.va_name, kth(root,1)->text);
        insertInterCode(ic);
    } else { // inFunc == FALSE
        if (root->nchild == 1 && spec->tp->kind == BASIC) return;
        int sz = typeSize(spec->tp);
        if (root->nchild == 4) {
            myassert(root->tp->kind == ARRAY);
            sz *= root->tp->v.array.size;
        }
        // DEC va_name size
        struct InterCode* ic = createInterCode(14);
        ic->u.kind_14.sz = sz;
        ic->u.kind_14.op->kind = VARIABLE;
        strcpy(ic->u.kind_14.op->u.va_name, kth(root,1)->text);
        insertInterCode(ic);
    }
}

void translate_CompSt(struct Node* root) {
    translate_DefList(kth(root,2));
    translate_StmtList(kth(root,3));
}

void translate_StmtList(struct Node* root) {
    if (root->nchild == 0) return;
    translate_Stmt(kth(root,1));
    translate_StmtList(kth(root,2));
}

// kind == 0: unknown; kind == 1: var; kind == 2: temp
struct Place* createPlace(int kind, char** va_name, struct Temp* tmp) { // create a 'place' struct (for easy-reading)
    struct Place* p = malloc(sizeof(struct Place));
    struct Place* q = p;
    p->kind = kind;
    p->u.tmp = tmp;
    p->u.va_name = va_name;
    return p;
}

void translate_Stmt(struct Node* root) {
    switch(root->nchild) {
        case 1: translate_CompSt(kth(root,1)); break; // CompSt
        case 2: translate_Exp(kth(root,1), NULL); break; // Exp SEMI
        case 3: { // RETURN Exp SEMI
            struct Temp* t1 = createTemp(); struct Place* p1 = createPlace(2, NULL, t1);
            struct Temp* t2 = NULL;
            translate_Exp(kth(root,2), p1);
            if (t1->isAddress == FALSE || eq2(kth(root,2)->tp->kind, ARRAY, STRUCTURE)) t2 = t1;
            else { // t2 = *t1;
                t2 = createTemp();
                struct InterCode* ic1 = createInterCode(9);
                ic1->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
                ic1->u.kind_389_10_16.op1->u.tmp = t1;
                ic1->u.kind_389_10_16.result->kind = TEMP;
                ic1->u.kind_389_10_16.result->u.tmp = t2;
                insertInterCode(ic1);
            }
            // RETURN x
            struct InterCode* ic2 = createInterCode(13);
            ic2->u.kind_2_13_15_17_18_19.op->kind = TEMP;
            ic2->u.kind_2_13_15_17_18_19.op->u.tmp = t2;
            insertInterCode(ic2);
            break; 
        }
        case 5: {
            if (eq(kth(root,1)->type, "IF")) { // IF LP Exp RP Stmt
                struct Label* label1 = createLabel();
                struct Label* label2 = createLabel();
                // Condition
                translate_Cond(kth(root,3), label1, label2);
                // LABEL L1:
                struct InterCode* ic1 = createInterCode(1);
                ic1->u.kind_1_11.l = label1;
                insertInterCode(ic1);
                // Statement
                translate_Stmt(kth(root,5));
                // LABEL L2:
                struct InterCode* ic2 = createInterCode(1);
                ic2->u.kind_1_11.l = label2;
                insertInterCode(ic2);
                break;
            } else { // WHILE LP Exp RP Stmt 
                struct Label* label1 = createLabel();
                struct Label* label2 = createLabel();
                struct Label* label3 = createLabel();
                // LABEL L1:
                struct InterCode* ic1 = createInterCode(1);
                ic1->u.kind_1_11.l = label1;
                insertInterCode(ic1);
                // Cond:
                translate_Cond(kth(root,3), label2, label3);
                // LABEL L2:
                struct InterCode* ic2 = createInterCode(1);
                ic2->u.kind_1_11.l = label2;
                insertInterCode(ic2);
                // Stmt:
                translate_Stmt(kth(root,5));
                // GOTO L1:
                struct InterCode* ic = createInterCode(11);
                ic->u.kind_1_11.l = label1;
                insertInterCode(ic);
                // LABEL L3:
                struct InterCode* ic3 = createInterCode(1);
                ic3->u.kind_1_11.l = label3;
                insertInterCode(ic3);
                break; 
            } 
        }
        case 7: { // IF LP Exp RP Stmt ELSE Stmt
            struct Label* label1 = createLabel();
            struct Label* label2 = createLabel();
            struct Label* label3 = createLabel();
            // Cond:
            translate_Cond(kth(root,3), label1, label2);
            // LABEL L1:
            struct InterCode* ic1 = createInterCode(1);
            ic1->u.kind_1_11.l = label1;
            insertInterCode(ic1);
            // Stmt_1:
            translate_Stmt(kth(root,5));
            // GOTO L3:
            struct InterCode* ic = createInterCode(11);
            ic->u.kind_1_11.l = label3;
            insertInterCode(ic);
            // LABEL L2:
            struct InterCode* ic2 = createInterCode(1);
            ic2->u.kind_1_11.l = label2;
            insertInterCode(ic2);
            // Stmt_2:
            translate_Stmt(kth(root,7));
            // LABEL L3:
            struct InterCode* ic3 = createInterCode(1);
            ic3->u.kind_1_11.l = label3;
            insertInterCode(ic3);
            break;
        }
    }   
}

void translate_Exp(struct Node* root, struct Place* place) {
    if (root->nchild == 1) {
        if (place == NULL) return; // No side effect, so, for stmt like:"1; 1.1; id;", no code needs to be generated
        struct InterCode* ic = createInterCode(3);
        if (eq(kth(root,1)->type, "INT")) { // INT
            // x := int_const
            ic->u.kind_389_10_16.op1->kind = INT_CONST;
            sscanf(root->text, "%d", &ic->u.kind_389_10_16.op1->u.int_const);
        } else if (eq(kth(root,1)->type, "FLOAT")) { // FLOAT
            // x := float_const
            ic->u.kind_389_10_16.op1->kind = FLOAT_CONST;
            sscanf(root->text, "%f", &ic->u.kind_389_10_16.op1->u.float_const);
        } else { // ID
            // x := [&]some_var
            if (root->tp->kind == BASIC) {
                ic->u.kind_389_10_16.op1->kind = VARIABLE;
                strcpy(ic->u.kind_389_10_16.op1->u.va_name, kth(root,1)->text);
            } else if (root->tp->kind == STRUCTURE && svt_search(kth(root,1)->text) != NULL) {
                ic->u.kind_389_10_16.op1->kind = TEMP;
                ic->u.kind_389_10_16.op1->u.tmp = svt_search(kth(root,1)->text);
                place->u.tmp->isAddress = TRUE;
            } else { // root->tp->kind == ARRAY or STRUCTURE
                ic->kind = 8; // NOTE! ic->kind has changed!
                ic->u.kind_389_10_16.op1->kind = GET_VAR_ADDRESS;
                strcpy(ic->u.kind_389_10_16.op1->u.va_name, kth(root,1)->text);
                place->kind = 2;
                place->u.tmp->isAddress = TRUE;
            }
        }
        if (place->kind == 1) { // not sure whether it functions
            ic->u.kind_389_10_16.result->kind = VARIABLE;
            strcpy(ic->u.kind_389_10_16.result->u.va_name, *(place->u.va_name));
        } else if (place->kind == 2) { 
            ic->u.kind_389_10_16.result->kind = TEMP;
            ic->u.kind_389_10_16.result->u.tmp = place->u.tmp; 
        } else { // place->kind == 0: Special case designed for assignment lvalue. InterCode ic is not needed. So below goes to a direct 'return'.
            myassert(eq(kth(root,1)->type, "ID"));
            place->kind = 1; // Pass to above
            char** t = place->u.va_name; *t = kth(root,1)->text;
            return;
        }
        insertInterCode(ic);
    } else if (root->nchild == 2 && eq(kth(root,1)->type, "MINUS")) { // MINUS Exp: this branch is a little bit lengthy (by me)
        // if (place == NULL) return; // Side effect exists such as "-(a=2);", optimization method exists.
        if (place == NULL) place = createPlace(2, NULL, createTemp());
        struct Temp* t1 = createTemp();
        struct Place* p1 = createPlace(2, NULL, t1);
        // code1
        translate_Exp(kth(root, 2), p1);
        if (p1->u.tmp->isAddress == FALSE || eq2(kth(root,2)->tp->kind, ARRAY, STRUCTURE)) {
            // place := #0 - t1
            struct InterCode* ic = createInterCode(5);
            ic->u.kind_4567.op1->kind = INT_CONST; ic->u.kind_4567.op1->u.int_const = 0;
            ic->u.kind_4567.op2->kind = TEMP;      ic->u.kind_4567.op2->u.tmp = t1;
            if (place->kind == 1) {
                ic->u.kind_4567.result->kind = VARIABLE;
                strcpy(ic->u.kind_4567.result->u.va_name, *(place->u.va_name));
            } else { 
                ic->u.kind_4567.result->kind = TEMP;
                ic->u.kind_4567.result->u.tmp = place->u.tmp; 
            }
            insertInterCode(ic);
        } else { // p1->u.tmp->isAddress == TRUE: e.g. -arr[yyy], -struct_A.yyy, etc.
            // t2 := *t1
            struct Temp* t2 = createTemp();
            struct InterCode* ic1 = createInterCode(9);
            ic1->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE; ic1->u.kind_389_10_16.op1->u.tmp = t1;
            ic1->u.kind_389_10_16.result->kind = TEMP;        ic1->u.kind_389_10_16.result->u.tmp = t2;
            insertInterCode(ic1);
            // place := #0 - t2
            struct InterCode* ic2 = createInterCode(5);
            ic2->u.kind_4567.op1->kind = INT_CONST; ic2->u.kind_4567.op1->u.int_const = 0;
            ic2->u.kind_4567.op2->kind = TEMP;      ic2->u.kind_4567.op2->u.tmp = t2;
            if (place->kind == 1) {
                ic2->u.kind_4567.result->kind = VARIABLE;
                strcpy(ic2->u.kind_4567.result->u.va_name, *(place->u.va_name));
            } else { 
                ic2->u.kind_4567.result->kind = TEMP;
                ic2->u.kind_4567.result->u.tmp = place->u.tmp; 
            }
            insertInterCode(ic2);
        }
    } else if ((root->nchild == 2 && eq(kth(root,1)->type, "NOT")) || 
        (root->nchild == 3 && (eq(kth(root,2)->type, "RELOP") || eq(kth(root,2)->type, "AND") || eq(kth(root,2)->type, "OR")))) {
        // if (place == NULL) return; // Side effect exists such as "a&&(b=2);", optimization method exists.
        if (place == NULL) place = createPlace(2, NULL, createTemp());
        struct Label* label1 = createLabel();
        struct Label* label2 = createLabel();
        // place := #0
        struct InterCode* ic1 = createInterCode(3);
        ic1->u.kind_389_10_16.op1->kind = INT_CONST;
        ic1->u.kind_389_10_16.op1->u.int_const = 0;
        if (place->kind == 1) {
            ic1->u.kind_389_10_16.result->kind = VARIABLE;
            strcpy(ic1->u.kind_389_10_16.result->u.va_name, *(place->u.va_name));
        } else { 
            ic1->u.kind_389_10_16.result->kind = TEMP;
            ic1->u.kind_389_10_16.result->u.tmp = place->u.tmp; 
        }
        insertInterCode(ic1);
        // Cond:
        translate_Cond(root, label1, label2);
        // LABEL label1:
        struct InterCode* ic2 = createInterCode(1);
        ic2->u.kind_1_11.l = label1;
        insertInterCode(ic2);
        // place := #1 (DO NOT just change above code by 'u.int_const = 1' and re-insert!)
        // PLEASE create new 'InterCode'
        struct InterCode* ic3 = createInterCode(3);
        ic3->u.kind_389_10_16.op1->kind = INT_CONST;
        ic3->u.kind_389_10_16.op1->u.int_const = 1;
        if (place->kind == 1) {
            ic3->u.kind_389_10_16.result->kind = VARIABLE;
            strcpy(ic3->u.kind_389_10_16.result->u.va_name, *(place->u.va_name));
        } else { 
            ic3->u.kind_389_10_16.result->kind = TEMP;
            ic3->u.kind_389_10_16.result->u.tmp = place->u.tmp; 
        }
        insertInterCode(ic3);
        // LABEL label2:
        struct InterCode* ic4 = createInterCode(1);
        ic4->u.kind_1_11.l = label2;
        insertInterCode(ic4);
    } else if (root->nchild == 3 && (eq(kth(root,2)->type,"PLUS") || eq(kth(root,2)->type,"MINUS") 
        || eq(kth(root,2)->type,"STAR") || eq(kth(root,2)->type,"DIV"))) {
        // if (place == NULL) return; // Side effect exists such as: "a+(b=2)", optimization method exists.
        if (place == NULL) place = createPlace(2, NULL, createTemp());                                      
        struct Temp* tmp1 = createTemp(); struct Place* p1 = createPlace(2, NULL, tmp1);                    
        struct Temp* tmp2 = createTemp(); struct Place* p2 = createPlace(2, NULL, tmp2);
        // code1                         // code2
        translate_Exp(kth(root,1), p1);  translate_Exp(kth(root,3), p2);
        struct Temp* tmp3 = NULL; struct Temp* tmp4 = NULL;
        if (tmp1->isAddress == FALSE || eq2(kth(root,1)->tp->kind, ARRAY, STRUCTURE)) tmp3 = tmp1;
        else { 
            // NEW CODE: tmp3 = *tmp1
            tmp3 = createTemp();
            struct InterCode* ic2 = createInterCode(9);
            ic2->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic2->u.kind_389_10_16.op1->u.tmp = tmp1;
            ic2->u.kind_389_10_16.result->kind = TEMP;
            ic2->u.kind_389_10_16.result->u.tmp = tmp3;
            insertInterCode(ic2);
        }
        if (tmp2->isAddress == FALSE || eq2(kth(root,3)->tp->kind, ARRAY, STRUCTURE)) tmp4 = tmp2;
        else { 
            // NEW CODE: tmp4 = *tmp2
            tmp4 = createTemp();
            struct InterCode* ic3 = createInterCode(9);
            ic3->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic3->u.kind_389_10_16.op1->u.tmp = tmp2;
            ic3->u.kind_389_10_16.result->kind = TEMP;
            ic3->u.kind_389_10_16.result->u.tmp = tmp4;
            insertInterCode(ic3);
        }
        // place := tmp3 [+-*/] tmp4
        struct InterCode* ic = createInterCode(4); 
        if (eq(kth(root,2)->type, "PLUS")) ic->kind = 4;
        else if (eq(kth(root,2)->type, "MINUS")) ic->kind = 5;
        else if (eq(kth(root,2)->type, "STAR")) ic->kind = 6;
        else ic->kind = 7;
        ic->u.kind_4567.op1->kind = TEMP; ic->u.kind_4567.op1->u.tmp = tmp3;
        ic->u.kind_4567.op2->kind = TEMP; ic->u.kind_4567.op2->u.tmp = tmp4;
        if (place->kind == 1) {
            ic->u.kind_4567.result->kind = VARIABLE;
            strcpy(ic->u.kind_4567.result->u.va_name, *(place->u.va_name));
        } else { 
            ic->u.kind_4567.result->kind = TEMP;
            ic->u.kind_4567.result->u.tmp = place->u.tmp; 
        }
        insertInterCode(ic);
    } else if (root->nchild == 3 && eq(kth(root,1)->type, "LP")) { // LP Exp RP
        translate_Exp(kth(root,2), place);
    } else if (root->nchild == 3 && eq(kth(root,1)->type, "ID")) { // ID LP RP
        if (eq(kth(root,1)->text, "read")) {
            // Do not deal with 'place'
            myassert(root->parent->nchild == 3 && eq(kth(root->parent,2)->type,"ASSIGNOP"));
            struct Temp* t1 = createTemp(); char* var_name = NULL; struct Place* p1 = createPlace(0, &var_name, t1);
            translate_Exp(kth(root->parent,1), p1);
            // READ x
            struct InterCode* ic = createInterCode(18);
            if (p1->kind == 1) {
                // READ var_name
                ic->u.kind_2_13_15_17_18_19.op->kind = VARIABLE;
                strcpy(ic->u.kind_2_13_15_17_18_19.op->u.va_name, var_name);
                insertInterCode(ic);
            } else if (p1->kind == 2) {
                // READ t2
                myassert(p1->u.tmp->isAddress == TRUE); 
                struct Temp* t2 = createTemp();
                ic->u.kind_2_13_15_17_18_19.op->kind = TEMP;
                ic->u.kind_2_13_15_17_18_19.op->u.tmp = t2;
                insertInterCode(ic);
                // *t1 = t2
                struct InterCode* ic2 = createInterCode(10);
                ic2->u.kind_389_10_16.op1->kind = TEMP;
                ic2->u.kind_389_10_16.op1->u.tmp = t2;
                ic2->u.kind_389_10_16.result->kind = GET_TEMP_VALUE;
                ic2->u.kind_389_10_16.result->u.tmp = t1;
                insertInterCode(ic2);
            } else myassert(0);
        } else {
            if (place == NULL) return; // No global variables and no parameters and no 'Exp': function has no side effect (optimization)
            // x := CALL f
            struct InterCode* ic = createInterCode(16);
            ic->u.kind_389_10_16.op1->kind = FUNCTION;
            strcpy(ic->u.kind_389_10_16.op1->u.va_name, kth(root,1)->text);
            if (place->kind == 1) {
                ic->u.kind_389_10_16.result->kind = VARIABLE;
                strcpy(ic->u.kind_389_10_16.result->u.va_name, *(place->u.va_name));
            } else { 
                ic->u.kind_389_10_16.result->kind = TEMP;
                ic->u.kind_389_10_16.result->u.tmp = place->u.tmp; 
            }
            insertInterCode(ic);
        }
    } else if (root->nchild == 4 && eq(kth(root,1)->type, "ID")) { // ID LP Args RP
        // "if (place == NULL) return;" is not allowed!! Although no global variables, struct types exist. So side effect exists.
        // and if "f(g(struct_A, b), c);", parameter "g(struct_A, b)" has side effect as well. So difficult to optimize here.
        struct ArgList* args = malloc(sizeof(struct ArgList));
        args->t = NULL; // Invalid (idle head)
        args->last = NULL; args->next = NULL;
        translate_Args(kth(root,3), &args);
        if (eq(kth(root,1)->text, "write")) {
            // WRITE x
            struct InterCode* ic = createInterCode(19);
            ic->u.kind_2_13_15_17_18_19.op->kind = TEMP;
            ic->u.kind_2_13_15_17_18_19.op->u.tmp = args->next->t;
            insertInterCode(ic);
        } else { // normal function
            for (struct ArgList* ptr = args; ptr->next != NULL; ptr = ptr->next) ptr->next->last = ptr;
            args->next->last = NULL; // first(args->next)->last = NULL
            struct ArgList* p = args->next; while (p->next) p = p->next; // p moves to the last (there is at least one parameter)
            for (struct ArgList* ptr = p; ptr != NULL; ptr = ptr->last) {
                // ARG x
                struct InterCode* ic = createInterCode(15);
                ic->u.kind_2_13_15_17_18_19.op->kind = TEMP;
                ic->u.kind_2_13_15_17_18_19.op->u.tmp = ptr->t;
                insertInterCode(ic);
            }
            // x := CALL f
            if (place == NULL) place = createPlace(2, NULL, createTemp()); // idle place, just in order to match the format "x := CALL f"
            struct InterCode* ic2 = createInterCode(16);
            ic2->u.kind_389_10_16.op1->kind = FUNCTION;
            strcpy(ic2->u.kind_389_10_16.op1->u.va_name, kth(root,1)->text);
            if (place->kind == 1) {
                ic2->u.kind_389_10_16.result->kind = VARIABLE;
                strcpy(ic2->u.kind_389_10_16.result->u.va_name, *(place->u.va_name));
            } else { 
                ic2->u.kind_389_10_16.result->kind = TEMP;
                ic2->u.kind_389_10_16.result->u.tmp = place->u.tmp; 
            }
            insertInterCode(ic2);
        }
    } else if (root->nchild == 3 && eq(kth(root,2)->type, "ASSIGNOP")) { // Exp ASSIGNOP Exp 
        char* var_name = NULL;
        struct Temp* tmp1 = createTemp(); struct Place* place1 = createPlace(2, NULL, tmp1);
        struct Temp* tmp2 = createTemp(); struct Place* place2 = createPlace(0, &var_name, tmp2); // Expect a kind-2 (struct.ele, arr[i]) lvalue or a kind-1 (id) lvalue
        struct Temp* tmp3 = NULL;
        Bool b = (kth(root,3)->nchild == 3) && eq(kth(kth(root,3),1)->type, "ID") && eq(kth(kth(root,3),1)->text, "read");
        if (b == FALSE) translate_Exp(kth(root,1), place2);
        translate_Exp(kth(root,3), place1);
        if (b == TRUE) return; // READ x
        if (tmp1->isAddress == FALSE || eq2(kth(root,3)->tp->kind, ARRAY, STRUCTURE)) tmp3 = tmp1;
        else { // tmp3 := *tmp1
            tmp3 = createTemp();
            struct InterCode* ic0 = createInterCode(9);
            ic0->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic0->u.kind_389_10_16.op1->u.tmp = tmp1;
            ic0->u.kind_389_10_16.result->kind = TEMP;
            ic0->u.kind_389_10_16.result->u.tmp = tmp3;
            insertInterCode(ic0);
        }
        // va_name/[*]tmp2 := tmp3
        struct InterCode* ic1 = NULL;
        if (place2->kind == 2) {
            if (tmp2->isAddress == FALSE || eq2(kth(root,1)->tp->kind, ARRAY, STRUCTURE)) { ic1 = createInterCode(3);  ic1->u.kind_389_10_16.result->kind = TEMP;           }
            else  /* actually the same */                                                 { ic1 = createInterCode(10); ic1->u.kind_389_10_16.result->kind = GET_TEMP_VALUE; }
            ic1->u.kind_389_10_16.result->u.tmp = tmp2; 
        } else if (place2->kind == 1) {
            ic1 = createInterCode(3); 
            ic1->u.kind_389_10_16.result->kind = VARIABLE; 
            strcpy(ic1->u.kind_389_10_16.result->u.va_name, *(place2->u.va_name));
        } else myassert(0);
        ic1->u.kind_389_10_16.op1->kind = TEMP;
        ic1->u.kind_389_10_16.op1->u.tmp = tmp3;
        insertInterCode(ic1);
        if (place == NULL) return; // NOTE! This statement cannot be moved to top!
        // place := tmp3 (different from the book here)
        struct InterCode* ic2 = createInterCode(3);
        ic2->u.kind_389_10_16.op1->kind = TEMP;
        ic2->u.kind_389_10_16.op1->u.tmp = tmp3;
        if (place->kind == 1) {
            ic2->u.kind_389_10_16.result->kind = VARIABLE;
            strcpy(ic2->u.kind_389_10_16.result->u.va_name, *(place->u.va_name));
        } else { 
            ic2->u.kind_389_10_16.result->kind = TEMP;
            ic2->u.kind_389_10_16.result->u.tmp = place->u.tmp; 
        }
        insertInterCode(ic2);
    } else if (root->nchild == 4 && eq(kth(root,2)->type, "LB")) { // Exp LB Exp RB
        if (place == NULL) place = createPlace(2, NULL, createTemp());
        // Not from the book
        struct Temp* t1 = createTemp();    struct Place* p1 = createPlace(2, NULL, t1);
        struct Temp* t1_5 = createTemp();  struct Place* p1_5 = createPlace(2, NULL, t1_5);
        struct Temp* t2 = NULL;
        translate_Exp(kth(root,1), p1);  translate_Exp(kth(root,3), p1_5);
        if (p1_5->u.tmp->isAddress == FALSE || eq2(kth(root,3)->tp->kind, ARRAY, STRUCTURE)) t2 = t1_5;
        else { // NEW CODE: t2 := *t1_5
            t2 = createTemp();
            struct InterCode* ic0 = createInterCode(9);
            ic0->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic0->u.kind_389_10_16.op1->u.tmp = t1_5;
            ic0->u.kind_389_10_16.result->kind = TEMP;
            ic0->u.kind_389_10_16.result->u.tmp = t2;
            insertInterCode(ic0);
        }

        // t3 := t2 * #4
        struct Temp* t3 = createTemp();
        struct InterCode* ic1 = createInterCode(6);
        ic1->u.kind_4567.op1->kind = TEMP;
        ic1->u.kind_4567.op1->u.tmp = t2;
        ic1->u.kind_4567.op2->kind = INT_CONST;
        ic1->u.kind_4567.op2->u.int_const = typeSize(root->tp); // for any kind of array
        ic1->u.kind_4567.result->kind = TEMP;
        ic1->u.kind_4567.result->u.tmp = t3;
        insertInterCode(ic1);
        // place := t1 + t3
        struct InterCode* ic2 = createInterCode(4);
        if (p1->u.tmp->isAddress == TRUE) ic2->u.kind_4567.op1->kind = TEMP; 
        else { ic2->u.kind_4567.op1->kind = GET_TEMP_ADDRESS; myassert(0); } // Not likely
        ic2->u.kind_4567.op1->u.tmp = t1;
        ic2->u.kind_4567.op2->kind = TEMP;
        ic2->u.kind_4567.op2->u.tmp = t3;
        // place->kind can be 0 (from above)
        place->kind = 2;
        place->u.tmp->isAddress = TRUE; // NOTE HERE
        ic2->u.kind_4567.result->kind = TEMP;
        ic2->u.kind_4567.result->u.tmp = place->u.tmp;
        insertInterCode(ic2);
    } else if (root->nchild == 3 && eq(kth(root,2)->type, "DOT")) { // Exp DOT ID
        struct FieldList* fl = kth(root,1)->tp->v.structure;
        struct Temp* t1 = createTemp();  struct Place* place1 = createPlace(2, NULL, t1);
        translate_Exp(kth(root,1), place1);
        // caculate relative distance from head of structure
        int dist = relDistance(fl, kth(root,3)->text);
        // place := [&]t1 + #distance_const
        struct InterCode* ic = createInterCode(4);
        if (t1->isAddress == TRUE) ic->u.kind_4567.op1->kind = TEMP; 
        else { ic->u.kind_4567.op1->kind = GET_TEMP_ADDRESS; printf("%s\n", kth(root,1)->text); fflush(stdout); myassert(0); } // Not likely
        ic->u.kind_4567.op1->u.tmp = t1;
        ic->u.kind_4567.op2->kind = INT_CONST;
        ic->u.kind_4567.op2->u.int_const = dist;
        if (place == NULL) place = createPlace(2, NULL, createTemp());
        // place->kind can be 0 (from above)
        place->kind = 2;
        place->u.tmp->isAddress = TRUE; // NOTE HERE
        ic->u.kind_4567.result->kind = TEMP;
        ic->u.kind_4567.result->u.tmp = place->u.tmp;
        insertInterCode(ic);
    } else {
        printf("%d\n", root->parent->nchild);
        printf("%s\n", root->type);
        myassert(0);
    }
}

void translate_DefList(struct Node* root) {
    if (root->nchild == 0) return;
    translate_Def(kth(root,1));
    translate_DefList(kth(root,2));
}

void translate_Def(struct Node* root) {
    translate_DecList(kth(root,1), kth(root,2));
}

void translate_DecList(struct Node* spec, struct Node* root) {
    translate_Dec(spec, kth(root,1));
    if (root->nchild == 3) translate_DecList(spec, kth(root,3));
}

void translate_Dec(struct Node* spec, struct Node* root) {
    if (root->nchild == 1) { // VarDec
        translate_VarDec(spec, kth(root,1), FALSE);
    } else { // VarDec ASSIGNOP Exp
        myassert(spec->tp->kind == BASIC); // STRUCTURE/ARRAY cannot assign
        struct Temp* t1 = createTemp(); struct Place* p1 = createPlace(2, NULL, t1);
        struct Temp* t2 = NULL;
        translate_Exp(kth(root,3), p1);
        if (t1->isAddress == FALSE || eq2(kth(root,3)->tp->kind, ARRAY, STRUCTURE)) t2 = t1;
        else { // t2 := *t1
            t2 = createTemp();
            struct InterCode* ic = createInterCode(9);
            ic->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic->u.kind_389_10_16.op1->u.tmp = t1;
            ic->u.kind_389_10_16.result->kind = TEMP;
            ic->u.kind_389_10_16.result->u.tmp = t2;
            insertInterCode(ic);
        }
        // va_name := t2
        struct InterCode* ic2 = createInterCode(3);
        ic2->u.kind_389_10_16.op1->kind = TEMP;
        ic2->u.kind_389_10_16.op1->u.tmp = t2;
        ic2->u.kind_389_10_16.result->kind = VARIABLE;
        strcpy(ic2->u.kind_389_10_16.result->u.va_name, kth(kth(root,1),1)->text);
        insertInterCode(ic2);
    }
}

void translate_Cond(struct Node* root, struct Label* l_true, struct Label* l_false) {
    if (root->nchild == 3 && eq(kth(root,2)->type, "RELOP")) { // Exp RELOP Exp
        struct Temp* t1 = createTemp(); struct Place* p1 = createPlace(2, NULL, t1);
        struct Temp* t2 = createTemp(); struct Place* p2 = createPlace(2, NULL, t2);
        struct Temp* t3 = NULL, *t4 = NULL;
        translate_Exp(kth(root,1),p1);
        if (t1->isAddress == FALSE || eq2(kth(root,1)->tp->kind, ARRAY, STRUCTURE)) t3 = t1;
        else { // t3 = *t1
            t3 = createTemp();
            struct InterCode* ic = createInterCode(9);
            ic->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic->u.kind_389_10_16.op1->u.tmp = t1;
            ic->u.kind_389_10_16.result->kind = TEMP;
            ic->u.kind_389_10_16.result->u.tmp = t3;
            insertInterCode(ic);
        }
        translate_Exp(kth(root,3),p2);
        if (t2->isAddress == FALSE || eq2(kth(root,3)->tp->kind, ARRAY, STRUCTURE)) t4 = t2;
        else { // t4 = *t2
            t4 = createTemp();
            struct InterCode* ic = createInterCode(9);
            ic->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic->u.kind_389_10_16.op1->u.tmp = t2;
            ic->u.kind_389_10_16.result->kind = TEMP;
            ic->u.kind_389_10_16.result->u.tmp = t4;
            insertInterCode(ic);
        }
        // IF t1 op t2 GOTO label_true
        struct InterCode* ic3 = createInterCode(12);
        ic3->u.kind_12.op1->kind = TEMP;
        ic3->u.kind_12.op1->u.tmp = t3;
        ic3->u.kind_12.op2->kind = TEMP;
        ic3->u.kind_12.op2->u.tmp = t4;
        ic3->u.kind_12.l = l_true;
        if (eq(kth(root,2)->text,"==")) ic3->u.kind_12.r = EQ;
        else if (eq(kth(root,2)->text,"!=")) ic3->u.kind_12.r = NE;
        else if (eq(kth(root,2)->text,">=")) ic3->u.kind_12.r = GE;
        else if (eq(kth(root,2)->text,"<=")) ic3->u.kind_12.r = LE;
        else if (eq(kth(root,2)->text,">")) ic3->u.kind_12.r = G;
        else ic3->u.kind_12.r = L;
        insertInterCode(ic3);
        // GOTO label_false
        struct InterCode* ic4 = createInterCode(11);
        ic4->u.kind_1_11.l = l_false;
        insertInterCode(ic4);
    } else if (root->nchild == 2 && eq(kth(root,1)->type, "NOT")) { // NOT Exp
        translate_Cond(kth(root,2), l_false, l_true);
    } else if (root->nchild == 3 && eq(kth(root,2)->type, "AND")) { // Exp AND Exp
        struct Label* l_continue = createLabel();
        translate_Cond(kth(root,1), l_continue, l_false);
        // LABEL l_continue
        struct InterCode* ic = createInterCode(1);
        ic->u.kind_1_11.l = l_continue;
        insertInterCode(ic);
        translate_Cond(kth(root,3), l_true, l_false);
    } else if (root->nchild == 3 && eq(kth(root,2)->type, "OR")) { // Exp OR Exp
        struct Label* l_continue = createLabel();
        translate_Cond(kth(root,1), l_true, l_continue);
        // LABEL l_continue
        struct InterCode* ic = createInterCode(1);
        ic->u.kind_1_11.l = l_continue;
        insertInterCode(ic);
        translate_Cond(kth(root,3), l_true, l_false);
    } else { // Other cases
        struct Temp* t1 = createTemp(); struct Place* p1 = createPlace(2, NULL, t1);
        translate_Exp(root, p1);
        struct Temp* t2 = NULL;
        if (t1->isAddress == FALSE || eq2(root->tp->kind, ARRAY, STRUCTURE)) t2 = t1; // array/structure type as boolean expression?
        else { // t2 = *t1;
            t2 = createTemp();
            struct InterCode* ic = createInterCode(9);
            ic->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic->u.kind_389_10_16.op1->u.tmp = t1;
            ic->u.kind_389_10_16.result->kind = TEMP;
            ic->u.kind_389_10_16.result->u.tmp = t2;
            insertInterCode(ic);
        }
        // IF t2 != #0 GOTO label_true
        struct InterCode* ic2 = createInterCode(12);
        ic2->u.kind_12.op1->kind = TEMP;
        ic2->u.kind_12.op1->u.tmp = t1;
        ic2->u.kind_12.op2->kind = INT_CONST;
        ic2->u.kind_12.op2->u.int_const = 0;
        ic2->u.kind_12.l = l_true;
        ic2->u.kind_12.r = NE;
        insertInterCode(ic2);
        // GOTO label_false
        struct InterCode* ic3 = createInterCode(11);
        ic3->u.kind_1_11.l = l_false;
        insertInterCode(ic3);
    }
}

void translate_Args(struct Node* root, struct ArgList** args) {
    struct ArgList* new_arg = malloc(sizeof(struct ArgList));
    struct Temp* tmp1 = createTemp();
    struct Place* place = createPlace(2, NULL, tmp1);
    translate_Exp(kth(root,1), place);
    struct Temp* t1 = NULL;
    if (kth(root,1)->tp->kind != STRUCTURE) {
        // Unlikely kind==ARRAY, because this program should have quitted during this function's definition.
        if (place->u.tmp->isAddress == FALSE) t1 = tmp1;
        else { // t1 = *t
            t1 = createTemp();
            struct InterCode* ic = createInterCode(9);
            ic->u.kind_389_10_16.op1->kind = GET_TEMP_VALUE;
            ic->u.kind_389_10_16.op1->u.tmp = tmp1;
            ic->u.kind_389_10_16.result->kind = TEMP;
            ic->u.kind_389_10_16.result->u.tmp = t1;
            insertInterCode(ic);
        }
    } else {
        t1 = tmp1; // No need to translate from an address to 'value'
        new_arg->isStruct = TRUE; // Helpful for printing out '&'
    }
    (*args)->next = new_arg; 
    new_arg->t = t1; new_arg->next = NULL;
    if (root->nchild == 3) translate_Args(kth(root,3), &new_arg);
}

