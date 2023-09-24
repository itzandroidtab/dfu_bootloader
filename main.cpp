#include <klib/klib.hpp>
#include <klib/usb/device/dfu.hpp>

#include <io/usb.hpp>
#include <io/system.hpp>
#include <io/pins.hpp>
#include <io/flash.hpp>

namespace target = klib::target;

// location of the user vector table
constexpr static uint32_t app_vector_address = 0x2000;

// transfer buffer size. Same value is used as the write amount
constexpr static uint32_t transfer_size = 256;

/**
 * @brief Helper class to write to flash memory
 * 
 */
class dfu {
public:
    /**
     * @brief Write memory to flash
     * 
     * @param offset 
     * @param data 
     * @param length 
     * @return true 
     * @return false 
     */
    static bool write(uint32_t offset, uint8_t* data, uint32_t length) {
        // get the address we are trying to write
        const uint32_t address = app_vector_address + offset;

        // check if we are at the start of the sector
        const bool start_sector = (
            target::io::flash::address_to_sector(address) == 
            target::io::flash::address_to_sector(address + (0x1000 - 1))
        );

        // check if we should erase the currect sector by 
        // checking if it is blank (only do it when we are
        // at the start of the sector)
        if (start_sector && !target::io::flash::is_blank(address)) {
            // flash is not blank. Erase it
            target::io::flash::erase(target::io::flash::erase_mode::sector, address);
        }

        // write the data to the flash (always write the full dfu buffer size)
        return target::io::flash::write(address, data, transfer_size);
    }

    /**
     * @brief Get the write timeout
     * 
     * @return uint32_t 
     */
    static uint32_t get_write_timeout() {
        return 1;
    }

    /**
     * @brief Callback that gets called when the CPU should 
     * be reset
     * 
     */
    static void reset() {
        // do a system reset
        NVIC_SystemReset();
    }
};

// using for the usb driver
using usb_bulk = target::io::usb<target::io::periph::lqfp_80::usb0, klib::usb::device::dfu<dfu, transfer_size>>;

/**
 * @brief Helper function that moves the vector table and 
 * starts the user application
 * 
 * @warning this function resets the stack pointer to the
 * address in the vector table of the user code. This 
 * function should not use the stack after this is done
 * 
 */
static __attribute__((__noreturn__, __naked__)) void start_application() {
    // helper using for moving the vector table
    using irq = klib::irq_flash<16>;

    // move the vector table to the vector table 
    // of the user
    irq::init(reinterpret_cast<irq::interrupt_callback*>(app_vector_address));

    // load the stack pointer of the application
    asm volatile ("MSR msp, %0" : : "r" (*reinterpret_cast<uint32_t*>(app_vector_address)) : );

    // call the user application
    (*reinterpret_cast<void(**)()>((app_vector_address + 0x4)))();
}

int main() {
    // get the bootloader pin
    using bootloader_pin = target::io::pin_in<target::pins::package::lqfp_80::p40>;

    // init it as a pin in
    bootloader_pin::init();

    // check if we should run the user application
    if (!bootloader_pin::get() && !target::io::flash::is_blank(app_vector_address)) {
        // boot the application
        start_application();

        // we will never get here as "start_application"
        // has the noreturn attribute. We are also 
        // overriding the stack. This prevents the user
        // application from returning to this place as
        // the stack is probably pointing somewhere 
        // else by then
    }

    // setup the flash wait state to 4 + 1 CPU clocks
    target::io::system::flash::setup<4>();

    // using for setting up the main clock
    using clock = target::io::system::clock;

    // setup the clock to 96Mhz from the 12mhz oscillator 
    // to speed up the dfu flash programming
    // (((15 + 1) * 2 * 12Mhz) / (0 + 1) = 384Mhz) / (3 + 1) = 96Mhz
    clock::set_main<clock::source::main, 96'000'000, 15, 0, 3>();

    // setup the vector table for the usb interrupts
    target::irq::init();

    // bootloader mode. configure the usb pll
    target::io::system::clock::set_usb<12'000'000>();

    // init the usb hardware
    usb_bulk::init<true, true, false>();

    // wait until we are reset in the dfu handler
    while (true) {
        // do nothing
    }
}
