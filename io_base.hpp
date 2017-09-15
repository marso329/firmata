////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2017 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL v3.
// For full terms see COPYING or visit https://www.gnu.org/licenses/gpl.html

////////////////////////////////////////////////////////////////////////////////
#ifndef FIRMATA_IO_BASE_HPP
#define FIRMATA_IO_BASE_HPP

////////////////////////////////////////////////////////////////////////////////
#include "firmata/types.hpp"

#include <functional>
#include <tuple>

////////////////////////////////////////////////////////////////////////////////
namespace firmata
{
namespace io
{

////////////////////////////////////////////////////////////////////////////////
class base
{
public:
    ////////////////////
    // blocking read/write
    virtual void write(msg_id, const payload& = { }) = 0;
    virtual std::tuple<msg_id, payload> read() = 0;

    // async read callback
    using callback = std::function<void(msg_id, const payload&)>;
    virtual void reset_async(callback = nullptr) = 0;
};

////////////////////////////////////////////////////////////////////////////////
}
}

////////////////////////////////////////////////////////////////////////////////
#endif
