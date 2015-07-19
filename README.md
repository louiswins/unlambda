## Unlambda

[Unlambda](http://www.madore.org/~david/programs/unlambda/) is an esoteric, nearly-pure, functional programming language that has no variables, data structures or code constructs (such as loops or conditionals). Unlambda only has functions. Every function takes exactly one argument, which is also a function, and returns a function. Nevertheless, Unlambda is fully Turing complete.

Unlambda is the closest place you can get to the pure [SKI lambda calculus](https://en.wikipedia.org/wiki/SKI_combinator_calculus), from whence it gets its Turing completeness. However, it also includes such modern programming constructs as promises and [call-with-current-continuation](https://en.wikipedia.org/wiki/Call-with-current-continuation). They are included mostly to make it harder to understand.

## This implementation

This implementation is my first attempt at coding an interpreter for Unlambda. It includes all the core functions except `c` (call/cc) from Unlambda 1.

Currently I use the C call stack as the Unlambda call stack. I plan to rewrite it in continuation-passing style so that I can implement `c` and keep myself from blowing the stack.