#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	} else if ((where)->capacity > (where)->count+1) { \
		(where)->capacity *= 2; \
		(where)->items = realloc((where)->items, sizeof(*(where)->items) * (where)->capacity); \
	} \
	(where)->items[(where)->count++] = (what); \
} while(0)

struct string_pool
{
	char const* str;
	size_t strlen, total;
	struct string_pool *next;
};

static struct string_pool *string_intering_pool = NULL;

char const* inter(char const *str)
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

		TOK_CURLY_CLOSE = '}',
		TOK_CURLY_OPEN = '{',
		TOK_DIV = '/',
		TOK_MINUS = '-',
		TOK_PAREN_CLOSE = ')',
		TOK_PAREN_OPEN = '(',
		TOK_SEMICOLON = ';',
		TOK_ASSIGN = '=',
		TOK_COMMA = ',',
		TOK_PLUS = '+',

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

		// Literals:
		TOK_INTEGER,
		TOK_CHARACTER,
		TOK_STRING,
	} kind;

	char const* text;
	uint64_t ival;

	int line, column;
	char const* filename;
};

struct tokenizer
{
	FILE *source;
	char const* filename;
	int line, column;
	struct string_pool strings;
};

struct token scan(struct tokenizer *ctx);
void dump_token(FILE *out, struct token tok);
char const* token_short_name(struct token tok);

static char const* KEYWORDS[] = {
	[TOK_AUTO]   = "auto",
	[TOK_CASE]   = "case",
	[TOK_ELSE]   = "else",
	[TOK_EXTRN]  = "extrn",
	[TOK_GOTO]   = "goto",
	[TOK_IF]     = "if",
	[TOK_RETURN] = "return",
	[TOK_SWITCH] = "switch",
	[TOK_WHILE]  = "while",
};

struct parser
{
	struct tokenizer tokenizer;
	struct token backlog;
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

	size_t stack_capacity;
	size_t stack_offset;
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
};

size_t alloc_stack(struct compiler *compiler)
{
	if (compiler->stack_capacity == 0) {
		compiler->stack_capacity = 16;
		printf("\tsub rsp, %zu\n", compiler->stack_capacity);
	}

	size_t offset = compiler->stack_current_offset;
	compiler->stack_current_offset += sizeof(uint64_t);
	if (compiler->stack_current_offset > compiler->stack_capacity) {
		printf("\tadd rsp, -%zu\n", compiler->stack_capacity);
		compiler->stack_capacity *= 2;
	}
	return offset;
}

void enter_scope(struct compiler *compiler)
{
	++compiler->nesting;
	assert(compiler->nesting < MAX_SCOPE_NESTING);
	compiler->scope[compiler->nesting].count = 0;
	compiler->scope[compiler->nesting].stack_capacity = compiler->stack_capacity;
	compiler->scope[compiler->nesting].stack_offset = compiler->stack_current_offset;
}

void leave_scope(struct compiler *compiler)
{
	assert(compiler->nesting > 0 && "Trying to leave when in global scope");
	size_t prev_capacity = compiler->scope[compiler->nesting].stack_capacity;
	if (compiler->stack_capacity > prev_capacity) {
		printf("\tadd rsp, %zu\n", compiler->stack_capacity - prev_capacity);
		compiler->stack_capacity = prev_capacity;
	}
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
		fprintf(stderr, "%s:%d:%d: error: symbol %s has already been defined\n", name.filename, name.line, name.column, name.text);
		exit(1);
	}
	++compiler->last_symbol_id;
	assert(compiler->last_symbol_id > 0);
	symbol.id = compiler->last_symbol_id;
	da_append(&compiler->scope[compiler->nesting], symbol);
	return symbol;
}

struct token get_token(struct parser *p);
void putback_token(struct parser *p, struct token tok);
bool expect_token(struct parser *p, struct token *tok, enum token_kind kind);

void parse_program(struct parser *p, struct compiler *compiler);
bool parse_statement(struct parser *p, struct compiler *compiler);

int main()
{
#if 0
	struct tokenizer tokctx = {
		.source = stdin,
		.filename = "(stdin)",
		.line = 1,
		.column = 1,
	};
	struct token tok;

	while ((tok = scan(&tokctx)).kind != TOK_EOF) {
		dump_token(stdout, tok);
	}

	int i = 0;
	printf("string intering pool:\n");
	for (struct string_pool *p = string_intering_pool; p != NULL; p = p->next) {
		printf("[%d] = \"%s\"\n", i++, p->str);
	}

#else
	struct compiler compiler = {
	};

	struct parser parser = {
		.tokenizer = {
			.source = stdin,
			.filename = "(stdin)",
			.line = 1,
			.column = 1,
		},
	};

	printf("format ELF64\n");
	printf("section \".text\" executable\n");
	parse_program(&parser, &compiler);

	printf("section \".rodata\"\n");

	if (string_intering_pool) {
		printf("db 0x00");
		for (struct string_pool *p = string_intering_pool; p != NULL; p = p->next) {
			for (char const *s = p->str; *s; ++s) {
				printf(",0x%02x", *s);
			}
			printf(",0x00");
		}
		printf("\nstrend = $\n");
	}

#endif
}

struct token get_token(struct parser *p)
{
	if (p->backlog.kind == TOK_EOF)
		return scan(&p->tokenizer);
	struct token tok = p->backlog;
	p->backlog = (struct token){};
	return tok;
}

struct token peek_token(struct parser *p)
{
	if (p->backlog.kind != TOK_EOF) {
		p->backlog = scan(&p->tokenizer);
	}
	return p->backlog;
}

void putback_token(struct parser *p, struct token tok)
{
	assert(p->backlog.kind == TOK_EOF);
	p->backlog = tok;
}

bool expect_token(struct parser *p, struct token *tok, enum token_kind kind)
{
	*tok = get_token(p);
	if (tok->kind == kind) {
		return true;
	}
	putback_token(p, *tok);
	return false;
}

bool parse_rvalue(struct parser *p, struct compiler *compiler, size_t target);

bool parse_return(struct parser *p, struct compiler *compiler)
{
	struct token return_;
	if (!expect_token(p, &return_, TOK_RETURN)) {
		return false;
	}

	struct token open, semicolon;
	if (expect_token(p, &open, TOK_PAREN_OPEN)) {
		size_t retval = alloc_stack(compiler);

		if (!parse_rvalue(p, compiler, retval)) {
			fprintf(stderr, "%s:%d:%d: error: expected rvalue\n", open.filename, open.line, open.column);
			exit(1);
		}

		printf("\tmov rax, [rbp-%zu]\n", retval);
		printf("\tleave\n");
		printf("\tret\n");

		struct token close;
		if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
			fprintf(stderr, "%s:%d:%d: error: expected close paren, got %s\n", close.filename, close.line, close.column, token_short_name(close));
			fprintf(stderr, "%s:%d:%d: note: paren was open here\n", open.filename, open.line, open.column);
			exit(2);
		}
		if (!expect_token(p, &semicolon, TOK_SEMICOLON)) {
			fprintf(stderr, "%s:%d:%d: error: return expect ; after closing ), got %s\n", semicolon.filename, semicolon.line, semicolon.column, token_short_name(semicolon));
			exit(2);
		}
	} else if (expect_token(p, &semicolon, TOK_SEMICOLON)) {
		printf("\tleave\n");
		printf("\tret\n");
	} else {
		fprintf(stderr, "%s:%d:%d: error: return expect ; or (, got %s\n", return_.filename, return_.line, return_.column, token_short_name(semicolon));
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

	struct token name;
	if (expect_token(p, &name, TOK_IDENTIFIER)) {
		define_symbol(compiler, (struct symbol) { .name = name.text, .kind = EXTERNAL }, name);

		bool found = false;
		for (size_t i = 0; i < compiler->defined_externs.count; ++i) {
			if (strcmp(compiler->defined_externs.items[i], name.text) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			printf("\textrn %s\n", name.text);
			da_append(&compiler->defined_externs, name.text);
		}
		struct token semicolon;
		if (!expect_token(p, &semicolon, TOK_SEMICOLON)) {
			fprintf(stderr, "%s:%d:%d: error: extrn expect ; after closing ), got %s\n", semicolon.filename, semicolon.line, semicolon.column, token_short_name(semicolon));
			exit(2);
		}
		return parse_statement(p, compiler);
	} else {
		fprintf(stderr, "%s:%d:%d: error: extrn expected identifier, got %s\n", extrn.filename, extrn.line, extrn.column, token_short_name(name));
	}

	return true;
}

bool parse_auto(struct parser *p, struct compiler *compiler)
{
	struct token auto_;
	if (!expect_token(p, &auto_, TOK_AUTO)) {
		return false;
	}

	struct token name;
	if (expect_token(p, &name, TOK_IDENTIFIER)) {
		struct symbol s = define_symbol(compiler, (struct symbol) { .name = name.text, .kind = LOCAL, .offset = alloc_stack(compiler) }, name);
		printf("\t; auto [rbp-%zu] = %s\n", s.offset, name.text);

		struct token semicolon;
		if (!expect_token(p, &semicolon, TOK_SEMICOLON)) {
			fprintf(stderr, "%s:%d:%d: error: auto expect ; after closing ), got %s\n", semicolon.filename, semicolon.line, semicolon.column, token_short_name(semicolon));
			exit(2);
		}
		return parse_statement(p, compiler);
	} else {
		fprintf(stderr, "%s:%d:%d: error: auto expected identifier, got %s\n", auto_.filename, auto_.line, auto_.column, token_short_name(name));
	}

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
		fprintf(stderr, "%s:%d:%d: error: expected close curly or statement, got %s\n", close.filename, close.line, close.column, token_short_name(close));
		fprintf(stderr, "%s:%d:%d: note: curly was open here\n", open.filename, open.line, open.column);
		exit(2);
	}
	return true;
}

bool parse_funccall(struct parser *p, struct compiler *compiler, size_t target, struct symbol *symbol)
{
	struct token open;
	if (!expect_token(p, &open, TOK_PAREN_OPEN)) {
		return false;
	}

	char const* call_registers[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };

	size_t arg = alloc_stack(compiler);

	for (size_t i = 0; i < ARRAY_LEN(call_registers); ++i) {
		if (i > 0) {
			struct token comma;
			if (!expect_token(p, &comma, TOK_COMMA)) {
				if (comma.kind == TOK_PAREN_CLOSE) break;
				fprintf(stderr, "%s:%d:%d: error: expected comma, got %s\n", comma.filename, comma.line, comma.column, token_short_name(comma));
				exit(2);
			}
		}

		if (!parse_rvalue(p, compiler, arg)) {
			break;
		}
		printf("\tmov %s, [rbp-%zu]\n", call_registers[i], arg);
	}

	struct token close;
	if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
		fprintf(stderr, "%s:%d:%d: error: expected close paren, got %s\n", close.filename, close.line, close.column, token_short_name(close));
		fprintf(stderr, "%s:%d:%d: note: paren was open here\n", open.filename, open.line, open.column);
		exit(2);
	}

	switch (symbol->kind) {
		case EXTERNAL: printf("\tcall PLT %s\n", symbol->name); break;
		case GLOBAL: printf("\tcall sym_%zu\n", symbol->id); break;
		case LOCAL: printf("\tlea rax, QWORD [rbp-%zu]\n\tcall rax\n", symbol->offset); break;
	}

	printf("\tmov [rbp-%zu], rax\n", target);

	return true;
}

bool parse_assign(struct parser *p, struct compiler *compiler, size_t lvalue)
{
	struct token assign;
	if (!expect_token(p, &assign, TOK_ASSIGN))
		return false;

	return parse_rvalue(p, compiler, lvalue);
}

bool parse_lvalue(struct parser *p, struct compiler *compiler, struct symbol **symbol)
{
	// TODO: Implement dereference and indexing
	struct token name;
	if (!expect_token(p, &name, TOK_IDENTIFIER)) {
		return false;
	}
	*symbol = search_symbol(compiler, name.text);
	if (!*symbol) {
		fprintf(stderr, "%s:%d:%d: error: '%s' has not been defined yet\n", name.filename, name.line, name.column, name.text);
		exit(1);
	}
	return true;
}

bool parse_constant(struct parser *p, struct compiler *compiler, size_t target)
{
	(void)compiler;
	struct token constant;
	if (expect_token(p, &constant, TOK_INTEGER)) {
		printf("\tmov QWORD [rbp-%zu], %"PRIu64"\n", target, constant.ival);
		return true;
	}
	if (expect_token(p, &constant, TOK_CHARACTER)) {
		printf("\tmov QWORD [rbp-%zu], %"PRIu64"\n", target, constant.ival);
		return true;
	}
	if (expect_token(p, &constant, TOK_STRING)) {
		printf("\tlea rax, [strend-%zu]\n", string_offset(constant.text));
		printf("\tmov [rbp-%zu], rax\n", target);
		return true;
	}

	return false;
}

bool maybe_parse_addition_or_subtraction(struct parser *p, struct compiler *compiler, size_t target)
{
	struct token op;
	if (expect_token(p, &op, TOK_PLUS) || expect_token(p, &op, TOK_MINUS)) {
		size_t tmp = alloc_stack(compiler);

		if (!parse_rvalue(p, compiler, tmp)) {
			fprintf(stderr, "%s:%d:%d: expected rvalue on the right of %s\n", op.filename, op.line, op.column, token_short_name(op));
			exit(1);
		}

		printf("\tmov rax, [rbp-%zu]\n", target);
		printf("\t%s rax, [rbp-%zu]\n", op.kind == TOK_PLUS ? "add" : "sub", tmp);
		printf("\tmov [rbp-%zu], rax\n", target);
	}

	return true;
}

bool parse_rvalue(struct parser *p, struct compiler *compiler, size_t target)
{
	struct symbol *symbol;

	if (parse_constant(p, compiler, target)) {
		return maybe_parse_addition_or_subtraction(p, compiler, target);
	}

	if (!parse_lvalue(p, compiler, &symbol)) {
		return false;
	}

	if (parse_funccall(p, compiler, target, symbol))
		return true;

	assert(symbol->kind == LOCAL && "external or global symbols are only supported by function calls for now");

	if (parse_assign(p,compiler, symbol->offset)) {
		return true;
	}

	printf("\tmov rax, [rbp-%zu]\n\tmov [rbp-%zu], rax\n", symbol->offset, target);
	return maybe_parse_addition_or_subtraction(p, compiler, target);
}

bool parse_while(struct parser *p, struct compiler *compiler)
{
	struct token while_;
	if (!expect_token(p, &while_, TOK_WHILE)) {
		return false;
	}

	struct token open;
	if (!expect_token(p, &open, TOK_PAREN_OPEN)) {
		fprintf(stderr, "%s:%d:%d: error: expected open paren after while, got %s\n", open.filename, open.line, open.column, token_short_name(open));
		exit(2);
	}


	size_t cond = alloc_stack(compiler);

	enter_scope(compiler);

	size_t loop_again = compiler->last_local_id++;
	size_t loop_exit = compiler->last_local_id++;

	printf(".local_%zu:\n", loop_again);

	if (!parse_rvalue(p, compiler, cond)) {
		fprintf(stderr, "%s:%d:%d: error: expected condition inside while loop\n", open.filename, open.line, open.column);
		exit(2);
	}

	printf("\tmov rax, [rbp-%zu]\n", cond);
	printf("\tcmp rax, 0\n");
	printf("\tje .local_%zu\n", loop_exit);

	struct token close;
	if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
		fprintf(stderr, "%s:%d:%d: error: expected close paren, got %s\n", close.filename, close.line, close.column, token_short_name(close));
		fprintf(stderr, "%s:%d:%d: note: paren was open here\n", open.filename, open.line, open.column);
		exit(2);
	}

	if (!parse_statement(p, compiler)) {
		fprintf(stderr, "%s:%d:%d: error: expected statement after while\n", close.filename, close.line, close.column);
		exit(2);
	}
	leave_scope(compiler);
	printf("\tjmp .local_%zu\n", loop_again);
	printf(".local_%zu:\n", loop_exit);


	return true;
}

bool parse_statement(struct parser *p, struct compiler *compiler)
{
	if (parse_empty_statement(p)
	|| parse_auto(p, compiler)
	|| parse_extern(p, compiler)
	|| parse_compund_statement(p, compiler))
	{
		return true;
	}

	size_t stack_offset = compiler->stack_current_offset;

	if (parse_return(p, compiler) || parse_while(p, compiler)) {
		compiler->stack_current_offset = stack_offset;
		return true;
	}

	if (parse_rvalue(p, compiler, alloc_stack(compiler))) {
		struct token semicolon;
		if (!expect_token(p, &semicolon, TOK_SEMICOLON)) {
			fprintf(stderr, "%s:%d:%d: error: expected ; at the end of the statement, got %s\n", semicolon.filename, semicolon.line, semicolon.column, token_short_name(semicolon));
			exit(2);
		}
		compiler->stack_current_offset = stack_offset;
		return true;
	}

	return false;
}

bool parse_definition(struct parser *p, struct compiler *compiler)
{
	struct token name;
	if (!expect_token(p, &name, TOK_IDENTIFIER)) {
		return false;
	}

	struct token open;
	if (expect_token(p, &open, TOK_PAREN_OPEN)) {
		struct token close;
		if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
			fprintf(stderr, "%s:%d:%d: error: definition expected close paren, got %s\n", close.filename, close.line, close.column, token_short_name(close));
			fprintf(stderr, "%s:%d:%d: note: paren was open here\n", open.filename, open.line, open.column);
			exit(2);
		}

		assert(compiler->nesting == 0);
		struct symbol fun = define_symbol(compiler, ((struct symbol) { .kind = GLOBAL, .name = name.text }), name);

		printf("public sym_%zu as '%s'\n", fun.id, name.text);
		printf("sym_%zu:\n", fun.id);
		printf("\tpush rbp\n");
		printf("\tmov rbp, rsp\n");

		enter_scope(compiler);
		if (!parse_statement(p, compiler)) {
			struct token tok = peek_token(p);
			fprintf(stderr, "%s:%d:%d: error: expected statement after function definition, got %s\n", tok.filename, tok.line, tok.column, token_short_name(tok));
			exit(1);
		}
		leave_scope(compiler);

		if (strcmp(name.text, "main") == 0) {
			printf("\tleave\n");
			printf("\txor rax, rax\n");
			printf("\tret\n");
		}
		return true;
	} else {
		assert(0 && "other definition syntax not implemented yet");
	}
}

void parse_program(struct parser *p, struct compiler *compiler)
{
	while (parse_definition(p, compiler)) {}
	assert(peek_token(p).kind == TOK_EOF);
}

char const* token_short_name(struct token tok)
{
	switch (tok.kind) {
	case TOK_COMMA: return ",";
	case TOK_EOF: return "end of file";
	case TOK_IDENTIFIER: return "identifier";
	case TOK_INTEGER: return "integer literal";
	case TOK_CHARACTER: return "character literal";
	case TOK_CURLY_CLOSE: return "}";
	case TOK_CURLY_OPEN: return "{";
	case TOK_DIV: return "/";
	case TOK_MINUS: return "-";
	case TOK_PAREN_CLOSE: return ")";
	case TOK_PAREN_OPEN: return "(";
	case TOK_SEMICOLON: return ";";
	case TOK_ASSIGN: return "=";
	case TOK_PLUS: return "+";
	case TOK_AUTO: return "auto keyword";
	case TOK_CASE: return "case keyword";
	case TOK_ELSE: return "else keyword";
	case TOK_EXTRN: return "extrn keyword";
	case TOK_GOTO: return "goto keyword";
	case TOK_IF: return "if keyword";
	case TOK_RETURN: return "return keyword";
	case TOK_SWITCH: return "switch keyword";
	case TOK_WHILE: return "while keyword";
	case TOK_STRING: return "string literal";
	}

	assert(0 && "unreachable");
}

void dump_token(FILE *out, struct token tok)
{
	fprintf(out, "%s:%d:%d: ", tok.filename, tok.line, tok.column);
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

	case TOK_AUTO:
	case TOK_CASE:
	case TOK_ELSE:
	case TOK_EXTRN:
	case TOK_GOTO:
	case TOK_IF:
	case TOK_RETURN:
	case TOK_SWITCH:
	case TOK_WHILE:
		fprintf(out, "%s\n", KEYWORDS[tok.kind]);
		break;

	case TOK_COMMA:
	case TOK_CURLY_CLOSE:
	case TOK_CURLY_OPEN:
	case TOK_DIV:
	case TOK_MINUS:
	case TOK_PAREN_CLOSE:
	case TOK_PAREN_OPEN:
	case TOK_SEMICOLON:
	case TOK_ASSIGN:
	case TOK_PLUS:
		fprintf(out, "%c\n", tok.kind);
		break;
	}
}

uint64_t escape_seq(char c, char const* filename, int line, int column)
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
		fprintf(stderr, "%s:%d:%d: error: unknown escape sequence *%c\n", filename, line, column, c);
		exit(1);
	}
}

struct token scan(struct tokenizer *ctx)
{
	int c;
	struct string_builder buf = {};
	struct token ret = {
		.filename = ctx->filename,
	};

again:
	switch (c = fgetc(ctx->source)) {
	case EOF:
		return ret;

#define ASCII(T) \
	case T: ret.column = ctx->column++; ret.line = ctx->line; ret.kind = T; return ret

	ASCII(TOK_CURLY_CLOSE);
	ASCII(TOK_CURLY_OPEN);
	ASCII(TOK_MINUS);
	ASCII(TOK_PAREN_CLOSE);
	ASCII(TOK_PAREN_OPEN);
	ASCII(TOK_SEMICOLON);
	ASCII(TOK_ASSIGN);
	ASCII(TOK_COMMA);
	ASCII(TOK_PLUS);
#undef ASCII

	case '/':
		if ((c = fgetc(ctx->source)) == '*') {
			ctx->column += 2;

			char prev = '\0';
			while ((c = fgetc(ctx->source)) != EOF && !(prev == '*' && c == '/')) {
				switch (c) {
				case '\n': ctx->line++; __attribute__ ((fallthrough));
				case '\r': ctx->column = 1; break;
				default: ctx->column++;
				}
				prev = c;
			}
			goto again;
		} else {
			ungetc(c, ctx->source);
			ret.column = ctx->column++;
			ret.line = ctx->line;
			ret.kind = TOK_DIV;
			return ret;
		}

	case '\n':
		ctx->line++; __attribute__ ((fallthrough));
	case '\r':
		ctx->column = 1;
		goto again;

	case '"':
		ret.column = ctx->column++;
		ret.line = ctx->line;
		ret.kind = TOK_STRING;

		{
			struct string_builder sb = {};

			for (;;) {
				switch (c = fgetc(ctx->source)) {
				case EOF:
					fprintf(stderr, "%s:%d:%d: error: expected end of string literal, got end of file\n", ctx->filename, ctx->line, ctx->column);
					exit(1);

				case '\"':
					ctx->column++;
					da_append(&sb, '\0');
					ret.text = inter(sb.items);
					return ret;

				case '*':
					ctx->column++;
					c = fgetc(ctx->source);
					da_append(&sb, escape_seq(c, ctx->filename, ctx->line, ctx->column));
					break;

				case '\n':
					ctx->line++; __attribute__ ((fallthrough));
				case '\r':
					ctx->column = 1;
					da_append(&sb, c);
					break;

				default:
					ctx->column++;
					da_append(&sb, c);
				}
			}
		}
		break;

	case '\'':
		ret.column = ctx->column++;
		ret.line = ctx->line;
		ret.kind = TOK_CHARACTER;

		switch (c = fgetc(ctx->source)) {
		case '\'':
			fprintf(stderr, "%s:%d:%d: error: empty character constant\n", ret.filename, ret.line, ret.column);
			exit(1);

		case '*':
			ctx->column++;
			ret.text = strdup((char[]) { '*', escape_seq(fgetc(ctx->source), ctx->filename, ctx->line, ctx->column), '\0' });
			ctx->column++;
			break;


		default:
			ctx->column++;
			ret.text = strdup((char[]) { c, '\0' });
			ret.ival = c;
			break;
		}

		if (fgetc(ctx->source) != '\'') {
			fprintf(stderr, "%s:%d:%d: error: multicharacter character constants are not implemented yet\n", ret.filename, ret.line, ret.column);
			exit(1);
		}
		ctx->column++;

		return ret;


	default:
		if (isspace(c)) {
			ctx->column++;
			goto again;
		}

		if (isdigit(c)) {
			ret.column = ctx->column++;
			ret.line = ctx->line;

			da_append(&buf, c);
			while ((c = fgetc(ctx->source)) != EOF && isdigit(c)) {
				da_append(&buf, c);
				ctx->column++;
			}
			da_append(&buf, '\0');
			ungetc(c, ctx->source);
			ret.kind = TOK_INTEGER;
			ret.text = buf.items;
			int r = sscanf(ret.text, "%lu", &ret.ival);
			assert(r == 1);
			return ret;
		}

		if (isalpha(c) || c == '_') {
			ret.column = ctx->column++;
			ret.line = ctx->line;

			da_append(&buf, c);
			while ((c = fgetc(ctx->source)) != EOF && (isalnum(c) || c == '_')) {
				da_append(&buf, c);
				ctx->column++;
			}
			da_append(&buf, '\0');
			ungetc(c, ctx->source);
			ret.kind = TOK_IDENTIFIER;
			ret.text = buf.items;

			for (size_t i = 0; i < ARRAY_LEN(KEYWORDS); ++i) {
				if (KEYWORDS[i] && strcmp(KEYWORDS[i], ret.text) == 0) {
					ret.kind = i;
					break;
				}
			}
			return ret;
		}
	}

	fprintf(stderr, "%s:%d:%d: error: unknown character '%c'\n", ctx->filename, ctx->line, ctx->column, c);
	exit(1);
}
