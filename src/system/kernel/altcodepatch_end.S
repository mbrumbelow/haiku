#define OBJECT(x) .global x; .type x,@object; x

.section .altcodepatch
OBJECT(altcodepatch_end):
