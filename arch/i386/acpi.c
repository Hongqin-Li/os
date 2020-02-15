#include <arch/i386/inc.h>

struct rsdp {
    char signature[8];  //"RSD PTR ", Notice the last blank char
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;   //0 for v1.0
    uint32_t rsdtaddr;  // physical addr of RSDT
} __attribute__ ((packed));

//  System Description Table Header.
struct sdthdr {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oem_tableid[8];
    uint32_t oem_revision;
    uint32_t creatorid;
    uint32_t creator_revision;
} __attribute__ ((packed));

static uint8_t
sum(void *addr, int len)
{
	int i, sum;
	sum = 0;
	for (i = 0; i < len; i++)
		sum += ((uint8_t *)addr)[i];
	return sum;
}

// Look for an RSDP structure in the len bytes at physical address addr.
static struct rsdp *
rsdp_search1(uint32_t a, int len)
{
	struct rsdp *p = P2V(a), *end = P2V(a + len);

	for (; p < end; p = (void *)p + 16)
		if (memcmp(p->signature, "RSD PTR ", 8) == 0 &&
		    sum(p, sizeof(*p)) == 0)
			return p;
	return 0;
}

static struct rsdp *
rsdp_search() {
    struct rsdp *p;
    uint32_t ebda = (*(uint16_t *)(P2V(0x40E)))<<4;

    // The first 1KB for EBDA
    if (ebda && (p = rsdp_search1(ebda, 1024))) 
        return p;
    // The main BIOS area below 1 MB
    return rsdp_search1(0xE0000, EXTMEM - 0xE0000);
}


static void
parse_madt(struct sdthdr *h) 
{
    assert(!strncmp(h->signature, "APIC", 4) && sum(h, h->length) == 0);
    assert(!lapic);
       
    lapic = (volatile uint32_t *)*(uint32_t *)((void *)h + sizeof(*h));
    cprintf("MADT: lapic addr: 0x%x\n", lapic);

    int apicid, ioapic_id;
    uint32_t ioapic_addr;
    for (char *p = (void *)h + sizeof(*h) + 8; (void *)p < (void *)h + h->length; p += p[1]) {
        switch(p[0]) {
        case 0:
            assert(p[1] == 8);
            cpus[ncpu++].apicid = p[3];
            cprintf("MADT: Found apicid %d\n", p[3]);
            break;
        case 1:
            assert(p[1] == 12);
            ioapic_id = p[2];
            ioapic_addr = *(uint32_t *)(p+4);
            cprintf("MADT: Found ioapic: id %d, addr 0x%x\n", ioapic_id, ioapic_addr);
            break;
        default:
            break;
        }
    }
}

// Tutorial: https://wiki.osdev.org/PCI_Express
static void
parse_mcfg(struct sdthdr *h) 
{
    assert(!strncmp(h->signature, "MCFG", 4) && sum(h, h->length) == 0);
    panic("MCFG: TODO");
}

static struct sdthdr *
parse_rsdt(struct sdthdr *rsdt)
{
    assert(memcmp(rsdt->signature, "RSDT", 4) == 0)
    assert(rsdt->revision == 1);

    int n = (rsdt->length - sizeof(*rsdt)) / 4;
    struct sdthdr **entry = (void *)rsdt + sizeof(*rsdt);

    for (int i = 0; i < n; i ++) {
        struct sdthdr *h = P2V(entry[i]);

        if (!strncmp(h->signature, "APIC", 4)) 
            parse_madt(h);
        if (!strncmp(h->signature, "MCFG", 4)) 
            parse_madt(h);
    }
}

void acpi_init() 
{
    struct rsdp *p = rsdp_search();
    if (p) {
        cprintf("ACPI: Found RSDP at 0x%x, RSDT at 0x%x\n", p, p->rsdtaddr);
        cprintf("ACPI RSDP: OEM id %s\n", p->oemid);
        cprintf("ACPI RSDP: revision %d\n", p->revision);

        parse_rsdt((void *)P2V(p->rsdtaddr));
    }
    else 
        panic("ACPI: RSDP not Found!\n");
}

