#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"
#include "interrupt/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
const int LIMIT = 0xFFFFF;
const int NULL = 0;
const int BASE = 0;
const int TYPE_BIT = 1;
const int TYPE_CODE = 0xA;
const int TYPE_DATA = 0x2;
const int PRESENT = 1;
const int GRANULARITY = 1;
const int OP_SIZE = 1;
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,

            NULL,
            NULL,
            NULL,
            NULL,
            NULL,

            NULL,
            NULL,
            NULL
        },
        {
            (uint16_t) LIMIT,

            BASE,  
            BASE,

            TYPE_CODE,
            TYPE_BIT,
            NULL,
            PRESENT,

            (char) LIMIT,

            NULL,
            NULL,
            OP_SIZE,
            GRANULARITY,

            BASE
        },
        {
            (uint16_t) LIMIT,
            
            BASE,
            BASE,

            TYPE_DATA,
            TYPE_BIT,
            NULL,
            PRESENT,

            (char) LIMIT,

            NULL,
            NULL,
            OP_SIZE,
            GRANULARITY,

            BASE
        }
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    sizeof(global_descriptor_table), &global_descriptor_table
};

// Referensinya ada di sini https://wiki.osdev.org/GDT_Tutorial

static struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            .segment_low       = 0,
            .base_low          = 0,
            .base_mid          = 0,
            .type_bit          = 0,
            .non_system        = 0,
            .privilege_level   = 0,
            .segment_present   = 0,
            .segment_limit     = 0,
            .available_sys     = 0,
            .bitsegment_64     = 0,
            .default_opsize    = 0,
            .granularity       = 0,
            .base_high         = 0
        }, // Null Descriptor
        {
            .segment_low       = 0xFFFF,
            .base_low          = 0x0000,
            .base_mid          = 0x00,
            .type_bit          = 0xA,  // Code Segment (Executable, Readable)
            .non_system        = 1,    // Non-system segment
            .privilege_level   = 0,    // Ring 0
            .segment_present   = 1,    // Segment is present
            .segment_limit     = 0xFFFF,
            .available_sys     = 0,
            .bitsegment_64     = 1,    // 64-bit segment
            .default_opsize    = 0,    // 16-bit operands
            .granularity       = 1,    // Segment limit scaled by 4K
            .base_high         = 0x00
        }, // Kernel Code Descriptor
        {
            .segment_low       = 0xFFFF,
            .base_low          = 0x0000,
            .base_mid          = 0x00,
            .type_bit          = 0x2,  // Data Segment (Writable)
            .non_system        = 1,    // Non-system segment
            .privilege_level   = 0,    // Ring 0
            .segment_present   = 1,    // Segment is present
            .segment_limit     = 0xFFFF,
            .available_sys     = 0,
            .bitsegment_64     = 1,    // 64-bit segment
            .default_opsize    = 0,    // 16-bit operands
            .granularity       = 1,    // Segment limit scaled by 4K
            .base_high         = 0x00
        }, // Kernel Data Descriptor
        {
            .segment_low       = 0xFFFF,
            .base_low          = 0x0000,
            .base_mid          = 0x00,
            .type_bit          = 0xB,  // Code Segment (Executable, Readable)
            .non_system        = 1,    // Non-system segment
            .privilege_level   = 3,    // Ring 3
            .segment_present   = 1,    // Segment is present
            .segment_limit     = 0xFFFF,
            .available_sys     = 0,
            .bitsegment_64     = 1,    // 64-bit segment
            .default_opsize    = 0,    // 16-bit operands
            .granularity       = 1,    // Segment limit scaled by 4K
            .base_high         = 0x00
        }, // User Code Descriptor
        {
            .segment_low       = 0xFFFF,
            .base_low          = 0x0000,
            .base_mid          = 0x00,
            .type_bit          = 0x2,   // Type: Data, Writable, Non-Expandable
            .non_system        = 1,     // S bit
            .privilege_level   = 3,     // DPL
            .segment_present   = 1,     // P bit
            .segment_limit     = 0xF,   // Limit: 4KB
            .available_sys     = 0,
            .bitsegment_64     = 0,
            .default_opsize    = 1,     // Default operand size is 32 bits
            .granularity       = 1,     // Granularity: 4KB
            .base_high         = 0x00
        },
        {
            .segment_limit      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low       = sizeof(struct TSSEntry),
            .base_high         = 0,
            .base_mid          = 0,
            .base_low          = 0,

            .type_bit          = 0x9,
            .non_system        = 0,    // S bit
            .privilege_level   = 0,    // DPL
            .segment_present   = 1,    // P bit

            .bitsegment_64     = 1,    // D/B bit
            .default_opsize    = 0,    // L bit
            .granularity       = 0,    // G bit
        },
        {0}
    }
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}
