## Unlambda

[Unlambda](http://www.madore.org/~david/programs/unlambda/) is an esoteric,
nearly-pure, functional programming language that has no variables, data
structures or code constructs (such as loops or conditionals). Unlambda only
has functions. Every function takes exactly one argument, which is also a
function, and returns a function. Nevertheless, Unlambda is fully Turing
complete.

Unlambda is perhaps the closest you can get to the pure [SKI lambda
calculus](https://en.wikipedia.org/wiki/SKI_combinator_calculus), from whence
it gets its Turing completeness. However, it also includes such modern
programming constructs as promises/thunks and
[call/cc](https://en.wikipedia.org/wiki/Call-with-current-continuation). They
are included mostly to make it harder to understand.

## This implementation

This is a complete (modulo bugs) interpreter for Unlambda 2. It is pretty
similar to the c-refcnt version included in the Unlambda distribution because
I followed the [implementation
hints](http://www.madore.org/~david/programs/unlambda/#impl) on the language's
home page. I also peeked at that implementation a few times when I was stuck,
but there are some differences (for example, his `task`s became my `action`s,
are allocated on the heap, and have an extra type).
