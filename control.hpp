////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2017 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL v3.
// For full terms see COPYING or visit https://www.gnu.org/licenses/gpl.html

////////////////////////////////////////////////////////////////////////////////
#ifndef FIRMATA_CONTROL_HPP
#define FIRMATA_CONTROL_HPP

////////////////////////////////////////////////////////////////////////////////
#include "firmata/command.hpp"
#include "firmata/pin.hpp"

////////////////////////////////////////////////////////////////////////////////
namespace firmata
{

////////////////////////////////////////////////////////////////////////////////
class control
{
public:
    ////////////////////
    explicit control(io::base*);

    ////////////////////
    auto const& protocol() const { return protocol_; }
    auto const& firmware() const { return firmware_; }

    void reset();

    ////////////////////
    auto pin_count() const noexcept { return pins_.size(); }

    auto const& pin(pos n) const { return pins_.at(n); }
    auto& pin(pos n) { return pins_.at(n); }

private:
    ////////////////////
    command command_;

    firmata::protocol protocol_;
    firmata::firmware firmware_;
    firmata::pins pins_;

    void mode(firmata::pin&, firmata::mode);
    void value(firmata::pin&, int);

    ////////////////////
    void async_read(msg_id, const payload&);

    // for debugging
    void info();
};


////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////
#endif
