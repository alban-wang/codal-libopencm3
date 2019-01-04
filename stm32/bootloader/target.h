/*
 * Copyright (c) 2016, Devan Lai
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef TARGET_H_INCLUDED
#define TARGET_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/usb/usbd.h>

enum StartupMode {
    UNKNOWN_MODE = 0,
    APPLICATION_MODE,
    BOOTLOADER_MODE,
};

typedef int restart_callback_type(void);
extern void boot_target_clock_setup(void);
extern void boot_target_gpio_setup(void);
extern void boot_target_set_led(int on);
extern const usbd_driver* boot_target_usb_init(void);
extern enum StartupMode boot_target_get_startup_mode(void);  //  Get the startup mode: Bootloader or Application.
extern void boot_target_set_restart_callback(restart_callback_type *func);
extern bool boot_target_get_force_bootloader(void);
extern bool boot_target_get_force_app(void);
extern void boot_target_manifest_app(void);
extern void boot_target_manifest_bootloader(void);
extern void boot_target_flash_unlock(void);
extern void boot_target_flash_lock(void);
extern bool boot_target_flash_program_array(uint16_t* dest, const uint16_t* data, size_t half_word_count);
extern void boot_target_get_serial_number(char* dest, size_t max_chars);
extern size_t boot_target_get_max_firmware_size(void);

// extern void boot_target_pre_main(void);
// extern void boot_target_log(const char* str);
// extern void boot_target_relocate_vector_table(void);

#endif
