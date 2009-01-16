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

#ifndef __NTNATIVE_CLIENT_H__
#define __NTNATIVE_CLIENT_H__

enum tt_req_type {
	tt_req_exit,
	tt_req_map,
	tt_req_umap,
	tt_req_prot,
};

struct tt_req_map {
	unsigned int pid;
	unsigned int fd;
	unsigned int addr;
	unsigned int len;
	unsigned int ofs;
	unsigned int prot;
};

struct tt_req_umap {
	unsigned int addr;
	unsigned int len;
};

struct tt_req_prot {
	unsigned int addr;
	unsigned int len;
	unsigned int prot;
};

struct tt_req {
	enum tt_req_type type;
	union {
		struct tt_req_map map;
		struct tt_req_umap umap;
		struct tt_req_prot prot;
	} u;
};

struct tt_reply {
	int r;
};

#endif // __NTNATIVE_CLIENT_H__

