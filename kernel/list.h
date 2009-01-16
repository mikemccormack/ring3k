/*
 * Lightweight list template
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

#ifndef __LIST_H__
#define __LIST_H__

#include <assert.h>

template<class T, const int X> class list_iter;
template<class T, const int X> class list_anchor;
template<class T> class list_element_accessor;

template<class T> class list_element
{
	friend class list_element_accessor<T>;
protected:
	T *prev;
	T *next;
public:
	void init() { prev = (T*)-1; next = (T*)-1; }
	explicit list_element() {init();}
	~list_element() {}
	bool is_linked() { return prev != (T*)-1; }
	T* get_next() {return next;}
	T* get_prev() {return prev;}
};

template<class T> class list_element_accessor
{
protected:
	T*& prevptr(list_element<T>& elem) {return elem.prev;}
	T*& nextptr(list_element<T>& elem) {return elem.next;}
};

template<class T, const int X> class list_anchor : public list_element_accessor<T>
{
	T *_head;
	T *_tail;
public:
	explicit list_anchor() { _head = 0; _tail = 0; }
	~list_anchor() {}
	bool empty() { return !(_head || _tail); }
	T *head() { return _head; }
	T *tail() { return _tail; }
	void unlink(T* elem)
	{
		assert(elem->entry[X].is_linked());
		if (_head == elem)
			_head = nextptr(elem->entry[X]);
		else
			nextptr(prevptr(elem->entry[X])->entry[X]) = nextptr(elem->entry[X]);
		if (_tail == elem)
			_tail = prevptr(elem->entry[X]);
		else
			prevptr(nextptr(elem->entry[X])->entry[X]) = prevptr(elem->entry[X]);
		elem->entry[X].init();
	}

	void append( T* elem )
	{
		assert(!elem->entry[X].is_linked());
		if (_tail)
			nextptr(_tail->entry[X]) = elem;
		else
			_head = elem;
		prevptr(elem->entry[X]) = _tail;
		nextptr(elem->entry[X]) = 0;
		_tail = elem;
	}

	void prepend( T* elem )
	{
		assert(!elem->entry[X].is_linked());
		if (_head)
			prevptr(_head->entry[X]) = elem;
		else
			_tail = elem;
		nextptr(elem->entry[X]) = _head;
		prevptr(elem->entry[X]) = 0;
		_head = elem;
	}

	void insert_after( T* point, T* elem )
	{
		assert(!elem->entry[X].is_linked());
		if (nextptr(point->entry[X]))
			prevptr(nextptr(point->entry[X])->entry[X]) = elem;
		else
			_tail = elem;
		nextptr(elem->entry[X]) = nextptr(point->entry[X]);
		nextptr(point->entry[X]) = elem;
		prevptr(elem->entry[X]) = point;
	}

	void insert_before( T* point, T* elem )
	{
		assert(!elem->entry[X].is_linked());
		if (prevptr(point->entry[X]))
			nextptr(prevptr(point->entry[X])->entry[X]) = elem;
		else
			_head = elem;
		prevptr(elem->entry[X]) = prevptr(point->entry[X]);
		prevptr(point->entry[X]) = elem;
		nextptr(elem->entry[X]) = point;
	}
};

template<class T, const int X> class list_iter : public list_element_accessor<T>
{
	list_anchor<T,X>& list;
	T* i;
public:
	explicit list_iter(list_anchor<T,X>& l) : list(l), i(l.head()) {}
	T* next() { i = nextptr(i->entry[X]); return i; }
	T* cur() { return i; }
	operator bool() { return i != 0; }
	operator T*() { return i; }
	void reset() {i = list.head();}
};

#endif // __LIST_H__
