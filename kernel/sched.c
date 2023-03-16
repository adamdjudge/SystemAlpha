#include "paging.h"
#include "idt.h"
#include "util.h"
#include "io.h"

#include "sched.h"

extern void switch_task();

extern uint32_t page_directory[];

static struct task process_table[NUM_TASKS];
static uint32_t next_pid;
static uint32_t schedule_timer;

struct task *current;
struct task *_next;
uint32_t jiffies;

static struct task *find_empty_task()
{
        for (int i = 0; i < NUM_TASKS; i++) {
                if (process_table[i].state == TASK_NONE)
                        return &process_table[i];
        }
        return NULL;
}

struct task *get_process(int pid)
{
        if (!pid)
                return NULL;

        for (int i = 0; i < NUM_TASKS; i++) {
                if (process_table[i].pid == pid)
                        return &process_table[i];
        }
        return NULL;
}

struct task *spawn_kthread(void (*code)())
{
        struct task *t = find_empty_task();
        memset(t, 0, sizeof(*t));

        t->pdir = page_directory;
        t->cr3 = (uint32_t) page_directory;
        t->regs.cs = 0x8;
        t->regs.ds = 0x10;
        t->regs.ss = 0x10;
        t->regs.es = 0x10;
        t->regs.fs = 0x10;
        t->regs.gs = 0x10;
        t->regs.eflags = 1 << 9; // enable interrupts
        t->regs.eip = (uint32_t) code;
        t->regs.esp = alloc_kernel_page(PAGE_WRITABLE) + PAGE_SIZE;
        t->state = TASK_RUN;

        return t;
}

struct task *spawn_user_process()
{
        struct task *t = find_empty_task();
        memset(t, 0, sizeof(*t));
        
        t->pdir = (uint32_t*) alloc_kernel_page(PAGE_WRITABLE);
        if (!t->pdir)
                return NULL;
        t->cr3 = vtophys((uint32_t) t->pdir);
        memcpy(t->pdir, process_table[0].pdir, PAGE_SIZE);

        t->kstack = alloc_kernel_page(PAGE_WRITABLE);
        if (!t->kstack) {
                free_page(t->cr3);
                return NULL;
        }
        
        t->regs.cs = 0x1b;
        t->regs.ds = 0x23;
        t->regs.ss = 0x23;
        t->regs.es = 0x23;
        t->regs.fs = 0x23;
        t->regs.gs = 0x23;
        t->regs.eflags = 1 << 9; // enable interrupts
        t->regs.eip = 0x80000000;
        t->regs.esp = 0xfffff000;
        t->regs.eax = t->pid = next_pid++;
        t->state = TASK_RUN;
        t->counter = 0;

        return t;
}

/*
 * Selects the next running task to grant CPU time and switches to it.
 */
void schedule()
{
        int i;

        for (i = 1; i < NUM_TASKS; i++) {
                if (process_table[i].state != TASK_RUN)
                        continue;
                if (next->pid == 0)
                        /* Idle task only picked if everything else is asleep */
                        next = &process_table[i];
                if (process_table[i].counter > next->counter)
                        next = &process_table[i];
        }

        for (i = 1; i < NUM_TASKS; i++)
                process_table[i].counter++;
        next->counter = 0;
        
        schedule_timer = 10;
        switch_task();
}

/*
 * Handles interrupts from the PIT. Checks if any task's sleep timer has expired
 * and switches to that task if it has. Otherwise invokes the scheduler on a
 * regular basis.
 */
static void handle_timer()
{
        int i;

        jiffies++;
        schedule_timer--;

        current->rtime++;
        if (in_kernel(current))
                current->ktime++;
        else
                current->utime++;

        for (i = 0; i < NUM_TASKS; i++) {
                if (!process_table[i].alarm)
                        continue;
                else if (process_table[i].alarm < 10)
                        process_table[i].alarm = 0;
                else
                        process_table[i].alarm -= 10;
        }

        if (schedule_timer == 0)
                schedule();
}

void sched_init()
{
        memset(process_table, 0, NUM_TASKS * sizeof(struct task));
        next_pid = 1;
        jiffies = 0;
        schedule_timer = 10;
        current = process_table;

        /* Initialize the idle task, which main() jumps to later */
        process_table[0].pdir = page_directory;
        process_table[0].cr3 = (uint32_t) page_directory;
        process_table[0].state = TASK_RUN;

        /* Set up the PIT */
        outb(PIT_CMD, 0x36, false); /* binary, rate gen, 16-bit, counter 0 */
        outb(PIT_DATA, TIMER_DIVIDER & 0xff, false);
        outb(PIT_DATA, (TIMER_DIVIDER >> 8) & 0xff, false);
        idt_install_isr(0, handle_timer);
}

/*
 * The idle task, which the kernel jumps to after initializing everything. The
 * scheduler only runs this process if there are no other running tasks.
 */
void idle_task()
{
        asm("sti");
        for (;;) {}
}
