#include <sbi/sbi_iopmp.h>
#include <stddef.h>
#include <sbi/sbi_pmp.h>
#include <sbi/sbi_console.h>
#include <sm/sm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_types.h>

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

void set_IOPMP(int iopmp_index, struct iopmp_config_t iopmp_cfg_t) {
    uintptr_t pmp_address = 0;
    switch(iopmp_cfg_t.mode)
	{
		case IOPMP_A_NAPOT:
			if(iopmp_cfg_t.paddr == 0 && iopmp_cfg_t.size == -1UL)
				pmp_address = -1UL;
			else {
				pmp_address = (iopmp_cfg_t.paddr | ((iopmp_cfg_t.size>>1)-1)) >> 2;
			}
			break;
		case IOPMP_A_TOR:
			pmp_address = iopmp_cfg_t.paddr;
			break;
		case IOPMP_A_NA4:
			pmp_address = iopmp_cfg_t.paddr;
		default:
			pmp_address = 0;
			break;
	}
    writeb(iopmp_cfg_t.perm | iopmp_cfg_t.mode, (void *)IOPMP_CFD0 + iopmp_index);
    sbi_printf("sIOPMP: set IOPMP entry %d permission %x\n", iopmp_index, iopmp_cfg_t.perm | iopmp_cfg_t.mode);
    writel((u32)pmp_address, (void *)IOPMP_ADDR0 + iopmp_index*4);
    sbi_printf("sIOPMP: set IOPMP entry %d address %x\n", iopmp_index, (u32) pmp_address);
}

void set_IOPMP_CFG(int iopmp_index, u8 value) {
    writeb(value, (void *)IOPMP_CFD0 + iopmp_index);
}

void sIOPMP_setup() {
    uint64_t SD0 = 0x00000001, SD1 = 0x00000002;
    uint32_t MD0 = 0x0001, MD1 = 0x0002;

    // uint8_t read_dma, start_dma = 0xFF;
    // uint32_t addr_iopmp = 0x89001FFFL;
    // uint32_t addr_dma = 0x88000000L;
    // uint16_t size_dma = 0x1000L;
    /*
     * Bit 7: Read
     * Bit 6: write
     * Bit 5: execution
     * Bit 4,3: Mode
     * Bit 2,1: res
     * Bit 0: Lock
     */

    // IOPMP entry has no permission
    // uint8_t CFG0 = 0x19, CFG1 = 0x19, CFG2 = 0x19, CFG3 = 0x19;

    // Bind the SID0 to the MD0
    sbi_printf("sIOPMP: sIOPMP setup\n");
    set_SRCMD(0, SD0);;
    sbi_printf("sIOPMP: set the SRCMD0 SD0: %lx\n", SD0);
    // Bind the SID1 to the MD1
    set_SRCMD(1, SD1);
    // Bind MD0 to the IOPMP0
    set_MDCFG(0, MD0);
    sbi_printf("sIOPMP: set the MDCFG0 MD0: %x\n", MD0);
    // Bind the MD1 to the IOPMP1
    set_MDCFG(1, MD1);

    struct iopmp_config_t iopmp_cfg_t_0 = {.paddr = 0x80000000, .size = 0x1000, .perm = 0, .mode = IOPMP_A_NAPOT};

    set_IOPMP(0, iopmp_cfg_t_0);
    sbi_printf("sIOPMP: set IOPMP entry\n");
    set_IOPMP(1, iopmp_cfg_t_0);
    set_IOPMP(2, iopmp_cfg_t_0);
    set_IOPMP(3, iopmp_cfg_t_0);
    sbi_printf("sIOPMP: setup is finished\n");
}
