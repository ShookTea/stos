#include <kernel/drivers/pit.h>
#include <stdint.h>
#include "../idt/idt.h"
#include "../idt/pic.h"
#include <stdio.h>
#include "../io.h"

#define PIT_INPUT_CLOCK_FREQUENCY 1193180
#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL_0_DATA_PORT 0x40

static uint32_t tick = 0;
static uint32_t second = 0;

static void pit_timer_callback()
{
    tick++;
    if (tick == 100) {
        tick = 0;
        second++;
        printf("Second %d\n", second);
    }

}

void pit_init(uint32_t frequency)
{
    // Register timer callback
    idt_register_irq_handler(PIC_LINE_PIT, &pit_timer_callback);

    // Calculating divisor. It will be sent in two bytes separately,
    // first low, then high
    uint32_t divisor = PIT_INPUT_CLOCK_FREQUENCY / frequency;
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    // Send a command:
    // 00 11 011 0
    // [00]  - channel 0
    // [11]  - access mode lobyte/hibyte
    // [011] - square wave generator
    // [0]   - 16-bit binary
    outb(PIT_COMMAND_PORT, 0b00110110);

    // Send the frequency divisor
    outb(PIT_CHANNEL_0_DATA_PORT, low);
    outb(PIT_CHANNEL_0_DATA_PORT, high);

    // Enable PIT
    pic_enable(PIC_LINE_PIT);
}
