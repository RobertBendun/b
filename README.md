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


	while (WindowShouldClose() == 0) {
		if (x+100 >= 800) dx = (0 - 1) * dx;
		if (x <=   0) dx = (0 - 1) * dx;
		if (y+60 >= 600) dy = (0 - 1) * dy;
		if (y <=   0) dy = (0 - 1) * dy;

		x = x + dx * 2;
		y = y + dy * 2;

		BeginDrawing();
		ClearBackground(0);
		DrawRectangle(x, y, 100, 60, 4278255360);
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

- [ ] Literals
    - [x] Character literals
    - [x] String literals
    - [x] Decimal integer literals
    - [ ] Hexadecimal integer literals
    - [ ] Octal integer literals
    - [ ] Multicharacter literals
- [ ] Definitions
    - [x] Functions
    - [ ] Global variable definition
- [ ] Statements
    - [x] `extrn`
    - [x] `auto`
    - [x] `return`
    - [x] `while`
    - [x] expression statements
    - [x] compound statement
    - [x] `if`
    - [x] empty statement
    - [ ] `goto`
    - [ ] `switch`
    - [ ] `case`
    - [ ] statement labels
- [ ] Expressions
    - [x] Function call
    - [x] Constant
    - [ ] Binary operators
        - [x] Additive operators: `+ -`
        - [x] Assigment `=`
        - [x] Bitwise operators: `& ^ |`
        - [x] Multiplicative operators: `* / %`
        - [x] Relational operatos: `< > <= >= == !=`
        - [ ] Logical (short circuting): `&& ||`
        - [ ] Shift operators: `<< >>`
        - [ ] Compound assigment `op=`
        - [ ] Index
    - [ ] Unary operators
        - [x] Address of: `&`
        - [x] Indirection: `*`
        - [ ] Bitwise complement: `~`
        - [ ] Negation: `-`
        - [ ] Logical Not: `!`
        - [ ] Pre-increment/decrement
        - [ ] Post-increment/decrement

## Resources

- [Users' Reference to B](https://www.nokia.com/bell-labs/about/dennis-m-ritchie/kbman.html)
- [B Language Reference Manual](https://www.thinkage.ca/gcos/expl/b/index.html)
