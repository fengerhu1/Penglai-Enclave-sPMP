#include <sbi/sbi_iopmp.h>
#include <stddef.h>
#include <sbi/sbi_pmp.h>
#include <sbi/sbi_console.h>
#include <sm/sm.h>
#include <sbi/riscv_io.h>

void handle_sIOPMP_violation() 
{
    /* The violation address in the sIOPMP violation */ 
    u32 violated_addr = readl((void *)ADDR_INTER_IOPMP);
    u32 SID = readl((void *)SID_INDEX_INTER_IOPMP);
    /* The permission type in the IOPMP table */ 
    u8 permission_type = readb((void *)PERMISSION_TYPE_INTER_IOPMP);
    sbi_printf("sIOPMP: violated_addr %x SID %u permission_type %u\n", violated_addr, SID, permission_type);
}

void handle_cold_device_switching() 
{    
    /* Read the current SID for sIOPMP interrupt */ 
    u8 device_id = readb((void *)DEVICE_INT_IOPMP);
    sbi_printf("sIOPMP: device_iopmp result %u\n", device_id);

    /* Read the SID */  
    u32 CSTL = readl((void *)CSTL_IOPMP + device_id * 4);

    /* Disable the interrupt from this device */
    writeb(0x1, (void *)CLEARINT_IOPMP + device_id * 1);

    sbi_printf("sIOPMP: CSTL(SID) %u \n", CSTL);

    // unsigned long mcause_iopmp = csr_read(mcause);
    // if (mcause_iopmp == 11){

    //     // reg_write64(IOPMP_SD1, virtual_SID2DM[CSTL]);
    //     // //U32(IOPMP_MD1) = virtual_MD1[CSTL];
    //     // reg_write32(IOPMP_MD1, virtual_MD1[CSTL]);
    //     // reg_write32(IOPMP_ADDR0, virtual_addr_iopmp[CSTL]);
    //     // reg_write32(IOPMP_ADDR1, virtual_addr_iopmp[CSTL]);
    //     // reg_write32(IOPMP_ADDR2, virtual_addr_iopmp[CSTL]);
    //     // reg_write32(IOPMP_ADDR3, virtual_addr_iopmp[CSTL]);
    //     // reg_write32(IOPMP_ADDR0, virtual_addr_iopmp[CSTL]);
    //     // reg_write32(IOPMP_ADDR1, virtual_addr_iopmp[CSTL]);
    //     // reg_write32(IOPMP_ADDR2, virtual_addr_iopmp[CSTL]);
    //     // reg_write32(IOPMP_ADDR3, virtual_addr_iopmp[CSTL]);

    //     reg_write64(0x10028008, virtual_SID2DM[CSTL]);
    //     //U32(IOPMP_MD1) = virtual_MD1[CSTL];
    //     reg_write32(0x10028204, virtual_MD1[CSTL]);
    //     reg_write32(0x10028800, virtual_addr_iopmp[CSTL]);
    //     reg_write32(0x10028804, virtual_addr_iopmp[CSTL]);
    //     reg_write32(0x10028808, virtual_addr_iopmp[CSTL]);
    //     reg_write32(0x1002880C, virtual_addr_iopmp[CSTL]);
    //     reg_write32(0x10028800, virtual_addr_iopmp[CSTL]);
    //     reg_write32(0x10028804, virtual_addr_iopmp[CSTL]);
    //     reg_write32(0x10028808, virtual_addr_iopmp[CSTL]);
    //     reg_write32(0x1002880C, virtual_addr_iopmp[CSTL]);

    //     // 赋值给virtual SID
    //     reg_write32(VSTL_IOPMP + device_id * 4, CSTL);
        
    // }

    /* Resume the interrupt for this device */
    writeb(0x0, (void *)CLEARINT_IOPMP + device_id * 1);
}

void set_SRCMD(int sid, unsigned long value) {
    writeq(value, (void *)IOPMP_SD0 + sid*8);
}

void set_MDCFG(int mid, u32 value) {
    writel(value, (void *)IOPMP_MD0 + mid*4);
}

void set_IOPMP_ADDR(int iopmp_index, u32 value) {
    writel(value, (void *)IOPMP_ADDR0 + iopmp_index*4);
}

void set_IOPMP_CFG(int iopmp_index, u8 value) {
    writel(value, (void *)IOPMP_CFD0 + iopmp_index);
}
