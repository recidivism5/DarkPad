#include <stdio.h>
#include <stdlib.h>
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COUNT(arr) (sizeof(arr)/sizeof(*arr))
#define FOR(var,count) for(i32 var = 0; var < (count); var++)
typedef struct {
    u32 size;
    u8 *bytes;
}Bytes;
i32 main(i32 argc, u8 **argv){
    FILE *out = fopen(argv[argc-1],"wb");
    i32 numImages = argc-2;
    u16 header[3] = {0,1,numImages};
    fwrite(header,sizeof(header),1,out);
    i32 offset = 6+16*numImages;
    Bytes *pngs = malloc(numImages*sizeof(Bytes));
    for (i32 i = 1; i < argc-1; i++){
        Bytes *p = pngs+i-1;
        FILE *in = fopen(argv[i],"rb");
        fseek(in,0,SEEK_END);
        p->size = ftell(in);
        fseek(in,0,SEEK_SET);
        p->bytes = malloc(p->size);
        fread(p->bytes,p->size,1,in);
        fclose(in);
        u8 winfo[4]={p->bytes[16+3],p->bytes[16+4+3],0,0};
        fwrite(winfo,sizeof(winfo),1,out);
        u16 pinfo[2]={0,32};
        fwrite(pinfo,sizeof(pinfo),1,out);
        u32 binfo[2]={p->size,offset};
        fwrite(binfo,sizeof(binfo),1,out);
        offset += p->size;
    }
    for (i32 i = 0; i < numImages; i++){
        fwrite(pngs[i].bytes,pngs[i].size,1,out);
        free(pngs[i].bytes);
    }
    free(pngs);
    fclose(out);
}