# B Programming Language

Implementation of a language similar to the B as a recreational excercise.
The goal is to represent the spirit of the language and there is no need to match it perfectly.
It's B for modern systems.

```b
/* Prints hello, world and a newline to the standard output using libc */
main()
	extrn printf;
{
    printf("hello, world*n");
}
```

Long term goals:

- Give myself a platform to explore algorithms used in compilers
- Allow to extend this language in weird ways and see results (base language doesn't have even types)
- Use self-hosted `libb` instaed of `libc` by default (but option to link with `libc` should be possible)
- Create some alternative history C
- Create self hosted version

## Resources

- [Users' Reference to B](https://www.nokia.com/bell-labs/about/dennis-m-ritchie/kbman.html)
- [B Language Reference Manual](https://www.thinkage.ca/gcos/expl/b/index.html)
