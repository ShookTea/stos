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

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} acpi_sdt_header_t;

typedef struct {
    acpi_sdt_header_t header;
    uint32_t pointers_to_ther_sdt[0];
} acpi_rsdt_t;

void acpi_init_old(rsdp_old_t);
void acpi_init_new(rsdp_new_t);

#endif
