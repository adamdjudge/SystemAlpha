#ifndef TIMER_H
#define TIMER_H

/* Divider frequency for the PIT chip, which should cause an IRQ 1 interrupt
   approximately 99.998 times per second, the closest we can get to 100 Hz */
#define TIMER_DIVIDER 11932

void timer_init();
uint32_t jiffies();
int timer_set_timeout(uint32_t jiffies, void (*callback)());
int timer_set_interval(uint32_t jiffies, void (*callback)());
void timer_clear(uint8_t id);

#endif
