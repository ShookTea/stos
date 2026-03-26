#include <kernel/drivers/pit.h>
#include <stdbool.h>
#include <stdint.h>
#include "../idt/idt.h"
#include "../idt/pic.h"
#include "../io.h"
#include "kernel/spinlock.h"

#define PIT_INPUT_CLOCK_FREQUENCY 1193180
#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL_0_DATA_PORT 0x40
// 1 kHz
#define PIT_FREQUENCY 1000
#define PIT_TIMEOUT_CALLBACK_SIZE 32

static uint32_t tick = 0;
static bool initialized = false;

static struct timeout_entries {
    uint32_t expire_tick;
    timeout_callback_t callback;
    void* data;
    bool active;
} timeouts[PIT_TIMEOUT_CALLBACK_SIZE];

static spinlock_t pit_lock = SPINLOCK_INIT;

static void pit_timer_callback()
{
    spinlock_acquire(&pit_lock);
    tick++;
    for (int i = 0; i < PIT_TIMEOUT_CALLBACK_SIZE; i++) {
        if (timeouts[i].active && tick >= timeouts[i].expire_tick) {
            timeouts[i].active = false;
            timeout_callback_t cb = timeouts[i].callback;
            void* data = timeouts[i].data;

            // Run callback outside the lock
            spinlock_release(&pit_lock);
            cb(data);
            spinlock_acquire(&pit_lock);
        }
    }
    spinlock_release(&pit_lock);
}

int pit_register_timeout(
    uint32_t ms,
    timeout_callback_t callback,
    void* data
) {
    spinlock_acquire(&pit_lock);
    int id = -1;
    for (int i = 0; i < PIT_TIMEOUT_CALLBACK_SIZE; i++) {
        if (!timeouts[i].active) {
            id = i;
            break;
        }
    }
    if (id != -1) {
        timeouts[id].expire_tick = tick + ms;
        timeouts[id].callback = callback;
        timeouts[id].data = data;
        timeouts[id].active = true;
    }

    spinlock_release(&pit_lock);
    return id;
}

void pit_cancel_timeout(int id)
{
    spinlock_acquire(&pit_lock);

    if (id >= 0 && id < PIT_TIMEOUT_CALLBACK_SIZE) {
        timeouts[id].active = false;
    }

    spinlock_release(&pit_lock);
}

void pit_init()
{
    if (initialized) {
        return;
    }
    initialized = true;

    // Register timer callback
    idt_register_irq_handler(PIC_LINE_PIT, &pit_timer_callback);

    // Calculating divisor. It will be sent in two bytes separately,
    // first low, then high
    uint32_t divisor = PIT_INPUT_CLOCK_FREQUENCY / PIT_FREQUENCY;
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
