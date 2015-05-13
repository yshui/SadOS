#include <sys/defs.h>
#define AHCI_BASE 0x400000
#define BYTE uint8_t
#define WORD uint16_t
#define DWORD uint32_t
typedef enum
{
    FIS_TYPE_REG_H2D    = 0x27, // Register FIS - host to device
    FIS_TYPE_REG_D2H    = 0x34, // Register FIS - device to host
    FIS_TYPE_DMA_ACT    = 0x39, // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP  = 0x41, // DMA setup FIS - bidirectional
    FIS_TYPE_DATA       = 0x46, // Data FIS - bidirectional
    FIS_TYPE_BIST       = 0x58, // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP  = 0x5F, // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS   = 0xA1, // Set device bits FIS - device to host
} FIS_TYPE;

typedef struct tagFIS_REG_H2D
{
    // int32_t 0
    int8_t    fis_type;   // FIS_TYPE_REG_H2D
 
    int8_t    pmport:4;   // Port multiplier
    int8_t    rsv0:3;     // Reserved
    int8_t    c:1;        // 1: Command, 0: Control
 
    int8_t    command;    // Command register
    int8_t    featurel;   // Feature register, 7:0
 
    // int32_t 1
    int8_t    lba0;       // LBA low register, 7:0
    int8_t    lba1;       // LBA mid register, 15:8
    int8_t    lba2;       // LBA high register, 23:16
    int8_t    device;     // Device register
 
    // int32_t 2
    int8_t    lba3;       // LBA register, 31:24
    int8_t    lba4;       // LBA register, 39:32
    int8_t    lba5;       // LBA register, 47:40
    int8_t    featureh;   // Feature register, 15:8
 
    // int32_t 3
    int8_t    countl;     // Count register, 7:0
    int8_t    counth;     // Count register, 15:8
    int8_t    icc;        // Isochronous command completion
    int8_t    control;    // Control register
 
    // int32_t 4
    int8_t    rsv1[4];    // Reserved
} FIS_REG_H2D;

typedef struct tagFIS_REG_D2H
{
    // int32_t 0
    int8_t    fis_type;    // FIS_TYPE_REG_D2H
 
    int8_t    pmport:4;    // Port multiplier
    int8_t    rsv0:2;      // Reserved
    int8_t    i:1;         // Interrupt bit
    int8_t    rsv1:1;      // Reserved
 
    int8_t    status;      // Status register
    int8_t    error;       // Error register
 
    // int32_t 1
    int8_t    lba0;        // LBA low register, 7:0
    int8_t    lba1;        // LBA mid register, 15:8
    int8_t    lba2;        // LBA high register, 23:16
    int8_t    device;      // Device register
 
    // int32_t 2
    int8_t    lba3;        // LBA register, 31:24
    int8_t    lba4;        // LBA register, 39:32
    int8_t    lba5;        // LBA register, 47:40
    int8_t    rsv2;        // Reserved
 
    // int32_t 3
    int8_t    countl;      // Count register, 7:0
    int8_t    counth;      // Count register, 15:8
    int8_t    rsv3[2];     // Reserved
 
    // int32_t 4
    int8_t    rsv4[4];     // Reserved
} FIS_REG_D2H;

typedef struct tagFIS_DATA
{
    // int32_t 0
    int8_t    fis_type;   // FIS_TYPE_DATA
 
    int8_t    pmport:4;   // Port multiplier
    int8_t    rsv0:4;     // Reserved
 
    int8_t    rsv1[2];    // Reserved
 
    // int32_t 1 ~ N
    int32_t   data[1];    // Payload
} FIS_DATA;

typedef volatile struct tagHBA_PORT
{
    int32_t   clb;        // 0x00, command list base address, 1K-byte aligned
    int32_t   clbu;       // 0x04, command list base address upper 32 bits
    int32_t   fb;     // 0x08, FIS base address, 256-byte aligned
    int32_t   fbu;        // 0x0C, FIS base address upper 32 bits
    int32_t   is;     // 0x10, interrupt status
    int32_t   ie;     // 0x14, interrupt enable
    int32_t   cmd;        // 0x18, command and status
    int32_t   rsv0;       // 0x1C, Reserved
    int32_t   tfd;        // 0x20, task file data
    int32_t   sig;        // 0x24, signature
    int32_t   ssts;       // 0x28, SATA status (SCR0:SStatus)
    int32_t   sctl;       // 0x2C, SATA control (SCR2:SControl)
    int32_t   serr;       // 0x30, SATA error (SCR1:SError)
    int32_t   sact;       // 0x34, SATA active (SCR3:SActive)
    int32_t   ci;     // 0x38, command issue
    int32_t   sntf;       // 0x3C, SATA notification (SCR4:SNotification)
    int32_t   fbs;        // 0x40, FIS-based switch control
    int32_t   rsv1[11];   // 0x44 ~ 0x6F, Reserved
    int32_t   vendor[4];  // 0x70 ~ 0x7F, vendor specific
} HBA_PORT;

typedef volatile struct tagHBA_MEM
{
    // 0x00 - 0x2B, Generic Host Control
    int32_t   cap;        // 0x00, Host capability
    int32_t   ghc;        // 0x04, Global host control
    int32_t   is;     // 0x08, Interrupt status
    int32_t   pi;     // 0x0C, Port implemented
    int32_t   vs;     // 0x10, Version
    int32_t   ccc_ctl;    // 0x14, Command completion coalescing control
    int32_t   ccc_pts;    // 0x18, Command completion coalescing ports
    int32_t   em_loc;     // 0x1C, Enclosure management location
    int32_t   em_ctl;     // 0x20, Enclosure management control
    int32_t   cap2;       // 0x24, Host capabilities extended
    int32_t   bohc;       // 0x28, BIOS/OS handoff control and status
 
    // 0x2C - 0x9F, Reserved
    int8_t    rsv[0xA0-0x2C];
 
    // 0xA0 - 0xFF, Vendor specific registers
    int8_t    vendor[0x100-0xA0];
 
    // 0x100 - 0x10FF, Port control registers
    HBA_PORT    ports[1];   // 1 ~ 32
} HBA_MEM;

typedef struct tagHBA_PRDT_ENTRY
{
    DWORD   dba;        // Data base address
    DWORD   dbau;       // Data base address upper 32 bits
    DWORD   rsv0;       // Reserved
 
    // DW3
    DWORD   dbc:22;     // Byte count, 4M max
    DWORD   rsv1:9;     // Reserved
    DWORD   i:1;        // Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct tagHBA_CMD_TBL
{
    // 0x00
    BYTE    cfis[64];   // Command FIS
 
    // 0x40
    BYTE    acmd[16];   // ATAPI command, 12 or 16 bytes
 
    // 0x50
    BYTE    rsv[48];    // Reserved
 
    // 0x80
    HBA_PRDT_ENTRY  prdt_entry[1];  // Physical region descriptor table entries, 0 ~ 65535
} HBA_CMD_TBL;

typedef struct tagHBA_CMD_HEADER
{
    // DW0
    BYTE    cfl:5;      // Command FIS length in DWORDS, 2 ~ 16
    BYTE    a:1;        // ATAPI
    BYTE    w:1;        // Write, 1: H2D, 0: D2H
    BYTE    p:1;        // Prefetchable
 
    BYTE    r:1;        // Reset
    BYTE    b:1;        // BIST
    BYTE    c:1;        // Clear busy upon R_OK
    BYTE    rsv0:1;     // Reserved
    BYTE    pmp:4;      // Port multiplier port
 
    WORD    prdtl;      // Physical region descriptor table length in entries
 
    // DW1
    volatile
    DWORD   prdbc;      // Physical region descriptor byte count transferred
 
    // DW2, 3
    DWORD   ctba;       // Command table descriptor base address
    DWORD   ctbau;      // Command table descriptor base address upper 32 bits
 
    // DW4 - 7
    DWORD   rsv1[4];    // Reserved
} HBA_CMD_HEADER;

uint64_t checkAllBuses(void);
void probe_port(HBA_MEM *abar);
uint8_t read_sata(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t* buf);
uint8_t write_sata(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t* buf);
