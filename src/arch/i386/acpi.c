#include <kernel/acpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t rsdt_physical_address = 0;

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

    // TODO: in 64-bit mode this will contain a full 64-bit address
    rsdt_physical_address = (uint32_t)(rsdp.rsdt_address & 0xFFFFFFFF);
}
