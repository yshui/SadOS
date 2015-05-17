#include <stdio.h>
//#include <sys/printk.h>
#include <sys/defs.h>
#include <sys/portio.h>
#include <ahci.h>
#include <sys/mm.h>
#include <string.h>
#include <ipc.h>
#include <sendpage.h>
#include <uapi/mem.h>
#include <uapi/ioreq.h>
#include <uapi/portio.h>
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
#define HBA_PxIS_TFES   (1 << 30)
#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX    0x35
#define HBA_PxCMD_CR    (1 << 15)
#define HBA_PxCMD_FR    (1 << 14)
#define HBA_PxCMD_FRE   (1 << 4)
#define HBA_PxCMD_SUD   (1 << 1)
#define HBA_PxCMD_ST    (1 << 0)
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

uint64_t get_virtual_mem(uint64_t phys_addr)
{
    struct response res;
    struct mem_req req;
    req.phys_addr = (uint64_t) phys_addr;
    req.len = 4096;
    req.dest_addr = 0;
    int rd = port_connect(1, sizeof(req), &req);
    get_response(rd, &res);
    return (uint64_t)res.buf;
}

HBA_MEM* glob_abar;
// Check device type
static int get_type(HBA_PORT *port)
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

static inline void outl(uint16_t port, uint32_t val )
{
    __asm__ volatile( "outl %0, %1"
    :: "a"(val), "Nd"(port) );
}
static inline uint32_t inl( uint16_t port )
{
    uint32_t ret = 0;
    __asm__ volatile( "inl %1, %0"
    : "=a"(ret) : "Nd"(port) );
    return ret;
}
 
// Start command engine
void start_cmd(HBA_PORT *port)
{
    // Set FRE (bit4) and ST (bit0)
    // Wait until CR (bit15) is cleared
    while (port->cmd & HBA_PxCMD_CR)
        //printk("cmd: %d %d\n", port->cmd, port->cmd & HBA_PxCMD_CR);
        ;
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST; 
}
 
// Stop command engine
void stop_cmd(HBA_PORT *port)
{
    // Clear ST (bit0)
    port->cmd &= ~HBA_PxCMD_ST;
    port->cmd &= ~HBA_PxCMD_FRE;
    //printk("cmd before: %d\n", port->cmd);
 
    // Wait until FR (bit14), CR (bit15) are cleared
    while(1)
    {
        //printk("cmd: %d\n", port->cmd);
        if (port->cmd & HBA_PxCMD_FR)
            continue;
        if (port->cmd & HBA_PxCMD_CR)
            continue;
        break;
    } 
    //printk("here.\n");
}

void port_rebase(HBA_PORT *port, int portno)
{
    //printk("Port: %x\n", port);
    stop_cmd(port); // Stop command engine
 
    uint64_t cur_addr = 0;
    // Command list offset: 1K*portno
    // Command list entry size = 32
    // Command list entry maxim count = 32
    // Command list maxim size = 32*32 = 1K per port
    port->clb = AHCI_BASE + (portno<<10);
    port->clbu = 0;
    uint64_t mapped_clb = get_virtual_mem((uint64_t)port -> clb); 
    //cur_addr = port -> clb + KERN_VMBASE;
    cur_addr = mapped_clb;
    memset((void*)cur_addr, 0, 1024);
 
    // FIS offset: 32K+256*portno
    // FIS entry size = 256 bytes per port
    port->fb = AHCI_BASE + (32<<10) + (portno<<8);
    port->fbu = 0;
    uint64_t mapped_fb = get_virtual_mem((uint64_t)port -> fb);
    //cur_addr = port->fb + KERN_VMBASE;
    cur_addr = mapped_fb;
    memset((void*)cur_addr, 0, 256);
 
    // Command table offset: 40K + 8K*portno
    // Command table size = 256*32 = 8K per port

    //HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(KERN_VMBASE + port->clb);
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(mapped_clb);

    for (int i=0; i<32; i++)
    {
        cmdheader[i].prdtl = 8; // 8 prdt entries per command table
                    // 256 bytes per command table, 64+16+48+16*8
        // Command table offset: 40K + 8K*portno + cmdheader_index*256
        cmdheader[i].ctba = AHCI_BASE + (40<<10) + (portno<<13) + (i<<8);
        cmdheader[i].ctbau = 0;
        cur_addr = cmdheader[i].ctba + KERN_VMBASE;
        memset((void*)cur_addr, 0, 256);
    }
 
    start_cmd(port);    // Start command engine
}

void probe_port(HBA_MEM *abar)
{
    glob_abar = abar;
    // Search disk in impelemented ports
    int32_t pi = abar->pi;
    int i = 0;
    while (i<32)
    {
        if (pi & 1)
        {
            int dt = get_type(&abar->ports[i]);
            if (dt == AHCI_DEV_SATA)
            {
                //printk("SATA drive found at port %d\n", i);
                port_rebase(abar -> ports, i);
            }
            else if (dt == AHCI_DEV_SATAPI)
            {
                //printk("SATAPI drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_SEMB)
            {
                //printk("SEMB drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_PM)
            {
                //printk("PM drive found at port %d\n", i);
            }
            else
            {
                //printk("No drive found at port %d\n", i);
            }
        }
 
        pi >>= 1;
        i ++;
    }
}

uint32_t pciConfigReadWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
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
    if (offset == 0x24)
        tmp = inl(0xCFC) ;
    else
        tmp = (uint16_t)( (inl(0xCFC) >> ((offset & 2) * 8) ) & 0xffff);
    return (tmp);
}

uint16_t getVendorID(uint8_t bus, uint8_t slot)
{
    return pciConfigReadWord(bus, slot, 0, 0);
}

uint16_t getDeviceID(uint8_t bus, uint8_t slot)
{
    return pciConfigReadWord(bus, slot, 0, 2);
}

uint64_t checkDevice(uint8_t bus, uint8_t device) {
    //uint8_t function = 0;
 
    uint16_t vendorID = getVendorID(bus, device);
    if(vendorID == 0xFFFF) return 0;        // Device doesn't exist
    uint16_t deviceID = getDeviceID(bus, device);

    if (vendorID == 0x8086 && deviceID == 0x2922)
    {
        //printk("vendor: %d device: %d\n", vendorID, deviceID);
        return pciConfigReadWord(bus, device, 0, 0x24);
    }
     //checkFunction(bus, device);
     //headerType = getHeaderType(bus, device, function);
     return 0;
 }

uint64_t checkAllBuses(void)
{
    uint8_t bus;
    uint8_t device;
    uint64_t bar5;
 
    for(bus = 0; bus < 256; bus++)
    {
        for(device = 0; device < 32; device++)
        {
            bar5 = checkDevice(bus, device);
            if (bar5 != 0)
                return bar5;
        }
        if (bus == 255)
            break;
    }
    return 0;
}

// Find a free command list slot
int find_cmdslot(HBA_PORT *port)
{
    // If not set in SACT and CI, the slot is free
    uint32_t slots = (port->sact | port->ci);
    int cmdslots = (glob_abar -> cap & 0x0f00) >> 8;
    for (int i=0; i<cmdslots; i++)
    {
        if ((slots&1) == 0)
            return i;
        slots >>= 1;
    }
    //printk("Cannot find free command list entry\n");
    return -1;
}
uint8_t read_sata(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count,char *buf)
{
    port->is = (uint32_t)-1;       // Clear pending interrupt bits
    int spin = 0; // Spin lock timeout counter
    int slot = find_cmdslot(port);
    //uint64_t buf_phys = (uint64_t)buf - KERN_VMBASE;
    uint64_t buf_phys = get_physical((uint64_t)buf);
    if (slot == -1)
        return 0;
 
    uint64_t mapped_clb = get_virtual_mem((uint64_t)port -> clb);
    //HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*) (KERN_VMBASE + port->clb);
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*) mapped_clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t); // Command FIS size
    cmdheader->w = 0;       // Read device
    cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;    // PRDT entries count
 
    uint64_t mapped_ctba = get_virtual_mem(cmdheader -> ctba);
    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*) mapped_ctba;
    //HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(KERN_VMBASE + cmdheader->ctba);
    //memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) +
    //    (cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));
 
    int i;
    // 8K bytes (16 sectors) per PRDT
    for (i=0; i<cmdheader->prdtl-1; i++)
    {
        cmdtbl->prdt_entry[i].dba = (uint32_t) (buf_phys & 0xffffffff);
        cmdtbl->prdt_entry[i].dbau = (uint32_t) ( ( (buf_phys) >> 32) & 0xffffffff);
        cmdtbl->prdt_entry[i].dbc = 8*1024; // 8K bytes
        cmdtbl->prdt_entry[i].i = 1;
        buf += 4*1024;  // 4K words
        count -= 16;    // 16 sectors
    }
    // Last entry
    cmdtbl->prdt_entry[i].dba = (uint32_t) (buf_phys & 0xffffffff);
    cmdtbl->prdt_entry[i].dbau = (uint32_t) ( (buf_phys >> 32) & 0xffffffff);
    //printk("dba & dbau: %p %p\n", cmdtbl ->prdt_entry[i].dba, cmdtbl -> prdt_entry[i].dbau);
    cmdtbl->prdt_entry[i].dbc = count<<9;   // 512 bytes per sector
    cmdtbl->prdt_entry[i].i = 1;
 
    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
 
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_READ_DMA_EX;
 
    cmdfis->lba0 = (uint8_t)startl;
    cmdfis->lba1 = (uint8_t)(startl>>8);
    cmdfis->lba2 = (uint8_t)(startl>>16);
    cmdfis->device = 1<<6;  // LBA mode
 
    cmdfis->lba3 = (uint8_t)(startl>>24);
    cmdfis->lba4 = (uint8_t)starth;
    cmdfis->lba5 = (uint8_t)(starth>>8);
 
    cmdfis->countl = (count & 0xff);
    cmdfis->counth = (count >> 8);
 
    // The below loop waits until the port is no longer busy before issuing a new command
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
    {
        spin++;
    }
    if (spin == 1000000)
    {
        //printk("Port is hung\n");
        return 0;
    }
 
    port->ci = (1<<slot); // Issue command
    //printk("PORT INFO: %x %d %d\n", port, port->ci, port->tfd);
 
    // Wait for completion
    while (1)
    {
        //printk("Reading disk...\n");
        // In some longer duration reads, it may be helpful to spin on the DPS bit 
        // in the PxIS port field as well (1 << 5)
        //printk("value: %d\n", (port -> ci & (1<<slot) )  );
        if ((port->ci & (1<<slot)) == 0) 
            break;
        if (port->is & HBA_PxIS_TFES)   // Task file error
        {
            //printk("Read disk error\n");
            return 0;
        }
    }
 
    // Check again
    if (port->is & HBA_PxIS_TFES)
    {
        //printk("Read disk error\n");
        return 0;
    }
 
    return 1;
}
 
uint8_t write_sata(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count, char *buf)
{
    port->is = (uint32_t)-1;       // Clear pending interrupt bits
    int spin = 0; // Spin lock timeout counter
    int slot = find_cmdslot(port);
    //uint64_t buf_phys = (uint64_t)buf - KERN_VMBASE;
    uint64_t buf_phys = get_physical((uint64_t)buf);
 
    uint64_t mapped_clb = get_virtual_mem((uint64_t)port -> clb);
    //HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*) (KERN_VMBASE + port->clb);
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)mapped_clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t); // Command FIS size
    cmdheader->w = 1;       // Write device
    cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;    // PRDT entries count

    uint64_t mapped_ctba = get_virtual_mem((uint64_t)cmdheader -> ctba);
    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*) mapped_ctba;
    //HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(KERN_VMBASE + cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) +
        (cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));
 
    int i;
    // 8K bytes (16 sectors) per PRDT
    for (i=0; i<cmdheader->prdtl-1; i++)
    {
        cmdtbl->prdt_entry[i].dba = (uint32_t) (buf_phys & 0xffffffff);
        cmdtbl->prdt_entry[i].dbau = (uint32_t) ( ( (buf_phys) >> 32) & 0xffffffff);
        cmdtbl->prdt_entry[i].dbc = 8*1024; // 8K bytes
        cmdtbl->prdt_entry[i].i = 1;
        buf += 4*1024;  // 4K words
        count -= 16;    // 16 sectors
    }
    // Last entry
    cmdtbl->prdt_entry[i].dba = (uint32_t) (buf_phys & 0xffffffff);
    cmdtbl->prdt_entry[i].dbau = (uint32_t) ( (buf_phys >> 32) & 0xffffffff);
    //printk("dba & dbau: %p %p\n", cmdtbl ->prdt_entry[i].dba, cmdtbl -> prdt_entry[i].dbau);
    cmdtbl->prdt_entry[i].dbc = count<<9;   // 512 bytes per sector
    cmdtbl->prdt_entry[i].i = 1;
 
    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
 
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_WRITE_DMA_EX;
 
    cmdfis->lba0 = (uint8_t)startl;
    cmdfis->lba1 = (uint8_t)(startl>>8);
    cmdfis->lba2 = (uint8_t)(startl>>16);
    cmdfis->device = 1<<6;  // LBA mode
 
    cmdfis->lba3 = (uint8_t)(startl>>24);
    cmdfis->lba4 = (uint8_t)starth;
    cmdfis->lba5 = (uint8_t)(starth>>8);
 
    cmdfis->countl = (count & 0xff);
    cmdfis->counth = (count >> 8);
 
    // The below loop waits until the port is no longer busy before issuing a new command
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
    {
        spin++;
    }
    if (spin == 1000000)
    {
        //printk("Port is hung\n");
        return 0;
    }
 
    port->ci = (1<<slot); // Issue command
    //printk("PORT INFO: %x %d %d\n", port, port->ci, port->tfd);
 
    // Wait for completion
    while (1)
    {
        //printk("Writing disk...\n");
        // In some longer duration reads, it may be helpful to spin on the DPS bit 
        // in the PxIS port field as well (1 << 5)
        //printk("value: %d\n", (port -> ci & (1<<slot) )  );
        if ((port->ci & (1<<slot)) == 0) 
            break;
        if (port->is & HBA_PxIS_TFES)   // Task file error
        {
            //printk("Write disk error\n");
            return 0;
        }
    }
 
    // Check again
    if (port->is & HBA_PxIS_TFES)
    {
        printf("Write disk error\n");
        return 0;
    }
 
    return 1;
}
