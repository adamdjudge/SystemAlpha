#include <asm/io.h>
#include <kernel/kernel.h>
#include <kernel/pci.h>

#define PCI_ADDR 0xcf8
#define PCI_DATA 0xcfc

static inline uint32_t pci_set_config(struct pci_addr addr, uint8_t offset)
{
        uint32_t address = ((uint32_t) addr.bus << 16)
                           | ((uint32_t) addr.device << 11)
                           | ((uint32_t) addr.function << 8)
                           | (offset & 0xfc)
                           | (1 << 31);
        outl(PCI_ADDR, address);
}

static uint8_t pci_config_read8(struct pci_addr addr, uint8_t offset)
{
        pci_set_config(addr, offset);
        return inl(PCI_DATA) >> ((offset & 0x3) * 8);
}

static uint16_t pci_config_read16(struct pci_addr addr, uint8_t offset)
{
        pci_set_config(addr, offset);
        return inl(PCI_DATA) >> (((offset >> 1) & 0x1) * 16);
}

static uint32_t pci_config_read32(struct pci_addr addr, uint8_t offset)
{
        pci_set_config(addr, offset);
        return inl(PCI_DATA);
}

/* The jankiest possible way to do this. Hopefully temporary. */
char *pci_bdf_string(struct pci_addr addr)
{
        char *hex = "0123456789abcdef";
        static char buf[8];

        buf[0] = hex[addr.bus >> 4];
        buf[1] = hex[addr.bus & 0xf];
        buf[2] = ':';
        buf[3] = hex[addr.device >> 4];
        buf[4] = hex[addr.device & 0xf];
        buf[5] = '.';
        buf[6] = hex[addr.function & 0xf];
        buf[7] = '\0';

        return buf;
}

char *pci_class_string(struct pci_addr addr)
{
        uint8_t class, subclass;

        class = pci_config_read8(addr, 11);
        subclass = pci_config_read8(addr, 10);

        switch (class) {
        case 1:
                switch (subclass) {
                case 0: return "SCSI Controller";
                case 1: return "IDE Controller";
                case 2: return "Floppy Disk Controller";
                case 3: return "IPI Controller";
                case 4: return "RAID Controller";
                case 5: return "ATA Controller";
                case 6: return "SATA Controller";
                case 7: return "SAS Controller";
                case 8: return "Non-Volatile Memory Controller";
                }
                break;
        case 2:
                switch (subclass) {
                case 0: return "Ethernet Controller";
                case 1: return "Token Ring Controller";
                }
                break;
        case 3:
                switch (subclass) {
                case 0: return "VGA Display Controller";
                }
                break;
        case 4:
                switch (subclass) {
                case 0: return "Multimedia Video Controller";
                case 1: return "Multimedia Audio Controller";
                case 2: return "Computer Telephony Device";
                case 3: return "Audio Device";
                }
                break;
        case 5:
                switch (subclass) {
                case 0: return "RAM Controller";
                case 1: return "Flash Controller";
                }
                break;
        case 6:
                switch (subclass) {
                case 0: return "Host Bridge";
                case 1: return "ISA Bridge";
                case 2: return "EISA Bridge";
                case 3: return "MCA Bridge";
                case 4:
                case 9:
                        return "PCI-to-PCI Bridge";
                }
                break;
        case 7:
                switch (subclass) {
                case 0: return "Serial Controller";
                case 1: return "Parallel Controller";
                case 2: return "Multiport Serial Controller";
                case 3: return "Modem";
                case 5: return "Smart Card Controller";
                }
                break;
        case 8:
                switch (subclass) {
                case 0: return "PIC";
                case 1: return "DMA Controller";
                case 2: return "Timer";
                case 3: return "RTC Controller";
                case 4: return "PCI Hotplug Controller";
                case 5: return "SD Host Controller";
                case 6: return "IOMMU";
                }
                break;
        case 9:
                switch (subclass) {
                case 0: return "Keyboard Controller";
                case 2: return "Mouse Controller";
                }
                break;
        case 11:
                switch (subclass) {
                case 0: return "i386";
                case 1: return "i486";
                case 2: return "Pentium";
                case 3: return "Pentium Pro";
                case 0x40: return "Coprocessor";
                }
                break;
        case 12:
                switch (subclass) {
                case 0: return "FireWire Controller";
                case 3: return "USB Controller";
                }
                break;
        case 13:
                switch (subclass) {
                case 17: return "Bluetooth Controller";
                case 32: return "802.1a Ethernet Controller";
                case 33: return "802.1b Ethernet Controller";
                }
                break;
        }

        return "Unkown Device";
}

static void pci_check_device(int bus, int dev)
{
        struct pci_addr addr;
        int func;
        uint16_t vid;

        for (func = 0; func < 8; func++) {
                addr.bus = bus;
                addr.device = dev;
                addr.function = func;

                vid = pci_config_read16(addr, 0);
                if (vid == 0xffff) {
                        /* Device not present */
                        return;
                }
                else {
                        kprintf("pci: found %s vid=0x%w %s\n",
                                pci_bdf_string(addr), vid,
                                pci_class_string(addr));
                }
        }
}

static void pci_enumerate()
{
        int bus, dev;

        for (bus = 0; bus < 256; bus++) {
                for (dev = 0; dev < 32; dev++) {
                        pci_check_device(bus, dev);
                }
        }
}

void pci_init()
{
        pci_enumerate();
}
