#include "io.h"
#include "idt.h"
#include "timer.h"
#include "console.h"

#include "fdc.h"

/* Floppy controller I/O ports */
enum {
        FDC_DOR = 0x3f2,
        FDC_MSR = 0x3f4,
        FDC_DATA = 0x3f5,
        FDC_CCR = 0x3f7
};

/* FDC DOR fields */
enum {
        FDC_DOR_DRIVE0 = 0x00,
        FDC_DOR_DRIVE1 = 0x01,
        FDC_DOR_DRIVE2 = 0x02,
        FDC_DOR_DRIVE3 = 0x03,
        FDC_DOR_ENABLE = 0x04,
        FDC_DOR_DMA = 0x08,
        FDC_DOR_MOTOR0 = 0x10,
        FDC_DOR_MOTOR1 = 0x20,
        FDC_DOR_MOTOR2 = 0x40,
        FDC_DOR_MOTOR3 = 0x80
};

/* FDC MSR fields */
enum {
        FDC_MSR_SEEK_DRIVE0 = 0x01,
        FDC_MSR_SEEK_DRIVE1 = 0x02,
        FDC_MSR_SEEK_DRIVE2 = 0x04,
        FDC_MSR_SEEK_DRIVE3 = 0x08,
        FDC_MSR_BUSY = 0x10,
        FDC_MSR_NON_DMA = 0x20,
        FDC_MSR_DATADIR = 0x40,
        FDC_MSR_RDY = 0x80
};

/* FDC command opcodes */
enum {
        FDC_CMD_READ_TRACK = 2,
        FDC_CMD_SPECIFY = 3,
        FDC_CMD_CHECK_STATUS = 4,
        FDC_CMD_WRITE_SECTOR = 5,
        FDC_CMD_READ_SECTOR = 6,
        FDC_CMD_CALIBRATE = 7,
        FDC_CMD_SENSE_INTERRUPT = 8,
        FDC_CMD_FORMAT_TRACK = 13,
        FDC_CMD_SEEK = 15
};

static uint8_t wait_done = 0;

void fdc_irq(struct exception *e)
{
        wait_done = 1;
}

static void fdc_wait_irq()
{
        while (!wait_done);
        wait_done = 0;
        kprintf("fdc: done int wait\n");
}

static void fdc_send_data(uint8_t val)
{
        while (!(inb(FDC_MSR) & FDC_MSR_RDY));
        outb(FDC_DATA, val);
}

static uint8_t fdc_read_data()
{
        while (!(inb(FDC_MSR) & FDC_MSR_RDY));
        return inb(FDC_DATA);
}

static void fdc_set_motor(uint8_t drive, uint8_t setting)
{
        outb(FDC_DOR, FDC_DOR_ENABLE
                      | ((FDC_DOR_MOTOR0 * setting) << drive));
        int jif = jiffies();
        while (jiffies() < jif+30);
}

static void fdc_sense_interrupt(uint8_t *st0, uint8_t *cyl)
{
        kprintf("fdc: sense interrupt\n");
        fdc_send_data(FDC_CMD_SENSE_INTERRUPT);
        *st0 = fdc_read_data();
        *cyl = fdc_read_data();
}

static void fdc_specify(uint8_t steprate, uint8_t loadt, uint8_t unloadt)
{
        kprintf("fdc: specify %d,%d,%d\n", steprate, loadt, unloadt);
        fdc_send_data(FDC_CMD_SPECIFY);
        fdc_send_data((steprate << 4) | unloadt);
        fdc_send_data((loadt << 1) | 1);
}

static int fdc_calibrate(uint8_t drive)
{
        uint8_t st0, cyl;

        if (drive > 3)
                return -1;
        
        fdc_set_motor(drive, 1);

        for (int i = 0; i < 5; i++) {
                kprintf("fdc: calibrate attempt %d\n", i+1);
                fdc_send_data(FDC_CMD_CALIBRATE);
                fdc_send_data(drive);
                //fdc_wait_irq();
                int jif = jiffies();
                while (jiffies() < jif+100);
                fdc_sense_interrupt(&st0, &cyl);
                if (!cyl) {
                        fdc_set_motor(drive, 0);
                        return 0;
                }
        }

        kprintf("fdc: calibrate failed\n");
        fdc_set_motor(drive, 0);
        return -1;
}

static void fdc_configure()
{
        fdc_send_data(0x13);
        fdc_send_data(0);
        fdc_send_data((1<<6) | (0<<5) | (0<<4) | 0);
        fdc_send_data(0);
}

void fdc_timeout()
{
        kprintf("fdc: msr %w\n", inb(FDC_MSR));
        kpanic("fdc: reset timed out\n");
}

static int fdc_reset()
{
        int id = timer_set_timeout(1000, fdc_timeout);
        kprintf("fdc: reset\n");
        wait_done = 0;
        outb(FDC_DOR, 0);
        outb(FDC_DOR, FDC_DOR_ENABLE);
        //fdc_wait_irq();

        uint8_t st0, cyl;
        fdc_sense_interrupt(&st0, &cyl);
        fdc_sense_interrupt(&st0, &cyl);
        fdc_sense_interrupt(&st0, &cyl);
        fdc_sense_interrupt(&st0, &cyl);

        outb(FDC_CCR, 0); /* 500 kb/s transfer speed */

        fdc_configure();
        fdc_specify(8, 20, 0);
        int ret = fdc_calibrate(0);
        timer_clear(id);
        return ret;
}

int fdc_read_sector(uint8_t head, uint8_t track, uint8_t sector, void *buffer)
{
        uint8_t st0, st1, cyl;
        uint8_t *buf = (uint8_t*) buffer;

        fdc_set_motor(0, 1);

        fdc_send_data(FDC_CMD_READ_SECTOR | 0xe0);
        fdc_send_data(head << 2);
        fdc_send_data(track);
        fdc_send_data(head);
        fdc_send_data(sector);
        fdc_send_data(2);
        fdc_send_data(18);
        fdc_send_data(27);
        fdc_send_data(0xff);

        //fdc_wait_irq();
        for (int i = 0; i < 512; i++)
                *(buf++) = fdc_read_data();
        
        st0 = fdc_read_data();
        st1 = fdc_read_data();
        kprintf("fdc: read status %w %w\n", st0, st1);
        for (int i = 0; i < 5; i++)
                fdc_read_data();
        
        fdc_set_motor(0, 0);
        return 0;
}

char buf[512] = {0};

void fdc_init()
{
        idt_install_isr(6, fdc_irq);
        if (fdc_reset())
                kpanic("fdc: failed to init");
        else
                kprintf("fdc: init success\n");
        
        fdc_read_sector(0, 0, 1, buf);
        for (int i = 0; i < 512; i++)
                *((char*) (0xff000 + 160*18 + 2*i)) = buf[i];
}
