/* z65 - Z-machine v3 interpreter for CP/M-65
 *
 * Copyright (C) 2026 by Henrik Löfgren
 *
 * BSD-2 License. See LICENSE file in root directory.
 *
 */

#include <stdint.h>
#include <cpm.h>
#include "lib/printi.h"
#include "lib/screen.h"
#include "zmalloc.h"

/* ============================================================
 * Opcode definitions
 * ============================================================
 */

// 2OP
#define OP2_JE 0x01
#define OP2_JL 0x02
#define OP2_JG 0x03
#define OP2_DEC_CHK 0x04
#define OP2_INC_CHK 0x05
#define OP2_JIN 0x06
#define OP2_TEST 0x07
#define OP2_OR 0x08
#define OP2_AND 0x09
#define OP2_STORE 0x0D
#define OP2_LOADW 0x0F
#define OP2_LOADB 0x10
#define OP2_ADD 0x14
#define OP2_SUB 0x15
#define OP2_MUL 0x16
#define OP2_DIV 0x17
#define OP2_MOD 0x18
#define OP2_TEST_ATTR 0x0A
#define OP2_SET_ATTR 0x0B
#define OP2_CLEAR_ATTR 0x0C
#define OP2_INSERT_OBJ 0x0E
#define OP2_GET_PROP 0x11
#define OP2_GET_PROP_ADDR 0x12
#define OP2_GET_NEXT_PROP 0x13

// 1OP
#define OP1_JZ 0x00
#define OP1_GET_SIBLING 0x01
#define OP1_GET_CHILD 0x02
#define OP1_GET_PARENT 0x03
#define OP1_GET_PROP_LEN 0x04
#define OP1_INC 0x05
#define OP1_DEC 0x06
#define OP1_PRINT_ADDR 0x07
#define OP1_CALL_1S 0x08
#define OP1_REMOVE_OBJ 0x09
#define OP1_PRINT_OBJ 0x0A
#define OP1_RET 0x0B
#define OP1_JUMP 0x0C
#define OP1_PRINT_PADDR 0x0D
#define OP1_LOAD 0x0E
#define OP1_NOT 0x0F

// 0OP
#define OP0_RTRUE 0x00
#define OP0_RFALSE 0x01
#define OP0_PRINT 0x02
#define OP0_PRINT_RET 0x03
#define OP0_NOP 0x04
#define OP0_SAVE 0x05
#define OP0_RESTORE 0x06
#define OP0_RESTART 0x07
#define OP0_RET_POPPED 0x08
#define OP0_POP 0x09
#define OP0_QUIT 0x0A
#define OP0_NEW_LINE 0x0B
#define OP0_SHOW_STATUS 0x0C
#define OP0_VERIFY 0x0D

// VAR
#define OPV_CALL 0x00
#define OPV_JE 0x01
#define OPV_JL 0x02
#define OPV_JG 0x03
#define OPV_DEC_CHK 0x04
#define OPV_INC_CHK 0x05
#define OPV_JIN 0x06
#define OPV_TEST 0x07
#define OPV_STOREW 0x21
#define OPV_STOREB 0x22
#define OPV_PUT_PROP 0x23
#define OPV_SREAD 0x24
#define OPV_PRINT_CHAR 0x25
#define OPV_PRINT_NUM 0x26
#define OPV_RANDOM 0x27
#define OPV_AND 0x09
#define OPV_OR 0x08
#define OPV_TEST_ATTR 0x0A
#define OPV_SET_ATTR 0x0B
#define OPV_INSERT_OBJ 0x0E
#define OPV_LOADW 0x0F
#define OPV_STORE 0x0D
#define OPV_CALL_EXT 0x20
#define OPV_PUSH 0x28
#define OPV_PULL 0x29
#define OPV_LOADB 0x50
#define OPV_ADD 0x54
#define OPV_SUB 0x55
#define OPV_MUL 0x56

/* ============================================================
 * Header definitions
 * ============================================================
 */

#define HDR_VERSION 0x00
#define HDR_FLAGS 0x01
#define HDR_PC 0x06
#define HDR_DICT 0x08
#define HDR_OBJ 0x0A
#define HDR_GLB 0x0C
#define HDR_STAT 0x0E
#define HDR_ABBR 0x18

/* ============================================================
 * Configuration
 * ============================================================
 */

#define PAGE_SIZE 512
#define NUM_PAGES 8

#define STACK_SIZE 512
#define MAX_FRAMES 16
#define MAX_LOCALS 16
#define MAX_OPERANDS 8

#define INPUT_MAX 80
#define MAX_TOKENS 16

#define DEBUG 0
#define STATUS 1

/* ============================================================
 * Structures
 * ============================================================
 */

typedef struct {
	uint32_t return_pc;
	uint8_t store_var;
	uint8_t num_locals;
	uint16_t locals[MAX_LOCALS];
	uint16_t saved_sp;
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

static uint8_t *dynamic_mem;
static uint16_t dynamic_size;

static Page page_cache[NUM_PAGES];
static uint8_t next_victim;

static uint32_t pc;
static uint32_t initial_pc;
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
static uint8_t zscii_esc = 0;
static uint8_t zscii_high;
static uint8_t zalph[3][26] = {
	{ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' },
	{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' },
	{ ' ', '\n', '0', '1', '2', '3',  '4',	'5', '6',  '7', '8', '9', '.',
	  ',', '!',	 '?', '_', '#', '\'', '\"', '/', '\\', '-', ':', '(', ')' }
};

static uint8_t abbrev_print;
static uint8_t abbrev_table;
static uint16_t abbrev_base;

static uint16_t rng_state = 1;
static uint8_t rng_enabled = 1;

static void z_ret(uint16_t v);

static uint8_t w_addr;

static uint8_t status = 0;

#if DEBUG
static long int opcnt = 0;
#endif

/* ============================================================
 * Console
 * ============================================================
 */

static inline void putc(char c)
{
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

#if DEBUG
static void print_hex(uint16_t val)
{
	cpm_printstring("0x");
	for (uint8_t i = 4; i > 0; i--) {
		uint8_t nibble = (val >> ((i - 1) << 2)) & 0xf;

		if (nibble < 10)
			cpm_conout(nibble + '0');
		else
			cpm_conout(nibble - 10 + 'A');
	}
}

static void print_hex_32(uint32_t val)
{
	cpm_printstring("0x");
	for (uint8_t i = 8; i > 0; i--) {
		uint8_t nibble = (val >> ((i - 1) << 2)) & 0xf;

		if (nibble < 10)
			cpm_conout(nibble + '0');
		else
			cpm_conout(nibble - 10 + 'A');
	}
}
#endif

static void fatal(const char *s)
{
	cpm_printstring(s);
	crlf();
	cpm_warmboot();
}

static uint8_t read_line(char *buf)
{
	uint8_t len = 0;
	for (;;) {
		char c = cpm_conin();

		/* force lowercase */
		if (c >= 'A' && c <= 'Z')
			c = c - 'A' + 'a';

		if (c == '\r' || c == '\n') {
			crlf();
			buf[len] = 0;
		return len;
		}
		if ((c == 8 || c == 127) && len) {
			len--;
			continue;
		}
		if (len < INPUT_MAX - 1) {
			buf[len++] = c;
		}
	}
}

/* ============================================================
 * Paging
 * ============================================================
 */

static void load_page(Page *p, uint16_t page)
{
	uint16_t record = page << 2;
	cpm_fcb.r = record;
	for (uint8_t i = 0; i < 4; i++) {
		cpm_set_dma(&dma);
		cpm_read_random(&cpm_fcb);
		for (uint8_t j = 0; j < 128; j++) {
			p->data[(i << 7) + j] = dma[j];
		}
		record++;
		cpm_fcb.r = record;
	}
	p->page = page;
	p->valid = 1;
}

static Page *get_page(uint16_t page)
{
	for (uint8_t i = 0; i < NUM_PAGES; i++)
		if (page_cache[i].valid && page_cache[i].page == page)
			return &page_cache[i];
	Page *p = &page_cache[next_victim];
	next_victim = (next_victim + 1) % NUM_PAGES;
	load_page(p, page);
	return p;
}

/* ============================================================
 * Memory
 * ============================================================
 */

static uint8_t zm_read8(uint32_t a)
{
	uint8_t data;
	if (a < dynamic_size) {
		data = dynamic_mem[a];
	} else {
		Page *p = get_page(a >> 9);
		data = p->data[a & 0x1FF];
	}
	return data;
}

static uint16_t zm_read16(uint32_t a)
{
	return ((uint16_t)zm_read8(a) << 8) | zm_read8(a + 1);
}

static void zm_write8(uint32_t a, uint8_t v)
{
	if (a >= dynamic_size) {
		fatal(" Illegal write address");
	}
	dynamic_mem[a] = v;
}

/* ============================================================
 * Stack & variables
 * ============================================================
 */

static void push(int16_t v)
{
	stack[sp++] = v;
}
static uint16_t pop(void)
{
	uint16_t val;
	val = stack[--sp];
	return val;
}

static uint16_t get_var(uint8_t v, uint8_t indirect)
{
	if (v == 0) {
		uint16_t val;
		if (indirect)
			val = (sp > 0) ? stack[sp - 1] : 0;
		else
			val = pop();
		return val;
	}
	if (v < 16) {
	    return frames[fp - 1].locals[v - 1];
	}
	uint16_t a = (hdr[HDR_GLB] << 8) + hdr[HDR_GLB + 1] + 2 * (v - 16);
	uint16_t ret_data = zm_read16(a);
#if DEBUG
	cpm_printstring("Read global variable ");
	print_hex(v);
	cpm_printstring(" data: ");
	print_hex(ret_data);
	cpm_printstring(" address: ");
	print_hex(a);
	crlf();
#endif
	return ret_data;
}

static void set_var(uint8_t v, uint16_t val, uint8_t indirect)
{
	if (v == 0) {
		if (indirect) {
			if (sp > 0)
				stack[sp - 1] = val;
			else
				push(val);
		} else {
			push(val);
		}
	} else if (v < 16) {
		frames[fp - 1].locals[v - 1] = val;
	} else {
		uint16_t a = (hdr[HDR_GLB] << 8) + hdr[HDR_GLB + 1] + 2 * (v - 16);
		zm_write8(a, val >> 8);
		zm_write8(a + 1, val & 0xFF);
	}
}

/* ============================================================
 * Operand decoding
 * ============================================================
 */

enum { OP_LARGE, OP_SMALL, OP_VAR, OP_OMIT };

static uint16_t operands[MAX_OPERANDS];
static uint8_t operand_count;

static uint16_t read_operand(uint8_t t, uint8_t indirect)
{
	uint16_t value = 0;
	if (t == OP_LARGE) {
		pc += 2;
		value = zm_read16(pc - 2);
	}
	if (t == OP_SMALL)
		value = zm_read8(pc++);
	if (t == OP_VAR) {
		uint8_t var_num = zm_read8(pc++);
		if (indirect)
			value = var_num;
		else
			value = get_var(var_num, 0);
	}
	return value;
}

static void decode_operands(uint8_t spec, uint8_t indirect)
{
	operand_count = 0;
	for (int s = 6; s >= 0; s -= 2) {
		uint8_t t = (spec >> s) & 3;
		if (t == OP_OMIT)
			break;
		operands[operand_count++] = read_operand(t, indirect);
	}
}

/* ============================================================
 * Branching
 * ============================================================
 */

static void branch(uint8_t cond)
{
	uint8_t b = zm_read8(pc++);
	int16_t off;

	if (b & 0x40)
		off = b & 0x3F;
	else {
		off = ((b & 0x3F) << 8) | zm_read8(pc++);
		// Sign extension for 14-bit long offset
		if (off & 0x2000)
			off |= 0xC000;
	}

	uint8_t take_branch = 0;

	if (b & 0x80) {
		if (cond)
			take_branch = 1;
	} else {
		if (!cond)
			take_branch = 1;
	}

	if (take_branch) {
		if (off == 0) {
			z_ret(0);
		} else if (off == 1) {
			z_ret(1);
		} else
			pc += off - 2;
	}
}

/* ============================================================
 * ZSCII output
 * ============================================================
 */

static void print_zstring(uint32_t addr)
{
	while (1) {
		uint16_t w = zm_read16(addr);
		addr += 2;
		// print_hex(w);
		for (int i = 10; i >= 0; i -= 5) {
			uint8_t c = (w >> i) & 0x1F;

			if (abbrev_print == 1) {
				uint16_t string_addr;
				alphabet = 0;
				string_addr =
					zm_read16(abbrev_base + (((abbrev_table << 5) + c) << 1));
				abbrev_print = 0;
				print_zstring(string_addr << 1);
				alphabet = 0;
			} else if (zscii_esc == 1) {
				zscii_high = c;
				zscii_esc = 2;
			} else if (zscii_esc == 2) {
				putc((zscii_high << 5) | c);
				zscii_esc = 0;
			} else if (alphabet == 2 && c == 6) {
				// ZSCII escape
				zscii_esc = 1;
				alphabet = 0;
			} else if (c >= 6) {
				putc(zalph[alphabet][c - 6]);
				// Also print \r when \n is printed
                if(alphabet == 2 && c == 7)
                    putc('\r');
                alphabet = 0;
			} else if (c == 4) {
				alphabet = 1;
			} else if (c == 5) {
				alphabet = 2;
			} else if ((c > 0) && (c < 4)) {
				abbrev_print = 1;
				abbrev_table = c - 1;
			} else if (c == 0)
				putc(' ');
		}
		if (w & 0x8000) {
			alphabet = 0;
			break;
		}
	}
}

static void print_num(int16_t v)
{
	char buf[6];
	uint8_t i = 0;

	if (v < 0) {
		putc('-');
		v = -v;
	}
	printi(v);
}

/* ============================================================
 * Dictionary
 * ============================================================
 */


static void dict_init(void)
{
	dict_addr = hdr[HDR_DICT] << 8 | hdr[HDR_DICT + 1];
	dict_sep_count = zm_read8(dict_addr++);
	for (uint8_t i = 0; i < dict_sep_count; i++) {
		dict_seps[i] = zm_read8(dict_addr++);
	}

	dict_entry_len = zm_read8(dict_addr++);

	dict_entry_count = zm_read16(dict_addr);
}

static uint8_t encode_a2(char c)
{
	for (uint8_t i = 0; zalph[2][i]; i++) {
		if (zalph[2][i] == c)
			return i;
	}
	return 0;
}

void encode_zchars(const char *s, uint8_t zchars[6])
{
	uint8_t zi = 0;

	while (*s && zi < 6) {
		char c = *s++;

		if (c >= 'a' && c <= 'z') {
			zchars[zi++] = (c - 'a') + 6;
		} else {
			// shift to A2
			if (zi < 6)
				zchars[zi++] = 5;
			if (zi < 6)
				zchars[zi++] = encode_a2(c);
		}
	}

	// pad with 5
	while (zi < 6)
		zchars[zi++] = 5;
}

uint16_t dict_lookup(const char *word)
{
	uint8_t zchars[6];
	encode_zchars(word, zchars);

	uint16_t w0 = (zchars[0] << 10) | (zchars[1] << 5) | zchars[2];
	uint16_t w1 = (zchars[3] << 10) | (zchars[4] << 5) | zchars[5] | 0x8000;
	uint16_t e = dict_addr + 2;

	for (uint16_t i = 0; i < dict_entry_count; i++) {
		if (zm_read16(e) == w0 && zm_read16(e + 2) == w1) {
			return e;
		}

		e += dict_entry_len;
	}

	return 0;
}

/* ============================================================
 * Tokenization
 * ============================================================
 */

static uint8_t is_sep(char c)
{
	for (uint8_t i = 0; i < dict_sep_count; i++)
		if (c == dict_seps[i])
			return 1;
	return c == ' ';
}

static uint8_t tokenize(char *in, uint16_t parse_buf, uint16_t text_buf)
{
	uint8_t count = 0;
	uint8_t i = 0;

	uint8_t max_tok = zm_read8(parse_buf);

	if (max_tok > MAX_TOKENS)
		max_tok = MAX_TOKENS;

	while (in[i] && count < max_tok) {
		while (is_sep(in[i]))
			i++;
		if (!in[i])
			break;

		uint8_t start = i;
		char word[10] = { 0 };
		uint8_t dict_len = 0;
		uint8_t word_len = 0;

		while (in[i] && !is_sep(in[i])) {
			if (dict_len < 7)
				word[dict_len++] = in[i];
			word_len++;
			i++;
		}

		uint16_t dict = dict_lookup(word);

		uint16_t entry = parse_buf + 2 + (count << 2);
		zm_write8(entry, dict >> 8);
		zm_write8(entry + 1, dict & 0xff);
		zm_write8(entry + 2, word_len);
		zm_write8(entry + 3, start + 1); // 1);

		count++;
	}

	zm_write8(parse_buf + 1, count);
	return count;
}

/* ============================================================
 * Objects
 * ============================================================
 */

static uint16_t obj_addr(uint8_t obj)
{
	return object_table + 62 + (obj - 1) * 9;
}

static uint8_t obj_test_attr(uint8_t obj, uint8_t attr)
{
	uint16_t a = obj_addr(obj) + (attr >> 3);
	return zm_read8(a) & (0x80 >> (attr & 7));
}

static void obj_set_attr(uint8_t obj, uint8_t attr)
{
	uint16_t a = obj_addr(obj) + (attr >> 3);
	if (obj != 0)
		zm_write8(a, zm_read8(a) | (0x80 >> (attr & 7)));
}

static void obj_clear_attr(uint8_t obj, uint8_t attr)
{
	uint16_t a = obj_addr(obj) + (attr >> 3);
	zm_write8(a, zm_read8(a) & ~(0x80 >> (attr & 7)));
}

static uint8_t obj_parent(uint8_t obj)
{
	return zm_read8(obj_addr(obj) + 4);
}

static uint8_t obj_sibling(uint8_t obj)
{
	return zm_read8(obj_addr(obj) + 5);
}

static uint8_t obj_child(uint8_t obj)
{
	return zm_read8(obj_addr(obj) + 6);
}

static void obj_remove(uint8_t obj)
{
	if (!obj)
		return;
	uint8_t parent = obj_parent(obj);
	if (!parent)
		return;

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

static void obj_insert(uint8_t obj, uint8_t dest)
{
	if (obj == 0 || dest == 0)
		return;
	obj_remove(obj);

	uint16_t daddr = obj_addr(dest) + 6;
	zm_write8(obj_addr(obj) + 4, dest);
	zm_write8(obj_addr(obj) + 5, zm_read8(daddr));
	zm_write8(daddr, obj);
}

static uint16_t obj_prop_table(uint8_t obj)
{
	return zm_read16(obj_addr(obj) + 7);
}

static uint16_t obj_prop_start(uint8_t obj)
{
	uint16_t p = obj_prop_table(obj);
	return p + 1 + 2 * zm_read8(p);
}

static uint16_t obj_find_prop(uint8_t obj, uint8_t prop)
{
	uint16_t p = obj_prop_start(obj);

	while (1) {
		uint8_t h = zm_read8(p++);
		if (!h)
			return 0;

		uint8_t num = h & 0x1F;
		uint8_t len = (h >> 5) + 1;

		if (num == prop)
			return p;
		p += len;
	}
}

static uint16_t obj_get_prop(uint8_t obj, uint8_t prop)
{
	uint16_t p = obj_find_prop(obj, prop);
	if (!p) {
		return zm_read16(object_table + (prop - 1) * 2);
	}

	uint8_t len = ((zm_read8(p - 1) >> 5) & 7) + 1;
	if (len == 1)
		return zm_read8(p);
	return zm_read16(p);
}

static uint8_t obj_get_prop_len(uint16_t addr)
{
	if (!addr)
		return 0;
	return ((zm_read8(addr - 1) >> 5) & 7) + 1;
}

static void obj_put_prop(uint8_t obj, uint8_t prop, uint16_t val)
{
	uint16_t p = obj_find_prop(obj, prop);

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
 * Saving and Restoring
 * ============================================================
 */

void write_save_sector(FCB *fcb, void *dmaptr)
{
	cpm_set_dma(dmaptr);
	cpm_write_sequential(fcb);
}

void add_save_byte(FCB *fcb, uint8_t *writeSector, uint8_t data)
{
	writeSector[w_addr++] = data;
	if (w_addr == 0x80) {
		write_save_sector(fcb, writeSector);
		w_addr = 0;
	}
}

void write_zero_block(FCB *fcb, uint8_t *writeSector, uint8_t zero_cnt)
{
	add_save_byte(fcb, writeSector, 0x00);
	add_save_byte(fcb, writeSector, zero_cnt);
}

uint8_t save_game(void)
{
	FCB saveFile;
	char filename_input[14];

	filename_input[0] = 13;
	filename_input[1] = 0;

	cpm_printstring("Enter filename: ");
	cpm_readline((uint8_t *)filename_input);

	cpm_set_dma(&saveFile);
	if (!cpm_parse_filename(&filename_input[2])) {
		return 0;
	}

	if (cpm_make_file(&saveFile)) {
		return 0;
	}

	uint16_t enc = 0;

	// Write delta of dynamic ram to file
	uint16_t cmp_addr = 0;
	cpm_fcb.r = 0;
	uint8_t writeSector[128];
	w_addr = 0;
	uint8_t zero_cnt = 0;

	while (cmp_addr < dynamic_size) {
		// Read a sector from file
		uint8_t checkSector[128];
		cpm_set_dma(&checkSector);
		cpm_read_random(&cpm_fcb);
		cpm_fcb.r++;
		// Compare byte by byte with ram
		uint8_t next_len =
			(dynamic_size - cmp_addr) > 128 ? 128 : (dynamic_size - cmp_addr);
		for (uint8_t i = 0; i < next_len; i++) {
			uint8_t diff = dynamic_mem[cmp_addr] ^ checkSector[i];
			if (diff) {
				if (zero_cnt != 0) {
					write_zero_block(&saveFile, writeSector, zero_cnt);
					enc += zero_cnt;
					zero_cnt = 0;
				}
				add_save_byte(&saveFile, writeSector, diff);
				enc++;
			} else {
				zero_cnt++;
				if (zero_cnt == 0xff) {
					write_zero_block(&saveFile, writeSector, zero_cnt);
					enc += zero_cnt;
					zero_cnt = 0;
				}
			}
			cmp_addr++;
		}
	}
	// Write trailing zeroes
	if (zero_cnt != 0) {
		write_zero_block(&saveFile, writeSector, zero_cnt);
		enc += zero_cnt;
	}

	// Write frames
	add_save_byte(&saveFile, writeSector, fp);
	for (uint8_t i = 0; i < fp + 2; i++) {
		add_save_byte(&saveFile, writeSector, (frames[i].return_pc >> 24));
		add_save_byte(&saveFile, writeSector, (frames[i].return_pc >> 16));
		add_save_byte(&saveFile, writeSector, (frames[i].return_pc >> 8));
		add_save_byte(&saveFile, writeSector, (frames[i].return_pc & 0xFF));

		add_save_byte(&saveFile, writeSector, frames[i].store_var);

		add_save_byte(&saveFile, writeSector, frames[i].num_locals);
		for (uint8_t j = 0; j < MAX_LOCALS; j++) {
			add_save_byte(&saveFile, writeSector, (frames[i].locals[j] >> 8));
			add_save_byte(&saveFile, writeSector, (frames[i].locals[j] & 0xFF));
		}

		add_save_byte(&saveFile, writeSector, (frames[i].saved_sp >> 8));
		add_save_byte(&saveFile, writeSector, (frames[i].saved_sp & 0xFF));
	}
	// Write stack
	add_save_byte(&saveFile, writeSector, (sp >> 8));
	add_save_byte(&saveFile, writeSector, (sp & 0xFF));

	for (uint16_t i = 0; i < sp; i++) {
		add_save_byte(&saveFile, writeSector, (stack[i] >> 8));
		add_save_byte(&saveFile, writeSector, (stack[i] & 0xFF));
	}
	// Save PC
	add_save_byte(&saveFile, writeSector, (pc >> 24));
	add_save_byte(&saveFile, writeSector, (pc >> 16));
	add_save_byte(&saveFile, writeSector, (pc >> 8));
	add_save_byte(&saveFile, writeSector, (pc & 0xFF));

	// Perform a final write if necessary

	if (w_addr != 0) {
		write_save_sector(&saveFile, writeSector);
	}

	cpm_close_file(&saveFile);

	return 1;
}

uint8_t read_save_byte(FCB *fcb, uint8_t *restoreSector)
{
	uint8_t data;
	// Refill buffer if out of data
	if (w_addr == 0x80) {
		cpm_set_dma(restoreSector);
		cpm_read_random(fcb);
		fcb->r++;
		w_addr = 0;
	}

	data = restoreSector[w_addr++];
	return data;
}

uint8_t read_game_byte(uint8_t *r_addr)
{
	uint8_t data;
	// Refill buffer if out of data
	if ((*r_addr) == 0x80) {
		cpm_set_dma(&dma);
		cpm_read_random(&cpm_fcb);
		cpm_fcb.r++;
		(*r_addr) = 0;
	}

	data = dma[(*r_addr)];
	(*r_addr) = (*r_addr) + 1;
	return data;
}

uint8_t restore_game(void)
{
	FCB saveFile;
	char filename_input[14];

	filename_input[0] = 13;
	filename_input[1] = 0;

	cpm_printstring("Enter filename: ");
	cpm_readline((uint8_t *)filename_input);

	cpm_set_dma(&saveFile);
	if (!cpm_parse_filename(&filename_input[2])) {
		return 0;
	}

	saveFile.cr = 0;
	if (cpm_open_file(&saveFile)) {
		return 0;
	}
	saveFile.r = 0;

	// Restore dynamic ram from file
	uint16_t cmp_addr = 0;

	cpm_fcb.r = 0;
	uint8_t restoreSector[128];
	uint8_t gameFileSector[128];
	// uint8_t zero_cnt;

	w_addr = 0x80; // trigger sector read
	uint8_t r_addr = 0x80;
	uint8_t gameFileAddr = 0x80;

	// Restore dynamic memory
	while (cmp_addr < dynamic_size) {
		uint8_t data = read_save_byte(&saveFile, restoreSector);

		if (data == 0x00) {
			uint8_t zero_cnt = read_save_byte(&saveFile, restoreSector);

			for (uint8_t i = 0; i < zero_cnt; i++) {
				dynamic_mem[cmp_addr++] = read_game_byte(&gameFileAddr);
			}
		} else {
			dynamic_mem[cmp_addr++] = read_game_byte(&gameFileAddr) ^ data;
		}
	}

	// Restore frames
	fp = read_save_byte(&saveFile, restoreSector); // Frame pointer
	for (uint8_t i = 0; i < fp + 2; i++) {
		frames[i].return_pc =
			((uint32_t)read_save_byte(&saveFile, restoreSector) << 24) |
			((uint32_t)read_save_byte(&saveFile, restoreSector) << 16) |
			((uint32_t)read_save_byte(&saveFile, restoreSector) << 8) |
			(uint32_t)read_save_byte(&saveFile, restoreSector);
		frames[i].store_var = read_save_byte(&saveFile, restoreSector);
		frames[i].num_locals = read_save_byte(&saveFile, restoreSector);
		for (uint8_t j = 0; j < MAX_LOCALS; j++) {
			frames[i].locals[j] =
				(read_save_byte(&saveFile, restoreSector) << 8) |
				read_save_byte(&saveFile, restoreSector);
		}
		frames[i].saved_sp = (read_save_byte(&saveFile, restoreSector) << 8) |
							 read_save_byte(&saveFile, restoreSector);
	}
	// Restore stack
	sp = (read_save_byte(&saveFile, restoreSector) << 8) |
		 read_save_byte(&saveFile, restoreSector);
	for (uint16_t i = 0; i < sp; i++) {
		stack[i] = (read_save_byte(&saveFile, restoreSector) << 8) |
				   read_save_byte(&saveFile, restoreSector);
	}
	// Restore PC
	pc = ((uint32_t)read_save_byte(&saveFile, restoreSector) << 24) |
		 ((uint32_t)read_save_byte(&saveFile, restoreSector) << 16) |
		 ((uint32_t)read_save_byte(&saveFile, restoreSector) << 8) |
		 (uint32_t)read_save_byte(&saveFile, restoreSector);

	cpm_close_file(&saveFile);
	return 1;
}

/* ============================================================
 * Execution
 * ============================================================
 */

static void z_ret(uint16_t v)
{
	CallFrame *f = &frames[--fp];
	pc = f->return_pc;
	sp = f->saved_sp;
	if (f->store_var == 0) {
		push(v);
	} else
		set_var(f->store_var, v, 0);
}

static void store_result(uint16_t v, uint8_t indirect)
{
	uint8_t var = zm_read8(pc++);
	set_var(var, v, indirect);
}

static void restart(void)
{
	pc = initial_pc;
	sp = fp = 0;
}

static uint16_t rng_next(void)
{
	uint16_t x = rng_state;

	x ^= x << 7;
	x ^= x >> 9;
	x ^= x << 8;

	rng_state = x & 0x7fff;

	return rng_state;
}

static void unimplemented(uint8_t opcode)
{
	crlf();
	printi(opcode);
#if DEBUG
	spc();
    print_hex(pc);
#endif
	fatal(" Non-implemented opcode");
}

#if STATUS
static void update_status(void) {
    if(!status) return;

    uint8_t save_x, save_y;
    screen_getcursor(&save_x, &save_y);
    screen_setcursor(0, 0);
    screen_setstyle(1);

    // Score/turn mode
    uint16_t room = get_var(16,0);
    uint16_t v1 = get_var(17,0);
    uint16_t v2 = get_var(18,0);
   	
    if (room) {
		uint16_t entry = obj_addr(room);
		uint16_t prop_table = zm_read16(entry + 7);
		uint8_t name_len = zm_read8(prop_table);
		if (name_len != 0)
			print_zstring(prop_table + 1);
    }
    spc();
    if((hdr[HDR_FLAGS] & 0x02) == 0) {
        // Score/turn mode
        cpm_printstring("Score: ");
        printi(v1);
        cpm_printstring(" Turns: ");
        printi(v2);
    } else {
        // Time mode
        if(v1<10)
            putc('0');
        printi(v1);
        putc(':');
        if(v2<10)
            putc('0');
        printi(v2);
    }
    uint8_t xsize, ysize,x,y;
    screen_getsize(&xsize, &ysize);
    screen_getcursor(&x,&y);
    while(x<=xsize) {
        putc(' ');
        x++;
    }
    screen_setstyle(0);
    screen_setcursor(save_x, save_y);
 
}
#endif

static void step(void)
{
	uint8_t op = zm_read8(pc++);

#if DEBUG
	printi(opcnt++);
	spc();
	cpm_printstring("OP: ");
	print_hex(op);
	cpm_printstring(" PC: ");
	print_hex_32(pc);
	cpm_printstring(" SP: ");
	print_hex(sp);
	cpm_printstring(" FP: ");
	print_hex(fp);
	cpm_printstring(" [SP]: ");
	print_hex(stack[sp - 1]);
	crlf();
#endif

	// -------- 2OP -------- 
	if (op < 0x80) {
		uint8_t t1 = (op & 0x40) ? OP_VAR : OP_SMALL;
		uint8_t t2 = (op & 0x20) ? OP_VAR : OP_SMALL;
		uint8_t oc = op & 0x1f;
		if (oc == OP2_INC_CHK || oc == OP2_DEC_CHK)
			t1 = OP_VAR;
		uint8_t var_num = zm_read8(pc);
		uint8_t indirect = ((oc == OP2_INC_CHK && t1 == OP_VAR) ||
							(oc == OP2_DEC_CHK && t1 == OP_VAR));
		uint8_t d_indirect =
			!(op == 0x45 || op == 0x65 || op == 0x44 || op == 0x64);
		operands[0] = read_operand(t1, indirect & d_indirect);
		operands[1] = read_operand(t2, 0);
		switch (oc) {
			case OP2_JE:
				branch(operands[0] == operands[1]);
				break;
			case OP2_JL:
				branch((int16_t)operands[0] < (int16_t)operands[1]);
				break;
			case OP2_JG:
				branch((int16_t)operands[0] > (int16_t)operands[1]);
				break;
			case OP2_JIN: {
				uint8_t parent;
				parent = obj_parent(operands[0]);
				branch(parent == operands[1]);
				break;
			}
			case OP2_DEC_CHK: {
				int16_t value = (int16_t)get_var(operands[0], 1);
				value--;
				set_var(operands[0], (uint16_t)value, 1);
				branch(value < (int16_t)operands[1]);
				break;
			}
			case OP2_INC_CHK: {
				int16_t value = (int16_t)get_var(operands[0], 1);
				value++;
				set_var(operands[0], value, 1);
				branch((int16_t)value > (int16_t)operands[1]);
				break;
			}
			case OP2_TEST:
				branch((operands[0] & operands[1]) == operands[1]);
				break;
			case OP2_OR:
				store_result(operands[0] | operands[1], indirect);
				break;
			case OP2_AND:
				store_result(operands[0] & operands[1], indirect);
				break;
			case OP2_STORE:
				set_var(operands[0], operands[1], 1);
				break;
			case OP2_LOADW:
				store_result(zm_read16(operands[0] + 2 * operands[1]),
							 indirect);
				break;
			case OP2_LOADB:
				store_result(zm_read8(operands[0] + operands[1]), indirect);
				break;
			case OP2_ADD:
				store_result((int16_t)operands[0] + (int16_t)operands[1],
							 indirect);
				break;
			case OP2_SUB:
				store_result((int16_t)operands[0] - (int16_t)operands[1],
							 indirect);
				break;
			case OP2_MUL:
				store_result((int16_t)operands[0] * (int16_t)operands[1],
							 indirect);
				break;
			case OP2_DIV:
				store_result((int16_t)operands[0] / (int16_t)operands[1],
							 indirect);
				break;
			case OP2_MOD:
				store_result((int16_t)operands[0] % (int16_t)operands[1],
							 indirect);
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
				store_result(obj_get_prop(operands[0], operands[1]), indirect);
				break;

			case OP2_GET_PROP_ADDR: {
				uint16_t p = obj_find_prop(operands[0], operands[1]);
				store_result(p, indirect);
				break;
			}

			case OP2_GET_NEXT_PROP: {
				uint8_t obj = (uint8_t)operands[0];
				uint8_t prop = (uint8_t)operands[1];
				uint16_t addr;

				if (prop == 0) {
					addr = obj_prop_start(obj);
				} else {
					addr = obj_find_prop(obj, prop);
					if (!addr) {
						store_result(0, indirect);
						break;
					}
					addr += obj_get_prop_len(addr);
				}

				uint8_t next_header = zm_read8(addr);

				if (next_header == 0)
					store_result(0, indirect);
				else
					store_result(next_header & 0x1F, indirect);

				break;
			}

			default:
#if DEBUG
				crlf();
				printi(op);
				spc();
				print_hex(pc);
				spc();
#endif
				fatal(" - Non-implemented opcode!!!");
		}
		return;
	}

	// -------- 1OP --------
	if (op < 0xB0) {
		uint8_t type = (op >> 4) & 3;
		uint8_t oc = op & 0x0F;
		uint8_t indirect = (oc == OP1_LOAD);
		if (type != OP_OMIT)
			operands[0] = read_operand(type, 0);
		switch (oc) {
			case OP1_JUMP:
				pc += (int16_t)operands[0] - 2;
				break;
			case OP1_PRINT_ADDR:
				print_zstring(operands[0]);
				break;
			case OP1_PRINT_OBJ: {
				uint16_t obj = operands[0];

				if (obj == 0)
					break;

				uint16_t entry = obj_addr(obj);
				uint16_t prop_table = zm_read16(entry + 7);
				uint8_t name_len = zm_read8(prop_table);
				if (name_len == 0)
					break;
				print_zstring(prop_table + 1);
				break;
			}
			case OP1_REMOVE_OBJ:
				obj_remove(operands[0]);
				break;
			case OP1_GET_CHILD: {
				uint8_t result;
				if (operands[0] == 0)
					result = 0;
				else
					result = obj_child(operands[0]);
				store_result(result, indirect);
				branch(result != 0);
				break;
			}
			case OP1_GET_SIBLING: {
				uint8_t result;
				if (operands[0] == 0)
					result = 0;
				else
					result = obj_sibling(operands[0]);
				store_result(result, indirect);
				branch(result != 0);
				break;
			}

			case OP1_GET_PARENT: {
				uint8_t result;
				if (operands[0] == 0)
					result = 0;
				else
					result = obj_parent(operands[0]);
				store_result(result, indirect);
				break;
			}
			case OP1_GET_PROP_LEN: {
				uint8_t result;
				if (operands[0] == 0)
					result = 0;
				else
					result = obj_get_prop_len(operands[0]);
				store_result(result, indirect);
				break;
			}
			case OP1_LOAD: {
				uint16_t result;
				result = get_var(operands[0], indirect);
				store_result(result, 0);
				break;
			}
			case OP1_NOT: {
				uint16_t result;
				result = ~operands[0];
				store_result(result, indirect);
				break;
			}
			case OP1_RET:
				z_ret(operands[0]);
				break;
			case OP1_INC: {
				uint16_t value = get_var(operands[0], indirect);
				value++;
				set_var(operands[0], value, indirect);
				break;
			}
			case OP1_DEC: {
				uint16_t value = get_var(operands[0], indirect);
				value--;
				set_var(operands[0], value, indirect);
				break;
			}
			case OP1_JZ:
				branch(operands[0] == 0);
				break;
			case OP1_PRINT_PADDR: {
				uint32_t addr = (uint32_t)operands[0] << 1;
				print_zstring(addr);
				break;
			}
			default:
				unimplemented(op);
		}
		return;
	}
	// -------- 0OP -------- 
	if (op < 0xC0) {
		switch (op & 0x0F) {
			case OP0_RTRUE:
				z_ret(1);
				break;
			case OP0_RFALSE:
				z_ret(0);
				break;
			case OP0_PRINT:
				print_zstring(pc);
				while (!(zm_read16(pc) & 0x8000))
					pc += 2;
				pc += 2;
				break;
			case OP0_PRINT_RET:
				print_zstring(pc);
				while (!(zm_read16(pc) & 0x8000))
					pc += 2;
				pc += 2;
				crlf();
				z_ret(1);
				break;

			case OP0_NOP:
				break;

			case OP0_SAVE:
				branch(save_game());
				break;

			case OP0_RESTORE:
				branch(restore_game());
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
            case OP0_SHOW_STATUS:
#if STATUS
                update_status();
#endif
                break;
			case OP0_QUIT:
				cpm_warmboot();
				break;
			case OP0_VERIFY:
				// Not supported, just assume it's OK
				branch(1);
				break;
			default:
				unimplemented(op);
		}
		return;
	}
	// -------- VAR --------
	uint8_t var_opcode = op & 0x1f;
	uint8_t indirect = (var_opcode == OPV_PULL);

	decode_operands(zm_read8(pc++), indirect);
	if ((op & 0xE0) == 0xE0)
		var_opcode += 0x20;
	if ((op & 0xD0) == 0xD0)
		var_opcode += 0x40;
	switch (var_opcode) {
		case OPV_CALL_EXT:
		case OPV_CALL: {
			if (operands[0] == 0) {
				uint8_t sv = zm_read8(pc++);
				set_var(sv, 0, indirect);
				return;
			}

			CallFrame *f = &frames[fp++];

			uint8_t sv = zm_read8(pc++);
			f->store_var = sv;
			f->return_pc = pc;
			f->saved_sp = sp;

			uint32_t addr = (uint32_t)operands[0] << 1;
			f->num_locals = zm_read8(addr++);

			for (uint8_t i = 0; i < f->num_locals; i++) {
				f->locals[i] = zm_read16(addr);
				addr += 2;
			}

			/* overwrite locals with arguments */
			uint8_t argc = operand_count - 1;
			if (argc > f->num_locals)
				argc = f->num_locals;
			for (uint8_t i = 0; i < argc; i++) {
				f->locals[i] = operands[i + 1];
			}

			pc = addr;
			break;
		}
		case OPV_JE:
			if (operand_count == 2)
				branch(operands[0] == operands[1]);
			else if (operand_count == 3)
				branch(operands[0] == operands[1] ||
					   operands[0] == operands[2]);
			else if (operand_count == 4)
				branch(operands[0] == operands[1] ||
					   operands[0] == operands[2] ||
					   operands[0] == operands[3]);
			break;
		case OPV_JL:
			branch((int16_t)operands[0] < (int16_t)operands[1]);
			break;
		case OPV_JG:
			branch((int16_t)operands[0] > (int16_t)operands[1]);
			break;
		case OPV_TEST:
			branch((operands[0] & operands[1]) == operands[1]);
			break;
		case OPV_STOREW:
			zm_write8(operands[0] + 2 * operands[1], operands[2] >> 8);
			zm_write8(operands[0] + 2 * operands[1] + 1, operands[2]);
			break;
		case OPV_STOREB:
			zm_write8(operands[0] + operands[1], operands[2]);
			break;
		case OPV_STORE:
			set_var(operands[0], operands[1], 0);
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
		case OPV_PULL: {
			uint16_t value = pop();
			set_var(operands[0], value, 1);
			break;
		}
		case OPV_SREAD: {
			uint16_t text = operands[0];
			uint16_t parse = operands[1];
			char line[INPUT_MAX] = { 0 };
#if STATUS
            update_status();
#endif
            uint8_t len = read_line(line);
			uint8_t max = zm_read8(text);
			if (len > max)
				len = max;

			for (uint8_t i = 0; i < len; i++)
				zm_write8(text + 1 + i, line[i]);

			zm_write8(text + 1 + len, 0);
#if STATUS
            update_status();
#endif
			tokenize(line, parse, text);
			break;
		}
		case OPV_PUT_PROP:
			obj_put_prop((uint8_t)operands[0], (uint8_t)operands[1],
						 operands[2]);
			break;
		case OPV_OR:
			store_result(operands[0] | operands[1], indirect);
			break;
		case OPV_AND:
			store_result(operands[0] & operands[1], indirect);
			break;
		case OPV_RANDOM: {
			int16_t range = (int16_t)operands[0];
			if (range == 0) {
				rng_state = (uint16_t)pc;
				store_result(0, indirect);
				break;
			}
			if (range < 0) {
				rng_state = (uint16_t)(-range);
				store_result(0, indirect);
				break;
			}

			uint16_t r = rng_next();
			uint16_t result = (r % range) + 1;
			store_result(result, indirect);
			break;
		}
		case OPV_LOADW:
			store_result(zm_read16(operands[0] + 2 * operands[1]), indirect);
			break;
		case OPV_LOADB:
			store_result(zm_read8(operands[0] + operands[1]), indirect);
			break;
		case OPV_MUL:
			store_result((int16_t)operands[0] * (int16_t)operands[1], indirect);
			break;
		case OPV_ADD:
			store_result((int16_t)operands[0] + (int16_t)operands[1], indirect);
			break;
		case OPV_SUB:
			store_result((int16_t)operands[0] - (int16_t)operands[1], indirect);
			break;
		case OPV_INC_CHK: {
			int16_t value = (int16_t)get_var(operands[0], 1);
			value++;
			set_var(operands[0], (uint16_t)value, 1);
			branch(value > (int16_t)operands[1]);
			break;
		}
		case OPV_DEC_CHK: {
			int16_t value = (int16_t)get_var(operands[0], 1);
			value--;
			set_var(operands[0], (uint16_t)value, 1);
			branch(value < (int16_t)operands[1]);
			break;
		}
		case OPV_INSERT_OBJ:
			obj_insert(operands[0], operands[1]);
			break;

        case OPV_JIN: {
			uint8_t parent;
			parent = obj_parent(operands[0]);
			branch(parent == operands[1]);
			break;
		}   
        case OPV_TEST_ATTR:
			branch(obj_test_attr(operands[0], operands[1]));
			break;
        case OPV_SET_ATTR:
			obj_set_attr(operands[0], operands[1]);
			break;

		default:
			unimplemented(op);
	}
}

/* ============================================================
 * Main
 * ============================================================
 */

int main(int agrv, char **argv)
{
#if STATUS
    // Initate screen driver for status bar if available
    if(screen_init()) {
        status = 1;
        screen_clear();
        screen_setcursor(0,1);
    }
#endif

    cpm_printstring("z65 - Z-machine v3 interpreter");
	crlf();
	crlf();

	// Read story file (filename from commandline)
	cpm_fcb.cr = 0;
	if (cpm_open_file(&cpm_fcb))
		fatal("Could not open input file!");

	cpm_set_dma(&hdr);
	cpm_read_sequential(&cpm_fcb);

	if (hdr[HDR_VERSION] != 3)
		fatal("Only v3 files are supported!");

	// Allocate dynamic memory
	dynamic_size = (hdr[HDR_STAT] << 8) | hdr[HDR_STAT + 1];

	uint16_t tpa = cpm_bios_gettpa();
	uint8_t *top = (uint8_t *)(tpa & 0xff00);
	zmalloc_init(cpm_ram, top - cpm_ram);
	dynamic_mem = zmalloc(dynamic_size);

	// Fail if dynamic memory does not fit in RAM
	if (!dynamic_mem)
		fatal("Out of memory");

	// Read dynamic memory
	for (uint16_t i = 0; i < 128; i++)
		dynamic_mem[i] = hdr[i];

	for (uint16_t i = 128; i < dynamic_size; i++) {
		if ((i & 0x7f) == 0) {
			cpm_set_dma(&dma);
			cpm_read_sequential(&cpm_fcb);
		}
		dynamic_mem[i] = dma[i & 0x7f];
	}

	// Initiate Z-machine
	pc = initial_pc = (hdr[HDR_PC] << 8) | hdr[HDR_PC + 1];
	sp = fp = next_victim = 0;
	for (uint8_t i = 0; i < NUM_PAGES; i++)
		page_cache[i].valid = 0;

	dict_init();
	object_table = (hdr[HDR_OBJ] << 8) | hdr[HDR_OBJ + 1];

	alphabet = 0;
	abbrev_print = 0;
	abbrev_table = 0;
	abbrev_base = (hdr[HDR_ABBR] << 8) | hdr[HDR_ABBR + 1];

    // Start game execution
	for (;;)
		step();
}
