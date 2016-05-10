#define a(x) b(x) + 1
#define b(x) x + 2
a(0)
a (0)
b(b(0))
#define c(x) c(x + 1)
c(0)

const int x = 1;
#if x
foo
#endif

#define e \\
foo
e

#define f (0)
#define g() a
g()f

#define h(x) x(0)
h(a)

#define i(x) k(x)
#define j 1,2
#define k(x, y) x foo y
i(j)
