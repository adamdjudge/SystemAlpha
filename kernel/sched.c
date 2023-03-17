#include "paging.h"
#include "idt.h"
#include "util.h"
#include "io.h"

#include "sched.h"

extern void switch_task();
extern void iret_to_task();

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

/* Initial kernel stack for a newly created process. */
struct kstack_template {
        uint32_t regs[8];
        uint32_t ret;
        struct exception e;
};

/*
 * Creates a new process table entry, allocates it a page directory and kernel
 * stack, and sets up the initial registers and kernel stack so that, when the
 * task is scheduled for the first time, switch_task returns to iret_to_task.
 */
struct task *spawn_task(uint32_t entry)
{
        struct task *t;
        uint32_t tmp;
        struct kstack_template *kstack;
        
        t = find_empty_task();
        if (!t)
                return NULL;
        memset(t, 0, sizeof(*t));

        t->pdir = (uint32_t*) alloc_kernel_page(PAGE_WRITABLE);
        if (!t->pdir)
                return NULL;
        t->cr3 = vtophys((uint32_t) t->pdir);
        memcpy(t->pdir, process_table[0].pdir, PAGE_SIZE);

        tmp = alloc_kernel_page(PAGE_WRITABLE);
        if (!tmp) {
                free_page(t->cr3);
                return NULL;
        }
        t->tss_esp0 = tmp + PAGE_SIZE;
        t->esp = tmp + PAGE_SIZE - sizeof(struct kstack_template);
        kstack = (struct kstack_template*) t->esp;
        memset(kstack, 0, sizeof(*kstack));
        
        kstack->e.cs = 0x1b;
        kstack->e.ds = 0x23;
        kstack->e.ss = 0x23;
        kstack->e.es = 0x23;
        kstack->e.fs = 0x23;
        kstack->e.gs = 0x23;
        kstack->e.eflags = 1 << 9; /* Enable interrupts */
        kstack->e.eip = entry;
        kstack->e.esp = 0xfffff000;
        kstack->e.eno = 32; /* Pretend we came from a timer interrupt */
        kstack->ret = (uint32_t) iret_to_task;

        t->pid = next_pid++;
        t->state = TASK_SLEEP;
        return t;
}

struct task *spawn_kthread(void (*code)())
{
        struct task *t;
        struct kstack_template *kstack;

        t = find_empty_task();
        if (!t)
                return NULL;
        memset(t, 0, sizeof(*t));

        t->esp = alloc_kernel_page(PAGE_WRITABLE);
        if (!t->esp)
                return NULL;
        t->esp += PAGE_SIZE - sizeof(struct kstack_template);
        kstack = (struct kstack_template*) t->esp;
        memset(kstack, 0, sizeof(*kstack));

        kstack->e.cs = 0x08;
        kstack->e.ds = 0x10;
        kstack->e.es = 0x10;
        kstack->e.fs = 0x10;
        kstack->e.gs = 0x10;
        kstack->e.eflags = 1 << 9; /* Enable interrupts */
        kstack->e.eip = (uint32_t) code;
        kstack->e.eno = 32; /* Pretend we came from a timer interrupt */
        kstack->ret = (uint32_t) iret_to_task;

        t->pdir = page_directory;
        t->cr3 = (uint32_t) page_directory;
        t->state = TASK_RUN;
        return t;
}

/*
 * Selects the next running task to grant CPU time and switches to it.
 * NOTE: Interrupts should be disabled before calling this!
 */
void schedule()
{
        int i;

        _next = process_table;
        for (i = 1; i < NUM_TASKS; i++) {
                if (process_table[i].state != TASK_RUN)
                        continue;
                if (_next == process_table + 0)
                        /* Idle task only picked if everything else is asleep */
                        _next = process_table + i;
                if (process_table[i].counter > _next->counter)
                        _next = process_table + i;
        }

        for (i = 1; i < NUM_TASKS; i++)
                process_table[i].counter++;
        _next->counter = 0;
        
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
