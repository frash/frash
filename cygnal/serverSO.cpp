// rc.cpp:  "Run Command" configuration file, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010,
//   2011 Free Software Foundation, Inc
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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "gnashconfig.h"
#endif

#include "StringPredicates.h"
#include "log.h"
#include "serverSO.h"

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#include <sys/types.h>
#include <boost/cstdint.hpp>

#include <cctype>  // for toupper
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;
using namespace gnash;

/// \namespace cygnal
///	This namespace is for all the Cygnal specific classes.
namespace cygnal {


ServerSO::ServerSO()
{
//    GNASH_REPORT_FUNCTION;
}

//Never destroy (TODO: add a destroyDefaultInstance)
ServerSO::~ServerSO()
{
//    GNASH_REPORT_FUNCTION;    
}

/// \brief Dump the internal data of this class in a human readable form.
/// @remarks This should only be used for debugging purposes.
void
ServerSO::dump(std::ostream& os) const
{
    os << endl << "Dump ServerSO:" << endl;

}

} // end of namespace cygnal

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

