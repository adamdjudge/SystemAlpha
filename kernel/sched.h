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
        /* Used for task switching */
        uint32_t esp;
        uint32_t tss_esp0;
        uint32_t cr3;

        uint32_t state;
        uint32_t pid;

        /* Scheduling and timekeeping */
        uint32_t counter;
        uint32_t alarm;
        uint32_t rtime;
        uint32_t utime;
        uint32_t ktime;

        /* Virtual memory management */
        uint32_t *pdir;
        struct user_page *pages;
        struct user_page *ptabs;
};

#define in_user(t) (t->regs.cs == 0x1b)
#define in_kernel(t) (t->regs.cs == 0x8)

/* The currently executing task */
extern struct task *current;

/* System uptime in jiffies */
extern uint32_t jiffies;

void sched_init();
void schedule();
struct task *spawn_task();
struct task *spawn_kthread(void (*code)());
struct task *get_process(int pid);
void idle_task();

#endif
