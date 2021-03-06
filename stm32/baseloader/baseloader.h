//  Baseloader Functions
#ifndef BASELOADER_H_INCLUDED
#define BASELOADER_H_INCLUDED
#include <stdint.h>

#ifdef __cplusplus
extern "C" {  //  Allows functions below to be called by C and C++ code.
#endif

//  Bootloader version number e.g. 0x 00 01 00 01 for 1.01
#define BOOTLOADER_VERSION ((uint32_t) 0x00010001) 

//  Arbitrary magic numbers to verify that this is a valid Baseloader Vector Table
#define BASE_MAGIC_NUMBER  ((uint32_t) 0x22051969)
#define BASE_MAGIC_NUMBER2 ((uint32_t) 0x19690522)

typedef void (*baseloader_func)(void);
typedef void (*application_func)(void);

//  Baseloader Vector Table. Located just after STM32 Vector Table.
typedef struct {
	uint32_t magic_number;			//  Magic number to verify this as a Baseloader Vector Table: 0x22051969
	uint32_t version;				//  Bootloader version number e.g. 0x 00 01 00 01 for 1.01
	baseloader_func baseloader;		//  Address of the Baseloader function in ROM.
	uint32_t *baseloader_end;       //  End of Baseloader ROM (code and data).
	application_func application;	//  Start address of Application ROM. Also where the bootloader ends.
	uint32_t magic_number2;			//  Second magic number to verify that the Baseloader Vector Table was not truncated: 0x19690522
} __attribute__((packed)) base_vector_table_t;

extern base_vector_table_t base_vector_table;

//  To support different calling conventions for Baseloader we don't allow stack parameters.  All parameters must be passed via base_para at a fixed address (start of RAM).
typedef struct {
	uint32_t dest;  		//  Destination address for new Bootloader.
	uint32_t src;  			//  Source address for new Bootloader.
	uint32_t byte_count;	//  Byte size of new Bootloader.
	uint32_t restart;  		//  Set to 1 if we should restart the device after copying Bootloader.
	uint32_t preview;       //  Set to 1 if this is a preview, i.e. don't change flash ROM.
	int result;				//  Number of bytes copied, or negative for error.
	uint32_t fail;  		//  Address that caused the Baseloader to fail.
} __attribute__((packed)) base_para_t;

extern base_para_t base_para;

extern void baseloader_start(void);
extern int baseloader_fetch(baseloader_func *baseloader_addr, uint32_t **dest, const uint32_t **src, size_t *byte_count);
extern int base_flash_program_array(uint16_t* dest0, const uint16_t* src0, size_t half_word_count0);

extern void test_copy_bootloader(void);
extern void test_copy_baseloader(void);
extern void test_copy_end(void);
extern void test_baseloader1(void);
extern void test_baseloader2(void);
extern void test_baseloader_end(void);

//  Offset of Base Vector Table from the start of the flash page.
#define BASE_VECTOR_TABLE_OFFSET ( ((uint32_t) &base_vector_table) & (FLASH_PAGE_SIZE - 1) )

//  Given an address X, compute the location of the Base Vector Table of the flash memory page that contains X.
#define BASE_VECTOR_TABLE(x) 	 ( (base_vector_table_t *) ((uint32_t) FLASH_ADDRESS(x) + BASE_VECTOR_TABLE_OFFSET) )

//  Given an address X, is the Base Vector Table in that flash memory page valid (checks magic numbers)
#define IS_VALID_BASE_VECTOR_TABLE(x)  ( \
	BASE_VECTOR_TABLE(x)->magic_number  == BASE_MAGIC_NUMBER && \
	BASE_VECTOR_TABLE(x)->magic_number2 == BASE_MAGIC_NUMBER2 \
)

#ifdef __cplusplus
}  //  End of extern C scope.
#endif

#endif  //  BASELOADER_H_INCLUDED
