// AsBroadcaster.cpp - AsBroadcaster AS interface
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

#include "Array_as.h" // for _listeners construction
#include "log.h"
#include "AsBroadcaster.h"
#include "fn_call.h"
#include "builtin_function.h"
#include "NativeFunction.h" 
#include "VM.h" // for getPlayerVersion() 
#include "Object.h" // for getObjectInterface
#include "Global_as.h"
#include "namedStrings.h"

namespace gnash {

// Forward declarations.
namespace {
    as_value asbroadcaster_addListener(const fn_call& fn);
    as_value asbroadcaster_removeListener(const fn_call& fn);
    as_value asbroadcaster_broadcastMessage(const fn_call& fn);
    as_value asbroadcaster_initialize(const fn_call& fn);
    as_value asbroadcaster_ctor(const fn_call& fn);
}


/// Helper for notifying listeners
namespace {

class BroadcasterVisitor
{
    
    /// Name of the event being broadcasted
    /// appropriately cased based on SWF version
    /// of the current VM
    std::string _eventName;
    string_table::key _eventKey;

    // These two will be needed for consistency checking
    //size_t _origEnvStackSize;
    //size_t _origEnvCallStackSize;

    /// Number of event dispatches
    unsigned int _dispatched;

    fn_call _fn;

public:

    /// @param eName name of event, will be converted to lowercase if needed
    ///
    /// @param env Environment to use for functions invocation.
    ///
    BroadcasterVisitor(const fn_call& fn)
        :
        _eventName(),
        _eventKey(0),
        _dispatched(0),
        _fn(fn)
    {
        _eventName = fn.arg(0).to_string();
        _eventKey = getStringTable(fn).find(_eventName);
        _fn.drop_bottom();
    }

    /// Call a method on the given value
    void visit(as_value& v)
    {
        boost::intrusive_ptr<as_object> o = v.to_object(*getGlobal(_fn));
        if ( ! o ) return;

        as_value method;
        o->get_member(_eventKey, &method);
        _fn.super = o->get_super(_eventName.c_str());

        if (method.is_function()) {
            _fn.this_ptr = o.get();
            method.to_as_function()->call(_fn);
        }

        ++_dispatched;
    }

    /// Return number of events dispached since last reset()
    unsigned int eventsDispatched() const { return _dispatched; }

    /// Reset count od dispatched events
    void reset() { _dispatched=0; }
};

}

/// AsBroadcaster class


void 
AsBroadcaster::initialize(as_object& o)
{
    Global_as* gl = getGlobal(o);

    // Find _global.AsBroadcaster.
    as_object* asb =
        gl->getMember(NSV::CLASS_AS_BROADCASTER).to_object(*gl);

    // If it's not an object, these are left undefined, but they are
    // always attached to the initialized object.
    as_value al, rl;

    if (asb) {
        al = asb->getMember(NSV::PROP_ADD_LISTENER);
        rl = asb->getMember(NSV::PROP_REMOVE_LISTENER);
    }
    
    o.set_member(NSV::PROP_ADD_LISTENER, al);
    o.set_member(NSV::PROP_REMOVE_LISTENER, rl);
    
    // The function returned by ASnative(101, 12) is attached, even though
    // this may not exist (e.g. if _global.ASnative is altered)
    const as_value& asn = gl->callMethod(NSV::PROP_AS_NATIVE, 101, 12);
    o.set_member(NSV::PROP_BROADCAST_MESSAGE, asn);

    o.set_member(NSV::PROP_uLISTENERS, new Array_as());

}

void
attachAsBroadcasterStaticInterface(as_object& o)
{
    const int flags = PropFlags::dontEnum |
                      PropFlags::dontDelete |
                      PropFlags::onlySWF6Up;

    Global_as* gl = getGlobal(o);

    o.init_member("initialize",
            gl->createFunction(asbroadcaster_initialize), flags);
    o.init_member(NSV::PROP_ADD_LISTENER,
            gl->createFunction(asbroadcaster_addListener), flags);
    o.init_member(NSV::PROP_REMOVE_LISTENER,
            gl->createFunction(asbroadcaster_removeListener), flags);

    VM& vm = getVM(o);
    o.init_member(NSV::PROP_BROADCAST_MESSAGE, vm.getNative(101, 12),
            flags);

}


void
AsBroadcaster::registerNative(as_object& global)
{
    VM& vm = getVM(global);
    vm.registerNative(asbroadcaster_broadcastMessage, 101, 12);
}


void
AsBroadcaster::init(as_object& where, const ObjectURI& uri)
{
    // AsBroadcaster is a class, even though it doesn't look much like one.
    // Its prototype has no properties.
    registerBuiltinClass(where, asbroadcaster_ctor, 0,
            attachAsBroadcasterStaticInterface, uri);
}


namespace {

as_value
asbroadcaster_initialize(const fn_call& fn)
{
    if ( fn.nargs < 1 )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("AsBroadcaster.initialize() requires one argument, "
                "none given"));
        );
        return as_value();
    }

    // TODO: check if automatic primitive to object conversion apply here
    const as_value& tgtval = fn.arg(0);
    if ( ! tgtval.is_object() )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("AsBroadcaster.initialize(%s): first arg is "
                "not an object"), tgtval); 
        );
        return as_value();
    }

    boost::intrusive_ptr<as_object> tgt = tgtval.to_object(*getGlobal(fn));
    if ( ! tgt )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("AsBroadcaster.initialize(%s): first arg is an object"
            " but doesn't cast to one (dangling DisplayObject ref?)"), tgtval); 
        );
        return as_value();
    }

    AsBroadcaster::initialize(*tgt);

    return as_value();
}
as_value
asbroadcaster_addListener(const fn_call& fn)
{
    boost::intrusive_ptr<as_object> obj = fn.this_ptr;

    as_value newListener; assert(newListener.is_undefined());
    if ( fn.nargs ) newListener = fn.arg(0);

    obj->callMethod(NSV::PROP_REMOVE_LISTENER, newListener);

    as_value listenersValue;

    // TODO: test if we're supposed to crawl the target object's 
    //       inheritance chain in case it's own property _listeners 
    //       has been deleted while another one is found in any base
    //       class.
    if ( ! obj->get_member(NSV::PROP_uLISTENERS, &listenersValue) )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("%p.addListener(%s): this object has no _listeners "
                "member"), (void*)fn.this_ptr, fn.dump_args());
        );
        return as_value(true); // odd, but seems the case..
    }

    // assuming no automatic primitive-to-object cast will return an array...
    if (!listenersValue.is_object())
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("%p.addListener(%s): this object's _listener isn't "
                "an object: %s"), (void*)fn.this_ptr, fn.dump_args(),
                listenersValue);
        );
        return as_value(false); // TODO: check this
    }

    boost::intrusive_ptr<as_object> listenersObj =
        listenersValue.to_object(*getGlobal(fn));
    assert(listenersObj);

    boost::intrusive_ptr<Array_as> listeners = boost::dynamic_pointer_cast<Array_as>(listenersObj);
    if ( ! listeners )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("%p.addListener(%s): this object's _listener isn't "
                "an array: %s -- will call 'push' on it anyway"),
                (void*)fn.this_ptr,
                fn.dump_args(), listenersValue);
        );

        listenersObj->callMethod(NSV::PROP_PUSH, newListener);

    }
    else
    {
        listeners->callMethod(NSV::PROP_PUSH, newListener);
    }

    return as_value(true);

}


as_value
asbroadcaster_removeListener(const fn_call& fn)
{
    boost::intrusive_ptr<as_object> obj = fn.this_ptr;

    as_value listenersValue;

    // TODO: test if we're supposed to crawl the target object's 
    //       inheritance chain in case it's own property _listeners 
    //       has been deleted while another one is found in any base
    //       class.
    if (!obj->get_member(NSV::PROP_uLISTENERS, &listenersValue) )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("%p.addListener(%s): this object has no _listeners "
                "member"), (void*)fn.this_ptr, fn.dump_args());
        );
        return as_value(false); // TODO: check this
    }

    // assuming no automatic primitive-to-object cast will return an array...
    if ( ! listenersValue.is_object() )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("%p.addListener(%s): this object's _listener isn't "
                "an object: %s"), (void*)fn.this_ptr, fn.dump_args(),
                listenersValue);
        );
        return as_value(false); // TODO: check this
    }

    boost::intrusive_ptr<as_object> listenersObj =
        listenersValue.to_object(*getGlobal(fn));
    assert(listenersObj);

    as_value listenerToRemove; assert(listenerToRemove.is_undefined());
    if ( fn.nargs ) listenerToRemove = fn.arg(0);

    boost::intrusive_ptr<Array_as> listeners = 
        boost::dynamic_pointer_cast<Array_as>(listenersObj);
    if ( ! listeners )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("%p.addListener(%s): this object's _listener isn't an "
                "array: %s"), (void*)fn.this_ptr, fn.dump_args(),
                listenersValue);
        );

        // TODO: implement brute force scan of pseudo-array
        unsigned int length = 
            listenersObj->getMember(NSV::PROP_LENGTH).to_int();

        string_table& st = getStringTable(fn);

        for (unsigned int i=0; i<length; ++i)
        {
            as_value iVal(i);
            std::string n = iVal.to_string();
            as_value v = listenersObj->getMember(st.find(n));
            if ( v.equals(listenerToRemove) )
            {
                listenersObj->callMethod(NSV::PROP_SPLICE, iVal, as_value(1));
                return as_value(true); 
            }
        }

        return as_value(false); // TODO: check this
    }
    else
    {
        // Remove the first listener matching the new value
        // See http://www.senocular.com/flash/tutorials/
        // listenersasbroadcaster/?page=2
        // TODO: make this call as a normal (don't want to
        // rely on _listeners type at all)
        bool removed = listeners->removeFirst(listenerToRemove);
        return as_value(removed);
    }

}


as_value
asbroadcaster_broadcastMessage(const fn_call& fn)
{
    boost::intrusive_ptr<as_object> obj = fn.this_ptr;

    as_value listenersValue;

    // TODO: test if we're supposed to crawl the target object's 
    //       inheritance chain in case its own property _listeners 
    //       has been deleted while another one is found in any base
    //       class.
    if ( ! obj->get_member(NSV::PROP_uLISTENERS, &listenersValue) )
    {
        IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("%p.addListener(%s): this object has no "
                    "_listeners member"), (void*)fn.this_ptr,
                    fn.dump_args());
        );
        return as_value(); // TODO: check this
    }

    // assuming no automatic primitive-to-object cast will return an array...
    if ( ! listenersValue.is_object() )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("%p.addListener(%s): this object's _listener "
                "isn't an object: %s"), (void*)fn.this_ptr,
                fn.dump_args(), listenersValue);
        );
        return as_value(); // TODO: check this
    }

    boost::intrusive_ptr<Array_as> listeners =
        dynamic_cast<Array_as*>(listenersValue.to_object(*getGlobal(fn)));

    if ( ! listeners )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("%p.addListener(%s): this object's _listener "
                "isn't an array: %s"), (void*)fn.this_ptr,
                fn.dump_args(), listenersValue);
        );
        return as_value(); // TODO: check this
    }

    if (!fn.nargs)
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror("%p.broadcastMessage() needs an argument", 
            (void*)fn.this_ptr);
        );
        return as_value();
    }

    BroadcasterVisitor visitor(fn); 
    listeners->visitAll(visitor);

    unsigned int dispatched = visitor.eventsDispatched();

    if ( dispatched ) return as_value(true);

    return as_value(); 

}

// The constructor does nothing at all.
as_value
asbroadcaster_ctor(const fn_call& /*fn*/)
{
    return as_value();
}

} // anonymous namespace

} // end of gnash namespace
