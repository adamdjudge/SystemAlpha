#ifndef SCHED_H
#define SCHED_H

#include "types.h"
#include "interrupt.h"

/* Divider frequency for the PIT chip, which should cause an IRQ 0 interrupt
   approximately 99.998 times per second, the closest we can get to 100 Hz */
#define TIMER_DIVIDER 11932

/* Programmable Interrupt Timer (PIT) I/O ports */
#define PIT_DATA 0x40
#define PIT_CMD 0x43

#define NUM_TASKS 64

/* Process state types */
enum {
        TASK_NONE,
        TASK_RUN,
        TASK_SLEEP,
        TASK_WAIT,
};

/* List entry defining a page mapped into a user process's address space */
struct user_page {
        uint32_t kvaddr;
        uint32_t uvaddr;
        struct user_page *next;
};

/* Process table entry, containing a task's state */
struct task {
        uint32_t state;
        uint32_t pid;
        struct registers regs;
        uint32_t cr3;
        uint32_t *page_dir;
        uint32_t counter;
        uint32_t timer;

        struct user_page *pages;
        struct user_page *page_tables;
};

void sched_init();
void schedule();
struct task *spawn_user_process();
struct task *spawn_kernel_task(void (*code)());

extern struct task *current;

extern uint32_t jiffies;

#endif
