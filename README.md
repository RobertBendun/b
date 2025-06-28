# B Programming Language

Implementation of a language similar to the B as a recreational excercise.
The goal is to represent the spirit of the language and there is no need to match it perfectly.
It's B for modern systems.

## Examples

### `hello, world`

```c
/* Prints hello, world and a newline to the standard output using libc */
main()
	extrn printf;
{
    printf("hello, world*n");
}
```

### Variadic functions

```c
max(x0,x1,x2,x3,x4,x5) extrn printf; {
	auto i, p, m;
	p = &x0;
	m = p[0];

	i = 0;
	while (p[i] != 0) {
		if (p[i] > m) m = p[i];
		++i;
	}

	return(m);
}


main() extrn printf; {
	printf("max(1, 2) = %lld*n", max(1, 2, 0));
	printf("max(1, 3, 2) = %lld*n", max(1, 3, 2, 0));
}
```

### Raylib

Needs to be linked with Raylib. See Makefile.

```c
main() {
	extrn InitWindow, CloseWindow, WindowShouldClose, SetTargetFPS,
		BeginDrawing, EndDrawing,
		ClearBackground, DrawRectangle;

	auto x, y, dx, dy;
	dx = dy = 1;
	x = y = 10;

	InitWindow(800, 600, "Raylib from B");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		if (x <= 0 || x+100 >= 800) dx *= -1;
		if (y <= 0 || y+60  >= 600) dy *= -1;

		x += dx * 2;
		y += dy * 2;

		BeginDrawing();
		ClearBackground(0);
		DrawRectangle(x, y, 100, 60, 0xff80ff00);
		EndDrawing();
	}

	CloseWindow();
}
```


## Long term goals

- Give myself a platform to explore algorithms used in compilers
- Allow to extend this language in weird ways and see results (base language doesn't have even types)
- Use self-hosted `libb` instaed of `libc` by default (but option to link with `libc` should be possible)
- Create some alternative history C
- Create self hosted version

## Implementation progress [`b.c`](./b.c)

[One-pass compiler](https://en.wikipedia.org/wiki/One-pass_compiler) (meaning: compiler that in single pass outputs multi-pass assembly) that directly produces *very* unoptimized assembly.
When all of the features of the language are implemented then I would start to make code generation better (including some optimizations), possibly splitting project in half - into one pass and multi pass backends.

- [ ] Literals
    - [x] Character literals
    - [x] String literals
    - [x] Decimal integer literals
    - [x] Hexadecimal integer literals
    - [x] Octal integer literals
    - [ ] Multicharacter literals
- [x] Definitions
    - [x] Functions
    - [x] Global variable definition
- [ ] Statements
    - [x] `extrn`
    - [x] `auto`
    - [x] `return`
    - [x] `while`
    - [x] expression statements
    - [x] compound statement
    - [x] `if`
    - [x] empty statement
    - [x] `goto`
    - [x] statement labels
    - [x] `switch`
    - [x] `case`
    - [ ] `break`
    - [ ] `continue`
- [x] Expressions
    - [x] Function call
    - [x] Constant
    - [x] Binary operators
        - [x] Additive operators: `+ -`
        - [x] Assigment `=`
        - [x] Bitwise operators: `& ^ |`
        - [x] Multiplicative operators: `* / %`
        - [x] Relational operatos: `< > <= >= == !=`
        - [x] Compound assigment `op=`
        - [x] Shift operators: `<< >>`
        - [x] Index
        - [x] Logical (short circuting): `&& ||`
    - [x] Unary operators
        - [x] Address of: `&`
        - [x] Bitwise complement: `~`
        - [x] Indirection: `*`
        - [x] Logical Not: `!`
        - [x] Negation: `-`
        - [x] Post-increment/decrement
        - [x] Pre-increment/decrement

## Additional quirks and features inside `b.c`

- Integer literals can have `_` inside them, making constants like `0xdeadc0de` more readable: `0xdead_c0de`
- Index operator behaves differently then pointer arithmetic - `a[b] != *(a + b)`. This is due to the B assuming that memory is made from word size cells, making `a[1]` go to the second cell of array. Thus `a[b] == *(a + b * 8)`, making also index of operator not commmutative. To fix this B would need a type system (or treat every pointer as a index of cell in memory but that would potentialy break ABI). Note that this property doesn't allow us for byte like access: `*(a + 1)` wouldn't allow to read second byte allocated by `malloc(2)`. Sadly it makes such classic iteration pattern like `while (*p++)` incorrect.

## Resources

- [Users' Reference to B](https://www.nokia.com/bell-labs/about/dennis-m-ritchie/kbman.html)
- [B Language Reference Manual](https://www.thinkage.ca/gcos/expl/b/index.html)
