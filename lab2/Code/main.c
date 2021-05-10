#include "semantic.h"
#include "syntax.tab.h"
#include <assert.h>

extern int treeflag;
extern struct Node* Root;
extern void yyrestart (FILE *input_file);
extern void printTree(struct Node* head, int counts);
extern int yydebug;

extern int mydebug;

int main(int argc, char** argv) {
    if (argc <= 1) return 1;
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    // if(mydebug)
    //     printTree(Root,0);

    //return 0;
    // assert(treeflag == 1); // DEBUG: remove this finally
    if(treeflag == 1){
        if(!initial()){
            printf("initial fail!\n");
            return 0;
        };
        traverseTree(Root);
        printf("\n");
    }
    return 0;
}

