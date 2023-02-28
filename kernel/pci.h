#ifndef PCI_H
#define PCI_H

#include "types.h"

struct pci_addr {
        uint8_t bus;
        uint8_t device;
        uint8_t function;
};

void pci_init();

#endif
