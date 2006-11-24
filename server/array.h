// 
//   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// 
//
//

#ifndef GNASH_ARRAY_H
#define GNASH_ARRAY_H

#include "as_object.h" // for inheritance

#include <deque>
#include <memory> // for auto_ptr

#include <string>

// Forward declarations
namespace gnash {
	class fn_call;
	class as_value;
}

namespace gnash {

/// The Array ActionScript object
class as_array_object : public as_object
{

public:

	/// Sort flags
	enum SortFlags {

		/// Case-insensitive (z precedes A)
		fCaseInsensitive	= (1<<0), // 1

		/// Descending order (b precedes a)
		fDescending		= (1<<1), // 2

		/// Remove consecutive equal elements
		fUniqueSort		= (1<<2), // 4

		/// Don't modify the array, rather return
		/// a new array containing indexes into it
		/// in sorted order.
		fReturnIndexedArray	= (1<<3), // 8

		/// Numerical sort (9 preceeds 10)
		fNumeric		= (1<<4) // 16
	};

	as_array_object();

	as_array_object(const as_array_object& other);

	void push(as_value& val);

	void unshift(as_value& val);

	as_value shift();

	as_value pop();

	as_value at(unsigned int index);

	void reverse();

	std::string join(const std::string& separator);

	std::string toString();

	unsigned int size() const;

	//void resize(unsigned int);

	void concat(const as_array_object& other);

	/// \brief
	/// Return a newly created array containing elements
	/// from 'start' up to but not including 'end'.
	//
	///
	/// NOTE: assertions are:
	///
	///	assert(one_past_end >= start);
	///	assert(one_past_end <= size());
	///	assert(start <= size());
	///
	/// @param start
	///	index to first element to include in result
	///	0-based index.
	///
	/// @param one_past_end
	///	index to one-past element to include in result
	///	0-based index.
	///
	std::auto_ptr<as_array_object> slice(
		unsigned int start, unsigned int one_past_end);

	/// Sort the array, using given values comparator
	void sort(as_function& comparator, uint8_t flags=0);

	void sort(uint8_t flags=0);

	/// Return a new array containing sorted index of this array
	//
	/// NOTE: assert(flags & Array::fReturnIndexedArray)
	std::auto_ptr<as_array_object> sorted_indexes(uint8_t flags);

	std::auto_ptr<as_array_object> sorted_indexes(as_function& comparator, uint8_t flags);

	/// Overridden to provide 'length' member
	virtual bool get_member(const tu_stringi& name, as_value* val);

	/// Overridden to provide array[#]=x semantic
	virtual void set_member(const tu_stringi& name,
		const as_value& val );

private:

	std::deque<as_value> elements;

	// this function is used internally by set_member and get_member
	// it takes a string that is the member name of the array and returns -1
	// if the string does not refer to an index, or an appropriate int if the string does refer to an index
	int index_requested(const tu_stringi& name);

};


/// Initialize the global.Array object
// needed by SWFHandlers::ActionInitArray
void array_class_init(as_object& global);

/// Constructor for ActionScript class Array.
// needed by SWFHandlers::ActionInitArray
void	array_new(const fn_call& fn);

}

#endif
