#include "types.h"
#include "io.h"
#include "idt.h"
#include "console.h"

#include "timer.h"

/* Programmable Interrupt Timer (PIT) I/O ports */
#define PIT_DATA 0x40
#define PIT_CMD 0x43

/* Array of timers that can be set by other parts of the kernel */
static struct {
        uint8_t flags;
        uint32_t counter;
        uint32_t init;
        void (*callback)();
} timers[16] = {0};

/* Global measure of system uptime */
static uint32_t _jiffies = 0;

/* Called every time the PIT fires IRQ 0 */
void timer_handler(struct exception *e)
{
        _jiffies++;

        for (int i = 0; i < 16; i++) {
                if ((timers[i].flags != 0) && (--timers[i].counter == 0)) {
                        timers[i].callback();

                        /* Reset if interval, otherwise delete */
                        if (timers[i].flags & 0x2)
                                timers[i].counter = timers[i].init;
                        else
                                timers[i].flags = 0;
                }
        }
}

/* Get current system uptime in jiffies */
uint32_t jiffies()
{
        return _jiffies;
}

/* Initialize the PIT to fire IRQ 0 at a defined rate and install timer_handler
   as its interrupt handler */
void timer_init()
{
        outb(PIT_CMD, 0x36); /* binary, rate generator, 16-bit, counter 0 */
        outb(PIT_DATA, TIMER_DIVIDER & 0xff);
        outb(PIT_DATA, (TIMER_DIVIDER >> 8) & 0xff);

        idt_install_isr(0, timer_handler);
}

/* Set a one-shot timer that will call the provided callback after the specified
   number of jiffies passes. Return the ID of the timer used, or -1 if all
   allocated timers are active or the parameters are invalid. */
int timer_set_timeout(uint32_t jiffies, void (*callback)())
{
        if (!jiffies || !callback)
                return -1;

        for (int i = 0; i < 16; i++) {
                if (timers[i].flags == 0) {
                        timers[i].counter = jiffies;
                        timers[i].callback = callback;
                        timers[i].flags = 0x1;
                        return i;
                }
        }
        return -1;
}

/* Set an interval timer that will repeatedly call the provided callback each
   time the specified number of jiffies passes. Return the ID of the timer used,
   or -1 if all allocated timers are active or the parameters are invalid. */
int timer_set_interval(uint32_t jiffies, void (*callback)())
{
        if (!jiffies || !callback)
                return -1;

        for (int i = 0; i < 16; i++) {
                if (timers[i].flags == 0) {
                        timers[i].counter = jiffies;
                        timers[i].init = jiffies;
                        timers[i].callback = callback;
                        timers[i].flags = 0x3;
                        return i;
                }
        }
        return -1;
}

/* Clear the timer with the given ID returned by timer_set_timeout or
   timer_set_interval */
void timer_clear(uint8_t id)
{
        if (id > 15)
                return;
        
        timers[id].flags = 0;
}
