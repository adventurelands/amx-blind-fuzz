// Fuzz the AMX opcode space. Op field = bits 9:5 (32 ops); only 0-22 are
// documented (corsix). Run each, fork-protected, and see which decode.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "../amx.h"
static char buf[4096] __attribute__((aligned(128)));
#define CASE(n) case n: AMX_OP_GPR(n, operand); break;
static void run_op(int op, uint64_t operand){
    AMX_SET();
    switch(op){
        CASE(0)CASE(1)CASE(2)CASE(3)CASE(4)CASE(5)CASE(6)CASE(7)CASE(8)CASE(9)
        CASE(10)CASE(11)CASE(12)CASE(13)CASE(14)CASE(15)CASE(16)CASE(17)CASE(18)CASE(19)
        CASE(20)CASE(21)CASE(22)CASE(23)CASE(24)CASE(25)CASE(26)CASE(27)CASE(28)CASE(29)
        CASE(30)CASE(31)
    }
    AMX_CLR();
}
static const char* known[23]={"LDX","LDY","STX","STY","LDZ","STZ","LDZI","STZI","EXTRX","EXTRY",
    "FMA64","FMS64","FMA32","FMS32","MAC16","FMA16","FMS16","SET/CLR","VECINT","VECFP","MATINT","MATFP","GENLUT"};
int main(void){
    uint64_t operand=(uint64_t)(uintptr_t)buf;     // valid addr so loads/stores don't segfault
    printf("op  status            name\n");
    for(int op=0;op<32;op++){
        pid_t p=fork();
        if(p==0){ run_op(op,operand); _exit(0); }
        int st; waitpid(p,&st,0);
        const char* nm = op<23?known[op]:"(undocumented?)";
        if(WIFEXITED(st)&&WEXITSTATUS(st)==0) printf("%2d  DECODED (ran)     %s\n",op,nm);
        else if(WIFSIGNALED(st)){
            int s=WTERMSIG(st);
            if(s==SIGILL) printf("%2d  SIGILL (invalid)  %s\n",op,nm);
            else printf("%2d  DECODED, sig%-2d     %s   <-- exists, faulted on operand\n",op,s,nm);
        } else printf("%2d  ?                 %s\n",op,nm);
    }
    return 0;
}
