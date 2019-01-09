//  Baseloader Functions
#ifndef BASELOADER_H_INCLUDED
#define BASELOADER_H_INCLUDED
#include <stdint.h>

#ifdef __cplusplus
extern "C" {  //  Allows functions below to be called by C and C++ code.
#endif

#define BOOTLOADER_VERSION ((uint32_t) 0x00010001) 

#define BASE_MAGIC_NUMBER  ((uint32_t) 0x22051969)

typedef int (*baseloader_func)(void);
typedef void (*application_func)(void);

//  Baseloader Vector Table. Located just after STM32 Vector Table.
typedef struct {
	uint32_t magic_number;			//  Magic number to verify this as a Baseloader Vector Table: 0x22051969
	uint32_t version;				//  Bootloader version number e.g. 0x 00 01 00 01 for 1.01
	baseloader_func baseloader;		//  Address of the baseloader function.
	uint32_t baseloader_end;        //  End of the baseloader code and data.
	application_func application;	//  Address of application. Also where the bootloader ends.
} __attribute__((packed)) base_vector_table_t;

extern int baseloader_start(void);
// extern bool base_flash_program_array(uint16_t* dest0, const uint16_t* src0, size_t half_word_count0);

extern void test_copy_bootloader(void);
extern void test_copy_vector(void);
extern void test_copy_end(void);
extern void test_baseloader1(void);
extern void test_baseloader2(void);
extern void test_baseloader_end(void);

#ifdef __cplusplus
}  //  End of extern C scope.
#endif

#endif  //  BASELOADER_H_INCLUDED
