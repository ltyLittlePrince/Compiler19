#ifndef _OBJECT_CODE_H__
#define _OBJECT_CODE_H__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>//头文件包含rand和srand函数
#include <assert.h>
#include <time.h>
#include "ir.h"

extern struct InterCode** interCodesList;
extern int endInterCodesList;

#define define(number) void number##_code(struct InterCode* ic)
#define make(number) number##_code(interCodesList[i])

struct OcVar{
    struct Operand* op;
    int offset;
};

void kernels();
void initOC();

void varToT0(struct Operand* op);
void varToT1(struct Operand* op);
void varFromT2(struct Operand* op);

int findOcVar(struct Operand* op);
int insetOcVar(struct Operand* op);

define(one);
define(two);
define(three);
define(four);
define(five);
define(six);
define(seven);
define(eight);
define(nine);
define(ten);
define(eleven);
define(twleve);
define(threeteen);
define(fourteen);
define(fifteen);
define(sixteen);
define(seventeen);
define(eighteen);
define(nineteen);

#endif