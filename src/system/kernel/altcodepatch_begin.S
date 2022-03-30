#define OBJECT(x) .global x; .type x,@object; x

.section .altcodepatch, "a", @progbits
.align 4
OBJECT(altcodepatch_begin):
