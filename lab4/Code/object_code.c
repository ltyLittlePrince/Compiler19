#include "ir.h"
#include "object_code.h"

extern struct InterCode** interCodesList;
extern int endInterCodesList;
int offset_fp = 0;
struct OcVar oc_var[10000];
int oc_var_index = 0;
int func_var[1000]; //标记该函数变量在oc_var的范围
int func_var_index = 0;
int get_func_param = 1;//函数第几个参数计数
int count_func_param = 0;//函数第几个参数计数

void kernels(){
    // for(int i = 0;i < endInterCodesList;i++)
        // printInterCode(interCodesList[i]);
    
    // freopen("../irsim/out.s", "w", stdout);
    initOC();
    for(int i = 0;i < endInterCodesList;i++){
        // freopen("debug", "w", stdout);
        // printf("i = %d\n", i + 1);
        // freopen("../irsim/out.s", "w", stdout);
        // printInterCode(interCodesList[i]);
        switch (interCodesList[i]->kind) {
            case 1: make(one);break;
            case 2: make(two);break;
            case 3: make(three);break;
            case 4: make(four);break;
            case 5: make(five);break;
            case 6: make(six);break;
            case 7: make(seven);break;
            case 8: make(eight);break;
            case 9: make(nine);break;
            case 10: make(ten);break;
            case 11: make(eleven);break;
            case 12: make(twleve);break;
            case 13: make(threeteen);break;
            case 14: make(fourteen);break;
            case 15: make(fifteen);break;
            case 16: make(sixteen);break;
            case 17: make(seventeen);break;
            case 18: make(eighteen);break;
            case 19: make(nineteen);break;
            default: break;
        }
    }
    return ;
}

void initOC(){
    printf(".data\n\
_prompt: .asciiz \"Enter an integer\"\n\
_ret: .asciiz \"\\n\"\n\
.globl main\n\
.text\n\
read:\n\
    li $v0, 4\n\
    la $a0, _prompt\n\
    syscall\n\
    li $v0, 5\n\
    syscall\n\
    jr $ra\n \n\
\
write:\n\
    li $v0, 1\n\
    syscall\n\
    li $v0, 4\n\
    la $a0, _ret\n\
    syscall\n\
    move $v0, $0\n\
    jr $ra\n\n");
}

void nothing(struct InterCode* ic){
}

void varToT0(struct Operand* op){
    //数字
    if(op->kind == INT_CONST){
        printf("\tli $t0, %d\n", op->u.int_const);
        return ;
    }

    int ind = findOcVar(op);
    if(ind == -1) ind = insetOcVar(op);
    
    if(oc_var[ind].offset == 0)
        printf("\tlw $t0, %d($s8)\n", oc_var[ind].offset);
    else if(oc_var[ind].offset < 0)
        printf("\tlw $t0, %d($s8)\n", -oc_var[ind].offset); // 函数参数出现这种情况
    else if(oc_var[ind].offset > 0)
        printf("\tlw $t0, -%d($s8)\n", oc_var[ind].offset);
}
void varToT1(struct Operand* op){
    if(op->kind == INT_CONST){
        printf("\tli $t1, %d\n", op->u.int_const);
        return ;
    }

    int ind = findOcVar(op);
    if(ind == -1) ind = insetOcVar(op);
    if(oc_var[ind].offset == 0)
        printf("\tlw $t1, %d($s8)\n", oc_var[ind].offset);
    else if(oc_var[ind].offset < 0)
        printf("\tlw $t1, %d($s8)\n", -oc_var[ind].offset); // 函数参数出现这种情况
    else if(oc_var[ind].offset > 0)
        printf("\tlw $t1, -%d($s8)\n", oc_var[ind].offset);
}
void varFromT2(struct Operand* op){
    int ind = findOcVar(op);
    if(ind == -1) ind = insetOcVar(op);
    // fpr("varFromT2 ", 123);
    if(oc_var[ind].offset == 0)
        printf("\tsw $t2, %d($s8)\n", oc_var[ind].offset);
    else if(oc_var[ind].offset < 0)
        printf("\tsw $t2, %d($s8)\n", -oc_var[ind].offset); // 函数参数出现这种情况
    else if(oc_var[ind].offset > 0)
        printf("\tsw $t2, -%d($s8)\n", oc_var[ind].offset);
}

int findOcVar(struct Operand* op){
    // fpr("findOcVar ", 123);
    if(oc_var_index == 0) return -1;
    for(int i = oc_var_index;i > 0;i--){
        if(eqOperand(op, oc_var[i - 1].op)){
            // fpr("findOcVar win: ", 0);
            return i - 1;
        }
    }
    // fpr("findOcVar ", 321);
    return -1;
}

int insetOcVar(struct Operand* op){
    // printf("\taddi $sp, $sp, -4\n");
    // fpr("insetOcVar ", 123);
    offset_fp += 4;
    oc_var[oc_var_index].op = createOp(op);
    oc_var[oc_var_index].offset = offset_fp;
    oc_var_index++; 
    return oc_var_index - 1;
}

define(one){
    printf("\tlabel%d:\n", ic->u.kind_1_11.l->no);
}
define(two){
    //printf("\t\n");
    printf("%s:\n", ic->u.kind_2_13_15_17_18_19.op->u.va_name);
    if(strcmp(ic->u.kind_2_13_15_17_18_19.op->u.va_name, "main") == 0){
        printf("\tmove $30, $sp\n");
        printf("\taddi $sp, $sp, -26000\n");
    }else{
        // printf("\taddi $sp, $sp, -4\n");
        // printf("\tsw $30, 0($sp)\n");
        // printf("\tmove $30, $sp\n");
        get_func_param = 1;
    }
}
define(three){
    varToT0(ic->u.kind_389_10_16.op1);
    printf("\tmove $t2, $t0\n");
    varFromT2(ic->u.kind_389_10_16.result);
}
define(four){
    varToT0(ic->u.kind_4567.op1);
    varToT1(ic->u.kind_4567.op2);
    printf("\tadd $t2, $t0, $t1\n");
    varFromT2(ic->u.kind_4567.result);
}
define(five){
    varToT0(ic->u.kind_4567.op1);
    varToT1(ic->u.kind_4567.op2);
    printf("\tsub $t2, $t0, $t1\n");
    varFromT2(ic->u.kind_4567.result);
}
define(six){
    varToT0(ic->u.kind_4567.op1);
    varToT1(ic->u.kind_4567.op2);
    printf("\tmul $t2, $t0, $t1\n");
    varFromT2(ic->u.kind_4567.result);
}
define(seven){
    varToT0(ic->u.kind_4567.op1);
    varToT1(ic->u.kind_4567.op2);
    printf("\tdiv $t0, $t1\n");
    printf("\tmflo $t2\n");
    varFromT2(ic->u.kind_4567.result);
}
define(eight){
    varToT0(ic->u.kind_389_10_16.op1);
    printf("\tmove $t2, $t0\n");
    varFromT2(ic->u.kind_389_10_16.result);
}
define(nine){
    varToT0(ic->u.kind_389_10_16.op1);
    printf("\tlw $t2, 0($t0)\n");
    varFromT2(ic->u.kind_389_10_16.result);
}
define(ten){
    varToT0(ic->u.kind_389_10_16.op1);
    varToT1(ic->u.kind_389_10_16.result);
    printf("\tsw $t0, 0($t1)\n");
    // varFromT2(ic->u.kind_389_10_16.result);
}
define(eleven){
    printf("\tj label%d\n", ic->u.kind_1_11.l->no);
}
define(twleve){
    varToT0(ic->u.kind_12.op1);
    varToT1(ic->u.kind_12.op2);
    switch (ic->u.kind_12.r) {
    case EQ: printf("\tbeq $t0, $t1, label%d\n", ic->u.kind_12.l->no); break;
    case NE: printf("\tbne $t0, $t1, label%d\n", ic->u.kind_12.l->no); break;
    case GE: printf("\tbge $t0, $t1, label%d\n", ic->u.kind_12.l->no); break;
    case G : printf("\tbgt $t0, $t1, label%d\n", ic->u.kind_12.l->no); break;
    case LE: printf("\tble $t0, $t1, label%d\n", ic->u.kind_12.l->no); break;
    case L : printf("\tblt $t0, $t1, label%d\n", ic->u.kind_12.l->no); break;
    default:
        break;
    }
}
define(threeteen){
    varToT0(ic->u.kind_2_13_15_17_18_19.op);
    // printf("\taddi $sp, $sp, %d\n", 4 * get_func_param);
    printf("\tmove $v0, $t0\n");
    printf("\tjr $ra\n");
}
define(fourteen){
    // printf("\taddi $sp, $sp, %d\n", -ic->u.kind_14.sz * 4);
    offset_fp += ic->u.kind_14.sz;
    printf("\tmove $t2, $30\n");
    printf("\taddi $t2, %d\n", -offset_fp);
    varFromT2(ic->u.kind_14.op);
}
define(fifteen){
    printf("\taddi $sp, $sp, %d\n", -4);
    varToT0(ic->u.kind_2_13_15_17_18_19.op);
    printf("\tsw $t0, 0($sp)\n");
    count_func_param++;
}
define(sixteen){
    printf("\taddi $sp, $sp, -4\n");
    printf("\tsw $ra, 0($sp)\n");
    printf("\tjal %s\n", ic->u.kind_389_10_16.op1->u.va_name);
    printf("\tlw $ra, 0($sp)\n");
    printf("\taddi $sp, $sp, 4\n");
    printf("\taddi $sp, $sp, %d\n", 4 * count_func_param);
    count_func_param = 0;

    printf("\tmove $t2, $v0\n");
    varFromT2(ic->u.kind_389_10_16.result);
}
define(seventeen){
    printf("\tlw $t2, %d($sp)\n", 4 * get_func_param);
    varFromT2(ic->u.kind_2_13_15_17_18_19.op);
    get_func_param++;
}

define(eighteen){
    printf("\tmove $a0, $t0\n");
    printf("\taddi $sp, $sp, -4\n");
    printf("\tsw $ra, 0($sp)\n");//这两句看起来像是在压栈
    printf("\tjal read\n");
    printf("\tlw $ra, 0($sp)\n");
    printf("\taddi $sp, $sp, 4\n");

    printf("\tmove $t2, $v0\n");
    varFromT2(ic->u.kind_2_13_15_17_18_19.op);
}

define(nineteen){
    varToT0(ic->u.kind_2_13_15_17_18_19.op);
    printf("\tmove $a0, $t0\n");
    printf("\taddi $sp, $sp, -4\n");
    printf("\tsw $ra, 0($sp)\n");//这两句看起来像是在压栈
    printf("\tjal write\n");
    printf("\tlw $ra, 0($sp)\n");
    printf("\taddi $sp, $sp, 4\n");
}