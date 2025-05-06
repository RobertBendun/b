CFLAGS = -Wall -Wextra -Werror=switch -Werror=implicit-fallthrough -fsanitize=undefined

EXAMPLES = $(wildcard examples/*.b)

all: b snap

test: b snap
	./snap

clean: b snap
	rm -vf b snap $(EXAMPLES:.b=)

examples: $(EXAMPLES:.b=)

examples/%.asm: examples/%.b b
	./b <$< >$@

examples/%.o: examples/%.asm
	fasm $< $@

examples/%: examples/%.o
	$(CC) $< -o $@

.PHONY: all test examples
