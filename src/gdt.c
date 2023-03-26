#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"

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
