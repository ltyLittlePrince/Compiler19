
#include "syntax.tab.h"
#include <stdio.h>

extern int treeflag;
extern struct Node* roots;
extern void yyrestart (FILE *input_file);
extern void printTree(struct Node* head, int counts);
extern int yydebug;

int main(int argc, char** argv) {
    if (argc <= 1) return 1;
    FILE* f = fopen(argv[1], "r");
    if(!f){
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    //yydebug = 1;
    yyparse();
	if (treeflag) printTree(roots, 0);
    return 0;
}
