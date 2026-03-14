#include "stdlib.h"
#include <kernel/acpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void load_rsdt_pointer(uint32_t pointer)
{
    acpi_sdt_header_t* header = (acpi_sdt_header_t*)pointer;
    if (strncmp(header->signature, "RSDT", 0) != 0) {
        puts("Invalid RSDT signature");
        return;
    }
    // TODO: calculate checksum - https://wiki.osdev.org/RSDT
    acpi_rsdt_t* rsdt = (acpi_rsdt_t*)header;
    uint8_t entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4;
    for (uint8_t i = 0; i < entries; i++) {
        uint32_t sdt_pointer = rsdt->pointers_to_ther_sdt[i];
        acpi_sdt_header_t* sdt_header = (acpi_sdt_header_t*)sdt_pointer;
        putchar(sdt_header->signature[0]);
        putchar(sdt_header->signature[1]);
        putchar(sdt_header->signature[2]);
        putchar(sdt_header->signature[3]);
        puts("");
    }
    abort();
}

void acpi_init_old(rsdp_old_t rsdp)
{
    // Validating RSDP signature
    if (memcmp(rsdp.signature, "RSD PTR ", 8) != 0)
    {
        puts("Invalid RSDPv1: wrong signature");
        return;
    }

    // Validating RSDP checksum - adding all unsigned bytes should return 0
    uint8_t checksum = rsdp.checksum + rsdp.revision;
    for (int i = 0; i < 8; i++) {
        checksum += rsdp.signature[i];
        if (i < 6) {
            checksum += rsdp.oemid[i];
        }
    }
    checksum += (uint8_t)(rsdp.rsdt_address & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 8) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 16) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 24) & 0xFF);

    if (checksum != 0) {
        puts("Invalid RSDPv1: wrong checksum");
        return;
    }
    load_rsdt_pointer(rsdp.rsdt_address);
}

void acpi_init_new(rsdp_new_t rsdp)
{
    // Validating RSDP signature
    if (memcmp(rsdp.signature, "RSD PTR ", 8) != 0)
    {
        puts("Invalid RSDPv2: wrong signature");
        return;
    }

    // Validating RSDP checksum - adding all unsigned bytes should return 0
    uint8_t checksum = rsdp.checksum + rsdp.revision;
    for (int i = 0; i < 8; i++) {
        checksum += rsdp.signature[i];
        if (i < 6) {
            checksum += rsdp.oemid[i];
        }
    }
    checksum += (uint8_t)(rsdp.__deprecated & 0xFF);
    checksum += (uint8_t)((rsdp.__deprecated >> 8) & 0xFF);
    checksum += (uint8_t)((rsdp.__deprecated >> 16) & 0xFF);
    checksum += (uint8_t)((rsdp.__deprecated >> 24) & 0xFF);

    if (checksum != 0) {
        puts("Invalid RSDPv2: wrong checksum");
        return;
    }

    // Validating extended checksum
    checksum += rsdp.extended_checksum;
    checksum += rsdp.reserved[0];
    checksum += rsdp.reserved[1];
    checksum += rsdp.reserved[2];

    checksum += (uint8_t)(rsdp.length & 0xFF);
    checksum += (uint8_t)((rsdp.length >> 8) & 0xFF);
    checksum += (uint8_t)((rsdp.length >> 16) & 0xFF);
    checksum += (uint8_t)((rsdp.length >> 24) & 0xFF);

    checksum += (uint8_t)(rsdp.rsdt_address & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 8) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 16) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 24) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 32) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 40) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 48) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 56) & 0xFF);

    if (checksum != 0) {
        puts("Invalid RSDPv2: wrong extended checksum");
        return;
    }

    puts("RSDPv2 valid, but 64-bit mode not implemented yet.");
}
