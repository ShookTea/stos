#ifndef KERNEL_ACPI_H
#define KERNEL_ACPI_H

#include <stdint.h>

typedef struct {
 char signature[8];
 uint8_t checksum;
 char oemid[6];
 uint8_t revision;
 uint32_t rsdt_address;
} __attribute__((packed)) rsdp_old_t;

typedef struct {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t __deprecated; // deprecated since version 2.0

    uint32_t length;
    uint64_t rsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) rsdp_new_t;

void acpi_init_old(rsdp_old_t);
void acpi_init_new(rsdp_new_t);

#endif
