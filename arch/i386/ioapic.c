// The I/O APIC manages hardware interrupts for an SMP system.
// http://www.intel.com/design/chipsets/datashts/29056601.pdf
// See also picirq.c.

#include <arch/i386/inc.h>
#include <traps.h>

#define IOAPIC  0xFEC00000   // Default physical address of IO APIC

#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version (read only)
    #define VER_NO(ver_reg) (ver_reg&0xFF) // version number
    #define VER_TABLE_SIZE ((ver>>16)&0xFF) // maximum amount of redirection entries
#define REG_TABLE  0x10  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

volatile struct ioapic *ioapic;
uint8_t ioapicid; // Initialized in mp.c

// IO APIC MMIO structure: write reg, then read or write data.
struct ioapic {
    uint32_t reg;
    uint32_t pad[3];
    uint32_t data;
};

static uint32_t 
ioapic_read(int reg)
{
    ioapic->reg = reg;
    return ioapic->data;
}

static void
ioapic_write(int reg, uint32_t data)
{
    ioapic->reg = reg;
    ioapic->data = data;
}

void
ioapic_init(void)
{
    int i, id, maxintr;

    ioapic = (volatile struct ioapic*)IOAPIC;
    maxintr = (ioapic_read(REG_VER) >> 16) & 0xFF;
    id = ioapic_read(REG_ID) >> 24;
    cprintf("ioapic init: ioapic id %d\n", id);
    if (id != ioapicid)
        cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");

    // Mark all interrupts edge-triggered, active high, disabled,
    // and not routed to any CPUs.
    for (i = 0; i <= maxintr; i++) {
        ioapic_write(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
        ioapic_write(REG_TABLE+2*i+1, 0);
    }
}

void
ioapic_enable(int irq, int apicid)
{
    // Mark interrupt edge-triggered, active high,
    // enabled, and routed to the given cpu's APIC ID.
    ioapic_write(REG_TABLE+2*irq, T_IRQ0 + irq);
    ioapic_write(REG_TABLE+2*irq+1, apicid << 24);
}
