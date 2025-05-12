CFLAGS += -Wall -Wextra -Werror=switch -Werror=implicit-fallthrough -fsanitize=undefined

EXAMPLES = $(wildcard examples/*.b)
TESTS = $(wildcard tests/*.b)

all: b examples

test: b snap.sh $(TESTS)
	for test in $(TESTS); do echo $$test; ./snap.sh $$test; done

clean: b
	rm -vf b $(EXAMPLES:.b=)

examples: $(EXAMPLES:.b=)

examples/%.asm: examples/%.b b
	./b <$< >$@

examples/%.o: examples/%.asm
	fasm $< $@

examples/%: examples/%.o
	$(CC) $< -o $@

.PHONY: all test examples test
