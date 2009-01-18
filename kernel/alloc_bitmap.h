/*
 * nt loader
 *
 * Copyright 2006-2008 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __ALLOC_BITMAP__
#define __ALLOC_BITMAP__

#include <stdio.h>
#include <assert.h>
#include <string.h>

// TODO: optimize
class allocation_bitmap_t
{
	static const size_t allocation_granularity = 8;
	size_t size;
	size_t array_size;
	size_t max_bits;
	unsigned char *bitmap;
	unsigned char *ptr;
protected:
	bool bit_value( size_t n ) { return bitmap[n/8] & (1 << (n%8)); }
	void set_bit( size_t n ) { bitmap[n/8] |= (1 << (n%8)); }
	void clear_bit( size_t n ) { bitmap[n/8] &= ~(1 << (n%8)); }
	size_t count_zero_bits( size_t start, size_t max );
	size_t count_one_bits( size_t start, size_t max );
	void set_bits( size_t start, size_t count );
	void clear_bits( size_t start, size_t count );
	size_t bits_required( size_t len );
public:
	allocation_bitmap_t();
	void set_area( void *_ptr, size_t _size );
	unsigned char *alloc( size_t len );
	void free( unsigned char *mem, size_t len );
	void get_info( size_t& total, size_t& used, size_t& free );
	static void test(); // unit test for validating the code
};

#endif // __ALLOC_BITMAP__

