////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2017 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL v3.
// For full terms see COPYING or visit https://www.gnu.org/licenses/gpl.html

////////////////////////////////////////////////////////////////////////////////
#include "firmata/control.hpp"

#include <algorithm>
#include <functional>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////
namespace firmata
{

////////////////////////////////////////////////////////////////////////////////
control::control(io::base* io) : io_(io)
{
    reset_();

    query_version();
    query_firmware();

    query_capability();
    query_analog_mapping();
    query_state();
    report_all();

    //info();

    using namespace std::placeholders;
    io_->reset_async(std::bind(&control::async_read, this, _1, _2));
}

////////////////////////////////////////////////////////////////////////////////
void control::reset()
{
    reset_();
    query_state();
}

////////////////////////////////////////////////////////////////////////////////
payload control::read_until(msg_id reply_id)
{
    msg_id id;
    payload data;

    do { std::tie(id, data) = io_->read(); }
    while(id != reply_id);

    return data;
}

////////////////////////////////////////////////////////////////////////////////
void control::query_version()
{
    io_->write(version);
    auto data = read_until(version);

    assert(data.size() == 2);
    protocol_.major = data[0];
    protocol_.minor = data[1];
}

////////////////////////////////////////////////////////////////////////////////
void control::query_firmware()
{
    io_->write(firmware_query);
    auto data = read_until(firmware_response);

    assert(data.size() >= 2);
    firmware_.major = data[0];
    firmware_.minor = data[1];
    firmware_.name = to_string(data.begin() + 2, data.end());
}

////////////////////////////////////////////////////////////////////////////////
void control::query_capability()
{
    io_->write(capability_query);
    auto data = read_until(capability_response);

    using namespace std::placeholders;
    auto fn_mode = std::bind(&control::mode, this, _1, _2);
    auto fn_value = std::bind(&control::value, this, _1, _2);

    firmata::pos pos = 0;
    firmata::pin pin(pos, fn_mode, fn_value);

    for(auto ci = data.begin(); ci < data.end(); ++ci)
        if(*ci == 0x7f)
        {
            pins_.push_back(firmata::pin(++pos, fn_mode, fn_value));

            using std::swap;
            swap(pin, pins_.back());
        }
        else
        {
            auto mode = static_cast<firmata::mode>(*ci);
            auto res = static_cast<firmata::res>(*++ci);

            pin.delegate_.add(mode);
            pin.delegate_.add(mode, res);
        }

    // ensure no garbage at end
    assert(pin.modes().empty());
}

////////////////////////////////////////////////////////////////////////////////
void control::query_analog_mapping()
{
    io_->write(analog_mapping_query);
    auto data = read_until(analog_mapping_response);

    auto pi = pins_.begin();
    for(auto ci = data.begin(); ci < data.end(); ++ci, ++pi)
        if(*ci != 0x7f)
        {
            assert(pi < pins_.end());
            pi->delegate_.analog(*ci);
        }
}

////////////////////////////////////////////////////////////////////////////////
void control::query_state()
{
    for(auto& pin : pins_)
    {
        io_->write(pin_state_query, { pin.pos() });
        auto data = read_until(pin_state_response);

        assert(data.size() >= 3);
        assert(data[0] == pin.pos());

        auto mode = static_cast<firmata::mode>(data[1]);
        auto state = to_value(data.begin() + 2, data.end());

        pin.delegate_.mode(mode);
        pin.delegate_.state(state);
    }
}

////////////////////////////////////////////////////////////////////////////////
void control::report_all()
{
    for(auto& pin : pins_)
        if(input(pin.mode()))
        {
                if(digital(pin.mode())) report_digital(pin, true);
            else if(analog(pin.mode())) report_analog(pin, true);
        }
}

////////////////////////////////////////////////////////////////////////////////
void control::mode(firmata::pin& pin, firmata::mode mode)
{
    if(input(pin.mode()))
    {
            if(digital(pin.mode())) report_digital(pin, false);
        else if(analog(pin.mode())) report_analog(pin, false);
    }

    pin_mode(pin, mode);

    if(input(pin.mode()))
    {
            if(digital(pin.mode())) report_digital(pin, true);
        else if(analog(pin.mode())) report_analog(pin, true);
    }
}

////////////////////////////////////////////////////////////////////////////////
void control::value(firmata::pin& pin, int value)
{
        if(digital(pin.mode())) digital_value(pin, value);
    else if(analog(pin.mode())) analog_value(pin, value);
}

////////////////////////////////////////////////////////////////////////////////
void control::pin_mode(firmata::pin& pin, firmata::mode mode)
{
    pin.delegate_.mode(mode);
    io_->write(firmata::pin_mode, { pin.pos(), mode });
}

////////////////////////////////////////////////////////////////////////////////
void control::digital_value(firmata::pin& pin, bool value)
{
    pin.delegate_.value(value);
    io_->write(firmata::digital_value, { pin.pos(), value });
}

////////////////////////////////////////////////////////////////////////////////
void control::analog_value(firmata::pin& pin, int value)
{
    assert(pin.analog_pos() != npos);

    pin.delegate_.value(value);
    if(pin.analog_pos() <= 15 && value <= 16383)
    {
        auto id = static_cast<msg_id>(analog_value_base + pin.analog_pos());
        io_->write(id, to_data(value));
    }
    else
    {
        payload data = to_data(value);
        data.insert(data.begin(), pin.analog_pos());

        io_->write(ext_analog_value, data);
    }
}

////////////////////////////////////////////////////////////////////////////////
void control::report_digital(const firmata::pin& pin, bool value)
{
    int port = pin.pos() / 8, bit = pin.pos() % 8;
    assert(port <= 15);

    ports_[port].set(bit, value);

    auto id = static_cast<msg_id>(report_port_base + port);
    io_->write(id, { ports_[port].any() });
}

////////////////////////////////////////////////////////////////////////////////
void control::report_analog(const firmata::pin& pin, bool value)
{
    assert(pin.analog_pos() != npos);
    assert(pin.analog_pos() <= 15);

    auto id = static_cast<msg_id>(report_analog_base + pin.analog_pos());
    io_->write(id, { value });
}

////////////////////////////////////////////////////////////////////////////////
void control::sample_rate(const msec& time)
{
    int value = std::min<int>(time.count(), 16383);
    io_->write(firmata::sample_rate, to_data(value));
}

////////////////////////////////////////////////////////////////////////////////
void control::reset_() { io_->write(firmata::reset); }

////////////////////////////////////////////////////////////////////////////////
void control::async_read(msg_id, const payload&)
{

}

////////////////////////////////////////////////////////////////////////////////
namespace
{

auto to_string(firmata::mode mode)
{
    switch(mode)
    {
    case digital_in : return "digital_in" ;
    case digital_out: return "digital_out";
    case analog_in  : return "analog_in"  ;
    case pwm        : return "pwm"        ;
    case servo      : return "servo"      ;
    case shift      : return "shift"      ;
    case i2c        : return "i2c"        ;
    case onewire    : return "onewire"    ;
    case stepper    : return "stepper"    ;
    case encoder    : return "encoder"    ;
    case serial     : return "serial"     ;
    case pullup_in  : return "pullup_in"  ;
    default         : return "UNKNOWN"    ;
    };
}

}

////////////////////////////////////////////////////////////////////////////////
void control::info()
{
    using namespace std;

    cout << "PROTOCOL: "
         << protocol().major << "." << protocol().minor
         << endl;

    cout << "FIRMWARE: "
         << firmware().name  << " "
         << firmware().major << "." << firmware().minor
         << endl;

    for(auto n = 0; n < pin_count(); ++n)
    {
        cout << "PIN" << endl;
        auto& pin = this->pin(n);

        cout << "  digital: " << int(pin.pos()) << endl;
        if(pin.analog_pos() != npos)
            cout << "  analog: " << int(pin.analog_pos()) << endl;

        cout << "  mode: " << to_string(pin.mode()) << endl;
        cout << "  res: " << pin.res() << endl;

        cout << "  modes: ";
        for(auto mode : pin.modes()) cout << to_string(mode) << " ";
        cout << endl;

        cout << "  state: " << pin.state() << endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
}
