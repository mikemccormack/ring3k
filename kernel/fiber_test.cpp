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

#include <stdio.h>
#include "fiber.h"

class fiber_test_t: public fiber_t
{
	int num;
public:
	fiber_test_t(int n);
	virtual int run();
};

fiber_test_t::fiber_test_t(int n) :
	fiber_t( fiber_default_stack_size ),
	num(n)
{
}

int fiber_test_t::run()
{
	int i;

	for (i=0; i<10; i++)
	{
		printf("fiber%d i=%d\n", num, i);
		fiber_t::yield();
	}
	printf("fiber%d finished\n", num );
	return 0;
}

int main(int argc, char **argv)
{
	fiber_t::fibers_init();
	fiber_t* t1 = new fiber_test_t(1);
	fiber_t* t2 = new fiber_test_t(2);
	printf("scheduling...\n");
	t1->start();
	t2->start();
	while (!fiber_t::last_fiber())
		fiber_t::yield();
	delete t1;
	delete t2;
	fiber_t::fibers_finish();
	printf("done\n");
	return 0;
}
