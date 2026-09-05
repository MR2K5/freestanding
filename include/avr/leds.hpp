#pragma once

#include <avr/io.hpp>
#include <cstddef>
#include <cstdint>

namespace avr {

using std::byte;

struct port_t;

struct port_t {
    std::byte pin;   // Write toggles portxn
    std::byte dd;    // sets in/out. 1 is output
    std::byte port;  // In input mode: 1 enables the pull-up. In output mode 1 drives the pin high

    void set_dir(std::byte dirs) volatile { dd = dirs; }
    auto direction() const volatile { return dd; }
    bool direction(int idx) const volatile { return bool(dd & (byte(1) << idx)); }

    void set_dir(int idx, bool out) volatile {
        if (__builtin_constant_p(idx) && __builtin_constant_p(out)) {
            dd = (dd & ~(byte(1) << idx)) | (byte(out) << idx);
        } else {
            interrupt_guard _;
            dd = (dd & ~(byte(1) << idx)) | (byte(out) << idx);
        }
        // Prob no nop here is ok
    }

    byte read() const volatile { return pin; }
    byte read_latch() const volatile { return port; }
    void set(byte value) volatile { port = value; }
    void toggle() volatile { pin = byte(0xff); }

    template<std::integral Pin> consteval auto get_pin();
};

template<uintptr_t Base, int Pin> struct pin_t {
    static_assert(Pin < 8);
    static inline auto& pin_r  = *reinterpret_cast<byte volatile*>(Base);
    static inline auto& dd_r   = *reinterpret_cast<byte volatile*>(Base + 1);
    static inline auto& port_r = *reinterpret_cast<byte volatile*>(Base + 2);

    void set() { port_r = (port_r | (byte(1) << Pin)); }
    void reset() { port_r = (port_r & ~(byte(1) << Pin)); }
    void toggle() { pin_r = byte(1) << Pin; }
    void set_input() { dd_r = dd_r & ~(byte(1) << Pin); }
    void set_output() { dd_r = dd_r | (byte(1) << Pin); }
    bool read() const { return bool(pin_r & (byte(1) << Pin)); }
    bool read_dir() const { bool(dd_r & (byte(1) << Pin)); }
    bool read_latch() const { bool(port_r & (byte(1) << Pin)); }
};

inline port_t volatile& portb = *reinterpret_cast<port_t volatile*>(0x23);
inline port_t volatile& portc = *reinterpret_cast<port_t volatile*>(0x26);
inline port_t volatile& portd = *reinterpret_cast<port_t volatile*>(0x29);

template<unsigned Port, unsigned Pin> consteval auto pin() {
    static_assert(Port < 3 && Pin < 8);
    static constexpr auto base = 0x23 + 3 * Port;
    return pin_t<base, Pin>();
}

template<unsigned Port, unsigned Pin> struct direct_led {
    void set() { pin<Port, Pin>().set(); }
    void reset() { pin<Port, Pin>().reset(); }
    void toggle() { pin<Port, Pin>().toggle(); }

    // Configure the pins to work for leds
    void configure() { pin<Port, Pin>().set_output(); }
};

inline direct_led<2, 7> led_red;
inline direct_led<2, 6> led_yellow;
inline direct_led<2, 5> led_green;

}  // namespace avr
