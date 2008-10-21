// display_pkg.cpp:  ActionScript "flash.display" package, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
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

#include "Object.h" // for getObjectInterface
#include "as_object.h"

#include "string_table.h"
#include "VM.h"

#include "flash/display/BitmapData_as.h"
#include "Stage_as.h"
#include "flash/display/DisplayObject_as.h"
#include "flash/display/InteractiveObject_as.h"
#include "flash/display/DisplayObjectContainer_as.h"
#include "flash/display/Sprite_as.h"
#include "sprite_instance.h"

namespace gnash {

static as_value
get_flash_display_package(const fn_call& /*fn*/)
{
	log_debug("Loading flash.display package");

	as_object* pkg = new as_object(getObjectInterface());

	BitmapData_class_init(*pkg);
	stage_class_init(*pkg);
	display_object_class_init(*pkg);
	interactive_object_class_init(*pkg);
	display_object_container_class_init(*pkg);
	sprite_as_class_init(*pkg);
	movieclip_class_init(*pkg);

	return pkg;
}

void
flash_display_package_init(as_object& where)
{
	string_table& st = where.getVM().getStringTable();
	where.init_destructive_property(st.find("display"), get_flash_display_package);
}


} // end of gnash namespace
