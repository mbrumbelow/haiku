# Utility functions to aid in converting from the special data types the HVIF
# (aka flat icon format) has

# Example usage:
# >>> from FlatIconFormat import *
# >>> read_float_24(0x3ffffd)
def read_float_24(value):
	sign = (-1) ** ((value & 0x800000) >> 23 + 1)
	exponent = 2 ** (((value & 0x7e0000) >> 17) - 32)
	mantissa = (value & 0x01ffff) * 2 ** -17
	return sign * exponent * (mantissa + 1)
