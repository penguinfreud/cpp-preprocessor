#define x defined(x)
#if x
foo
#endif
#define y defined
#if y(x)
bar
#endif
