CFLAGS += -Wall -Wextra -Werror=switch -Werror=implicit-fallthrough -fsanitize=undefined

EXAMPLES = $(wildcard examples/*.b)
OPT_EXAMPLES = $(wildcard examples/opt/*.b)
TESTS = $(wildcard tests/*.b)

all: b examples

test: b snap.sh $(TESTS)
	for test in $(TESTS); do echo $$test; ./snap.sh $$test; done

clean: b
	rm -vf b $(EXAMPLES:.b=) $(OPT_EXAMPLES:.b=)

examples: $(EXAMPLES:.b=)

examples_opt: $(OPT_EXAMPLES:.b=)

examples/%.asm: examples/%.b b
	./b <$< >$@

examples/%.o: examples/%.asm
	fasm $< $@

examples/%: examples/%.o
	$(CC) $< -o $@

examples/opt/%.asm: examples/opt/%.b b
	./b <$< >$@

examples/opt/%.o: examples/opt/%.asm
	fasm $< $@

examples/opt/%: examples/opt/%.o
	$(CC) $< -o $@

examples/opt/raylib: examples/opt/raylib.o
	$(CC) $< -o $@ -lraylib

.PHONY: all test examples test
