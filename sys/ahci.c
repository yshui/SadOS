#include <sys/printk.h>
#include <sys/defs.h>
#include <sys/portio.h>
#include <sys/ahci.h>
#define SATA_SIG_ATA    0x00000101  // SATA drive
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive
#define SATA_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM 0x96690101  // Port multiplier
#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SATAPI 4
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1
#define AHCI_BASE   0x400000

static inline void outl(uint16_t port, uint32_t val ){
    __asm__ volatile( "outl %0, %1"
    :: "a"(val), "Nd"(port) );
}
static inline uint32_t inl( uint16_t port ){
uint32_t ret;
    __asm__ volatile( "inl %1, %0"
    : "=a"(ret) : "Nd"(port) );
    return ret;
}
 
// Check device type
static int check_type(HBA_PORT *port)
{
    int32_t ssts = port->ssts;
 
    int8_t ipm = (ssts >> 8) & 0x0F;
    int8_t det = ssts & 0x0F;
 
    if (det != HBA_PORT_DET_PRESENT)    // Check drive status
        return AHCI_DEV_NULL;
    if (ipm != HBA_PORT_IPM_ACTIVE)
        return AHCI_DEV_NULL;
 
    switch (port->sig)
    {
    case SATA_SIG_ATAPI:
        return AHCI_DEV_SATAPI;
    case SATA_SIG_SEMB:
        return AHCI_DEV_SEMB;
    case SATA_SIG_PM:
        return AHCI_DEV_PM;
    default:
        return AHCI_DEV_SATA;
    }
}

void probe_port(HBA_MEM *abar)
{
    // Search disk in impelemented ports
    int32_t pi = abar->pi;
    int i = 0;
    return;
    while (i<32)
    {
        if (pi & 1)
        {
            int dt = check_type(&abar->ports[i]);
            if (dt == AHCI_DEV_SATA)
            {
                printk("SATA drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_SATAPI)
            {
                printk("SATAPI drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_SEMB)
            {
                printk("SEMB drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_PM)
            {
                printk("PM drive found at port %d\n", i);
            }
            else
            {
                printk("No drive found at port %d\n", i);
            }
        }
 
        pi >>= 1;
        i ++;
    }
}

uint64_t pciConfigReadWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint64_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl (0xCF8, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    //tmp = (uint16_t)( (inl(0xCFC) >> ((offset & 2) * 8) ) & 0xffff);
    tmp = (inl(0xCFC) >> ((offset & 2) * 8) ) ;
    return (tmp);
}

uint16_t getVendorID(uint8_t bus, uint8_t slot)
{
    return pciConfigReadWord(bus, slot, 0, 0) & 0xffff;
}

uint16_t getDeviceID(uint8_t bus, uint8_t slot)
{
    return pciConfigReadWord(bus, slot, 0, 2) & 0xffff;
}

uint64_t checkDevice(uint8_t bus, uint8_t device) {
    //uint8_t function = 0;
 
    uint16_t vendorID = getVendorID(bus, device);
    if(vendorID == 0xFFFF) return 0;        // Device doesn't exist
    uint16_t deviceID = getDeviceID(bus, device);

    //printk("vendor: %d device: %d\n", vendorID, deviceID);
    if (vendorID == 0x8086 && deviceID == 0x2922)
    {
        return pciConfigReadWord(bus, device, 0, 0x24);
    }
     //checkFunction(bus, device);
     //headerType = getHeaderType(bus, device, function);
     return 0;
 }

uint64_t checkAllBuses(void) {
     uint8_t bus;
     uint8_t device;
     uint64_t bar5;
 
     for(bus = 0; bus < 256; bus++) {
         for(device = 0; device < 32; device++) {
             bar5 = checkDevice(bus, device);
             if (bar5 != 0)
                 return bar5;
         }
         if (bus == 255)
             break;
     }
     return 0;
 }
