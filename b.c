#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#define NOT_IMPLEMENTED_FOR(VALUE) \
	case VALUE: do { printf("%s:%d: case not implemented yet: %s\n", __FILE__, __LINE__, #VALUE); abort(); } while (0)

static char const* ABI_REGISTERS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };

struct string_builder
{
	char *items;
	size_t capacity, count;
};

#define ARRAY_LEN(A) (sizeof(A) / sizeof(*(A)))


#define INITIAL_CAPACITY 32

#define da_append(where, what) do { \
	if ((where)->capacity == 0) { \
		(where)->capacity = INITIAL_CAPACITY; \
		(where)->items = malloc(sizeof(*(where)->items) * INITIAL_CAPACITY); \
	} else if ((where)->capacity < (where)->count+1) { \
		(where)->capacity *= 2; \
		(where)->items = realloc((where)->items, sizeof(*(where)->items) * (where)->capacity); \
	} \
	(where)->items[(where)->count++] = (what); \
} while(0)

#define da_back(da) ((da).items[(da).count-1])

struct string_pool
{
	char const* str;
	size_t strlen, total;
	struct string_pool *next;
};

static struct string_pool *string_intering_pool = NULL;

static char const* inter(char const *str)
{
	size_t len = strlen(str);

	for (struct string_pool *p = string_intering_pool; p != NULL; p = p->next) {
		if (p->strlen >= len && strcmp(str, p->str + (p->strlen - len)) == 0) {
			free((void*)str);
			return p->str + (p->strlen - len);
		}
	}

	struct string_pool *p = malloc(sizeof(struct string_pool));
	p->str = str;
	p->strlen = len;
	p->next = string_intering_pool;
	p->total = p->strlen + 1 + (p->next ? p->next->total : 0);
	string_intering_pool = p;
	return p->str;
}

size_t string_offset(char const* str)
{
	for (struct string_pool *p = string_intering_pool; p != NULL; p = p->next) {
		if (p->str <= str && str <= p->str + p->strlen) {
			return p->total - (str -  p->str);
		}
	}
	return -1;
}

struct token
{
	enum token_kind
	{
		// Token used to mark end of file
		TOK_EOF = 0,

		TOK_AND = '&',
		TOK_ASSIGN = '=',
		TOK_ASTERISK = '*',
		TOK_COMMA = ',',
		TOK_CURLY_CLOSE = '}',
		TOK_CURLY_OPEN = '{',
		TOK_DIV = '/',
		TOK_GREATER = '>',
		TOK_LESS = '<',
		TOK_LOGICAL_NOT = '!',
		TOK_MINUS = '-',
		TOK_OR = '|',
		TOK_PAREN_CLOSE = ')',
		TOK_PAREN_OPEN = '(',
		TOK_PERCENT = '%',
		TOK_PLUS = '+',
		TOK_SEMICOLON = ';',
		TOK_XOR = '^',
		TOK_QUESTION_MARK = '?',
		TOK_COLON = ':',
		TOK_BITWISE_NOT = '~',
		TOK_BRACKET_OPEN = '[',
		TOK_BRACKET_CLOSE = ']',

		// Make sure that non ascii tokens start after ascii letters
		TOK_IDENTIFIER = 127,

		// Keywords:
		TOK_AUTO,
		TOK_CASE,
		TOK_ELSE,
		TOK_EXTRN,
		TOK_GOTO,
		TOK_IF,
		TOK_RETURN,
		TOK_SWITCH,
		TOK_WHILE,
		TOK_BREAK,
		TOK_CONTINUE,

		// Literals:
		TOK_INTEGER,
		TOK_CHARACTER,
		TOK_STRING,

		// Operators:
		TOK_EQUAL,
		TOK_LESS_OR_EQ,
		TOK_GREATER_OR_EQ,
		TOK_NOT_EQUAL,

		TOK_SHIFT_LEFT,
		TOK_SHIFT_RIGHT,

		TOK_INCREMENT,
		TOK_DECREMENT,

		TOK_ASSIGN_ADD,
		TOK_ASSIGN_SUB,
		TOK_ASSIGN_MUL,
		TOK_ASSIGN_DIV,
		TOK_ASSIGN_SHIFT_LEFT,
		TOK_ASSIGN_SHIFT_RIGHT,

		TOK_LOGICAL_OR,
		TOK_LOGICAL_AND,
	} kind;

	char const* text;
	uint64_t ival;

	char const *p;
};

// Operators that share the same first character must come first
struct {
	enum token_kind kind;
	char const* string;
} SYMBOLS[] = {
	{ TOK_ASSIGN_SHIFT_LEFT, "<<=" },
	{ TOK_ASSIGN_SHIFT_RIGHT, ">>=" },

	{ TOK_EQUAL, "==" },
	{ TOK_LESS_OR_EQ, "<=" },
	{ TOK_GREATER_OR_EQ, ">=" },
	{ TOK_NOT_EQUAL, "!=" },
	{ TOK_ASSIGN_ADD, "+=" },
	{ TOK_ASSIGN_SUB, "-=" },
	{ TOK_ASSIGN_MUL, "*=" },
	{ TOK_ASSIGN_DIV, "/=" },
	{ TOK_INCREMENT, "++" },
	{ TOK_DECREMENT, "--" },
	{ TOK_SHIFT_LEFT, "<<" },
	{ TOK_SHIFT_RIGHT, ">>" },
	{ TOK_LOGICAL_OR, "||" },
	{ TOK_LOGICAL_AND, "&&" },


	{ TOK_AND, "&" },
	{ TOK_ASSIGN, "=" },
	{ TOK_ASTERISK, "*" },
	{ TOK_COMMA, "," },
	{ TOK_CURLY_CLOSE, "}" },
	{ TOK_CURLY_OPEN, "{" },
	{ TOK_DIV, "/" },
	{ TOK_GREATER, ">" },
	{ TOK_LESS, "<" },
	{ TOK_LOGICAL_NOT, "!" },
	{ TOK_MINUS, "-" },
	{ TOK_OR, "|" },
	{ TOK_PAREN_CLOSE, ")" },
	{ TOK_PAREN_OPEN, "(" },
	{ TOK_PERCENT, "%" },
	{ TOK_PLUS, "+" },
	{ TOK_SEMICOLON, ";" },
	{ TOK_XOR, "^" },
	{ TOK_QUESTION_MARK, "?" },
	{ TOK_COLON, ":" },
	{ TOK_BITWISE_NOT, "~" },
	{ TOK_BRACKET_OPEN, "[" },
	{ TOK_BRACKET_CLOSE, "]" },

	{ TOK_AUTO, "auto" },
	{ TOK_CASE, "case" },
	{ TOK_ELSE, "else" },
	{ TOK_EXTRN, "extrn" },
	{ TOK_GOTO, "goto" },
	{ TOK_IF, "if" },
	{ TOK_RETURN, "return" },
	{ TOK_SWITCH, "switch" },
	{ TOK_WHILE, "while" },
	{ TOK_BREAK, "break" },
	{ TOK_CONTINUE, "continue" },
};

char const* token_kind_short_name(enum token_kind kind)
{
	switch (kind) {
	case TOK_CHARACTER: return "character literal";
	case TOK_IDENTIFIER: return "identifier";
	case TOK_INTEGER: return "integer literal";
	case TOK_EOF: return "end of file";
	case TOK_STRING: return "string literal";

	default:
		for (size_t i = 0; i < ARRAY_LEN(SYMBOLS); ++i) {
			if (SYMBOLS[i].kind == kind) {
				return SYMBOLS[i].string;
			}
		}
	}

	assert(0 && "unreachable");
}

char const* token_short_name(struct token tok)
{
	return token_kind_short_name(tok.kind);
}

static char const* source = NULL;

void dump_location(FILE *out, struct token tok)
{
	size_t line = 1, column = 1;
	char const *p = source;

	do switch (*p) {
	case '\n':
		line++; [[fallthrough]];
	case '\r':
		column = 1;
		break;
	default:
		column++;
	} while (*++p && p != tok.p);

	fprintf(out, "%s:%zu:%zu: ", "(stdin)", line, column);
}

__attribute__ ((format (printf, 2, 3)))
void errorf(struct token tok, char const* fmt, ...)
{
	dump_location(stderr, tok);
	fprintf(stderr, "error: ");
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

__attribute__ ((format (printf, 2, 3)))
void notef(struct token tok, char const* fmt, ...)
{
	dump_location(stderr, tok);
	fprintf(stderr, "note: ");
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}


struct tokenizer
{
	char const* source;
	struct string_pool strings;
	size_t head;
};

struct token scan(struct tokenizer *ctx);
void dump_token(FILE *out, struct token tok);
char const* token_short_name(struct token tok);

struct parser
{
	struct tokenizer tokenizer;
};

struct symbol
{
	enum symbol_kind {
		EXTERNAL,
		GLOBAL,
		LOCAL,
	} kind;
	char const* name;
	size_t id;
	size_t offset;
};

struct scope
{
	struct symbol *items;
	size_t capacity, count;
	size_t stack_offset;
};

struct label
{
	char const* name;
	bool defined;
	struct token first_usage;
};

struct data
{
	size_t id;
	bool is_vec;
	struct token declared_size;

	struct token *items;
	size_t count, capacity;
};


// TODO: Add literal value type
struct value
{
	enum {
		EMPTY,
		RVALUE,
		LVALUE_AUTO,
		LVALUE_PTR,
	} kind;
	union { size_t offset; };
};

struct control
{
	enum token_kind kind;
	struct value lhs; // value to compare to in switch
	size_t next;      // next case for switch, next iteration for while
	size_t end;       // end of switch, end of while
};

struct compiler
{
#define MAX_SCOPE_NESTING 64
	struct scope scope[MAX_SCOPE_NESTING];
	size_t nesting;
	size_t last_symbol_id;
	size_t stack_current_offset;
	size_t stack_capacity;
	size_t last_local_id;

	struct {
		char const **items;
		size_t count, capacity;
	} defined_externs;

	struct {
		struct label *items;
		size_t count, capacity;
	} function_labels;

	struct {
		struct data *items;
		size_t count, capacity;
	} data_section;

	struct {
		struct control *items;
		size_t count, capacity;
	} control;
};

size_t alloc_stack(struct compiler *compiler)
{
	if (compiler->stack_capacity == 0) {
		compiler->stack_capacity = 16;
	}

	size_t offset = (compiler->stack_current_offset += sizeof(uint64_t));
	if (compiler->stack_current_offset > compiler->stack_capacity) {
		compiler->stack_capacity *= 2;
	}
	return offset;
}

void enter_scope(struct compiler *compiler)
{
	++compiler->nesting;
	assert(compiler->nesting < MAX_SCOPE_NESTING);
	compiler->scope[compiler->nesting].count = 0;
	compiler->scope[compiler->nesting].stack_offset = compiler->stack_current_offset;
}

void leave_scope(struct compiler *compiler)
{
	assert(compiler->nesting > 0 && "Trying to leave when in global scope");
	compiler->stack_current_offset = compiler->scope[compiler->nesting].stack_offset;
	--compiler->nesting;
}

struct symbol* search_symbol_in_scope(struct compiler *compiler, char const* identifier)
{
	struct scope scope = compiler->scope[compiler->nesting];
	for (size_t i = 0; i < scope.count; ++i) {
		if (strcmp(scope.items[i].name, identifier) == 0)
			return &scope.items[i];
	}
	return NULL;
}

struct symbol* search_symbol(struct compiler *compiler, char const* identifier)
{
	for (int level = compiler->nesting; level >= 0; --level) {
		struct scope scope = compiler->scope[level];
		for (size_t i = 0; i < scope.count; ++i) {
			if (strcmp(scope.items[i].name, identifier) == 0)
				return &scope.items[i];
		}
	}
	return NULL;
}

struct symbol define_symbol(struct compiler *compiler, struct symbol symbol, struct token name)
{
	struct symbol *s;
	if ((s = search_symbol_in_scope(compiler, symbol.name))) {
		// TODO: Mention previous definition
		errorf(name, "symbol %s has already been defined\n", name.text);
		exit(1);
	}
	++compiler->last_symbol_id;
	assert(compiler->last_symbol_id > 0);
	symbol.id = compiler->last_symbol_id;
	da_append(&compiler->scope[compiler->nesting], symbol);
	return symbol;
}

void parse_program(struct parser *p, struct compiler *compiler);
bool parse_statement(struct parser *p, struct compiler *compiler);

int main()
{
	struct compiler compiler = {
		.stack_current_offset = 8,
	};


	// TODO: optimize me
	struct string_builder sb = {};

	for (;;) {
		char buf[4096];
		size_t read = fread(&buf, 1, sizeof(buf), stdin);
		if (read == 0) {
			break;
		}

		for (size_t i = 0; i < read; ++i) {
			da_append(&sb, buf[i]);
		}
	}
	da_append(&sb, '\0');
	source = sb.items;

	struct parser parser = {
		.tokenizer = { .source = sb.items, .head = 0, },
	};

#if 0
	struct token tok;

	while ((tok = scan(&parser.tokenizer)).kind != TOK_EOF) {
		dump_token(stdout, tok);
	}

	int i = 0;
	printf("string intering pool:\n");
	for (struct string_pool *p = string_intering_pool; p != NULL; p = p->next) {
		printf("[%d] = \"%s\"\n", i++, p->str);
	}

#else
	printf("BITS 64\n");
	printf("DEFAULT rel\n");

	printf("section \".text\" exec nowrite\n");
	parse_program(&parser, &compiler);

	// TODO: Move globals without init to bss
	printf("section \".data\" write\n");
	for (size_t i = 0; i < compiler.data_section.count; ++i) {
		struct data data = compiler.data_section.items[i];
		uint64_t actual_size = data.count;

		if (data.is_vec) {
			if (actual_size < data.declared_size.ival) {
				actual_size = data.declared_size.ival;
			}
			printf("sym_%zu: dq $+8\n", data.id);
			// TODO: error message
			assert(actual_size != 0);
		} else {
			printf("sym_%zu:\n", data.id);
			if (actual_size == 0) {
				printf("\tdq 0\n");
				continue;
			}
		}


		printf("\tdq ");

		for (size_t i = 0; i < actual_size; ++i) {
			if (i > 0) { printf(","); }
			if (i < data.count) {
				switch (data.items[i].kind) {
				case TOK_STRING:
					printf("strend - %zu", string_offset(data.items[i].text));
					break;

				case TOK_INTEGER: printf("%"PRIu64, data.items[i].ival); break;

				default:
					assert(0 && "not implemented yet");
				}
			} else {
				printf("0");
			}
		}
		printf("\n");
	}

	printf("section \".rodata\"\n");

	if (string_intering_pool) {
		printf("db 0x00");
		for (struct string_pool *p = string_intering_pool; p != NULL; p = p->next) {
			for (char const *s = p->str; *s; ++s) {
				printf(",0x%02x", *s);
			}
			printf(",0x00");
		}
		printf("\nstrend:\n");
	}

#endif
}

struct token peek_token(struct parser *p)
{
	size_t save = p->tokenizer.head;
	struct token next = scan(&p->tokenizer);
	p->tokenizer.head = save;
	return next;
}

bool expect_token(struct parser *p, struct token *tok, enum token_kind kind)
{
	size_t save = p->tokenizer.head;
	*tok = scan(&p->tokenizer);

	if (tok->kind != kind) {
		p->tokenizer.head = save;
		return false;
	}
	return true;
}

bool expect_token2(struct parser *p, struct token *tok1, enum token_kind kind1, struct token *tok2, enum token_kind kind2)
{
	size_t save = p->tokenizer.head;

	if ((*tok1 = scan(&p->tokenizer)).kind == kind1
	&&  (*tok2 = scan(&p->tokenizer)).kind == kind2) {
		return true;
	}

	p->tokenizer.head = save;
	return false;
}


bool expect_token_if(struct parser *p, struct token *tok, bool(*predicate)(enum token_kind))
{
	size_t save = p->tokenizer.head;
	if (predicate((*tok = scan(&p->tokenizer)).kind)) {
		return true;
	}
	p->tokenizer.head = save;
	return false;
}

void mov_into_reg(char const* dst, struct value src)
{
	switch (src.kind) {
	case LVALUE_AUTO:
	case RVALUE:
		printf("\tmov %s, [rbp-%zu]\n", dst, src.offset);
		break;

	case LVALUE_PTR:
		printf("\tmov %s, [rbp-%zu]\n", dst, src.offset);
		printf("\tmov %s, [%s]\n", dst, dst);
		break;

	case EMPTY:
		assert(0 && "unreachable");
	}
}


bool parse_expression(struct parser *p, struct compiler *compiler, struct value *result);
bool parse_unary(struct parser *p, struct compiler *compiler, struct value *lhs);

bool parse_return(struct parser *p, struct compiler *compiler)
{
	struct token return_;
	if (!expect_token(p, &return_, TOK_RETURN)) {
		return false;
	}

	struct token open, semicolon;
	if (expect_token(p, &open, TOK_PAREN_OPEN)) {
		struct value retval;

		if (!parse_expression(p, compiler, &retval)) {
			errorf(open, "expected rvalue\n");
			exit(1);
		}

		mov_into_reg("rax", retval);
		printf("\tleave\n");
		printf("\tret\n");

		struct token close;
		if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
			errorf(close, "expected close paren, got %s\n", token_short_name(close));
			notef(open, "paren was open here\n");
			exit(2);
		}
		if (!expect_token(p, &semicolon, TOK_SEMICOLON)) {
			errorf(semicolon, "return expect ; after closing ), got %s\n", token_short_name(semicolon));
			exit(2);
		}
	} else if (expect_token(p, &semicolon, TOK_SEMICOLON)) {
		printf("\tleave\n");
		printf("\tret\n");
	} else {
		errorf(return_, "return expects ; or (, got %s\n", token_short_name(semicolon));
		exit(2);
	}
	return true;
}

bool parse_extern(struct parser *p, struct compiler *compiler)
{
	struct token extrn;
	if (!expect_token(p, &extrn, TOK_EXTRN)) {
		return false;
	}

	for (;;) {
		struct token name;
		if (!expect_token(p, &name, TOK_IDENTIFIER)) {
			errorf(extrn, "extrn expected identifier, got %s\n", token_short_name(name));
			exit(1);
		}

		define_symbol(compiler, (struct symbol) { .name = name.text, .kind = EXTERNAL }, name);

		bool found = false;
		for (size_t i = 0; i < compiler->defined_externs.count; ++i) {
			if (strcmp(compiler->defined_externs.items[i], name.text) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			printf("\textern %s\n", name.text);
			da_append(&compiler->defined_externs, name.text);
		}

		struct token semicolon, comma;
		if (expect_token(p, &semicolon, TOK_SEMICOLON)) {
			break;
		} else if (expect_token(p, &comma, TOK_COMMA)) {
			continue;
		} else {
			errorf(semicolon, "extrn expected ; or comma, got %s\n", token_short_name(semicolon));
			exit(2);
		}
	}

	return parse_statement(p, compiler);
}

bool parse_auto(struct parser *p, struct compiler *compiler)
{
	struct token auto_;
	if (!expect_token(p, &auto_, TOK_AUTO)) {
		return false;
	}

	for (;;) {
		struct token name;
		if (!expect_token(p, &name, TOK_IDENTIFIER)) {
			errorf(auto_, "auto expected identifier, got %s\n", token_short_name(name));
			exit(1);
		}

		struct symbol s = define_symbol(compiler, (struct symbol) { .name = name.text, .kind = LOCAL, .offset = alloc_stack(compiler) }, name);
		printf("\t; auto [rbp-%zu] = %s\n", s.offset, name.text);

		struct token semicolon, comma;
		if (expect_token(p, &semicolon, TOK_SEMICOLON)) {
			break;
		} else if (expect_token(p, &comma, TOK_COMMA)) {
			continue;
		} else {
			errorf(semicolon, "auto expected ; or comma, got %s\n", token_short_name(semicolon));
			exit(2);
		}
	}

	return parse_statement(p, compiler);
}

bool parse_goto(struct parser *p, struct compiler *compiler)
{
	struct token goto_, identifier;
	if (!expect_token(p, &goto_, TOK_GOTO)) {
		return false;
	}
	if (!expect_token(p, &identifier, TOK_IDENTIFIER)) {
		errorf(identifier, "goto expects an identifier, got instead %s\n", token_short_name(identifier));
		return false;
	}
	size_t label = -1;

	for (size_t i = 0; i < compiler->function_labels.count; ++i) {
		if (strcmp(compiler->function_labels.items[i].name, identifier.text) == 0) {
			label = i;
			break;
		}
	}

	// If we haven't find label it will be defined in the future (hopefully)
	// at the end of the function we need to check if all labels have been defined
	if (label == (size_t)-1) {
		struct label new = { .name = identifier.text, .defined = false, .first_usage = identifier };
		da_append(&compiler->function_labels, new);
		label = compiler->function_labels.count;
	}

	printf("\tjmp .label_%zu\n", label);
	return true;
}

bool parse_empty_statement(struct parser *p)
{
	struct token semicolon;
	return expect_token(p, &semicolon, TOK_SEMICOLON);
}

bool parse_compund_statement(struct parser *p, struct compiler *compiler)
{
	struct token open;
	if (!expect_token(p, &open, TOK_CURLY_OPEN))
		return false;

	enter_scope(compiler);
	while (parse_statement(p, compiler))
		;
	leave_scope(compiler);

	struct token close;
	if (!expect_token(p, &close, TOK_CURLY_CLOSE)) {
		errorf(close, "expected close curly or statement, got %s\n", token_short_name(close));
		notef(open, "curly was open here\n");
		exit(2);
	}
	return true;
}

bool parse_funccall(struct parser *p, struct compiler *compiler, struct value *result, struct symbol *symbol)
{
	struct token open;
	if (!expect_token(p, &open, TOK_PAREN_OPEN)) {
		return false;
	}

	struct value args[ARRAY_LEN(ABI_REGISTERS)];
	size_t args_count = 0;

	// TODO: This code is awful, we _need_ register allocation to fix it
	for (size_t i = 0; i < ARRAY_LEN(ABI_REGISTERS); ++i) {
		if (i > 0) {
			struct token comma;
			if (!expect_token(p, &comma, TOK_COMMA)) {
				if (comma.kind == TOK_PAREN_CLOSE) break;
				errorf(comma, "expected comma, got %s\n", token_short_name(comma));
				exit(2);
			}
		}

		if (!parse_expression(p, compiler, &args[args_count])) {
			break;
		}
		++args_count;
	}

	for (size_t i = 0; i < args_count; ++i) {
		mov_into_reg(ABI_REGISTERS[i], args[i]);
	}


	struct token close;
	if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
		errorf(close, "expected close paren, got %s\n", token_short_name(close));
		notef(open, "paren was open here\n");
		exit(2);
	}

	printf("\txor rax, rax\n");

	switch (symbol->kind) {
		case EXTERNAL: printf("\tcall %s WRT ..plt\n", symbol->name); break;
		case GLOBAL: printf("\tcall sym_%zu\n", symbol->id); break;
		case LOCAL: printf("\tlea r10, QWORD [rbp-%zu]\n\tcall r10\n", symbol->offset); break;
	}

	result->kind = RVALUE;
	result->offset = alloc_stack(compiler);
	printf("\tmov [rbp-%zu], rax\n", result->offset);

	return true;
}

bool parse_name(struct parser *p, struct compiler *compiler, struct symbol **symbol, struct value *lhs)
{
	struct token name;
	if (!expect_token(p, &name, TOK_IDENTIFIER)) {
		return false;
	}
	*symbol = search_symbol(compiler, name.text);
	if (!*symbol) {
		errorf(name, "'%s' has not been defined yet\n", name.text);
		exit(1);
	}
	// TODO: This only works for local names, other kinds of symbols are loaded differently
	if ((*symbol)->kind == LOCAL) {
		lhs->kind = LVALUE_AUTO;
		lhs->offset = (*symbol)->offset;
	}
	return true;
}

bool parse_constant(struct parser *p, struct compiler *compiler, struct value *lhs)
{
	struct token constant;
	if (expect_token(p, &constant, TOK_INTEGER)) {
		lhs->kind = RVALUE;
		lhs->offset = alloc_stack(compiler);
		printf("\tmov rax, %"PRIu64"\n", constant.ival);
		printf("\tmov QWORD [rbp-%zu], rax\n", lhs->offset);
		return true;
	}
	if (expect_token(p, &constant, TOK_CHARACTER)) {
		lhs->kind = RVALUE;
		lhs->offset = alloc_stack(compiler);
		printf("\tmov QWORD [rbp-%zu], %"PRIu64"\n", lhs->offset, constant.ival);
		return true;
	}
	if (expect_token(p, &constant, TOK_STRING)) {
		lhs->kind = RVALUE;
		lhs->offset = alloc_stack(compiler);
		printf("\tlea rax, [strend-%zu]\n", string_offset(constant.text));
		printf("\tmov [rbp-%zu], rax\n", lhs->offset);
		return true;
	}

	return false;
}

struct binop {
	enum token_kind op;
	enum associativity { ASSOC_LEFT, ASSOC_RIGHT } associativity;
};

// TODO: This works on Clang and GCC but maybe not on other compilers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

// TODO: Note that we generaly have the same associativity for the same precedense level
struct binop* binary_operators[] = {
	(struct binop[]) {
		{TOK_ASSIGN, ASSOC_RIGHT},
		{TOK_ASSIGN_ADD, ASSOC_RIGHT},
		{TOK_ASSIGN_SUB, ASSOC_RIGHT},
		{TOK_ASSIGN_MUL, ASSOC_RIGHT},
		{TOK_ASSIGN_DIV, ASSOC_RIGHT},
		{TOK_ASSIGN_SHIFT_LEFT, ASSOC_RIGHT},
		{TOK_ASSIGN_SHIFT_RIGHT, ASSOC_RIGHT},
		{},
	},
	(struct binop[]) { {TOK_QUESTION_MARK, ASSOC_RIGHT}, {} },
	(struct binop[]) { {TOK_LOGICAL_OR}, {} },
	(struct binop[]) { {TOK_LOGICAL_AND}, {} },
	(struct binop[]) { {TOK_OR}, {} },
	(struct binop[]) { {TOK_XOR}, {} },
	(struct binop[]) { {TOK_AND}, {} },
	(struct binop[]) { {TOK_EQUAL}, {TOK_NOT_EQUAL}, {} },
	(struct binop[]) { {TOK_GREATER}, {TOK_GREATER_OR_EQ}, {TOK_LESS}, {TOK_LESS_OR_EQ}, {} },
	(struct binop[]) { {TOK_SHIFT_LEFT}, {TOK_SHIFT_RIGHT}, {} },
	(struct binop[]) { {TOK_PLUS}, {TOK_MINUS}, {} },
	(struct binop[]) { {TOK_ASTERISK}, {TOK_DIV}, {TOK_PERCENT}, {} },
};

#pragma GCC diagnostic pop

size_t precedense(enum token_kind kind)
{
	for (size_t i = 0; i < ARRAY_LEN(binary_operators); ++i) {
		for (size_t j = 0; binary_operators[i][j].op != 0; ++j) {
			if (binary_operators[i][j].op == kind) {
				return i+1;
			}
		}
	}
	return 0;
}

enum associativity associativity(enum token_kind kind)
{
	for (size_t i = 0; i < ARRAY_LEN(binary_operators); ++i) {
		for (size_t j = 0; binary_operators[i][j].op != 0; ++j) {
			if (binary_operators[i][j].op == kind) {
				return binary_operators[i][j].associativity;
			}
		}
	}
	assert(0 && "unreachable");
}


bool is_operator(enum token_kind kind)
{
	return precedense(kind) != 0;
}

void emit_op(struct compiler *compiler, struct value *result, struct value lhsv, enum token_kind op, struct value rhsv, size_t end_label)
{
	if (op == TOK_LOGICAL_OR || op == TOK_LOGICAL_AND || op == TOK_QUESTION_MARK) {
		mov_into_reg("rax", rhsv);
		printf("\tmov [rbp-%zu], rax\n", result->offset);
		printf(".local_%zu:\n", end_label);
		return;
	}

	// TODO: Static checking for this switch
	// TODO: Not needed for assign
	size_t res = alloc_stack(compiler);
	*result = (struct value) { .kind = RVALUE, .offset = res };

	switch (op) {
	case TOK_ASSIGN:
		mov_into_reg("rax", rhsv);
		switch (lhsv.kind) {
		case LVALUE_AUTO: printf("\tmov [rbp-%zu], rax\n", lhsv.offset); break;
		case LVALUE_PTR:  printf("\tmov rcx, [rbp-%zu]\n"
													   "\tmov [rcx], rax\n", lhsv.offset); break;
		case RVALUE:
			// TODO: Line information
			errorf((struct token){}, "trying to assign to rvalue\n");
			exit(1);

		case EMPTY: assert(0 && "unreachable");
		}
		*result = lhsv;
		return;

	case TOK_ASSIGN_ADD:
	case TOK_ASSIGN_DIV:
	case TOK_ASSIGN_MUL:
	case TOK_ASSIGN_SUB:
	case TOK_ASSIGN_SHIFT_LEFT:
	case TOK_ASSIGN_SHIFT_RIGHT:
		mov_into_reg("rax", lhsv);
		mov_into_reg("rcx", rhsv);

		switch (op) {
		case TOK_ASSIGN_MUL: printf("\timul rax, rcx\n"); break;
		case TOK_ASSIGN_ADD: printf("\tadd rax, rcx\n"); break;
		case TOK_ASSIGN_SUB: printf("\tsub rax, rcx\n"); break;
		case TOK_ASSIGN_SHIFT_LEFT: printf("\tshl rax, cl\n"); break;
		case TOK_ASSIGN_SHIFT_RIGHT: printf("\tshr rax, cl\n"); break;
		case TOK_ASSIGN_DIV:
			printf("\tcqo\n");
			printf("\tidiv QWORD rcx\n");
			break;
		default:
			assert(0 && "unreachable");
		}

		switch (lhsv.kind) {
		case LVALUE_AUTO: printf("\tmov [rbp-%zu], rax\n", lhsv.offset); break;
		case LVALUE_PTR:  printf("\tmov rcx, [rbp-%zu]\n"
													   "\tmov [rcx], rax\n", lhsv.offset); break;
		case RVALUE:
			// TODO: Line information
			errorf((struct token){}, "trying to assign to rvalue\n");
			exit(1);

		case EMPTY: assert(0 && "unreachable");
		}
		*result = lhsv;
		return;

	case TOK_PLUS:
	case TOK_MINUS:
	case TOK_ASTERISK:
	case TOK_OR:
	case TOK_AND:
	case TOK_XOR:
		{
			static char const* BIN_INSTR[] = {
				[TOK_AND] = "and",
				[TOK_ASTERISK] = "imul",
				[TOK_MINUS] = "sub",
				[TOK_OR] = "or",
				[TOK_PLUS] = "add",
				[TOK_XOR] = "xor",
			};
			// TODO: We can optimize this (remove one move, add accepts memory as argument)
			mov_into_reg("rax", lhsv);
			mov_into_reg("rcx", rhsv);
			printf("\t%s rax, rcx\n", BIN_INSTR[op]);
			printf("\tmov [rbp-%zu], rax\n", res);
			return;
		}


	// TODO: We can optimize this (remove one move, add accepts memory as argument)
	// TODO: Proof that this is correct
	case TOK_SHIFT_LEFT:
		mov_into_reg("rax", lhsv);
		mov_into_reg("rcx", rhsv);
		printf("\tshl rax, cl\n");
		printf("\tmov [rbp-%zu], rax\n", res);
		return;

	// TODO: We can optimize this (remove one move, add accepts memory as argument)
	// TODO: Proof that this is correct
	case TOK_SHIFT_RIGHT:
		mov_into_reg("rax", lhsv);
		mov_into_reg("rcx", rhsv);
		printf("\tshr rax, cl\n");
		printf("\tmov [rbp-%zu], rax\n", res);
		return;


	case TOK_EQUAL:
	case TOK_GREATER:
	case TOK_GREATER_OR_EQ:
	case TOK_LESS:
	case TOK_LESS_OR_EQ:
	case TOK_NOT_EQUAL:
		{
			static char const* SET_SUFFIX[] = {
				[TOK_EQUAL] = "e",
				[TOK_GREATER] = "g",
				[TOK_GREATER_OR_EQ] = "ge",
				[TOK_LESS] = "l",
				[TOK_LESS_OR_EQ] = "le",
				[TOK_NOT_EQUAL] = "ne",
			};
			printf("\txor rcx, rcx\n");
			mov_into_reg("rax", lhsv);
			mov_into_reg("rdx", rhsv);
			printf("\tcmp rax, rdx\n");
			printf("\tset%s cl\n", SET_SUFFIX[op]);
			printf("\tmov [rbp-%zu], rcx\n", res);
			return;
		}

	case TOK_DIV:
	case TOK_PERCENT:
		mov_into_reg("rax", lhsv);
		mov_into_reg("rcx", rhsv);
		printf("\tcqo\n");
		printf("\tidiv QWORD rcx\n");
		printf("\tmov [rbp-%zu], %s\n", res, op == TOK_DIV ? "rax" : "rdx");
		return;


	default:
		fprintf(stderr, "math not supported yet for operator: %s\n", token_kind_short_name(op));
		exit(1);
	}
}

void parse_rhs(struct parser *p, struct compiler *compiler, struct token op, struct value *result, struct value lhs)
{
	// Infrastructure for ternary:
	// result = condition ? then : else
	struct value condition, then;
	size_t else_label, end_label;

	if (op.kind == TOK_QUESTION_MARK) {
		else_label = compiler->last_local_id++;
		end_label = compiler->last_local_id++;
		condition = lhs;

		// TODO: if both then and else branches are lvalues we can return an lvalue
		*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };

		mov_into_reg("rax", condition);
		printf("\tcmp rax, 0\n");
		printf("\tje .local_%zu\n", else_label);

		if (!parse_expression(p, compiler, &then)) {
			errorf(op, "expected expression between ? and : of ternary operator\n");
			exit(1);
		}

		mov_into_reg("rax", then);
		printf("\tmov [rbp-%zu], rax\n", result->offset);
		printf("\tjmp .local_%zu\n", end_label);
		printf(".local_%zu:\n", else_label);

		struct token colon;
		if (!expect_token(p, &colon, TOK_COLON)) {
			errorf(colon, "expected : after expression started with ?, got %s instead\n", token_short_name(colon));
			exit(1);
		}
	} else if (op.kind == TOK_LOGICAL_AND) {
		*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };
		end_label = compiler->last_local_id++;
		condition = lhs;
		mov_into_reg("rax", condition);
		printf("\tmov [rbp-%zu], rax\n", result->offset);
		printf("\tcmp rax, 0\n");
		printf("\tje .local_%zu\n", end_label);
	} else if (op.kind == TOK_LOGICAL_OR) {
		*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };
		end_label = compiler->last_local_id++;
		condition = lhs;
		mov_into_reg("rax", condition);
		printf("\tmov [rbp-%zu], rax\n", result->offset);
		printf("\tcmp rax, 0\n");
		printf("\tjne .local_%zu\n", end_label);
	}

	// TODO: See if we can get away without allocating this varibale
	struct value rhs = { .kind = RVALUE, .offset = alloc_stack(compiler) };

	if (!parse_unary(p, compiler, &rhs)) {
		struct token tok = peek_token(p);
		errorf(tok, "%s\n", token_short_name(tok));
		assert(0 && "report an error");
	}

	struct token next;
	if (!expect_token_if(p, &next, is_operator)) {
		emit_op(compiler, result, lhs, op.kind, rhs, end_label);
		return;
	}


	// For expression a op b next c, op has higher precedense then next so we
	// should parse as (a op b) next c. otherwise we should parse a op (b next c)
	// If op has the same precedense then we should bind according to the rules of associativity
	bool bind_left = precedense(op.kind) >= precedense(next.kind);
	if (precedense(op.kind) == precedense(next.kind)) {
		bind_left = associativity(op.kind) == ASSOC_LEFT;
		assert(associativity(op.kind) == associativity(next.kind));
	}

	// For ternary:
	//   bind_left:  (condition ? then : 'else: rhs) 'end: op parse_rhs
	//   bind_right: condition ? then : 'else: (rhs op parse_rhs) 'end:
	// else label is already emitted at this point

	if (bind_left) {
		assert(op.kind != TOK_QUESTION_MARK && "is there ever situation where we bind left?");
		emit_op(compiler, result, lhs, op.kind, rhs, end_label);
		parse_rhs(p, compiler, next, result, *result);
	} else {
		struct value rhs_result;
		parse_rhs(p, compiler, next, &rhs_result, rhs);
		emit_op(compiler, result, lhs, op.kind, rhs_result, end_label);
	}
}

bool maybe_parse_binary(struct parser *p, struct compiler *compiler, struct value *result, struct value lhs)
{
	struct token op;
	if (!expect_token_if(p, &op, is_operator)) {
		*result = lhs;
		return true;
	}
	parse_rhs(p, compiler, op, result, lhs);
	return true;
}


bool parse_atomic(struct parser *p, struct compiler *compiler, struct value *lhs)
{
	struct symbol *symbol = NULL;

	struct token open;
	if (expect_token(p, &open, TOK_PAREN_OPEN)) {

		if (!parse_expression(p, compiler, lhs)) {
			errorf(open, "expected expression inside parens\n");
			exit(1);
		}

		struct token close;
		if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
			errorf(close, "expected close paren, got %s\n", token_short_name(close));
			notef(open, "opened paren here\n");
			exit(1);
		}

		return true;
	}

	if (parse_constant(p, compiler, lhs))
		return true;

	if (!parse_name(p, compiler, &symbol, lhs))
		return false;


	if (parse_funccall(p, compiler, lhs, symbol))
		return true;

#if 0
	assert(symbol->kind == LOCAL && "external or global symbols are only supported by function calls for now");

	if (parse_assign(p,compiler, symbol->offset)) {
		return true;
	}
#endif

	switch (symbol->kind) {
	case LOCAL:
		// TODO: Move handling from parse_name
		return true;

	case GLOBAL:
		*lhs = (struct value) { .kind = LVALUE_PTR, .offset = alloc_stack(compiler) };
		printf("\tlea rax, [sym_%zu]\n", symbol->id);
		printf("\tmov [rbp-%zu], rax\n", lhs->offset);
		return true;

	case EXTERNAL:
		*lhs = (struct value) { .kind = LVALUE_PTR, .offset = alloc_stack(compiler) };
		printf("\tlea rax, [rel %s]\n", symbol->name);
		printf("\tmov [rbp-%zu], rax\n", lhs->offset);
		return true;
	}

	return true;
}

bool parse_primary_expression(struct parser *p, struct compiler *compiler, struct value *result)
{
	if (!parse_atomic(p, compiler, result)) {
		return false;
	}

	struct value lhs = *result;

	struct token open;
	if (expect_token(p, &open, '[')) {
		struct value index;
		if (!parse_expression(p, compiler, &index)) {
			errorf(open, "expected index expression\n");
			exit(1);
		}

		*result = (struct value) { .kind = LVALUE_PTR, .offset = alloc_stack(compiler) };
		mov_into_reg("rax", lhs);
		mov_into_reg("rcx", index);
		printf("\tlea rax, [rax+rcx*8]\n");
		printf("\tmov [rbp-%zu], rax\n", result->offset);

		struct token close;
		if (!expect_token(p, &close, ']')) {
			errorf(close, "expected close ] for the open [, got %s\n", token_short_name(close));
			notef(open, "[ was opened here\n");
			exit(1);
		}
	}

	struct token post_inc;
	if (expect_token(p, &post_inc, TOK_INCREMENT)) {
		*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };
		mov_into_reg("rax", lhs);
		printf("\tmov [rbp-%zu], rax\n", result->offset);

		switch (lhs.kind) {
		case LVALUE_AUTO:
			printf("\tinc QWORD [rbp-%zu]\n", lhs.offset);
			break;

		case LVALUE_PTR:
			printf("\tmov rax, [rbp-%zu]\n", lhs.offset);
			printf("\tinc QWORD [rax]\n");
			break;

		default:
			errorf(post_inc, "post-increment operator expects lvalue\n");
			exit(1);
		}
	}

	struct token post_dec;
	if (expect_token(p, &post_dec, TOK_DECREMENT)) {
		*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };
		mov_into_reg("rax", lhs);
		printf("\tmov [rbp-%zu], rax\n", result->offset);

		switch (lhs.kind) {
		case LVALUE_AUTO:
			printf("\tdec QWORD [rbp-%zu]\n", lhs.offset);
			break;

		case LVALUE_PTR:
			printf("\tmov rax, [rbp-%zu]\n", lhs.offset);
			printf("\tdec QWORD [rax]\n");
			break;

		default:
			errorf(post_dec, "post-decrement operator expects lvalue\n");
			exit(1);
		}
	}

	return true;
}

bool parse_unary(struct parser *p, struct compiler *compiler, struct value *result)
{
	struct token and_;
	if (expect_token(p, &and_, TOK_AND)) {
		struct value val;
		if (!parse_unary(p, compiler, &val)) {
			errorf(and_, "expected atomic expression for address of operator\n");
			exit(1);
		}

		switch (val.kind) {
		case LVALUE_PTR:
			*result = (struct value) { .kind = RVALUE, .offset = val.offset };
			return true;

		case LVALUE_AUTO:
			*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };
			printf("\tlea rax, [rbp-%zu]\n", val.offset);
			printf("\tmov [rbp-%zu], rax\n", result->offset);
			return true;

		case RVALUE:
		case EMPTY:
			errorf(and_, "address of operator expects lvalue\n");
			exit(1);
		}

		return true;
	}

	struct token bnot;
	if (expect_token(p, &bnot, TOK_BITWISE_NOT)) {
		struct value val;
		if (!parse_unary(p, compiler, &val)) {
			errorf(bnot, "expected primary expression for bitwise not operator\n");
			exit(1);
		}
		*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };
		mov_into_reg("rax", val);
		printf("\tnot rax\n");
		printf("\tmov [rbp-%zu], rax\n", result->offset);
		return true;
	}

	struct token lnot;
	if (expect_token(p, &lnot, TOK_LOGICAL_NOT)) {
		struct value val;
		if (!parse_unary(p, compiler, &val)) {
			errorf(lnot, "expected primary expression for logicla not operator\n");
			exit(1);
		}
		*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };
		mov_into_reg("rcx", val);
		printf("\txor rax, rax\n");
		printf("\ttest rcx, rcx\n");
		printf("\tsete al\n");
		printf("\tmov [rbp-%zu], rax\n", result->offset);
		return true;
	}

	struct token pre_increment;
	if (expect_token(p, &pre_increment, TOK_INCREMENT)) {
		struct value val;
		if (!parse_unary(p, compiler, &val)) {
			errorf(pre_increment, "expected atomic expression for pre-increment operator\n");
			exit(1);
		}

		switch (val.kind) {
		case LVALUE_AUTO:
			*result = val;
			printf("\tinc QWORD [rbp-%zu]\n", val.offset);
			break;

		case LVALUE_PTR:
			*result = val;
			printf("\tmov rax, [rbp-%zu]\n", val.offset);
			printf("\tinc QWORD [rax]\n");
			break;

		default:
			errorf(pre_increment, "pre-increment operator expects lvalue\n");
			exit(1);
		}
		return true;
	}

	struct token pre_decrement;
	if (expect_token(p, &pre_decrement, TOK_DECREMENT)) {
		struct value val;
		if (!parse_unary(p, compiler, &val)) {
			errorf(pre_decrement, "expected atomic expression for pre-decrement operator\n");
			exit(1);
		}

		switch (val.kind) {
		case LVALUE_AUTO:
			*result = val;
			printf("\tdec QWORD [rbp-%zu]\n", val.offset);
			break;

		case LVALUE_PTR:
			*result = val;
			printf("\tmov rax, [rbp-%zu]\n", val.offset);
			printf("\tdec QWORD [rax]\n");
			break;

		default:
			errorf(pre_decrement, "pre-decrement operator expects lvalue\n");
			exit(1);
		}
		return true;
	}


	struct token deref;
	if (expect_token(p, &deref, TOK_ASTERISK)) {
		struct value val;
		if (!parse_unary(p, compiler, &val)) {
			errorf(deref, "expected atomic expression for dereference\n");
			exit(1);
		}

		switch (val.kind) {
		case LVALUE_AUTO:
		case RVALUE:
			*result = (struct value) { .kind = LVALUE_PTR, .offset = val.offset };
			break;

		NOT_IMPLEMENTED_FOR(LVALUE_PTR);

		case EMPTY: assert(0 && "unreachable");
		}
		return true;
	}

	struct token minus;
	if (expect_token(p, &minus, TOK_MINUS)) {
		struct value val = {};
		if (!parse_unary(p, compiler, &val)) {
			errorf(minus, "expected atomic expression for unary minus\n");
			exit(1);
		}

		switch (val.kind) {
		case RVALUE:
		case LVALUE_AUTO:
			printf("\tneg QWORD [rbp-%zu]\n", val.offset);
			*result = val;
			return true;

		case LVALUE_PTR:
			mov_into_reg("rax", val);
			printf("\tneg rax\n");
			*result = (struct value) { .kind = RVALUE, .offset = alloc_stack(compiler) };
			printf("\tmov [rbp-%zu], rax\n", result->offset);
			return true;

		NOT_IMPLEMENTED_FOR(EMPTY);
		}
	}

	return parse_primary_expression(p, compiler, result);
}

bool parse_expression(struct parser *p, struct compiler *compiler, struct value *result)
{
	struct value lhs;
	if (parse_unary(p, compiler, &lhs)) {
		return maybe_parse_binary(p, compiler, result, lhs);
	}
	return false;
}

bool parse_switch(struct parser *p, struct compiler *compiler)
{
	struct token switch_;
	if (!expect_token(p, &switch_, TOK_SWITCH)) {
		return false;
	}

	struct token open;
	if (!expect_token(p, &open, TOK_PAREN_OPEN)) {
		errorf(open, "expected open paren after switch, got %s\n", token_short_name(open));
		exit(2);
	}

	enter_scope(compiler);

	struct value lhs;
	if (!parse_expression(p, compiler, &lhs)) {
		errorf(open, "expected value inside switch\n");
		exit(2);
	}

	struct token close;
	if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
		errorf(close, "expected close paren, got %s\n", token_short_name(close));
		notef(open, "paren was open here\n");
		exit(2);
	}

	struct control info;
	info.kind = TOK_SWITCH;
	info.lhs = lhs;
	info.next = compiler->last_local_id++;
	info.end = compiler->last_local_id++;
	da_append(&compiler->control, info);

	printf("\tjmp .local_%zu\n", info.next);

	if (!parse_statement(p, compiler)) {
		errorf(close, "expected statement after switch\n");
		exit(2);
	}

	assert(da_back(compiler->control).kind == TOK_SWITCH);
	printf(".local_%zu:\n", da_back(compiler->control).next);
	printf(".local_%zu:\n", da_back(compiler->control).end);

	leave_scope(compiler);
	compiler->control.count--;

	return true;
}

bool parse_while(struct parser *p, struct compiler *compiler)
{
	struct token while_;
	if (!expect_token(p, &while_, TOK_WHILE)) {
		return false;
	}

	struct token open;
	if (!expect_token(p, &open, TOK_PAREN_OPEN)) {
		errorf(open, "expected open paren after while, got %s\n", token_short_name(open));
		exit(2);
	}

	enter_scope(compiler);

	struct control info;
	info.kind = TOK_WHILE;
	info.next = compiler->last_local_id++;
	info.end = compiler->last_local_id++;
	da_append(&compiler->control, info);

	printf(".local_%zu:\n", info.next);

	struct value cond;
	if (!parse_expression(p, compiler, &cond)) {
		errorf(open, "expected condition inside while loop\n");
		exit(2);
	}

	mov_into_reg("rax", cond);
	printf("\tcmp rax, 0\n");
	printf("\tje .local_%zu\n", info.end);

	struct token close;
	if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
		errorf(close, "expected close paren, got %s\n", token_short_name(close));
		notef(open, "paren was open here\n");
		exit(2);
	}

	if (!parse_statement(p, compiler)) {
		errorf(close, "expected statement after while\n");
		exit(2);
	}
	leave_scope(compiler);
	printf("\tjmp .local_%zu\n", info.next);
	printf(".local_%zu:\n", info.end);


	return true;
}

bool parse_if(struct parser *p, struct compiler *compiler)
{
	struct token if_;
	if (!expect_token(p, &if_, TOK_IF)) {
		return false;
	}

	struct token open;
	if (!expect_token(p, &open, TOK_PAREN_OPEN)) {
		errorf(open, "expected open paren after if, got %s\n", token_short_name(open));
		exit(2);
	}

	enter_scope(compiler);

	size_t else_label = compiler->last_local_id++;
	size_t fi_label = compiler->last_local_id++;

	struct value cond;
	if (!parse_expression(p, compiler, &cond)) {
		errorf(open, "expected condition inside if statement\n");
		exit(2);
	}

	assert(cond.kind != EMPTY);
	mov_into_reg("rax", cond);
	printf("\tcmp rax, 0\n");
	printf("\tje .local_%zu\n", else_label);

	struct token close;
	if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
		errorf(close, "expected close paren, got %s\n", token_short_name(close));
		errorf(open, "paren was open here\n");
		exit(2);
	}

	if (!parse_statement(p, compiler)) {
		errorf(close, "expected statement after if\n");
		exit(2);
	}

	struct token else_;
	if (!expect_token(p, &else_, TOK_ELSE)) {
		leave_scope(compiler);
		printf(".local_%zu:\n", else_label);
		return true;
	}

	printf("\tjmp .local_%zu\n", fi_label);
	printf(".local_%zu:\n", else_label);

	if (!parse_statement(p, compiler)) {
		errorf(else_, "expected statement after else\n");
		exit(2);
	}

	leave_scope(compiler);
	printf(".local_%zu:\n", fi_label);
	return true;
}

bool parse_break(struct parser *p, struct compiler *compiler)
{
	struct token break_;
	if (!expect_token(p, &break_, TOK_BREAK)) {
		return false;
	}

	if (compiler->control.count-1 > compiler->control.count) {
		errorf(break_, "break outside of switch or while\n");
		exit(1);
	}

	printf("\tjmp .local_%zu\n", compiler->control.items[compiler->control.count-1].end);
	return true;
}

bool parse_continue(struct parser *p, struct compiler *compiler)
{
	struct token continue_;
	if (!expect_token(p, &continue_, TOK_CONTINUE)) {
		return false;
	}

	struct control *while_info = NULL;

	for (size_t i = compiler->control.count-1; i < compiler->control.count; --i) {
		struct control *ctrl = &compiler->control.items[i];
		if (ctrl->kind == TOK_WHILE) {
			while_info = ctrl;
			break;
		}
	}

	if (!while_info) {
		errorf(continue_, "continue outside of while\n");
		exit(1);
	}

	printf("\tjmp .local_%zu\n", while_info->next);
	return true;
}

bool parse_statement(struct parser *p, struct compiler *compiler)
{
	// TODO: Add compiler flag that would output this information
#if 0
	{
		printf("\t;");
		if (p->backlog.kind != TOK_EOF) {
			dump_location(stdout, p->backlog);
			char const* d = strchr(p->backlog.p, '\n');
			fwrite(p->backlog.p, 1, d == NULL ? strlen(p->backlog.p) : d - p->backlog.p, stdout);
		} else {
			char const *s = p->tokenizer.source + p->tokenizer.head;
			dump_location(stdout, (struct token) { .p = s });
			char const* e = strchr(s, '\n');
			fwrite(s, 1, e == NULL ? strlen(s) : e - s, stdout);
		}
		printf("\n");
	}
#endif

	bool done_something;
	do {
		done_something = false;

		struct token identifier, colon;
		if (expect_token2(p, &identifier, TOK_IDENTIFIER, &colon, TOK_COLON)) {
			done_something = true;

			struct label *found = NULL;
			for (size_t i = 0; i < compiler->function_labels.count; ++i) {
				if (strcmp(identifier.text, compiler->function_labels.items[i].name) == 0) {
					found = &compiler->function_labels.items[i];
				}
			}

			if (!found) {
				struct label new = { .defined = true, .name = identifier.text };
				da_append(&compiler->function_labels, new);
				found = &compiler->function_labels.items[compiler->function_labels.count-1];
			} else if (found->defined) {
				errorf(identifier, "label has already been defined");
				exit(1);
			} else {
				found->defined = true;
			}
			printf(".label_%zu:\n", found - compiler->function_labels.items);
			done_something = true;
		}

		struct token case_;
		if (expect_token(p, &case_, TOK_CASE)) {
			size_t after_test = compiler->last_local_id++;

			done_something = true;
			struct control *switch_info = NULL;

			for (size_t i = compiler->control.count - 1; i < compiler->control.count; --i) {
				struct control *ctrl = &compiler->control.items[i];
				if (ctrl->kind == TOK_SWITCH) {
					switch_info = ctrl;
					break;
				}
			}
			if (!switch_info) {
				errorf(case_, "case outside of switch\n");
				exit(1);
			}

			printf("\tjmp .local_%zu\n", after_test);
			printf(".local_%zu:\n", switch_info->next);
			switch_info->next = compiler->last_local_id++;

			struct value rhs;
			// TODO: Parsing wrong item here
			if (!parse_atomic(p, compiler, &rhs)) {
				errorf(case_, "expected constant expression\n");
				exit(1);
			}

			mov_into_reg("rax", switch_info->lhs);
			mov_into_reg("rcx", rhs);
			printf("\tcmp rax, rcx\n");
			printf("\tjne .local_%zu\n", switch_info->next);
			printf(".local_%zu:\n", after_test);

			struct token colon;
			if (!expect_token(p, &colon, TOK_COLON)) {
				errorf(colon, "expected :, got %s\n", token_short_name(colon));
				exit(1);
			}
		}
	} while (done_something);

	if (parse_empty_statement(p)
	|| parse_auto(p, compiler)
	|| parse_extern(p, compiler)
	|| parse_goto(p, compiler)
	|| parse_compund_statement(p, compiler)
	|| parse_break(p, compiler)
	|| parse_continue(p, compiler))
	{
		return true;
	}

	size_t stack_offset = compiler->stack_current_offset;

	if (parse_return(p, compiler) || parse_while(p, compiler) || parse_if(p, compiler) || parse_switch(p, compiler)) {
		compiler->stack_current_offset = stack_offset;
		return true;
	}

	// TODO: parse_expression_statement?
	struct value result;
	if (parse_expression(p, compiler, &result)) {
		struct token semicolon;
		if (!expect_token(p, &semicolon, TOK_SEMICOLON)) {
			errorf(semicolon, "expected ; at the end of the statement, got %s\n", token_short_name(semicolon));
			exit(2);
		}
		compiler->stack_current_offset = stack_offset;
		return true;
	}

	return false;
}

bool parse_function_definition(struct parser *p, struct compiler *compiler, struct token name)
{
	struct token open;
	if (!expect_token(p, &open, TOK_PAREN_OPEN)) {
		return false;
	}

	assert(compiler->nesting == 0);
	compiler->function_labels.count = 0;

	struct symbol fun = define_symbol(compiler, ((struct symbol) { .kind = GLOBAL, .name = name.text }), name);

	// TODO:

	printf("global %s\n", name.text);
	printf("%s:\n", name.text);
	printf("sym_%zu:\n", fun.id);
	printf("\tpush rbp\n");
	printf("\tmov rbp, rsp\n");
	printf("\tjmp .alloc_stack\n");
	printf(".start:\n");

	enter_scope(compiler);

	size_t arguments_count = 0;
	struct token args[ARRAY_LEN(ABI_REGISTERS)];
	for (;;) {
		struct token arg = {};
		if (expect_token(p, &arg, TOK_IDENTIFIER)) {
			assert(arguments_count < ARRAY_LEN(ABI_REGISTERS));
			args[arguments_count++] = arg;
		}

		struct token next;
		if (expect_token(p, &next, TOK_PAREN_CLOSE)) {
			break;
		} else if (arg.kind == TOK_IDENTIFIER && expect_token(p, &next, TOK_COMMA)) {
			continue;
		} else {
			errorf(next, "function definition expectes either close paren or comma, got %s\n", token_short_name(next));
			notef(open, "paren was open here\n");
			exit(2);
		}
	}

	for (size_t i = arguments_count - 1; i < arguments_count; --i) {
		struct token arg = args[i];
		struct symbol arg_sym = define_symbol(compiler, ((struct symbol) {
			.kind = LOCAL,
			.name = arg.text,
			.offset = alloc_stack(compiler) }),
			arg);
		printf("\tmov [rbp-%zu], %s\n", arg_sym.offset, ABI_REGISTERS[i]);
	}


	if (!parse_statement(p, compiler)) {
		struct token tok = peek_token(p);
		errorf(tok, "expected statement after function definition, got %s\n", token_short_name(tok));
		exit(1);
	}

	if (strcmp(name.text, "main") == 0) {
		printf("\txor rax, rax\n");
	}

	printf("\tleave\n");
	printf("\tret\n");

	// TODO: This is silly
	printf(".alloc_stack:\n");
	printf("\tsub rsp, %zu\n", compiler->stack_capacity);
	printf("\tjmp .start\n");

	leave_scope(compiler);
	compiler->stack_capacity = 0;
	compiler->stack_current_offset = 0;

	for (size_t i = 0; i < compiler->function_labels.count; ++i) {
		if (!compiler->function_labels.items[i].defined) {
			errorf(
				compiler->function_labels.items[i].first_usage,
				"label %s has not been defined inside the function %s\n",
				compiler->function_labels.items[i].name,
				name.text
			);
			exit(1);
		}
	}

	return true;
}


void parse_definition_value_list(struct parser *p, struct compiler *compiler, struct data *data)
{
	(void)compiler;

	struct token semicolon, value, separator;

	while (expect_token(p, &value, TOK_INTEGER) || expect_token(p, &value, TOK_STRING)) {
		da_append(data, value);
		if (expect_token(p, &separator, ',')) continue;
		break;
	}

	if (!expect_token(p, &semicolon, ';')) {
		errorf(semicolon, "expected ; at the end of global variable definition, got %s\n", token_short_name(semicolon));
		exit(1);
	}
}

bool parse_global_variable_definition(struct parser *p, struct compiler *compiler, struct token name)
{
	struct token open, close, size;
	struct data data = {};

	if (expect_token(p, &open, '[')) {
		if (!expect_token(p, &close, ']') && !expect_token2(p, &size, TOK_INTEGER, &close, ']')) {
			errorf(close, "expected closing bracket or integer size, got %s\n", token_short_name(close));
			notef(open, "bracket was opened here\n");
			exit(1);
		}
		data.is_vec = true;
		data.declared_size = size;
	}

	data.id = define_symbol(compiler, (struct symbol) { .kind = GLOBAL, .name = name.text }, name).id;
	parse_definition_value_list(p, compiler, &data);
	da_append(&compiler->data_section, data);

	return true;
}

bool parse_definition(struct parser *p, struct compiler *compiler)
{
	struct token name;
	if (!expect_token(p, &name, TOK_IDENTIFIER)) {
		return false;
	}

	return parse_function_definition(p, compiler, name)
		|| parse_global_variable_definition(p, compiler, name);
}

void parse_program(struct parser *p, struct compiler *compiler)
{
	while (parse_definition(p, compiler)) {}

	struct token eof;
	if (!expect_token(p, &eof, TOK_EOF)) {
		errorf(eof, "stray '%s' at the end of the program\n", token_short_name(eof));
		exit(1);
	}
}

void dump_token(FILE *out, struct token tok)
{
	dump_location(out, tok);
	switch (tok.kind) {
	case TOK_EOF:
		fprintf(out, "EOF\n");
		break;

	case TOK_IDENTIFIER:
		fprintf(out, "Identifier \"%s\"\n", tok.text);
		break;

	case TOK_INTEGER:
		fprintf(out, "Int \"%s\", %" PRIu64 "\n", tok.text, tok.ival);
		break;

	case TOK_CHARACTER:
		fprintf(out, "Char '%s', %" PRIu64 "\n", tok.text, tok.ival);
		break;

	case TOK_STRING:
		fprintf(out, "String \"%s\"\n", tok.text);
		break;

	default:
		for (size_t i = 0; i < ARRAY_LEN(SYMBOLS); ++i) {
			if (SYMBOLS[i].kind == tok.kind) {
				fprintf(out, "%s\n", SYMBOLS[i].string);
				return;
			}
		}
	}
}

uint64_t escape_seq(struct token character, char c)
{
	switch (c) {
	case '0':  return 0;
	case 'e':  return EOF;
	case 'n':  return '\n';
	case 'r':  return '\r';
	case '*':  return '*';
	case '"':  return '"';
	case '\'': return '\'';

	default:
		errorf(character, "unknown escape sequence *%c\n", c);
		exit(1);
	}
}

bool startswith(char const *str, char const *prefix)
{
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool scan_integer_literal(char const* source, uint64_t *result)
{
	char const *p = source;
	size_t base = 10;
	bool seen_digit = false;
	bool require_nonempty = false;

	if (startswith(p, "0x") || startswith(p, "0X")) {
		p += 2;
		base = 16;
		require_nonempty = true;
	} else if (startswith(p, "0o")) {
		p += 2;
		base = 8;
		require_nonempty = true;
	} else if (startswith(p, "0")) {
		base = 8;
	}

	*result = 0;

	for (; *p; ++p) {
		if (*p == '_') continue;

		if (__builtin_mul_overflow(*result, base, result)) {
			assert(0 && "overflow error");
		}

		uint64_t digit = *p;

		if (digit >= '0' && digit <= '9') {
			digit -= '0';
			seen_digit = true;
		} else if (digit >= 'a' && digit <= 'z') {
			digit = digit - 'a' + 10;
			seen_digit = true;
		} else if (digit >= 'A' && digit <= 'Z') {
			digit = digit - 'A' + 10;
			seen_digit = true;
		} else {
			return false;
		}

		if (digit >= base) {
			return false;
		}

		if (__builtin_add_overflow(*result, digit, result)) {
			assert(0 && "overflow error");
		}
	}

	return (require_nonempty || seen_digit) && *p == '\0';
}

static inline int next(struct tokenizer *ctx)
{
	return ctx->source[ctx->head] == '\0' ? '\0' : ctx->source[ctx->head++];
}

static inline bool consume_if(struct tokenizer *ctx, int(*predicate)(int))
{
	if (predicate(ctx->source[ctx->head])) {
		ctx->head++;
		return true;
	}
	return false;
}

static inline bool consume(struct tokenizer *ctx, char const* expected)
{
	if (startswith(&ctx->source[ctx->head], expected)) {
		ctx->head += strlen(expected);
		return true;
	}
	return false;
}

int isalnumor_(int c)
{
	return c == '_' || isalnum(c);
}

struct token scan(struct tokenizer *ctx)
{
	struct token ret = {};

	{
		bool done_something; do {
			done_something = false;
			while (consume_if(ctx, isspace)) done_something = true;
			if (consume(ctx, "/*")) {
				done_something = true;
				while (!consume(ctx, "*/") && next(ctx));
			}
		} while (done_something);
	}

	if (!ctx->source[ctx->head]) {
		return ret;
	}

	ret.p = &ctx->source[ctx->head];

	for (size_t i = 0; i < ARRAY_LEN(SYMBOLS); ++i) {
		if (consume(ctx, SYMBOLS[i].string)) {
			ret.kind = SYMBOLS[i].kind;
			return ret;
		}
	}

	if (consume(ctx, "\"")) {
		ret.kind = TOK_STRING;
		struct string_builder sb = {};
		for (char c;;) {
			switch (c = next(ctx)) {
			case '\0':
				errorf(ret, "expected end of string literal, got end of file\n");
				exit(1);

			case '\"':
				da_append(&sb, '\0');
				ret.text = inter(sb.items);
				return ret;

			case '*':
				c = next(ctx);
				da_append(&sb, escape_seq(ret, c));
				break;

			default:
				da_append(&sb, c);
			}
		}
		return ret;
	}

	if (consume(ctx, "'")) {
		ret.kind = TOK_CHARACTER;
		int c;

		switch (c = next(ctx)) {
		case '\'':
			errorf(ret, "empty character constant\n");
			exit(1);

		case '*':
			ret.text = strdup((char[]) { '*', escape_seq(ret, next(ctx)), '\0' });
			ret.ival = ret.text[1];
			break;


		default:
			ret.text = strdup((char[]) { c, '\0' });
			ret.ival = c;
			break;
		}

		if (!consume(ctx, "'")) {
			errorf(ret, "multicharacter character constants are not implemented yet\n");
			exit(1);
		}
		return ret;
	}

	// TODO: Support UTF-8
	// TODO: Use consume_if
	char const* s = &ctx->source[ctx->head];
	size_t i = 0;
	while (consume_if(ctx, isalnumor_)) ++i;

	ret.text = strndup(s, i);

	if (scan_integer_literal(ret.text, &ret.ival)) {
		ret.kind = TOK_INTEGER;
		return ret;
	}

	if (i != 0) {
		ret.kind = TOK_IDENTIFIER;
		return ret;
	}

	errorf(ret, "unknown character '%c' (%d)\n", ctx->source[ctx->head], ctx->source[ctx->head]);
	exit(1);
}
