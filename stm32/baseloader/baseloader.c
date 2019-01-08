#include <logger/logger.h>
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/systick.h>  //  For STK_CSR
#include <libopencm3/stm32/desig.h>  //  For DESIG_FLASH_SIZE
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/rcc.h>    //  For RCC_CIR
#include "flash-config.h"
#include "baseloader.h"

extern void application_start(void);

//  Baseloader Vector Table. Located just after STM32 Vector Table.

__attribute__ ((section(".base_vectors")))
base_vector_table_t base_vector_table = {
	.magic_number = BASE_MAGIC_NUMBER,  //  Magic number to verify this as a Baseloader Vector Table.
	.version      = BOOTLOADER_VERSION, //  Bootloader version number e.g. 0x 00 01 00 01 for 1.01.
	.baseloader   = baseloader_start,   //  Address of the baseloader function.
	.application  = application_start,  //  Address of application. Also where the bootloader ends.
};

//  Offset of Base Vector Table from the start of the flash page.
#define BASE_VECTOR_TABLE_OFFSET (((uint32_t) &base_vector_table) & (FLASH_PAGE_SIZE - 1))
#define FLASH_ADDRESS(x) 		 (((uint32_t) x) & ~(FLASH_PAGE_SIZE - 1))

//  Given an address X, compute the location of the Base Vector Table of the memory block that contains X.
#define BASE_VECTOR_TABLE(x) 	 ((base_vector_table_t *) ((uint32_t) FLASH_ADDRESS(x) + BASE_VECTOR_TABLE_OFFSET))

//  Flash functions for Baseloader, prefixed by "base_flash". We define them here instead of using libopencm3 to prevent any external references.
//  We use macros to avoid absolute address references to functions, since the Baseloader must run in low and high memory.
//  TODO: Support other than F1

//  Based on https://github.com/libopencm3/libopencm3/blob/master/lib/stm32/common/flash_common_f.c

/* Authorize the FPEC access. */
#define base_flash_unlock() { \
	FLASH_KEYR = FLASH_KEYR_KEY1; \
	FLASH_KEYR = FLASH_KEYR_KEY2; \
}

#define base_flash_lock() { \
	FLASH_CR |= FLASH_CR_LOCK; \
}

//  Based on https://github.com/libopencm3/libopencm3/blob/master/lib/stm32/f1/flash.c

/*---------------------------------------------------------------------------*/
/** @brief Read All Status Flags
The programming error, end of operation, write protect error and busy flags
are returned in the order of appearance in the status register.
Flags for the upper bank, where appropriate, are combined with those for
the lower bank using bitwise OR, without distinction.
@returns uint32_t. bit 0: busy, bit 2: programming error, bit 4: write protect
error, bit 5: end of operation.
*/

uint32_t base_flash_get_status_flags(void)
{
	uint32_t flags = (FLASH_SR & (FLASH_SR_PGERR |
			FLASH_SR_EOP |
			FLASH_SR_WRPRTERR |
			FLASH_SR_BSY));
	if (DESIG_FLASH_SIZE > 512) {
		flags |= (FLASH_SR2 & (FLASH_SR_PGERR |
			FLASH_SR_EOP |
			FLASH_SR_WRPRTERR |
			FLASH_SR_BSY));
	}

	return flags;
}

//  Based on https://github.com/libopencm3/libopencm3/blob/master/lib/stm32/common/flash_common_f01.c

void base_flash_wait_for_last_operation(void)
{
	while ((base_flash_get_status_flags() & FLASH_SR_BSY) == FLASH_SR_BSY);
}

/*---------------------------------------------------------------------------*/
/** @brief Program a Half Word to FLASH
This performs all operations necessary to program a 16 bit word to FLASH memory.
The program error flag should be checked separately for the event that memory
was not properly erased.
Status bit polling is used to detect end of operation.
@param[in] address Full address of flash half word to be programmed.
@param[in] data half word to write
*/

void base_flash_program_half_word(uint32_t address, uint16_t data)
{
	base_flash_wait_for_last_operation();

	if ((DESIG_FLASH_SIZE > 512) && (address >= FLASH_BASE+0x00080000)) {
		FLASH_CR2 |= FLASH_CR_PG;
	} else {
		FLASH_CR |= FLASH_CR_PG;
	}

	MMIO16(address) = data;

	base_flash_wait_for_last_operation();

	if ((DESIG_FLASH_SIZE > 512) && (address >= FLASH_BASE+0x00080000)) {
		FLASH_CR2 &= ~FLASH_CR_PG;
	} else {
		FLASH_CR &= ~FLASH_CR_PG;
	}
}

static uint16_t* get_flash_end(void) {
#ifdef FLASH_SIZE_OVERRIDE
    /* Allow access to the flash size we define */
    return (uint16_t*)(FLASH_BASE + FLASH_SIZE_OVERRIDE);
#else
    /* Only allow access to the chip's self-reported flash size */
    return (uint16_t*)(FLASH_BASE + (size_t)DESIG_FLASH_SIZE*FLASH_PAGE_SIZE);
#endif
}

static inline uint16_t* get_flash_page_address(uint16_t* dest) {
    return (uint16_t*)(((uint32_t)dest / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE);
}

#define debug_flash() \
    debug_print("target_flash "); debug_printhex_unsigned((size_t) dest); \
    debug_print(", data "); debug_printhex_unsigned((size_t) data);  \
    debug_print(" to "); debug_printhex_unsigned((size_t) flash_end);  \
    debug_print(", hlen "); debug_printhex_unsigned((size_t) half_word_count);  \
    debug_println("");

//  TODO
#define ROM_START ((uint32_t) 0x08000000)
#define ROM_SIZE  ((uint32_t) 0x10000)

//  Get Old Base Vector Table at 0x800 0000.  Get Old Application Address from Old Base Vector Table, truncate to block of 1024 bytes.
//  If Base Magic Number exists at the Old Application Address, then use it as the New Base Vector Table.
//  Get New Application Address from New Base Vector Table.  
//  If Base Magic Number exists at the New Application Address, 
//  then the blocks between Old App Address and New App Address are the new Bootloader Blocks.
//  Flash them to 0x800 0000 and restart.

//  To perform flashing, jump to the New Baseloader Address in the New Base Vector Table,
//  adjusted to the New Base Vector Table Address.

bool base_flash_program_array(uint16_t* dest, const uint16_t* data, size_t half_word_count) {
	//  TODO: Validate before flashing.
	static uint32_t flags;
    static bool verified;
    static uint16_t* erase_start;
    static uint16_t* erase_end;
    static const uint16_t* flash_end;

	flags = 0;
    verified = true;
    erase_start = NULL;
    erase_end = NULL;
    flash_end = get_flash_end();  /* Remember the bounds of erased data in the current page */

	debug_flash(); ////
	
    while (half_word_count > 0) {
        /* Avoid writing past the end of flash */
        if (dest >= flash_end) {
            //  debug_println("dest >= flash_end"); debug_flush();
            verified = false;
            break;
        }
        if (dest >= erase_end || dest < erase_start) {
            erase_start = get_flash_page_address(dest);
            erase_end = erase_start + (FLASH_PAGE_SIZE)/sizeof(uint16_t);
            flash_erase_page((uint32_t)erase_start);
        }
        flash_program_half_word((uint32_t)dest, *data);
        erase_start = dest + 1;
        if (*dest != *data) {
            //  debug_println("*dest != *data"); debug_flush();
            verified = false;
            break;
        }
        dest++;
        data++;
        half_word_count--;
    }
    return verified;
}

#define debug_dump() \
    debug_print("src  "); debug_printhex_unsigned((size_t) src); debug_println(""); \
    debug_print("dest "); debug_printhex_unsigned((size_t) dest); debug_println(""); debug_force_flush(); \
    debug_print("before "); debug_printhex_unsigned(*dest); debug_println(""); debug_force_flush();

#define debug_dump2() \
    debug_print("after "); debug_printhex_unsigned(*dest); \
    debug_print(" / "); debug_printhex(ok); \
	debug_print((*dest == *src) ? " OK " : " FAIL "); \
	debug_println("\r\n"); debug_force_flush();

void baseloader_start(void) {
    debug_println("baseloader_start"); debug_force_flush();

	//  Disable interrupts because the vector table may be overwritten during flashing.
	__asm__("CPSID I\n");  //  Was: cm_disable_interrupts();

	//  From https://github.com/cnoviello/mastering-stm32
	STK_CSR = 0;  //  Disables SysTick timer and its related interrupt	
	RCC_CIR = 0;  //  Disable all interrupts related to clock

	//  Test the baseloader: Copy a page from low flash memory to high flash memory.
	uint32_t *src =  (uint32_t *) (ROM_START);
	uint32_t *dest = (uint32_t *) (ROM_START + ROM_SIZE - FLASH_PAGE_SIZE);

	debug_dump(); ////
	base_flash_unlock();
	bool ok = base_flash_program_array((uint16_t *) dest, (uint16_t *) src, FLASH_PAGE_SIZE / 2);
	base_flash_lock();
	debug_dump2(); ////

	src = (uint32_t *) (ROM_START + FLASH_PAGE_SIZE);

	debug_dump(); ////
	base_flash_unlock();
	ok = base_flash_program_array((uint16_t *) dest, (uint16_t *) src, FLASH_PAGE_SIZE / 2);
	base_flash_lock();
	debug_dump2(); ////

	//  Vector table may be overwritten. Restart to use the new vector table.
    //  TODO: scb_reset_system();
	//  Should not return.
	//  for (;;) {}
}