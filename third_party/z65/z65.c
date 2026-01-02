/* Z-machine v1–v3 interpreter for CP/M-65
 * Step 2: complete 1OP instruction set
 */

#include <stdint.h>
//#include <stddef.h>
#include <cpm.h>
#include "lib/printi.h"
/* ============================================================
 * Opcode definitions
 * ============================================================
 */

/* 2OP */
#define OP2_JE              0x01
#define OP2_JL              0x02
#define OP2_JG              0x03
#define OP2_DEC_CHK         0x04
#define OP2_INC_CHK         0x05
#define OP2_JIN             0x06
#define OP2_TEST            0x07
#define OP2_OR              0x08
#define OP2_AND             0x09
#define OP2_STORE           0x0D
#define OP2_LOADW           0x0F
#define OP2_LOADB           0x10
#define OP2_ADD             0x14
#define OP2_SUB             0x15
#define OP2_MUL             0x16
#define OP2_DIV             0x17
#define OP2_MOD             0x18
#define OP2_TEST_ATTR       0x0A
#define OP2_SET_ATTR        0x0B
#define OP2_CLEAR_ATTR      0x0C
#define OP2_INSERT_OBJ      0x0E
#define OP2_GET_PROP        0x11
#define OP2_GET_PROP_ADDR   0x12
#define OP2_GET_NEXT_PROP   0x13

/* 1OP */
/*#define OP1_RTRUE           0x00
#define OP1_RFALSE          0x01
#define OP1_PRINT           0x02
#define OP1_JUMP            0x03
#define OP1_PRINT_ADDR      0x04
#define OP1_VALUE           0x05
#define OP1_INC             0x06
#define OP1_DEC             0x07
#define OP1_PRINT_PADDR     0x08
#define OP1_REMOVE_OBJ      0x09
#define OP1_PRINT_OBJ       0x0A
#define OP1_RET             0x0B
#define OP1_JZ              0x0C
#define OP1_JZ              0x00
#define OP1_INC             0x05
#define OP1_DEC             0x06
#define OP1_PRINT_ADDR      0x07
#define OP1_PRINT_PADDR     0x0D
#define OP1_REMOVE_OBJ      0x09
#define OP1_PRINT_OBJ       0x0A
#define OP1_JUMP            0x0C
#define OP1_RET             0x0B*/

/* 1OP opcodes (v1–v3) */
#define OP1_JZ            0x00
#define OP1_GET_SIBLING   0x01
#define OP1_GET_CHILD     0x02
#define OP1_GET_PARENT    0x03
#define OP1_GET_PROP_LEN  0x04
#define OP1_INC           0x05
#define OP1_DEC           0x06
#define OP1_PRINT_ADDR    0x07
#define OP1_CALL_1S       0x08
#define OP1_REMOVE_OBJ    0x09
#define OP1_PRINT_OBJ     0x0A
#define OP1_RET           0x0B
#define OP1_JUMP          0x0C
#define OP1_PRINT_PADDR   0x0D
#define OP1_LOAD          0x0E
#define OP1_CALL_1N       0x0F

/* 0OP */
#define OP0_RTRUE           0x00
#define OP0_RFALSE          0x01
#define OP0_PRINT           0x02
#define OP0_PRINT_RET       0x03
#define OP0_NOP             0x04
#define OP0_SAVE            0x05
#define OP0_RESTORE         0x06
#define OP0_RESTART         0x07
#define OP0_RET_POPPED      0x08
#define OP0_POP             0x09
#define OP0_QUIT            0x0A
#define OP0_NEW_LINE        0x0B

/* VAR */
#define OPV_CALL            0x00
#define OPV_JE              0x01
#define OPV_JL              0x02
#define OPV_JG              0x03
#define OPV_STOREW          0x21
#define OPV_STOREB          0x22
#define OPV_PUT_PROP        0x23
#define OPV_SREAD           0x24
#define OPV_PRINT_CHAR      0x25
#define OPV_PRINT_NUM       0x26
#define OPV_AND             0x09
#define OPV_OR              0x0A
#define OPV_NOT             0x0B
#define OPV_CALL_EXT        0x20
#define OPV_PUSH            0x28
#define OPV_PULL            0x29



/* ============================================================
 * BDOS
 * ============================================================
 */

//extern uint8_t bdos(uint8_t fn, void *arg);

//#define BDOS_OPEN_R     15
//bin#define BDOS_READ_SEQ  20
//#define BDOS_READ_RAND 33
//#define BDOS_CONOUT    2

/*typedef struct {
    uint8_t drive;
    char name[8];
    char ext[3];
    uint8_t ex,s1,s2,rc;
    uint8_t d[16];
    uint8_t cr;
    uint8_t r0,r1,r2;
} FCB;
*/

/* ============================================================
 * Configuration
 * ============================================================
 */

#define PAGE_SIZE 512
#define NUM_PAGES 8
#define DYNAMIC_MEM_MAX 16384

#define STACK_SIZE 256
#define MAX_FRAMES 16
#define MAX_LOCALS 8
#define MAX_OPERANDS 8

#define INPUT_MAX 80
#define MAX_TOKENS 16

/* ============================================================
 * Structures
 * ============================================================
 */

typedef struct {
    uint16_t return_pc;
    uint8_t store_var;
    uint8_t num_locals;
    uint16_t locals[MAX_LOCALS];
} CallFrame;

typedef struct {
    uint16_t page;
    uint8_t data[PAGE_SIZE];
    uint8_t valid;
} Page;

/* ============================================================
 * Globals
 * ============================================================
 */

//static FCB story_fcb;

static uint8_t dynamic_mem[DYNAMIC_MEM_MAX];
static uint16_t dynamic_size;

static Page page_cache[NUM_PAGES];
static uint8_t next_victim;

static uint16_t pc;
static uint16_t initial_pc;
static int16_t stack[STACK_SIZE];
static uint16_t sp;

static CallFrame frames[MAX_FRAMES];
static uint8_t fp;

static uint8_t dma[128];
static uint8_t hdr[128];

static uint16_t dict_addr;
static uint8_t dict_sep_count;
static uint8_t dict_seps[32];
static uint8_t dict_entry_len;
static uint16_t dict_entry_count;

static uint16_t object_table;

static uint8_t alphabet;
static uint8_t zalph[3][26] = {{'a','b','c','d','e','f','g','h','i','j','k',
                                'l','m','n','o','p','q','r','s','t','u','v',
                                'w','x','y','z'},
                               {'A','B','C','D','E','F','G','H','I','J','K',
                                'L','M','N','O','P','Q','R','S','T','U','V',
                                'W','X','Y','Z'},
                               {' ','\n','0','1','2','3','4','5','6','7','8',
                                '9','.',',','!','?','_','#','\'','\"','/',
                                '\\','-',':','(',')'}};


static uint8_t abbrev_print;
static uint8_t abbrev_table;
static uint16_t abbrev_base;
/* ============================================================
 * Console
 * ============================================================
 */

static inline void putc(char c){
    //bdos(BDOS_CONOUT,(void *)(uintptr_t)c);
    cpm_conout(c);
}

static void crlf(void)
{
    cpm_printstring("\r\n");
}

static void spc(void) 
{
    cpm_conout(' ');
}


static void print_hex(uint16_t val)
{
    cpm_printstring("0x");
    for(uint8_t i = 4; i>0; i--) {
        uint8_t nibble = (val >> ((i-1) << 2)) & 0xf;

       if(nibble < 10) cpm_conout(nibble + '0');
       else cpm_conout(nibble-10 + 'A'); 
    }
}

static void printx(const char *s)
{
    cpm_printstring(s);
    crlf();
}

static void fatal(const char *s)
{
    printx(s);
    cpm_warmboot();
}


static uint8_t read_line(char *buf){
    uint8_t len=0;
    for(;;){
        char c=cpm_conin();
        if(c=='\r'||c=='\n'){
            crlf();
            buf[len]=0;
            return len;
        }
        if(c==8 && len){
            len--;
            putc(8); putc(' '); putc(8);
            continue;
        }
        if(len<INPUT_MAX-1){
            buf[len++]=c;
            putc(c);
        }
    }
}

/* ============================================================
 * Paging
 * ============================================================
 */

static void load_page(Page *p,uint16_t page){
    uint16_t record=page<<2;
    //cpm_printstring("Load page from record ");
    //printi(record);
    //crlf();
    cpm_fcb.r = record;
    //story_fcb.r0=record&0xFF;
    //story_fcb.r1=record>>8;
    //story_fcb.r2=0;
    //cmp_set_dma(dma);
    //crlf();
    for(uint8_t i=0;i<4;i++){
        //bdos(BDOS_READ_RAND,&story_fcb);
        cpm_set_dma(&dma);
        cpm_read_random(&cpm_fcb);
        for(uint8_t j=0;j<128;j++) {
            p->data[(i<<7)+j]=dma[j];
        }
        //crlf();
        //crlf();
        record++;
        cpm_fcb.r = record;
    }
    p->page=page;
    p->valid=1;
}

static Page *get_page(uint16_t page){
    //cpm_printstring("Page ");
    //printi(page);
    //cpm_printstring(" requested");
    //crlf();
    for(uint8_t i=0;i<NUM_PAGES;i++)
        if(page_cache[i].valid&&page_cache[i].page==page)
            return &page_cache[i];
    Page *p=&page_cache[next_victim];
    next_victim=(next_victim+1)%NUM_PAGES;
    load_page(p,page);
    return p;
}

/* ============================================================
 * Memory
 * ============================================================
 */

static uint8_t zm_read8(uint16_t a){
    //cpm_printstring("read 8 addr ");
    //printi(a);
    //cpm_printstring(" data ");
    
    if(a<dynamic_size) {
        //cpm_printstring("Read ");
        //print_hex(dynamic_mem[a]);
        //cpm_printstring(" from address ");
        //print_hex(a);
        //cpm_printstring(" (dynamic)");
        //crlf();
        return dynamic_mem[a];
    }
    Page *p=get_page(a>>9);
    //printi(p->data[a&0x1FF]);
    //crlf();
    //cpm_printstring("Read ");
    //print_hex(p->data[a&0x1FF]);
    //cpm_printstring(" from address ");
    //print_hex(a);
    //crlf();
    return p->data[a&0x1FF];
}

static uint16_t zm_read16(uint16_t a){
    return ((uint16_t)zm_read8(a)<<8)|zm_read8(a+1);
}

static void zm_write8(uint16_t a,uint8_t v){
    if(a>=dynamic_size) {
        crlf();
        print_hex(a);
        fatal(" Illegal write address");
    
    }
    dynamic_mem[a]=v;
}

/* ============================================================
 * Stack & variables
 * ============================================================
 */

static void push(int16_t v){stack[sp++]=v;}
static int16_t pop(void){return stack[--sp];}

static uint16_t get_var(uint8_t v){
//    cpm_printstring("get_var - v:");
//    print_hex(v);
//    crlf();
    if(v==0) {
        uint16_t stack;
        stack = pop();
     //   cpm_printstring("stack: ");
     //   print_hex(stack);
     //   crlf();
        return stack;
    }
    if(v<16) {
   //     cpm_printstring("local: ");
   //     print_hex(frames[fp-1].locals[v-1]);
  //      crlf();
        return frames[fp-1].locals[v-1];
    }
        //uint16_t a=zm_read16(0x0C)+2*(v-16);
    uint16_t a = (hdr[0x0c]<<8) + hdr[0x0d] + 2*(v-16);
    //cpm_printstring("get_var - a:");
    //print_hex(a);
    //crlf();
    uint16_t ret_data = zm_read16(a);
    //cpm_printstring("global: ");
    //print_hex(ret_data);
    //crlf();
    return ret_data;
}

static void set_var(uint8_t v,uint16_t val){
//    cpm_printstring("Setvar - var: ");
//    print_hex(v);
//    cpm_printstring(" val: ");
//    print_hex(val);
//    crlf();
    if(v==0) push(val);
    else if(v<16) frames[fp-1].locals[v-1]=val;
    else{
        //uint16_t a=zm_read16(0x0C)+2*(v-16);
        uint16_t a = (hdr[0x0c]<<8) + hdr[0x0d] + 2*(v-16);
        //cpm_printstring("Setvar ");
        //printi(a);
        //spc();
        //printi(val);
        //spc();
        //printi(v);
        zm_write8(a,val>>8);
        zm_write8(a+1,val&0xFF);
    }
}

/* ============================================================
 * Operand decoding
 * ============================================================
 */

enum{OP_LARGE,OP_SMALL,OP_VAR,OP_OMIT};

static uint16_t operands[MAX_OPERANDS];
static uint8_t operand_count;

static uint16_t read_operand(uint8_t t){
    if(t==OP_LARGE){pc+=2;return zm_read16(pc-2);}
    if(t==OP_SMALL)return zm_read8(pc++);
    if(t==OP_VAR)return get_var(zm_read8(pc++));
    return 0;
}


static void decode_operands(uint8_t spec){
    operand_count=0;
    for(int s=6;s>=0;s-=2){
        uint8_t t=(spec>>s)&3;
        if(t==OP_OMIT) break;
        operands[operand_count++]=read_operand(t);
    }
}


/* ============================================================
 * Branching
 * ============================================================
 */

static void branch(uint8_t cond){
    uint8_t b=zm_read8(pc++);
    uint8_t sense=b&0x80;
    int16_t off;

    if(b&0x40) off=b&0x3F;
    else{
        off=((b&0x3F)<<8)|zm_read8(pc++);
        if(off&0x2000) off|=0xC000;
    }

    if((cond!=0)==(sense!=0)){
        if(off==0) push(0),pc=frames[--fp].return_pc;
        else if(off==1) push(1),pc=frames[--fp].return_pc;
        else pc+=off-2;
    }
}

/* ============================================================
 * ZSCII output (minimal)
 * ============================================================
 */

static void print_zstring(uint16_t addr){
    while(1){
        uint16_t w=zm_read16(addr);
        addr+=2;
        
        for(int i=10;i>=0;i-=5){
            uint8_t c=(w>>i)&0x1F;
           
            if(abbrev_print == 1) {
                uint16_t string_addr;
                alphabet = 0;
                string_addr = zm_read16(abbrev_base+(((abbrev_table<<5)+c)<<1));
                abbrev_print = 0;
                print_zstring(string_addr<<1);
                alphabet = 0;
            }
            else if(c>=6) {
                putc(zalph[alphabet][c-6]);
                alphabet = 0;
            } 
            else if(c==4) {
                alphabet = 1;
            }
            else if(c==5) {
                alphabet = 2;
            }
            else if((c>0) && (c<4)) {
                abbrev_print = 1;
                abbrev_table = c-1;
            }
            else if(c==0) putc(' ');

        }
        if(w&0x8000) {
            alphabet = 0;
            break;
        }
    }
}

static void print_num(int16_t v){
    char buf[6];
    uint8_t i=0;

    if(v<0){ putc('-'); v=-v; }
    do{
        buf[i++]='0'+(v%10);
        v/=10;
    }while(v);
    while(i--) putc(buf[i]);
}

// Dictionary
static void dict_init(void){
    dict_addr = hdr[0x08]<<8 | hdr[0x09];
    cpm_printstring("Dict addr ");
    print_hex(dict_addr);
    dict_sep_count = zm_read8(dict_addr++);
    cpm_printstring(" Dict sep cnt ");
    printi(dict_sep_count);
    for(uint8_t i=0;i<dict_sep_count;i++) {
        dict_seps[i]=zm_read8(dict_addr++);
        spc();
        putc(dict_seps[i]);
        spc();
    }

    dict_entry_len   = zm_read8(dict_addr++);
    crlf();
    cpm_printstring("Dict entry length: ");
    print_hex(dict_entry_len);
    crlf();
    dict_entry_count = zm_read16(dict_addr);
    cpm_printstring("Dict entry count:" );
    print_hex(dict_entry_count);
    crlf();
}


static uint8_t encode_a2(char c)
{
    /* A2 table for v1–v3 */
    const char *a2 = " \n0123456789.,!?_#'\"/\\-:()";
    for (uint8_t i = 0; a2[i]; i++) {
        if (a2[i] == c)
            return i;
    }
    return 0; /* space */
}

void encode_zchars(const char *s, uint8_t zchars[6])
{
    uint8_t zi = 0;

    while (*s && zi < 6) {
        char c = *s++;

        /* force lowercase */
        if (c >= 'A' && c <= 'Z')
            c = c - 'A' + 'a';

        if (c >= 'a' && c <= 'z') {
            zchars[zi++] = (c - 'a') + 6;
        }
        else {
            /* shift to A2 */
            if (zi < 6)
                zchars[zi++] = 5;
            if (zi < 6)
                zchars[zi++] = encode_a2(c);
        }
    }

    /* pad with spaces (A2 space = shift 5 + 0, but Inform pads with 5) */
    while (zi < 6)
        zchars[zi++] = 5;
}

uint16_t dict_lookup(const char *word)
{
    uint8_t zchars[6];
    encode_zchars(word, zchars);

    uint16_t w0 = (zchars[0] << 10) | (zchars[1] << 5) | zchars[2];
    uint16_t w1 = (zchars[3] << 10) | (zchars[4] << 5) | zchars[5] | 0x8000;
    cpm_printstring("dict lookup ");
    cpm_printstring(word);
    print_hex(w0);
    spc();
    print_hex(w1);
    crlf();
    uint16_t dict = dict_addr;
    uint8_t sep = dict_sep_count;

    uint8_t entry_len = dict_entry_len;
    uint16_t count = dict_entry_count;
    
    cpm_printstring("sep: ");
    printi(sep);
    cpm_printstring(" entry_len: ");
    printi(entry_len);
    cpm_printstring(" count: ");
    printi(count);
    crlf();
    
    uint16_t e = dict_addr+2;

    cpm_printstring("Starting search at address ");
    print_hex(e);
    crlf();
    for (uint16_t i = 0; i < dict_entry_count; i++) {
        //uint16_t e = dict + i * entry_len;
        e += dict_entry_len;
        if (zm_read16(e) == w0 && zm_read16(e + 2) == w1) {
            cpm_printstring("Found match at address");
            print_hex(e);
            return e;
        
        }
    }

    return 0;
}



// Tokenization

static uint8_t is_sep(char c){
    cpm_printstring("is_sep: ");
    cpm_conout(c);
    crlf();
    for(uint8_t i=0;i<dict_sep_count;i++)
        if(c==dict_seps[i]) return 1;
    return c==' ';
}

static uint8_t tokenize(char *in, uint16_t parse_buf, uint16_t text_buf) 
{
    uint8_t count=0;
    uint8_t i=0;

    while(in[i] && count<MAX_TOKENS){
        while(is_sep(in[i])) i++;
        if(!in[i]) break;

        uint8_t start=i;
        char word[10]={0};
        uint8_t len=0;

        while(in[i] && !is_sep(in[i]) && len<6)
            word[len++]=in[i++];

        uint16_t dict = dict_lookup(word);

        uint16_t entry = parse_buf + 2 + count*4;
        zm_write8(entry, dict>>8);
        zm_write8(entry+1, dict);
        zm_write8(entry+2, len);
        zm_write8(entry+3, start+1);

        count++;
    }

    zm_write8(parse_buf+1,count);
    return count;
}


// Objects

static uint16_t obj_addr(uint8_t obj)
{
    return object_table + 62 + (obj - 1) * 9;
}

static uint8_t obj_test_attr(uint8_t obj, uint8_t attr){
    uint16_t a = obj_addr(obj) + (attr >> 3);
    return zm_read8(a) & (0x80 >> (attr & 7));
}

static void obj_set_attr(uint8_t obj, uint8_t attr){
    uint16_t a = obj_addr(obj) + (attr >> 3);
    zm_write8(a, zm_read8(a) | (0x80 >> (attr & 7)));
}

static void obj_clear_attr(uint8_t obj, uint8_t attr){
    uint16_t a = obj_addr(obj) + (attr >> 3);
    zm_write8(a, zm_read8(a) & ~(0x80 >> (attr & 7)));
}

static uint8_t obj_parent(uint8_t obj){
    return zm_read8(obj_addr(obj) + 4);
}

static uint8_t obj_sibling(uint8_t obj){
    return zm_read8(obj_addr(obj) + 5);
}

static uint8_t obj_child(uint8_t obj){
    return zm_read8(obj_addr(obj) + 6);
}

static void obj_remove(uint8_t obj){
    uint8_t parent = obj_parent(obj);
    if (!parent) return;

    uint16_t paddr = obj_addr(parent) + 6;
    uint8_t cur = zm_read8(paddr);

    if (cur == obj) {
        zm_write8(paddr, obj_sibling(obj));
    } else {
        while (cur) {
            uint16_t caddr = obj_addr(cur) + 5;
            uint8_t sib = zm_read8(caddr);
            if (sib == obj) {
                zm_write8(caddr, obj_sibling(obj));
                break;
            }
            cur = sib;
        }
    }

    zm_write8(obj_addr(obj) + 4, 0);
    zm_write8(obj_addr(obj) + 5, 0);
}

static void obj_insert(uint8_t obj, uint8_t dest){
    obj_remove(obj);

    uint16_t daddr = obj_addr(dest) + 6;
    zm_write8(obj_addr(obj) + 4, dest);
    zm_write8(obj_addr(obj) + 5, zm_read8(daddr));
    zm_write8(daddr, obj);
}


static uint16_t obj_prop_table(uint8_t obj){
    return zm_read16(obj_addr(obj) + 7);
}

static uint16_t obj_prop_start(uint8_t obj){
    uint16_t p = obj_prop_table(obj);
    return p + 1 + 2 * zm_read8(p);
}

static uint16_t obj_find_prop(uint8_t obj, uint8_t prop){
    uint16_t p = obj_prop_start(obj);

    while (1) {
        uint8_t h = zm_read8(p++);
        if (!h) return 0;

        uint8_t num = h & 0x1F;
        uint8_t len = (h >> 5) + 1;

        if (num == prop) return p;
        p += len;
    }
}

static uint16_t obj_get_prop(uint8_t obj, uint8_t prop){
    uint16_t p = obj_find_prop(obj, prop);
    if (!p) {
        return zm_read16(object_table + (prop - 1) * 2);
    }

    uint8_t len = ((zm_read8(p - 1) >> 5) & 7) + 1;
    if (len == 1) return zm_read8(p);
    return zm_read16(p);
}

static uint8_t obj_get_prop_len(uint16_t addr){
    if (!addr) return 0;
    return ((zm_read8(addr - 1) >> 5) & 7) + 1;
}

static void obj_put_prop(uint8_t obj, uint8_t prop, uint16_t val){
    cpm_printstring("obj_put_prop - obj: ");
    print_hex(obj);
    cpm_printstring(" prop: ");
    print_hex(prop);
    cpm_printstring(" val: ");
    print_hex(val);
    
    uint16_t p = obj_find_prop(obj, prop);
    
    cpm_printstring(" p: ");
    print_hex(p);
    crlf();
    
    if (!p) {
        // property must exist
        return;
    }

    uint8_t len = ((zm_read8(p - 1) >> 5) & 7) + 1;

    if (len == 1) {
        zm_write8(p, (uint8_t)val);
    } else {
        zm_write8(p, val >> 8);
        zm_write8(p + 1, val & 0xFF);
    }
}


/* ============================================================
 * Execution
 * ============================================================
 */

static void z_ret(uint16_t v){
    CallFrame *f=&frames[--fp];
    pc=f->return_pc;
    set_var(f->store_var,v);
}

static void store_result(uint16_t v){
    uint8_t var=zm_read8(pc++);
    //cpm_printstring("Store ");
    //printi(v);
    //cpm_printstring(" in variable ");
    //printi(var);
    //crlf();
    set_var(var,v);
}

static void restart(void)
{
    pc = initial_pc;
    sp = fp = 0;
}

static inline uint16_t unpack_routine(uint16_t p)
{
    // v1-v3 - only version supported for now
    return p << 1;
}

static void step(void){
    uint8_t op=zm_read8(pc++);
    //cpm_printstring("OP: ");
    //print_hex(op);
    //cpm_printstring(" PC: ");
    //print_hex(pc);
    //crlf();
    /* -------- 2OP -------- */
    if(op<0x80){
        uint8_t t1=(op&0x40)?OP_VAR:OP_SMALL;
        uint8_t t2=(op&0x20)?OP_VAR:OP_SMALL;
        if(op == 0x05 || op ==0x04) t1=OP_VAR;
        uint8_t var_num = zm_read8(pc);
        operands[0]=read_operand(t1);
        operands[1]=read_operand(t2);

        //cpm_printstring("2OP - ");
        //print_hex(operands[0]);
        //spc();
        //print_hex(operands[1]);
        //crlf();

        switch(op&0x1F){
        case OP2_JE:
            //cpm_printstring("JE ");
            //printi(operands[0]);
            //spc();
            //printi(operands[1]);
            //crlf();  
            branch(operands[0]==operands[1]); 
            break;
        case OP2_JL:  
            branch((int16_t)operands[0]<(int16_t)operands[1]); 
            break;
        case OP2_JG:  
            branch((int16_t)operands[0]>(int16_t)operands[1]); 
            break;
        case OP2_JIN: {
            uint8_t parent;
            parent = obj_parent(operands[0]);
            branch(parent == operands[1]); 
            break;
            }
        case OP2_DEC_CHK:
            operands[0]--;
            set_var(var_num,operands[0]);
            branch((int16_t)operands[0]<(int16_t)operands[1]);
            break;
        case OP2_INC_CHK:
            //cpm_printstring("OP2_INC_CHK entry - OP1: ");
            //print_hex(operands[0]);
            //cpm_printstring(" OP2: ");
            //print_hex(operands[1]);
            //cpm_printstring(" Var: ");
            //print_hex(var_num);
            //crlf(); 
            operands[0]++;
            set_var(var_num,operands[0]);
            branch((int16_t)operands[0]>(int16_t)operands[1]);
            break;
        case OP2_TEST:
            branch((operands[0]&operands[1])==operands[1]);
            break;
        case OP2_OR:    
            store_result(operands[0]|operands[1]); 
            break;
        case OP2_AND:   
            store_result(operands[0]&operands[1]); 
            break;
        case OP2_STORE: 
            set_var(operands[0],operands[1]); 
            break;
        case OP2_LOADW: 
            store_result(zm_read16(operands[0]+2*operands[1])); 
            break;
        case OP2_LOADB: 
            store_result(zm_read8(operands[0]+operands[1])); 
            break;
        case OP2_ADD:   
            store_result((int16_t)operands[0]+(int16_t)operands[1]); 
            break;
        case OP2_SUB:   
            store_result((int16_t)operands[0]-(int16_t)operands[1]); 
            break;
        case OP2_MUL:   
            store_result((int16_t)operands[0]*(int16_t)operands[1]); 
            break;
        case OP2_DIV:   
            store_result((int16_t)operands[0]/(int16_t)operands[1]); 
            break;
        case OP2_MOD:   
            store_result((int16_t)operands[0]%(int16_t)operands[1]); 
            break;
        case OP2_TEST_ATTR:
            branch(obj_test_attr(operands[0], operands[1]));
            break;

        case OP2_SET_ATTR:
            obj_set_attr(operands[0], operands[1]);
            break;

        case OP2_CLEAR_ATTR:
            obj_clear_attr(operands[0], operands[1]);
            break;

        case OP2_INSERT_OBJ:
            obj_insert(operands[0], operands[1]);
            break;

        case OP2_GET_PROP:
            store_result(obj_get_prop(operands[0], operands[1]));
            break;

        case OP2_GET_PROP_ADDR: {
            uint16_t p = obj_find_prop(operands[0], operands[1]);
            store_result(p);
            break;
        }

        case OP2_GET_NEXT_PROP: {
            uint16_t p = obj_find_prop(operands[0], operands[1]);
            if (!p) {
                store_result(0);
            } else {
                uint8_t h = zm_read8(p - 1);
                store_result(h & 0x1F);
            }
            break;
        }
        
        default:
            crlf();
            printi(op);
            fatal(" - Non-implemented opcode!!!"); 
        }
        return;
    }

    /* -------- 1OP -------- */
    if(op<0xB0){
        uint8_t type=(op>>4)&3;
        uint8_t oc=op&0x0F;
        if(type!=OP_OMIT) operands[0]=read_operand(type);
        //cpm_printstring("Type: ");
        //printi(type);
        //cpm_printstring(" Operand 1: ");
        //printi(operands[0]);
        //cpm_printstring(" oc: ");
        //print_hex(oc);
        //crlf();
        switch(oc){
        /*case OP1_RTRUE: 
            z_ret(1); 
            break;
        case OP1_RFALSE:
            z_ret(0); 
            break;*/
        /*case OP1_PRINT:
            print_zstring(pc);
            while(!(zm_read16(pc)&0x8000)) pc+=2;
            pc+=2;
            break;*/
        case OP1_JUMP:
            //cpm_printstring("Performing jump with offset");
            //printi(operands[0]-2);
            //crlf();
            pc+=(int16_t)operands[0]-2;
            break;
        case OP1_PRINT_ADDR:
            print_zstring(operands[0]);
            break;
        case OP1_PRINT_OBJ: {
            uint16_t obj = operands[0];

            if(obj==0) break;

            uint16_t entry = obj_addr(obj);
            uint16_t prop_table = zm_read16(entry + 7);
            uint8_t name_len = zm_read8(prop_table);
            if(name_len==0) break;
            print_zstring(prop_table+1);
            break;
        }
        case OP1_GET_CHILD: {
            uint8_t result;
            if(operands[0]==0) result = 0;
            else result = obj_child(operands[0]);
            store_result(result);
            branch(result!=0);
            break;
        }
        case OP1_GET_SIBLING: {
            uint8_t result;
            if(operands[0]==0) result = 0;
            else result = obj_sibling(operands[0]);
            store_result(result);
            branch(result!=0);
            break;
        }


        case OP1_GET_PARENT: {
            uint8_t result;
            if(operands[0]==0) result = 0;
            else result = obj_parent(operands[0]);
            store_result(result);
            break;
        }
        case OP1_RET:
        //case OP1_VALUE:
            z_ret(operands[0]);
            break;
        case OP1_INC:
            operands[0]++;
            set_var(zm_read8(pc-1),operands[0]);
            break;
        case OP1_DEC:
            operands[0]--;
            set_var(zm_read8(pc-1),operands[0]);
            break;
        //case OP1_RET:
        //    z_ret(operands[0]);
        //    break;
        case OP1_JZ:
            branch(operands[0]==0);
            break;
        case OP1_PRINT_PADDR: {
            uint16_t addr = unpack_routine(operands[0]);
            print_zstring(addr);
            break;
        }
        default:
            crlf();
            printi(op); 
            fatal(" - Non-implemented opcode!!!");
        }
        return;
    }
    /* 0OP */
    if(op<0xC0) {
        switch(op&0x0F){
        case OP0_RTRUE:        
            z_ret(1); 
            break;
        case OP0_RFALSE:       
            z_ret(0); 
            break;
        case OP0_PRINT:
            print_zstring(pc);
            while(!(zm_read16(pc)&0x8000)) pc+=2;
            pc+=2;
            break;
        case OP0_PRINT_RET:
            print_zstring(pc);
            while(!(zm_read16(pc)&0x8000)) pc+=2;
            pc+=2;
            crlf();
            z_ret(1);
            break;

        case OP0_NOP:
            break;

        case OP0_SAVE:
        case OP0_RESTORE:
            branch(0); /* always fail */
            break;

        case OP0_RESTART:
            restart();
            break;

        case OP0_RET_POPPED:
            z_ret(pop());
            break;

        case OP0_POP:
            pop();
            break;

        case OP0_NEW_LINE:
            crlf();
            break;

        case OP0_QUIT:
            cpm_warmboot();
        default:
            crlf();
            printi(op);
            fatal(" - Non-implemented opcode!!!");
        }
        return;
    }
    /* -------- VAR -------- */
    decode_operands(zm_read8(pc++));
    uint8_t var_opcode = op & 0x1f;
    if((op & 0xE0)==0xE0) var_opcode += 0x20;
    //cpm_printstring("Var opcode: ");
    //print_hex(var_opcode);
    //crlf();
    switch(var_opcode) {
    /*case OPV_CALL:
        cpm_printstring("Call - ");
        if(operands[0]==0) {
            uint8_t sv=zm_read8(pc++);
            set_var(sv,0);
            return;
        } else {
            CallFrame *f=&frames[fp++];
            f->return_pc=pc+1;
            f->store_var=zm_read8(pc++);
            uint16_t addr=unpack_routine(operands[0]);
            cpm_printstring("Addr: ");
            printi(addr);
            f->num_locals=zm_read8(addr++);
            cpm_printstring("Num lcls: ");
            printi(f->num_locals);
            for(uint8_t i=0;i<f->num_locals;i++)
                f->locals[i]=zm_read16(addr+2*i);
            pc=addr+2*f->num_locals;
            cpm_printstring(" New PC: ");
            printi(pc);
            crlf();
        }
        break;*/
    case OPV_CALL_EXT:
    case OPV_CALL: {
        if (operands[0] == 0) {
            uint8_t sv = zm_read8(pc++);
            set_var(sv, 0);
            return;
        }

        CallFrame *f = &frames[fp++];

        uint8_t sv = zm_read8(pc++);
        f->store_var = sv;
        f->return_pc = pc;

        uint16_t addr = unpack_routine(operands[0]);
        //cpm_printstring("CALL to address ");
        //print_hex(addr);
        //crlf();
        f->num_locals = zm_read8(addr++);

        for (uint8_t i = 0; i < f->num_locals; i++)
            f->locals[i] = zm_read16(addr + 2*i);

        /* overwrite locals with arguments */
        uint8_t argc = operand_count - 1;
        if (argc > f->num_locals)
            argc = f->num_locals;

        for (uint8_t i = 0; i < argc; i++)
            f->locals[i] = operands[i + 1];

        pc = addr + 2 * f->num_locals;
        break;
    }
    case OPV_JE:
        branch(operands[0] == operands[1]);
        break;
    case OPV_JL:
        branch((int16_t)operands[0] < (int16_t)operands[1]);
        break;
    case OPV_JG:
        branch((int16_t)operands[0] > (int16_t)operands[1]);
        break;

    case OPV_STOREW:
        //cpm_printstring("STOREW: ");
        //printi(operands[0]);
        //spc();
        //printi(operands[1]);
        //spc();
        //printi(operands[2]);
        //crlf();
        zm_write8(operands[0]+2*operands[1],operands[2]>>8);
        zm_write8(operands[0]+2*operands[1]+1,operands[2]);
        break;

    case OPV_STOREB:
        zm_write8(operands[0]+operands[1],operands[2]);
        break;

    case OPV_PRINT_CHAR:
        putc((char)operands[0]);
        break;

    case OPV_PRINT_NUM:
        print_num((int16_t)operands[0]);
        break;

    case OPV_PUSH:
        push(operands[0]);
        break;

    case OPV_PULL:
        set_var(operands[0],pop());
        break;
    case OPV_SREAD: {
        uint16_t text = operands[0];
        uint16_t parse = operands[1];

        char line[INPUT_MAX];
        uint8_t len = read_line(line);

        uint8_t max = zm_read8(text);
        if(len>max) len=max;

        zm_write8(text+1,len);
        for(uint8_t i=0;i<len;i++)
            zm_write8(text+2+i,line[i]);

        tokenize(line,parse,text);
        break; }
    case OPV_PUT_PROP:
            cpm_printstring("Putprop - ");
            print_hex(operands[0]);
            spc();
            print_hex(operands[1]);
            spc();
            print_hex(operands[2]);
            crlf();
            obj_put_prop((uint8_t)operands[0],(uint8_t)operands[1],
                         operands[2]);
        break;
    case OPV_OR:
        store_result(operands[0] | operands[1]);
        break;
    case OPV_AND:
        store_result(operands[0] & operands[1]);
        break;
    default:
        crlf();
        printi(op); 
        fatal(" - Non-implemented opcode!!!");
 
    }

}

/* ============================================================
 * Entry
 * ============================================================
 */

int main(int agrv, char **argv){
    /*
    story_fcb.drive=0;
    story_fcb.name[0]='S';story_fcb.name[1]='T';story_fcb.name[2]='O';
    story_fcb.name[3]='R';story_fcb.name[4]='Y';story_fcb.name[5]=' ';
    story_fcb.name[6]=' ';story_fcb.name[7]=' ';
    story_fcb.ext[0]='Z';story_fcb.ext[1]='3';story_fcb.ext[2]=' ';
    */
    // Get story filename from commandline

    //cpm_fcb.cr = 0;
    if(cpm_open_file(&cpm_fcb))
        fatal("Could not open input file!");

    cpm_set_dma(&hdr);
    //bdos(BDOS_READ_SEQ,&story_fcb);
    cpm_read_sequential(&cpm_fcb);
    //uint8_t *hdr=dma;

    dynamic_size=(hdr[0x0e]<<8)|hdr[0x0f];
    cpm_printstring("Dynamic size: ");
    
    for(uint16_t i=0; i<DYNAMIC_MEM_MAX; i++)
        dynamic_mem[i] = 0;

    printi(dynamic_size);
    crlf();
    if(dynamic_size>DYNAMIC_MEM_MAX) while(1);

    for(uint16_t i=0; i<128; i++)
        dynamic_mem[i] = hdr[i];

    for(uint16_t i=128;i<dynamic_size;i++){
        if((i&0x7f)==0) {
            cpm_set_dma(&dma);
            cpm_read_sequential(&cpm_fcb);//bdos(BDOS_READ_SEQ,&story_fcb);
        }
        dynamic_mem[i]=dma[i&0x7f];
    }

    pc=initial_pc=(hdr[0x06]<<8)|hdr[0x07];
    sp=fp=next_victim=0;
    for(uint8_t i=0;i<NUM_PAGES;i++) page_cache[i].valid=0;
    
    dict_init();
    object_table = (hdr[0x0a]<<8)|hdr[0x0b];

    alphabet = 0;
    abbrev_print = 0;
    abbrev_table = 0;
    abbrev_base = (hdr[0x18]<<8)|hdr[0x19];

    cpm_printstring("PC: ");
    print_hex(pc);
    crlf();
    for(;;) step();
}
