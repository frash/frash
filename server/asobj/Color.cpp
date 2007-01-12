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
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Color.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function

namespace gnash {

void color_getrgb(const fn_call& fn);
void color_gettransform(const fn_call& fn);
void color_setrgb(const fn_call& fn);
void color_settransform(const fn_call& fn);
void color_ctor(const fn_call& fn);

static void
attachColorInterface(as_object& o)
{
	o.set_member("getRGB", &color_getrgb);
	o.set_member("getTransform", &color_gettransform);
	o.set_member("setRGB", &color_setrgb);
	o.set_member("setTransform", &color_settransform);
}

static as_object*
getColorInterface()
{
	static boost::intrusive_ptr<as_object> o;
	if ( ! o )
	{
		o = new as_object();
		attachColorInterface(*o);
	}
	return o.get();
}

class color_as_object: public as_object
{

public:

	color_as_object()
		:
		as_object(getColorInterface())
	{}

	// override from as_object ?
	//const char* get_text_value() const { return "Color"; }

	// override from as_object ?
	//double get_numeric_value() const { return 0; }
};

void color_getrgb(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void color_gettransform(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void color_setrgb(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void color_settransform(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}

void
color_ctor(const fn_call& fn)
{
	boost::intrusive_ptr<as_object> obj = new color_as_object;
	
	fn.result->set_as_object(obj.get()); // will keep alive
}

// extern (used by Global.cpp)
void color_class_init(as_object& global)
{
	// This is going to be the global Color "class"/"function"
	static boost::intrusive_ptr<builtin_function> cl;

	if ( cl == NULL )
	{
		cl=new builtin_function(&color_ctor, getColorInterface());
		// replicate all interface to class, to be able to access
		// all methods as static functions
		attachColorInterface(*cl);
		     
	}

	// Register _global.Color
	global.set_member("Color", cl.get());

}


} // end of gnash namespace

