#ifndef KERNEL_ACPI_H
#define KERNEL_ACPI_H

#include <stdint.h>
#include <stdbool.h>

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

// Power management functions
void acpi_shutdown(void);
void acpi_reboot(void);
bool acpi_is_available(void);

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

typedef struct
{
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
} generic_address_structure_t;

typedef struct {
    acpi_sdt_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t reserved;
    uint8_t preferred_power_management_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_control;

    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_control;
    uint8_t worst_c2_latency;
    uint8_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t boot_architecture_flags;
    uint8_t reserved2;
    uint32_t flags;
    generic_address_structure_t reset_reg;
    uint8_t reset_value;
    uint8_t reserved3[3];

    // 64-bit pointers, available on ACPI 2.0+
    uint64_t x_firmware_control;
    uint64_t x_dsdt;

    generic_address_structure_t x_mp1a_event_block;
    generic_address_structure_t x_pm1b_event_block;
    generic_address_structure_t x_pm1a_control_block;
    generic_address_structure_t x_pm1b_control_block;
    generic_address_structure_t x_pm2_control_block;
    generic_address_structure_t x_pm_timer_block;
    generic_address_structure_t x_gpe0_block;
    generic_address_structure_t x_gpe1_block;
} acpi_fadt_t;

#endif
