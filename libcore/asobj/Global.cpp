// Global.cpp:  Global ActionScript class setup, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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


#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "as_object.h"
#include "as_prop_flags.h"
#include "as_value.h"
#include "as_function.h" // for function_class_init
#include "Array_as.h"
#include "AsBroadcaster.h"
#include "Boolean_as.h"
#include "Camera.h"
#include "Color_as.h"
#include "ContextMenu.h"
#include "CustomActions.h"
#include "Date_as.h" // for registerDateNative
#include "Error_as.h"
#include "Global.h"
#include "String_as.h"
#include "Key_as.h"
#include "Selection_as.h"
#include "Microphone.h"
#include "Number_as.h"
#include "Object.h"
#include "Math_as.h"
#include "XML_as.h"
#include "XMLSocket_as.h"
#include "Mouse_as.h"
#include "MovieClipLoader.h"
#include "movie_definition.h"
#include "NetConnection_as.h"
#include "NetStream_as.h"
#include "SharedObject_as.h"
#include "Stage_as.h"
#include "System_as.h"
#include "TextFormat_as.h"
#include "TextSnapshot_as.h"
#include "Video.h"
#include "extension.h"
#include "VM.h"
#include "timers.h"
#include "URL.h" // for URL::encode and URL::decode (escape/unescape)
#include "builtin_function.h"
#include "TextField.h"
#include "rc.h"
#include "ClassHierarchy.h"
#include "namedStrings.h"
#include "utility.h" // for isfinite replacement
#include "flash_pkg.h"

#include "fn_call.h"
#include "MovieClip.h"

#include <limits> // for numeric_limits<double>::infinity
#include <sstream>
#include <boost/lexical_cast.hpp>

// Common code to warn and return if a required single arg is not present
// and to warn if there are extra args.
#define ASSERT_FN_ARGS_IS_1                        \
    if (fn.nargs < 1) {                            \
    IF_VERBOSE_ASCODING_ERRORS(                    \
            log_aserror(_("%s needs one argument"), __FUNCTION__);        \
            )                                \
         return as_value();                            \
    }                                    \
    IF_VERBOSE_ASCODING_ERRORS(                        \
    if (fn.nargs > 1)                        \
            log_aserror(_("%s has more than one argument"), __FUNCTION__);    \
    )

namespace gnash {

// Forward declarations
static as_value as_global_trace(const fn_call& fn);
static as_value as_global_isNaN(const fn_call& fn);
static as_value as_global_isfinite(const fn_call& fn);
static as_value as_global_unescape(const fn_call& fn);
static as_value as_global_escape(const fn_call& fn);
static as_value as_global_parsefloat(const fn_call& fn);
static as_value as_global_parseint(const fn_call& fn);
static as_value as_global_assetpropflags(const fn_call& fn);
static as_value as_global_asnative(const fn_call& fn);
static as_value as_global_asnew(const fn_call& /*fn*/);
static as_value as_global_assetnative(const fn_call& /*fn*/);
static as_value as_global_assetnativeaccessor(const fn_call& /*fn*/);
static as_value as_global_asconstructor(const fn_call& /*fn*/);
static as_value as_global_updateAfterEvent(const fn_call& /*fn*/);

namespace {
    void registerNatives(as_object& global);
}

Global::Global(VM& vm, ClassHierarchy *ch)
    :
    as_object()
{

    registerNatives(*this);

    // No idea why, but it seems there's a NULL _global.o 
    // defined at player startup...
    // Probably due to the AS-based initialization 
    // Not enumerable but overridable and deletable.
    //
    as_value nullVal; nullVal.set_null();
    init_member("o", nullVal, as_prop_flags::dontEnum);

    // _global functions.            
    // These functions are only available in SWF6+, but this is just
    // because SWF5 or lower did not have a "_global"
    // reference at all.
    init_member("ASnative", new builtin_function(as_global_asnative));
    init_member("ASconstructor", new builtin_function(as_global_asconstructor));
    init_member("ASSetPropFlags", vm.getNative(1, 0));
    init_member("ASSetNative", vm.getNative(4, 0));
    init_member("ASSetNativeAccessor", vm.getNative(4, 1));
    init_member("updateAfterEvent", vm.getNative(9, 0));
    init_member("trace", vm.getNative(100, 4));

    // Defined in timers.h
    init_member("setInterval", vm.getNative(250, 0));
    init_member("clearInterval", vm.getNative(250, 1));
    init_member("setTimeout", new builtin_function(timer_settimeout));
    init_member("clearTimeout", new builtin_function(timer_clearinterval));

    ch->setGlobal(this);

// If extensions aren't used, then no extensions will be loaded.
#ifdef USE_EXTENSIONS
    ch->setExtension(&_et);
#endif

    ch->massDeclare();

    object_class_init(*this); 
    string_class_init(*this); 
    array_class_init(*this); 

    /// SWF6 visibility:
    function_class_init(*this);

    // SWF8 visibility:
    flash_package_init(*this); 

    const int version = vm.getSWFVersion();

    switch (version)
    {
        default:
            // Version 10 or above reported
        case 9:
        case 8:

        case 7:
        case 6:

            ch->getGlobalNs()->stubPrototype(NSV::CLASS_FUNCTION);
            ch->getGlobalNs()->getClass(NSV::CLASS_FUNCTION)->setDeclared();

        case 5:
        
            ch->getGlobalNs()->stubPrototype(NSV::CLASS_OBJECT);
            ch->getGlobalNs()->getClass(NSV::CLASS_OBJECT)->setDeclared();
            ch->getGlobalNs()->stubPrototype(NSV::CLASS_ARRAY);
            ch->getGlobalNs()->getClass(NSV::CLASS_ARRAY)->setDeclared();
            ch->getGlobalNs()->stubPrototype(NSV::CLASS_STRING);
            ch->getGlobalNs()->getClass(NSV::CLASS_STRING)->setDeclared();        
            // This is surely not correct, but they are not available
            // in SWF4
            init_member("escape", vm.getNative(100, 0));
            init_member("unescape", vm.getNative(100, 1));
            init_member("parseInt", vm.getNative(100, 2));
            init_member("parseFloat", vm.getNative(100, 3));
            init_member("isNaN", vm.getNative(200, 18));
            init_member("isFinite", vm.getNative(200, 19));

            init_member("NaN", as_value(NaN));
            init_member("Infinity", as_value(
                        std::numeric_limits<double>::infinity()));
        
        case 4:
        case 3:
        case 2:
        case 1:
            break;
    }

#ifdef USE_EXTENSIONS
    loadExtensions();
#endif

}

#ifdef USE_EXTENSIONS

//-----------------------
// Extensions
//-----------------------
// Scan the plugin directories for all plugins, and load them now.
// FIXME: this should actually be done dynamically, and only
// if a plugin defines a class that a movie actually wants to
// use.
void
Global::loadExtensions()
{

    if ( RcInitFile::getDefaultInstance().enableExtensions() )
    {
        log_security(_("Extensions enabled, scanning plugin dir for load"));
        _et.scanAndLoad(*this);
    }
    else
    {
        log_security(_("Extensions disabled"));
    }

}

#endif



as_value
as_global_trace(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    // Log our argument.
    //
    // @@ what if we get extra args?
    //
    // @@ Nothing needs special treatment,
    //    as_value::to_string() will take care of everything
    log_trace("%s", fn.arg(0).to_string());

    return as_value();
}


as_value
as_global_isNaN(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    return as_value( static_cast<bool>(isNaN(fn.arg(0).to_number()) ));
}


as_value
as_global_isfinite(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    return as_value( static_cast<bool>(utility::isFinite(fn.arg(0).to_number())) );
}

/// \brief Encode a string to URL-encoded format
/// converting all dodgy characters to %AB hex sequences
//
/// Characters that need escaping are:
/// - ASCII control characters: 0-31 and 127
/// - Non-ASCII chars: 128-255
/// - URL syntax characters: $ & + , / : ; = ? @
/// - Unsafe characters: SPACE " < > # % { } | \ ^ ~ [ ] `
/// Encoding is a % followed by two hexadecimal characters, case insensitive.
/// See RFC1738 http://www.rfc-editor.org/rfc/rfc1738.txt,
/// Section 2.2 "URL Character Encoding Issues"
as_value
as_global_escape(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    std::string input = fn.arg(0).to_string();
    URL::encode(input);
    return as_value(input);
}

/// \brief Decode a string from URL-encoded format
/// converting all hexadecimal sequences to ASCII characters.
//
/// A sequence to convert is % followed by two case-independent hexadecimal
/// digits, which is replaced by the equivalent ASCII character.
/// See RFC1738 http://www.rfc-editor.org/rfc/rfc1738.txt,
/// Section 2.2 "URL Character Encoding Issues"
as_value
as_global_unescape(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    std::string input = fn.arg(0).to_string();
    URL::decode(input);
    return as_value(input);
}

// parseFloat (string)
as_value
as_global_parsefloat(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    as_value rv;
    double result;
    
    std::istringstream s(fn.arg(0).to_string());
    
    if ( ! (s >> result)  )
    {
        rv.set_nan();
        return rv;   
    }    

    rv = result;
    return rv;
}

// parseInt(string[, base])
// The second argument, if supplied, is the base.
// If none is supplied, we have to work out the 
// base from the string. Decimal, octal and hexadecimal are
// possible, according to the following rules:
// 1. If the string starts with 0x or 0X, the number is hex.
// 2. The 0x or 0X may be *followed* by '-' or '+' to indicate sign. A number
//    with no sign is positive.
// 3. If the string starts with 0, -0 or +0 and contains only the characters
//    0-7.
// 4. If the string starts with *any* other sequence of characters, including
//    whitespace, it is decimal.
as_value
as_global_parseint(const fn_call& fn)
{
    if (!fn.nargs) {
        IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("%s needs at least one argument"), __FUNCTION__);
        )
        return as_value();
    }

    IF_VERBOSE_ASCODING_ERRORS(
        if (fn.nargs > 2) {
            log_aserror(_("%s has more than two arguments"), __FUNCTION__);
        }
    )

    const std::string& expr = fn.arg(0).to_string();

    // A second argument specifies the base.
    // Parsing still starts after any positive/negative 
    // sign or hex identifier (parseInt("0x123", 8) gives
    // 83, not 0; parseInt(" 0x123", 8) is 0), which is
    // why we do this here.
    size_t base;
    if (fn.nargs > 1)
    {
        base = fn.arg(1).to_int();
    
        // Bases from 2 to 36 are valid, otherwise return NaN
        if (base < 2 || base > 36) return as_value(NaN);
    }
    else
    {
        /// No radix specified, so try parsing as octal or hexadecimal
        try {
            double d;
            if (as_value::parseNonDecimalInt(expr, d, false)) return d;
        }
        catch (boost::bad_lexical_cast&)
        {
            return as_value(NaN);
        }

        /// The number is not hex or octal, so we'll assume it's base-10.
        base = 10;

    }

    std::string::const_iterator it = expr.begin();

    // Check for expectional case "-0x" or "+0x", which
    // return NaN
    if ((expr.length() > 2) && (*it == '-' || *it == '+') &&
            *(it + 1) == '0' && std::toupper(*(it + 2)) == 'X') {
        return as_value(NaN);
    }
    
    // Try hexadecimal first
    if (expr.substr(0, 2) == "0x" || expr.substr(0, 2) == "0X") it += 2;
    else {
        // Skip leading whitespace
        while(*it == ' ' || *it == '\n' || *it == '\t' || *it == '\r') {
            ++it;
        }
        if (it == expr.end()) return as_value(NaN);
    }    

    bool negative = false;
    if (*it == '-' || *it == '+')
    {
        if (*it == '-') negative = true;
        
        it++;
        if (it == expr.end()) return as_value(NaN);
    }
    
    // Now we have the base, parse the digits. The iterator should
    // be pointing at the first digit.
    
    const std::string digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    // Check to see if the first digit is valid, otherwise 
    // return NaN.
    std::string::size_type digit = digits.find(toupper(*it));

    if (digit >= base || digit == std::string::npos) return as_value(NaN);

    // The first digit was valid, so continue from the present position
    // until we reach the end of the string or an invalid character,
    // adding valid characters to our result.
    // Which characters are invalid depends on the base. 
    double result = digit;
    ++it;
    
    while (it != expr.end() && (digit = digits.find(toupper(*it))) < base
            && digit != std::string::npos)
    {
        result = result * base + digit;
        ++it;
    }

    // Now return the parsed string as an integer.
    return negative ? as_value(-result) : as_value(result);
}

// ASSetPropFlags function
as_value
as_global_assetpropflags(const fn_call& fn)
{
    //log_debug(_("ASSetPropFlags called with %d args"), fn.nargs);

    // Check the arguments
    // assert(fn.nargs == 3 || fn.nargs == 4);
    // assert((version == 5) ? (fn.nargs == 3) : true);

    if (fn.nargs < 3) {
    IF_VERBOSE_ASCODING_ERRORS(    
            log_aserror(_("%s needs at least three arguments"), __FUNCTION__);
            )
         return as_value();
    }
    IF_VERBOSE_ASCODING_ERRORS(
    if (fn.nargs > 4)
            log_aserror(_("%s has more than four arguments"), __FUNCTION__);
    )
    
    // object
    boost::intrusive_ptr<as_object> obj = fn.arg(0).to_object();
    if ( ! obj )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Invalid call to ASSetPropFlags: "
            "first argument is not an object: %s"),
            fn.arg(0));
        );
        return as_value();
    }

    // list of child names

    const as_value& props = fn.arg(1);

    const int flagsMask = as_prop_flags::dontEnum |
                          as_prop_flags::dontDelete |
                          as_prop_flags::readOnly |
                          as_prop_flags::onlySWF6Up |
                          as_prop_flags::ignoreSWF6 |
                          as_prop_flags::onlySWF7Up |
                          as_prop_flags::onlySWF8Up |
                          as_prop_flags::onlySWF9Up;

    // a number which represents three bitwise flags which
    // are used to determine whether the list of child names should be hidden,
    // un-hidden, protected from over-write, un-protected from over-write,
    // protected from deletion and un-protected from deletion
    const int setTrue = int(fn.arg(2).to_number()) & flagsMask;

    // Is another integer bitmask that works like set_true,
    // except it sets the attributes to false. The
    // set_false bitmask is applied before set_true is applied

    // ASSetPropFlags was exposed in Flash 5, however the fourth argument
    // 'set_false' was not required as it always defaulted to the value '~0'. 
    const int setFalse = (fn.nargs < 4 ? 0 : fn.arg(3).to_int()) &
        flagsMask;

    obj->setPropFlags(props, setFalse, setTrue);

    return as_value();
}

// ASNative function
// See: http://osflash.org/flashcoders/undocumented/asnative?s=asnative
as_value
as_global_asnative(const fn_call& fn)
{

    // Note: it's possible for 'this' to be undefined in ActionScript,
    // which would make this call return undefined. TODO: test it in
    // the testsuite! It's not even certain whether Gnash has implemented
    // an undefined this pointer.
    boost::intrusive_ptr<as_object> ptr = ensureType<as_object>(fn.this_ptr);

    as_value ret;

    if (fn.nargs < 2)
    {
        IF_VERBOSE_ASCODING_ERRORS(    
        log_aserror(_("ASNative(%s): needs at least two arguments"),
            fn.dump_args());
        )
        return ret;
    }

    const int sx = fn.arg(0).to_int();
    const int sy = fn.arg(1).to_int();

    if ( sx < 0 )
    {
        IF_VERBOSE_ASCODING_ERRORS(    
        log_aserror(_("ASNative(%s): first arg must be >= 0"), fn.dump_args());
        )
        return ret;
    }
    if ( sy < 0 )
    {
        IF_VERBOSE_ASCODING_ERRORS(    
        log_aserror(_("ASNative(%s): second arg must be >= 0"), fn.dump_args());
        )
        return ret;
    }

    const unsigned int x = static_cast<unsigned int>(sx);
    const unsigned int y = static_cast<unsigned int>(sy);

    VM& vm = ptr->getVM();
    as_function* fun = vm.getNative(x, y);
    if ( ! fun ) {
        log_debug(_("No ASnative(%d, %d) registered with the VM"), x, y);
        return ret;
    }
    ret.set_as_function(fun);
    return ret;
        
}

// Obsolete ASnew function (exists only as ASnative(2, 0))
as_value
as_global_asnew(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("ASNative (2, 0) - old ASnew"));
    return as_value();
}

// ASSetNative function
// TODO: find dox 
as_value
as_global_assetnative(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("ASSetNative"));
    return as_value();
}

// ASSetNativeAccessor function
// TODO: find dox 
as_value
as_global_assetnativeaccessor(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("ASSetNativeAccessor"));
    return as_value();
}

// ASconstructor function
// TODO: find dox 
as_value
as_global_asconstructor(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("ASconstructor"));
    return as_value();
}


// updateAfterEvent function
as_value
as_global_updateAfterEvent(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("updateAfterEvent()"));
    return as_value();
}

namespace {

void
registerNatives(as_object& global)
{
    
    VM& vm = global.getVM();

    // ASNew was dropped as a builtin function but exists
    // as ASnative.
    vm.registerNative(as_global_assetpropflags, 1, 0);
    vm.registerNative(as_global_asnew, 2, 0);    
    vm.registerNative(as_global_assetnative, 4, 0);    
    vm.registerNative(as_global_assetnativeaccessor, 4, 1);
    vm.registerNative(as_global_updateAfterEvent, 9, 0);    
    vm.registerNative(as_global_escape, 100, 0);
    vm.registerNative(as_global_unescape, 100, 1);
    vm.registerNative(as_global_parseint, 100, 2);
    vm.registerNative(as_global_parsefloat, 100, 3);
    vm.registerNative(as_global_trace, 100, 4);
    vm.registerNative(as_global_isNaN, 200, 18);
    vm.registerNative(as_global_isfinite, 200, 19);
    vm.registerNative(timer_setinterval, 250, 0);
    vm.registerNative(timer_clearinterval, 250, 1);

    registerSelectionNative(global);
    registerColorNative(global);
    registerMathNative(global);
    registerSystemNative(global);
    registerStageNative(global);
    registerSharedObjectNative(global);

    AsBroadcaster::registerNative(global);
    TextFormat_as::registerNative(global);
    Date_as::registerNative(global);
    Mouse_as::registerNative(global);

    // LoadableObject has natives shared between LoadVars and XML, so 
    // should be registered first.
    LoadableObject::registerNative(global);
    XML_as::registerNative(global);
    XMLNode_as::registerNative(global);

}

} // anonymous namespace
} // namespace gnash

