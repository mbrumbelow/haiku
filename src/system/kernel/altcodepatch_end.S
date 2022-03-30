#define OBJECT(x) .global x; .type x,@object; x

.section .altcodepatch, "a", @progbits
OBJECT(altcodepatch_end):
