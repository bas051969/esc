#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#define ASMDEBUG 0
#define LIBDEBUG 0


// cog register

typedef struct cogreg
{
	const char* name;
} cogreg;


typedef struct opcode opcode;
typedef struct line line;



// symbol

typedef enum symboltype
{
	SYM_UNKNOWN,
	SYM_LABEL,
	SYM_OPCODE,
	SYM_MODIFIER,
} symboltype;

typedef struct symbol
{
	union
	{
		line* line;
		opcode* op;
		//?? mod;
	} data;
	const char* name;
	symboltype type;
	int defined;
} symbol;

// instruction

typedef struct operand operand;

typedef struct instruction
{
	operand* operands;

	cogreg* reg;

	//struct* instruction next;
} instruction;

// line

typedef struct line
{
	operand* operand;
} line;

// opcode

typedef void (*opcode_func)(symbol* op, instruction* instr);

typedef struct opcode
{
	opcode_func func;
	void* data;
	const char* name;
} opcode;

// symbol table

typedef union symtabentry
{
	union symtabentry* next; // pointer to array of 256 symtabentries
	symbol* sym;
} symtabentry;

symtabentry symbols[256];


symbol* symbol_get(const char* p)
{
	const char* s2 = p;

	symtabentry* symtab = symbols;

	//printf("symbol_get(\"%s\"): ", p);
	while (*p != 0)
	{
#if LIBDEBUG
		putchar(*p);
#endif
		char c = tolower(*p);

		symtabentry* symtab2 = symtab[c].next;
		if (symtab2 == NULL)
		{
			symtab2 = symtab[c].next = calloc(256,sizeof(symtabentry));
		}
		symtab = symtab2;
		p++;
	}
#if LIBDEBUG
	putchar('\n');
#endif
	if (symtab[0].sym == NULL)
	{
		symbol* sym = malloc(sizeof(symbol));
		sym->type = SYM_UNKNOWN;
		sym->name = s2;
		sym->defined = 0;

		symtab[0].sym = sym;

		return sym;
	}

	return symtab[0].sym;
}

inline symbol* symbol_define(const char* s, enum symboltype type)
{
	symbol* sym = symbol_get(s);

	if (sym->defined)
	{
		printf("symbol %s already defined as %s!\n", s, sym->type == SYM_LABEL ? "label" : sym->type == SYM_OPCODE ? "opcode" : "unknown");
		return NULL;
	}

	sym->defined = 1;
	sym->type = type;

	return sym;
}



symbol* label_new(const char* s, line* l)
{
	symbol* sym = symbol_define(s, SYM_LABEL);

	if (sym == NULL) return NULL;

	sym->data.line = l;

	return sym;
}

symbol* mod_new(const char* s, const char* bits)
{
	symbol* sym = symbol_define(s, SYM_MODIFIER);

	if (sym == NULL) return NULL;

	/*
	opcode* op = malloc(sizeof(opcode));

	op->name = s;

	sym->data.op = op;
	*/

	return sym;
}

symbol* opcode_new(const char* s, const char* bits)
{
	symbol* sym = symbol_define(s, SYM_OPCODE);

	if (sym == NULL) return NULL;

	opcode* op = malloc(sizeof(opcode));

	op->name = s;

	sym->data.op = op;

	return sym;
}

// expression
typedef struct operand
{
	enum {INT, IDENT, REF, BINOP} tp;
	union
	{
		plong val;
		symbol* ident;
		line* line;
		struct
		{
			struct operand* operands[2];
			char op;
		} binop;
	} val;
} operand;

operand* int_new(plong x)
{
	operand* this = malloc(sizeof(operand));

	this->tp = INT;
	this->val.val = x;

	return this;
}

operand* ref_new(line* l)
{
	operand* this = malloc(sizeof(operand));

	this->tp = REF;
	this->val.line = l;

	return this;
}

operand* ident_new(char* p)
{
	operand* this = malloc(sizeof(operand));

	this->tp = IDENT;
	this->val.ident = symbol_get(p);

	return this;
}

operand* binop_new(char op, operand* lhs, operand* rhs)
{
	operand* this = malloc(sizeof(operand));

	this->tp = BINOP;
	this->val.binop.op = op;
	this->val.binop.operands[0] = lhs;
	this->val.binop.operands[1] = rhs;

	return this;
}

/*
int operand_eval(operand* this)
{
	if (this == NULL)
	{
		return 0;
	}

#if LIBDEBUG
	printf("operand_eval: ");
#endif

	switch (this->tp)
	{
		case INT:
		{
#if LIBDEBUG
			printf("int: %d\n", this->val.val);
#endif
			return this->val.val;
		}
		case IDENT:
		{
#if LIBDEBUG
			printf("label: %s\n", this->val.ident);
#endif
			return this->val.ident->data.reg->caddr;
		}
		case BINOP:
		{
#if LIBDEBUG
			printf("binop: %c\n", this->val.binop.op);
#endif
			int lhs = operand_eval(this->val.binop.operands[0]);
			int rhs = operand_eval(this->val.binop.operands[1]);
			switch (this->val.binop.op)
			{
				case '+':
				{
					return lhs + rhs;
				}
				case '-':
				{
					return lhs - rhs;
				}
				case '*':
				{
					return lhs * rhs;
				}
				case '/':
				{
					return lhs / rhs;
				}
				case 'n':
				{
					return -lhs;
				}
			}
			printf("Error: unknown op: %c\n", this->val.binop.op);
			exit(1);
			return 0;
		}
	}
	printf("Error: unknown value type: %d\n", this->tp);
	exit(1);
}
*/

void operand_print(operand* this)
{
	if (this == NULL)
	{
		printf("NULL");
		return;
	}

	switch (this->tp)
	{
		case INT:
		{
			printf("%d", this->val.val);
			break;
		}
		case IDENT:
		{
			symbol* sym = this->val.ident;

			switch (sym->type)
			{
				case SYM_LABEL:
				{
					printf("(label %s)", sym->name);
					break;
				}
				case SYM_OPCODE:
				{
					printf("(opcode %s)", sym->name);
					break;
				}
				case SYM_MODIFIER:
				{
					printf("(modifier %s)", sym->name);
					break;
				}
				default:
				{
					printf("(??? %s)", sym->name);
					break;
				}
			}
			break;
		}
		case BINOP:
		{
			printf("('%c'", this->val.binop.op);
			if (this->val.binop.operands[0] != NULL)
			{
				printf(" ");
				operand_print(this->val.binop.operands[0]);
			}
			if (this->val.binop.operands[1] != NULL)
			{
				printf(" ");
				operand_print(this->val.binop.operands[1]);
			}
			printf(")");
			break;
		}
		case REF:
		{
			//printf("[%s]", this->val.line->name);
			printf("[line]");
			break;
		}
		default:
		{
			printf("? type=%d ?", this->tp);
			break;
		}
	}
}

void operand_kill(operand* this)
{
	if (this == NULL)
	{
		return;
	}

	switch (this->tp)
	{
		case INT:
		{
			break;
		}
		case IDENT:
		{
			free(this->val.ident);
			break;
		}
		case BINOP:
		{
			operand_kill(this->val.binop.operands[0]);
			operand_kill(this->val.binop.operands[1]);
			break;
		}
		default:
		{
			printf("Error: unknown value type: %d\n", this->tp);
			//exit(1); // who cares about memory leaks? :)
			break;
		}
	}

	free(this);
}



// stack

typedef struct stack
{
	void** base;
	int top;
	int len;
} stack;

stack* stack_new()
{
	stack* this = malloc(sizeof(stack));

	this->len = 16;
	this->base = malloc(this->len*sizeof(void*));
	this->top = 0;

	return this;
}

void stack_push(stack* this, void* v)
{
	this->top++;

	if (this->top >= this->len)
	{
		this->len *= 2;
		this->base = realloc(this->base, this->len*sizeof(void*));
#if LIBDEBUG
		printf("resize stack to %d\n", this->len);
#endif
	}

	this->base[this->top] = v;
}

void* stack_pop(stack* this)
{
	if (this->top <= 0)
	{
		return NULL;
	}

	return this->base[this->top--];
}

void* stack_peek(stack* this)
{
	if (this->top <= 0)
	{
		return NULL;
	}

	return this->base[this->top];
}
	

// parser

stack* vstack;

stack* ostack;

char* tok2stra(char* base, char* start, char* end)
{
	size_t baselen = strlen(base);

	char* s = malloc(baselen + end - start + 1);

	strncpy(s, base, baselen);

	strncpy(&s[baselen], start, end - start);

	s[baselen + end-start] = 0;

	return s;
}

char* tok2str(char* start, char* end)
{
	char* s = malloc(end - start + 1);

	strncpy(s, start, end - start);

	s[end-start] = 0;

	return s;
}

int precedence(char op)
{
	switch (op)
	{
		case 'n' : return 7;
		case '/' : return 6;
		case '*' : return 5;
		case '-' : return 4;
		case '+' : return 3;
		case '#' : return 2;
		case ',' : return 1;
		case ' ' : return 0;
		case '(' : return INT_MIN;
		case ')' : return INT_MIN;
		default  : return INT_MIN;
	}
}

void fold(char nextop)
{
#if ASMDEBUG
	printf("fold: nextop = %c (0x%x)\n", nextop, nextop);
#endif

	char topop;

	while (stack_peek(ostack) != NULL && precedence(nextop) < precedence(topop = *(char*)stack_peek(ostack)))
	{
		char op = *(char*)stack_pop(ostack);
#if ASMDEBUG
		printf("fold: op %c (0x%x)\n", op, op);
#endif
		operand* rhs = (op != 'n' && op != '#') ? stack_pop(vstack) : NULL;
		operand* lhs = stack_pop(vstack);
		stack_push(vstack, binop_new(op, lhs, rhs));
	}

	if (nextop == 0 && stack_peek(ostack) != NULL && *(char*)stack_peek(ostack) == '(')
	{
		printf("Error: unmatched left parentheses\n");
		exit(1);
	}
}

unsigned int lineno = 0;

void eatblockcomment(char** pp)
{
#if ASMDEBUG >= 2
	printf("block comment on line %d: ", lineno);
#endif

	int level = 0;

	char* p = *pp;

	while (level > 0 || *p == '{')
	{
		switch(*p++)
		{
			case 0   : goto end;
			case '{' : level++; break;
			case '}' : --level; break;
			case '\n': lineno++;
		}
	}

end:
#if ASMDEBUG >= 2
	printf("ate %ld chars\n", p - *pp);
#endif

	*pp = p;
}

char islabelchar[256];

stack* parser(char* p)
{
	char* ps = p;

	char* ts = p;

	//char* s;

	int pc = 5;

	lineno = 1;
	char* linestart = p;

#if ASMDEBUG >= 2
	printf("Line %d\n", lineno);
#endif

	char* lastgloballabel = "!begin";

	stack* lines = stack_new();

	line* currline = malloc(sizeof(line));

	goto line;

newline:
#if ASMDEBUG
	printf("\\n\n");
#endif

	lineno++;
	linestart = p;

#if ASMDEBUG >= 2
	printf("Line %d\n", lineno);
#endif

	if (stack_peek(vstack) != NULL)
	{
		currline->operand = stack_pop(vstack);

#if 1
		operand_print(currline->operand);
		printf("\n");
#endif

		stack_push(lines, currline);

		currline = malloc(sizeof(line));
	}

	goto line;

line:
#if ASMDEBUG >= 2
	//printf("line\n", *p);
	printf("line: %c\n", *p);
#endif
	switch (*p)
	{
		case 0   : goto end;
		case '\'': p++; goto comment;
		case '{' : eatblockcomment(&p); goto line;
		case '\n':
		case '\r': lineno++; linestart=p+1; 
		case ' ' :
		case '\t': p++; goto line;
	}

	goto expr_entry;

comment:
#if ASMDEBUG >= 2
	printf("comment: %c\n", *p);
#endif
	switch (*p)
	{
		case 0   : goto end;
		case '\n':
		case '\r': p++; goto newline;
	}

	p++; goto comment;

expr_entry:
#if ASMDEBUG
	//printf("opcode\n");
#endif
	{
		char* stray = (char*)stack_peek(ostack);
		if (stray != NULL)
		{
			printf("Error at %d:%ld: stray operator on operator stack: %c\n", lineno, p-linestart, *stray);
			exit(1);
		}
	}
	{
		operand* stray = stack_peek(vstack);
		if (stray != NULL)
		{
			printf("Error at %d:%ld: stray value on value stack: ", lineno, p-linestart);
			operand_print(stray);
			printf("\n");
			exit(1);
		}
	}

expr:
#if ASMDEBUG >= 2
	printf("expr: %c\n", *p);
#endif
	switch (*p)
	{
		case 0   : goto end;
		case ' ' :
		case '\t': p++; goto expr;
		case '$' : goto here_or_hex;
		case '-' : goto unm;
		case '#' : goto indirect;
		case '(' : goto lparen;
		case '%' : p++; goto binnum;
		case '{' : eatblockcomment(&p); goto expr;
	}

	if (*p >= '0' && *p <= '9') goto decnum;
	if (islabelchar[*p] >= 3) goto ident;

	if ((*p == '\n' || *p == '\r') && stack_peek(vstack) == NULL)
	{
		goto newline;
	}

	goto error;

lparen:
#if ASMDEBUG
	printf("lparen: %c\n", *p);
#endif
	stack_push(ostack, "(");
	p++;

	goto expr;

	int nbase;

decnum:
	nbase = 10;
	goto num;

hexnum:
	nbase = 16;
	goto num;

binnum:
	if (*p == '%')
	{
		nbase = 4;
	}
	else
	{
		nbase = 2;
	}
num:
	ts = p;
num_l:
#if ASMDEBUG >= 2
	printf("num: %c\n", *p);
#endif
	switch (*p)
	{
		case 0   : goto error;
	}

	if ((*p >= '0' && *p <= '9') || *p == '_') { p++; goto num_l; }

	char* s = tok2str(ts, p);
#if ASMDEBUG
	printf("num: %s\n", s);
#endif
	stack_push(vstack, int_new(atoi(s)));
	free(s);

	goto operator;

ident:
	ts = p;
ident_l:
#if ASMDEBUG >= 2
	printf("ident: %c\n", *p);
#endif
	switch (*p)
	{
		case 0   : goto error;
	}
	if (islabelchar[*p] >= 2) { p++; goto ident_l; }

	char* base = "";

	if (*ts == ':')
	{
		base = lastgloballabel;
	}

	s = tok2stra(base, ts, p);

#if ASMDEBUG
	printf("ident: %s\n", s);
#endif
	operand* op = ident_new(s);

	if (stack_peek(vstack) == NULL)
	{
		symbol* sym = op->val.ident;
		if (sym->type == SYM_UNKNOWN)
		{
			if (*ts != ':')
			{
				lastgloballabel = s;
			}

			sym->type = SYM_LABEL;
		}
		else if (sym->type == SYM_LABEL)
		{
			printf("Duplicate symbol \"%s\"!\n", s);
			exit(1);
		}
	}

	stack_push(vstack, op);

	goto operator;

unm:
#if ASMDEBUG
	printf("unm: %c\n", *p);
#endif
	stack_push(ostack, "n");
	p++;
	goto expr;

indirect:
#if ASMDEBUG
	printf("indirect: %c\n", *p);
#endif
	stack_push(ostack, "#");
	p++;
	goto expr;

here_or_hex:
#if ASMDEBUG
	printf("here or hex: %c\n", *p);
#endif
	p++;
	if (isxdigit(*p)) goto hexnum;

	stack_push(vstack, ref_new(currline));
	goto operator;

operator:
#if ASMDEBUG >= 2
	printf("operator: %c\n", *p);
#endif
	switch (*p)
	{
		case 0   : goto error;
		case ' ' :
		case '\t': p++; goto operator;
		case '\n':
		case '\r': fold(0); p++; goto newline;
		case '\'': fold(0); p++; goto comment;
		case '{' : eatblockcomment(&p); goto operator;
		case ',' :
		case '+' :
		case '-' :
		case '*' :
		case '/' : 
		{
			fold(*p);
#if ASMDEBUG
			printf("operator: %c\n", *p);
#endif
			stack_push(ostack, p);
			p++; goto expr;
		}
		case ')' : goto rparen;
	}

	fold(' ');
#if ASMDEBUG >= 2
	printf("operator: whitespace\n");
#endif

	stack_push(ostack, " ");
	goto expr;

rparen:
#if ASMDEBUG
	printf("rparen: %c\n", *p);
#endif
	fold(')');
	char* lp = (char*)stack_pop(ostack);
	if (lp == NULL || *lp != '(')
	{
		printf("Error: unmatched right parentheses");
		exit(1);
	}
	p++;

	goto operator;

error:
	printf("Parse error at %d:%ld: %c (%02X)\n", lineno, p-linestart, *p, *p);
	exit(1);

end:	; // silly compile error without the ;

	stack_push(lines, currline);

	for (int i=0; i < lines->top; i++)
	{
		line* l = lines->base[i];
		operand_print(l->operand);
		printf("\n");

	}

	return lines;
}


// main

inline void islabelchar_r(size_t start, size_t end, char c)
{
	while (start <= end)
	{
		islabelchar[start++] = c;
	}
}

int main(int argc, char** argv)
{
	islabelchar_r(0, 255,     0);
	islabelchar       ['$'] = 1;
	islabelchar       ['%'] = 1;
	islabelchar       ['-'] = 1;
	islabelchar       ['#'] = 1;
	islabelchar       ['('] = 1;
	islabelchar_r('A', 'Z',   3);
	islabelchar_r('a', 'z',   3);
	islabelchar       ['_'] = 3;
	islabelchar       [':'] = 3;
	islabelchar       ['.'] = 3;
	islabelchar_r('0', '9',   2);

#include "opcodes.c"

	vstack = stack_new();
	ostack = stack_new();

// load file
	size_t slen = 65536;
	size_t sused = 0;
	char* s = malloc(slen);
	FILE* f = fopen(argv[1], "r");
	size_t n_read = 0;
	do
	{
		n_read = fread(&s[sused], slen - sused, 1, f);

		//printf("read %d, used %d, total %d\n", n_read, sused, slen);

		sused += n_read;

		if (slen < sused - 256)
		{
			s = realloc(s, slen *= 2);
		}
	} while (n_read);
	fclose(f);
	parser(s);
}
