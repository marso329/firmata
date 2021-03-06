////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#ifndef FIRMATA_ENCODER_HPP
#define FIRMATA_ENCODER_HPP

////////////////////////////////////////////////////////////////////////////////
#include "firmata/call_chain.hpp"
#include "firmata/pin.hpp"

////////////////////////////////////////////////////////////////////////////////
namespace firmata
{

////////////////////////////////////////////////////////////////////////////////
class encoder
{
public:
    ////////////////////
    encoder() = default;
    encoder(pin&, pin&);
    ~encoder();

    encoder(const encoder&) = delete;
    encoder(encoder&& rhs) noexcept { swap(rhs); }

    encoder& operator=(const encoder&) = delete;
    encoder& operator=(encoder&& rhs) noexcept { swap(rhs); return *this; }

    void swap(encoder&) noexcept;

    ////////////////////
    bool valid() const noexcept { return pin1_; }
    explicit operator bool() const noexcept { return valid(); }

    ////////////////////
    using int_call = call<void(int)>;
    using void_call = call<void()>;

    // install rotate callback
    cid on_rotate(int_call);
    cid on_rotate_cw(void_call);
    cid on_rotate_ccw(void_call);

    // remove callback
    bool remove_call(cid);

private:
    ////////////////////
    pin* pin1_ = nullptr; pin* pin2_ = nullptr; cid id_;

    // rotate call chains
    call_chain< int_call> rotate_     { 0 };
    call_chain<void_call> rotate_cw_  { 1 };
    call_chain<void_call> rotate_ccw_ { 2 };

    void pin_state_changed(int);

    enum { no, cw, ccw } step_ = no;
};

////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////
#endif
