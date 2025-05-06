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
	enum symbol_kind { EXTERNAL, OURS, } kind;
	char const* name;
};

struct symbols
{
	struct symbol *items;
	size_t capacity, count;
};

struct compiler
{
	struct symbols symbols;
};

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

bool parse_return(struct parser *p)
{
	struct token return_;
	if (!expect_token(p, &return_, TOK_RETURN)) {
		return false;
	}

	struct token open, semicolon;
	if (expect_token(p, &open, TOK_PAREN_OPEN)) {
		struct token integer;
		if (expect_token(p, &integer, TOK_INTEGER)) {
			printf("\tmov rax, %"PRIu64"\n", integer.ival);
			printf("\tmov rsp, rbp\n");
			printf("\tpop rbp\n");
			printf("\tret\n");
		} else {
			assert(0 && "Expression parsing is not supported yet");
		}

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
		printf("\tmov rsp, rbp\n");
		printf("\tpop rbp\n");
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
		bool found = false;
		for (size_t i = 0; i < compiler->symbols.count; ++i) {
			if (strcmp(compiler->symbols.items[i].name, name.text) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			printf("\textrn %s\n", name.text);
			da_append(&compiler->symbols, ((struct symbol) { .kind = EXTERNAL, .name = name.text }));
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

bool parse_funccall(struct parser *p, struct compiler *compiler)
{
	struct token name, open;
	if (!expect_token(p, &name, TOK_IDENTIFIER)) {
		return false;
	}
	if (!expect_token(p, &open, TOK_PAREN_OPEN)) {
		fprintf(stderr, "%s:%d:%d: error: expected open paren for funccall, got %s\n", open.filename, open.line, open.column, token_short_name(open));
		exit(2);
	}

	struct token character;
	if (expect_token(p, &character, TOK_CHARACTER)) {
		printf("\tmov rdi, %"PRIu64"\n", character.ival);
	}

	struct token close;
	if (!expect_token(p, &close, TOK_PAREN_CLOSE)) {
		fprintf(stderr, "%s:%d:%d: error: expected close paren, got %s\n", close.filename, close.line, close.column, token_short_name(close));
		fprintf(stderr, "%s:%d:%d: note: paren was open here\n", open.filename, open.line, open.column);
		exit(2);
	}

	struct symbol *s = NULL;
	size_t i;
	for (i = 0; i < compiler->symbols.count; ++i) {
		if (strcmp(compiler->symbols.items[i].name, name.text) == 0) {
			s = &compiler->symbols.items[i];
			break;
		}
	}
	if (s == NULL) {
		fprintf(stderr, "%s:%d:%d: error: unknown symbol '%s'\n", name.filename, name.line, name.column, name.text);
		exit(1);
	}
	switch (s->kind) {
		case OURS: printf("\tcall fun_%d\n", (int)i); break;
		case EXTERNAL: printf("\tcall PLT %s\n", name.text); break;
	}

	struct token semicolon;
	if (!expect_token(p, &semicolon, TOK_SEMICOLON)) {
		fprintf(stderr, "%s:%d:%d: error: funccall expect ; after closing ), got %s\n", semicolon.filename, semicolon.line, semicolon.column, token_short_name(semicolon));
		exit(2);
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

	while (parse_statement(p, compiler))
		;

	struct token close;
	if (!expect_token(p, &close, TOK_CURLY_CLOSE)) {
		fprintf(stderr, "%s:%d:%d: error: expected close curly or statement, got %s\n", close.filename, close.line, close.column, token_short_name(close));
		fprintf(stderr, "%s:%d:%d: note: curly was open here\n", open.filename, open.line, open.column);
		exit(2);
	}
	return true;
}

bool parse_statement(struct parser *p, struct compiler *compiler)
{
	return parse_empty_statement(p)
		|| parse_compund_statement(p, compiler)
		|| parse_return(p)
		|| parse_extern(p, compiler)
		|| parse_funccall(p, compiler);
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

		bool found = false;
		for (size_t i = 0; i < compiler->symbols.count; ++i) {
			if (strcmp(compiler->symbols.items[i].name, name.text) == 0) {
				found = true;
				break;
			}
		}
		if (found) {
			fprintf(stderr, "%s:%d:%d: error: redefinition\n", name.filename, name.line, name.column);
			exit(1);
		}

		da_append(&compiler->symbols, ((struct symbol) { .kind = OURS, .name = name.text }));

		printf("public fun_%d as '%s'\n", (int)compiler->symbols.count-1, name.text);
		printf("fun_%d:\n", (int)compiler->symbols.count-1);
		printf("\tpush rbp\n");
		printf("\tmov rbp, rsp\n");

		if (parse_statement(p, compiler)) {
			if (strcmp(name.text, "main") == 0) {
				printf("\tmov rsp, rbp\n");
				printf("\tpop rbp\n");
				printf("\txor rax, rax\n");
				printf("\tret\n");
			}
		} else {
			struct token tok = peek_token(p);
			fprintf(stderr, "%s:%d:%d: error: expected statement after function definition, got %s\n", tok.filename, tok.line, tok.column, token_short_name(tok));
			exit(1);
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
	case TOK_AUTO: return "auto keyword";
	case TOK_CASE: return "case keyword";
	case TOK_ELSE: return "else keyword";
	case TOK_EXTRN: return "extrn keyword";
	case TOK_GOTO: return "goto keyword";
	case TOK_IF: return "if keyword";
	case TOK_RETURN: return "return keyword";
	case TOK_SWITCH: return "switch keyword";
	case TOK_WHILE: return "while keyword";
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

	case TOK_CURLY_CLOSE:
	case TOK_CURLY_OPEN:
	case TOK_DIV:
	case TOK_MINUS:
	case TOK_PAREN_CLOSE:
	case TOK_PAREN_OPEN:
	case TOK_SEMICOLON:
		fprintf(out, "%c\n", tok.kind);
		break;
	}
}

struct token scan(struct tokenizer *ctx)
{
	char c;
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
			c = fgetc(ctx->source);
			switch (c) {
			case '0': ret.ival = 0; break;
			case 'e': ret.ival = EOF; break;
			case 'n': ret.ival = '\n'; break;
			case 'r': ret.ival = '\r'; break;
			case '*': ret.ival = '*'; break;

			default:
				fprintf(stderr, "%s:%d:%d: error: unknown escape sequence *%c\n", ret.filename, ret.line, ret.column, c);
			}
			ret.text = strdup((char[]) { '*', c, '\0' });
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
