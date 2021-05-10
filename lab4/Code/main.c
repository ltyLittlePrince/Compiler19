#include "semantic.h"
#include "syntax.tab.h"
#include <assert.h>

extern int treeflag;
extern struct Node* Root;
extern void yyrestart (FILE *input_file);
extern void printTree(struct Node* head, int counts);
extern void translate_Program(struct Node* root);
extern void printInterCodes();
extern int yydebug;



int main(int argc, char** argv) {
    // system("rm debug"); fprs("Program begin!", "");//for debug
    if (argc <= 1) return 1;
    
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if(treeflag == 1){
        if(!initial()){
            printf("initial fail!\n");
            return 0;
        };
        if (argc <= 2) return 2;
        //printTree(Root,0);
        traverseTree(Root);
        translate_Program(Root);
        myassert(freopen(argv[2], "w", stdout) != NULL);
        printInterCodes();
        printf("\n");
    }else {
        assert(0);
    }
    return 0;
}

