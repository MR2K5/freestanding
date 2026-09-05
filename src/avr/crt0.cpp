
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <utility>

extern std::byte __bss_start;
extern std::byte __bss_end;

extern std::byte const __flash __data_start;
extern std::byte const __flash __data_end;
extern std::byte __data_load_start;

int main();

extern "C" [[gnu::naked]] void _start() {
    // Enable stack
    asm volatile(
        R"asm(
        clr r1
        out 0x3f, r1
        ldi r16, hi8(__stack_top)
        out 0x3E, r16
        ldi r16, lo8(__stack_top)
        out 0x3D, r16
        jmp _start2)asm"
    );
}

extern "C" [[gnu::used]] void _start2() {
    std::memset(&__bss_start, 0, &__bss_end - &__bss_start);
    memcpy_p(&__data_load_start, &__data_start, &__data_end - &__data_start);

    // __libc_init_array();

    asm volatile("sei");
    int r = main();
    std::exit(r);
}

extern "C" {

[[gnu::interrupt, gnu::used, gnu::naked]] void default_interrupt_handler() {
    asm volatile("reti");
}

[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_int0();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_int1();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_pcint0();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_pcint1();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_pcint2();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_wdt();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer2_compa();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer2_compb();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer2_ovf();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer1_capt();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer1_compa();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer1_compb();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer1_ovf();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer0_compa();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer0_compb();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_timer0_ovf();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_spi_stc();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_usart_rx();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_usart_udre();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_usart_tx();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_adc();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_ee_ready();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_analog_comp();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_twi();
[[gnu::weak, gnu::alias("default_interrupt_handler")]] void irq_spm_ready();
}

[[gnu::section(".vectors"), gnu::naked, gnu::used]] static void vectors() {
    asm volatile(
        R"(jmp _start
    jmp irq_int0
    jmp irq_int1
    jmp irq_pcint0
    jmp irq_pcint1
    jmp irq_pcint2
    jmp irq_wdt
    jmp irq_timer2_compa
    jmp irq_timer2_compb
    jmp irq_timer2_ovf
    jmp irq_timer1_capt
    jmp irq_timer1_compa
    jmp irq_timer1_compb
    jmp irq_timer1_ovf
    jmp irq_timer0_compa
    jmp irq_timer0_compb
    jmp irq_timer0_ovf
    jmp irq_spi_stc
    jmp irq_usart_rx
    jmp irq_usart_udre
    jmp irq_usart_tx
    jmp irq_adc
    jmp irq_ee_ready
    jmp irq_analog_comp
    jmp irq_twi
    jmp irq_spm_ready
)"
    );
}
