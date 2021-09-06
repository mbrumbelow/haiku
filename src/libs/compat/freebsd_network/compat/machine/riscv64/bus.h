/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _RISCV_BUS_H_
#define _RISCV_BUS_H_

#include <machine/_bus.h>
#include <machine/cpufunc.h>
#include <machine/bus_dma.h>

/*
 * Values for the x86 bus space tag, not to be used directly by MI code.
 */
#define	X86_BUS_SPACE_IO	0	/* space is i/o space */
#define	X86_BUS_SPACE_MEM	1	/* space is mem space */

#define BUS_SPACE_MAXSIZE_24BIT	0xFFFFFF
#define BUS_SPACE_MAXSIZE_32BIT 0xFFFFFFFF
#define BUS_SPACE_MAXSIZE	0xFFFFFFFF
#define BUS_SPACE_MAXADDR_24BIT	0xFFFFFF
#define BUS_SPACE_MAXADDR_32BIT 0xFFFFFFFF
#define BUS_SPACE_MAXADDR	0xFFFFFFFFFFFFFFFFULL

#define BUS_SPACE_INVALID_DATA	(~0)
#define BUS_SPACE_UNRESTRICTED	(~0)


static __inline u_int8_t bus_space_read_1(bus_space_tag_t tag,
					  bus_space_handle_t handle,
					  bus_size_t offset);

static __inline u_int16_t bus_space_read_2(bus_space_tag_t tag,
					   bus_space_handle_t handle,
					   bus_size_t offset);

static __inline u_int32_t bus_space_read_4(bus_space_tag_t tag,
					   bus_space_handle_t handle,
					   bus_size_t offset);

static __inline uint64_t bus_space_read_8(bus_space_tag_t tag,
					  bus_space_handle_t handle,
					  bus_size_t offset);

static __inline void bus_space_write_1(bus_space_tag_t tag,
				       bus_space_handle_t bsh,
				       bus_size_t offset, u_int8_t value);

static __inline void bus_space_write_2(bus_space_tag_t tag,
				       bus_space_handle_t bsh,
				       bus_size_t offset, u_int16_t value);

static __inline void bus_space_write_4(bus_space_tag_t tag,
				       bus_space_handle_t bsh,
				       bus_size_t offset, u_int32_t value);

static __inline void bus_space_write_8(bus_space_tag_t tag,
				       bus_space_handle_t bsh,
				       bus_size_t offset, uint64_t value);


static __inline u_int8_t
bus_space_read_1(bus_space_tag_t tag, bus_space_handle_t handle,
		 bus_size_t offset)
{

	if (tag == X86_BUS_SPACE_IO)
		return BUS_SPACE_INVALID_DATA; // TODO: implement
	return (*(volatile u_int8_t *)(handle + offset));
}

static __inline u_int16_t
bus_space_read_2(bus_space_tag_t tag, bus_space_handle_t handle,
		 bus_size_t offset)
{

	if (tag == X86_BUS_SPACE_IO)
		return BUS_SPACE_INVALID_DATA; // TODO: implement
	return (*(volatile u_int16_t *)(handle + offset));
}

static __inline u_int32_t
bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t handle,
		 bus_size_t offset)
{

	if (tag == X86_BUS_SPACE_IO)
		return BUS_SPACE_INVALID_DATA; // TODO: implement
	return (*(volatile u_int32_t *)(handle + offset));
}

static __inline uint64_t
bus_space_read_8(bus_space_tag_t tag, bus_space_handle_t handle,
		 bus_size_t offset)
{

	if (tag == X86_BUS_SPACE_IO) /* No 8 byte IO space access on x86 */
		return (BUS_SPACE_INVALID_DATA);
	return (*(volatile uint64_t *)(handle + offset));
}


static __inline void
bus_space_write_1(bus_space_tag_t tag, bus_space_handle_t bsh,
		       bus_size_t offset, u_int8_t value)
{

	if (tag == X86_BUS_SPACE_IO)
		return; // TODO: implement
	else
		*(volatile u_int8_t *)(bsh + offset) = value;
}

static __inline void
bus_space_write_2(bus_space_tag_t tag, bus_space_handle_t bsh,
		       bus_size_t offset, u_int16_t value)
{

	if (tag == X86_BUS_SPACE_IO)
		return; // TODO: implement
	else
		*(volatile u_int16_t *)(bsh + offset) = value;
}

static __inline void
bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t bsh,
		       bus_size_t offset, u_int32_t value)
{

	if (tag == X86_BUS_SPACE_IO)
		return; // TODO: implement
	else
		*(volatile u_int32_t *)(bsh + offset) = value;
}

static __inline void
bus_space_write_8(bus_space_tag_t tag, bus_space_handle_t bsh,
		  bus_size_t offset, uint64_t value)
{

	if (tag == X86_BUS_SPACE_IO)
		return; // TODO: implement
	else
		*(volatile uint64_t *)(bsh + offset) = value;
}


#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

static __inline void
bus_space_barrier(bus_space_tag_t tag __unused, bus_space_handle_t bsh __unused,
		  bus_size_t offset __unused, bus_size_t len __unused, int flags)
{
	__compiler_membar();
}

#include <machine/bus_dma.h>

/*
 * Stream accesses are the same as normal accesses on x86; there are no
 * supported bus systems with an endianess different from the host one.
 */
#define	bus_space_read_stream_1(t, h, o)	bus_space_read_1((t), (h), (o))
#define	bus_space_read_stream_2(t, h, o)	bus_space_read_2((t), (h), (o))
#define	bus_space_read_stream_4(t, h, o)	bus_space_read_4((t), (h), (o))

#define	bus_space_read_multi_stream_1(t, h, o, a, c) \
	bus_space_read_multi_1((t), (h), (o), (a), (c))
#define	bus_space_read_multi_stream_2(t, h, o, a, c) \
	bus_space_read_multi_2((t), (h), (o), (a), (c))
#define	bus_space_read_multi_stream_4(t, h, o, a, c) \
	bus_space_read_multi_4((t), (h), (o), (a), (c))

#define	bus_space_write_stream_1(t, h, o, v) \
	bus_space_write_1((t), (h), (o), (v))
#define	bus_space_write_stream_2(t, h, o, v) \
	bus_space_write_2((t), (h), (o), (v))
#define	bus_space_write_stream_4(t, h, o, v) \
	bus_space_write_4((t), (h), (o), (v))

#define	bus_space_write_multi_stream_1(t, h, o, a, c) \
	bus_space_write_multi_1((t), (h), (o), (a), (c))
#define	bus_space_write_multi_stream_2(t, h, o, a, c) \
	bus_space_write_multi_2((t), (h), (o), (a), (c))
#define	bus_space_write_multi_stream_4(t, h, o, a, c) \
	bus_space_write_multi_4((t), (h), (o), (a), (c))

#define	bus_space_set_multi_stream_1(t, h, o, v, c) \
	bus_space_set_multi_1((t), (h), (o), (v), (c))
#define	bus_space_set_multi_stream_2(t, h, o, v, c) \
	bus_space_set_multi_2((t), (h), (o), (v), (c))
#define	bus_space_set_multi_stream_4(t, h, o, v, c) \
	bus_space_set_multi_4((t), (h), (o), (v), (c))

#define	bus_space_read_region_stream_1(t, h, o, a, c) \
	bus_space_read_region_1((t), (h), (o), (a), (c))
#define	bus_space_read_region_stream_2(t, h, o, a, c) \
	bus_space_read_region_2((t), (h), (o), (a), (c))
#define	bus_space_read_region_stream_4(t, h, o, a, c) \
	bus_space_read_region_4((t), (h), (o), (a), (c))

#define	bus_space_write_region_stream_1(t, h, o, a, c) \
	bus_space_write_region_1((t), (h), (o), (a), (c))
#define	bus_space_write_region_stream_2(t, h, o, a, c) \
	bus_space_write_region_2((t), (h), (o), (a), (c))
#define	bus_space_write_region_stream_4(t, h, o, a, c) \
	bus_space_write_region_4((t), (h), (o), (a), (c))

#define	bus_space_set_region_stream_1(t, h, o, v, c) \
	bus_space_set_region_1((t), (h), (o), (v), (c))
#define	bus_space_set_region_stream_2(t, h, o, v, c) \
	bus_space_set_region_2((t), (h), (o), (v), (c))
#define	bus_space_set_region_stream_4(t, h, o, v, c) \
	bus_space_set_region_4((t), (h), (o), (v), (c))

#define	bus_space_copy_region_stream_1(t, h1, o1, h2, o2, c) \
	bus_space_copy_region_1((t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_stream_2(t, h1, o1, h2, o2, c) \
	bus_space_copy_region_2((t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_stream_4(t, h1, o1, h2, o2, c) \
	bus_space_copy_region_4((t), (h1), (o1), (h2), (o2), (c))


#endif /* _RISCV_BUS_H_ */
