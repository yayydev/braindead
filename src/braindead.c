#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#define VERSION "0.0.1-beta"

static int dev_mode = 0;

static char *find_compiler() {
    if (system("which clang > /dev/null 2>&1") == 0) {
        return "clang";
    }
    if (system("which gcc > /dev/null 2>&1") == 0) {
        return "gcc";
    }
    fprintf(stderr, "Error: No suitable C compiler (clang/gcc) found in PATH.\n");
    exit(1);
}

static void print_help(){
    printf("Usage: braindead [options] [file]\n");
    printf("Options:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show program version\n");
    printf("  -ec, --error-check  Check Brainfuck file for errors without running\n");
    printf("  -d, --dev [on|off]  Enable or disable dev mode\n");
}

static void print_version(){
    printf("Braindead version: %s\n", VERSION);
}

static char *read_file(const char *path, size_t *len){
    FILE *f = fopen(path,"rb");
    if(!f){fprintf(stderr,"Error opening file: %s\n",strerror(errno));exit(1);}
    fseek(f,0,SEEK_END);
    long s = ftell(f);
    rewind(f);
    char *b = malloc(s+1);
    if(!b){fclose(f);fprintf(stderr,"OOM\n");exit(1);}
    fread(b,1,s,f);
    fclose(f);
    b[s]=0;
    *len = s;
    return b;
}

static int is_bf_char(char c){return c=='>'||c=='<'||c=='+'||c=='-'||c=='.'||c==','||c=='['||c==']'||c=='\n'||c=='\r'||c=='\t'||c==' ';}

static void mkdir_p(const char *path){
    struct stat st;
    if(stat(path,&st)==0) return;
    mkdir(path,0755);
}

static int rand_range(int a,int b){ if(b<=a) return a; return a + rand()%(b-a+1); }

static void emit_error_variation(const char *filename,int line,int col,const char *token){
    const char *heads[] = {
        "%d:%d     ... %s;          <----- THIS\n",
        "Error at %d:%d -> %s\n",
        "%s:%d:%d: unexpected token '%s'\n",
        "%d|%d >>> %s <<<\n"
    };
    const char *msgs[] = {
        "Error:  \"%s\" is not defined, you can check the documentation for help.\n",
        "Fatal: unknown symbol \"%s\" encountered.\n",
        "Compilation failed: token \"%s\" unrecognized.\n",
        "Parse error: '%s' — refer to docs.\n"
    };
    const char *docs[] = {
        "Documentation: bit.ly/BrainfuckDocumentation\n"
    };
    int h = rand_range(0,(int)(sizeof(heads)/sizeof(heads[0]))-1);
    int m = rand_range(0,(int)(sizeof(msgs)/sizeof(msgs[0]))-1);
    int d = rand_range(0,(int)(sizeof(docs)/sizeof(docs[0]))-1);
    int caret_len = (int)strlen(token);
    if(caret_len<3) caret_len = rand_range(3,8);
    char caret[256]; memset(caret,0,sizeof(caret));
    for(int i=0;i<caret_len && i<250;i++) caret[i]='^';
    caret[caret_len]=0;
    if(h==0) fprintf(stderr,heads[h],line,col,token);
    else if(h==1) fprintf(stderr,heads[h],line,col,token);
    else if(h==2) fprintf(stderr,heads[h],filename,line,col,token);
    else fprintf(stderr,heads[h],line,col,token);
    fprintf(stderr,"         %s\n\n",caret);
    fprintf(stderr,msgs[m],token);
    fprintf(stderr,"%s",docs[d]);
    mkdir_p(".bdlogs");
    int num = rand_range(10000,999999);
    fprintf(stderr,"         Log can be found in: .bdlogs/.log-%d\n",num);
}

static int check_errors(const char *code,size_t len,const char *filename){
    int line=1,col=1,errs=0;
    int *stack = malloc(sizeof(int)*(len+5));
    int sp=0;
    for(size_t i=0;i<len;i++){
        char c = code[i];
        if(c=='\n'){line++;col=1;continue;}
        if(is_bf_char(c)){ col++; continue; }
        if(c=='\0') break;
        if(c==' '||c=='\t'||c=='\r'){ col++; continue;}
        size_t j=i;
        char token[256]; int tk=0;
        while(j<len && !is_bf_char(code[j]) && code[j]!='\n' && code[j]!='\r' && tk<250){ token[tk++]=code[j++]; }
        token[tk]=0;
        emit_error_variation(filename,line,col,token);
        errs++;
        i = j-1;
        col += tk;
    }
    for(size_t i=0;i<len;i++){
        if(code[i]=='[') stack[sp++]=i;
        else if(code[i]==']'){
            if(sp==0){
                int l=1,cpos=1; for(size_t k=0;k<i;k++){ if(code[k]=='\n'){l++; cpos=1;} else cpos++; }
                emit_error_variation(filename,l,cpos,"]");
                errs++;
            } else sp--;
        }
    }
    while(sp>0){
        size_t idx = stack[--sp];
        int l=1,cpos=1; for(size_t k=0;k<idx;k++){ if(code[k]=='\n'){l++; cpos=1;} else cpos++; }
        emit_error_variation(filename,l,cpos,"[");
        errs++;
    }
    free(stack);
    return errs;
}

static char *sanitize_filename(const char *in){
    char *out = strdup(in);
    for(char *p=out;*p;p++){ if(*p=='/'||*p=='\\'||*p==' ') *p='_'; }
    return out;
}

static char *make_c_from_bf(const char *code,size_t len,const char *srcname){
    size_t cap = len*12 + 4096;
    char *out = malloc(cap);
    if(!out) exit(1);
    out[0]=0;
    strcat(out,"#include <stdio.h>\n#include <stdlib.h>\nint main(){ unsigned char *t = calloc(30000,1); unsigned char *p = t; (void)t;\n");
    size_t i=0;
    while(i<len){
        char c = code[i];
        if(c=='>'||c=='<'||c=='+'||c=='-'){
            int cnt=0; char op=c;
            while(i<len && code[i]==op){ cnt++; i++; }
            char buf[128];
            if(op=='>') snprintf(buf,sizeof(buf),"p += %d;\n",cnt);
            else if(op=='<') snprintf(buf,sizeof(buf),"p -= %d;\n",cnt);
            else if(op=='+') snprintf(buf,sizeof(buf),"(*p) += %d;\n",cnt);
            else snprintf(buf,sizeof(buf),"(*p) -= %d;\n",cnt);
            strcat(out,buf);
            continue;
        } else if(c=='.'){ strcat(out,"putchar(*p);\nfflush(stdout);\n"); }
        else if(c==','){ strcat(out,"int _in = getchar(); if(_in==EOF) *p = 0; else *p = _in;\n"); }
        else if(c=='['){ strcat(out,"while(*p){\n"); }
        else if(c==']'){ strcat(out,"}\n"); }
        i++;
    }
    strcat(out,"free(t); return 0; }\n");
    char *final = realloc(out,strlen(out)+1);
    return final?final:out;
}

static int write_file(const char *path,const char *data){
    FILE *f = fopen(path,"wb");
    if(!f) return 0;
    size_t w = fwrite(data,1,strlen(data),f);
    fclose(f);
    return w==strlen(data);
}

static int run_command(const char *cmd){
    int rc = system(cmd);
    if(rc==-1) return -1;
    return WEXITSTATUS(rc);
}

int main(int argc,char **argv){
    srand((unsigned)time(NULL) ^ (uintptr_t)&argc);
    if(argc<2){ print_help(); return 1; }
    if(!strcmp(argv[1],"-h")||!strcmp(argv[1],"--help")){ print_help(); return 0; }
    if(!strcmp(argv[1],"-v")||!strcmp(argv[1],"--version")){ print_version(); return 0; }
    if(!strcmp(argv[1],"-d")||!strcmp(argv[1],"--dev")){
        if(argc==3){ if(!strcmp(argv[2],"on")) dev_mode=1; else if(!strcmp(argv[2],"off")) dev_mode=0; }
        else dev_mode = !dev_mode;
        printf("Dev mode is now %s\n", dev_mode ? "ON" : "OFF");
        return 0;
    }
    if(!strcmp(argv[1],"-ec")||!strcmp(argv[1],"--error-check")){
        if(argc<3){ fprintf(stderr,"No file provided for error checking\n"); return 1; }
        size_t len; char *code = read_file(argv[2],&len);
        int errs = check_errors(code,len,argv[2]);
        if(!errs) printf("No errors detected in %s\n",argv[2]);
        free(code);
        return errs?1:0;
    }
    size_t len; char *code = read_file(argv[1],&len);
    printf("1) Checking for errors..... ");
    int errs = check_errors(code,len,argv[1]);
    if(errs){ printf("Failed with %d error(s)\n",errs); free(code); return 1; }
    printf("Done! No error detected.\n");
    printf("2) Running Code... ");
    char *csrc = make_c_from_bf(code,len,argv[1]);
    char *safe = sanitize_filename(argv[1]);
    char tmpc[1024]; snprintf(tmpc,sizeof(tmpc),"/tmp/braindead_%d_%s.c",getpid(),safe);
    char tmpexe[1024]; snprintf(tmpexe,sizeof(tmpexe),"/tmp/braindead_exec_%d_%s",getpid(),safe);
    write_file(tmpc,csrc);
    char cmd[4096];
    snprintf(cmd,sizeof(cmd),"clang -O2 \"%s\" -o \"%s\" 2> /tmp/braindead_clang_err_%d.txt",tmpc,tmpexe,getpid());
    int rc = run_command(cmd);
    if(rc!=0){
        fprintf(stderr,"Failed to compile temporary JIT binary (clang rc=%d). See /tmp/braindead_clang_err_%d.txt\n",rc,getpid());
        free(csrc); free(code); free(safe);
        return 1;
    }
    int runrc = run_command(tmpexe);
    if(runrc!=0){
        fprintf(stderr,"Program exited with code %d\n",runrc);
    }
    printf("Done!\n");
    char outname[1024];
    snprintf(outname,sizeof(outname),"%s.out",safe);
    snprintf(cmd,sizeof(cmd),"clang -O2 \"%s\" -o \"%s\" 2> /tmp/braindead_clang_final_%d.txt",tmpc,outname,getpid());
    int rc2 = run_command(cmd);
    if(rc2!=0){
        fprintf(stderr,"Failed to create final binary (clang rc=%d). See /tmp/braindead_clang_final_%d.txt\n",rc2,getpid());
        free(csrc); free(code); free(safe);
        return 1;
    }
    printf("3) Making the file binary... Done!\n");
    printf("(4) Process finished successfully, exiting....\n");
    free(csrc); free(code); free(safe);
    return 0;
}
