#include <sbi/sbi_iopmp.h>
#include <stddef.h>
#include <sbi/sbi_pmp.h>
#include <sbi/sbi_console.h>
#include <sm/sm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_types.h>
#include <sbi_utils/irqchip/plic.h>


volatile uint32_t sIOPMP_SID, sIOPMP_MD, sIOPMP_violated_addr;
volatile uint32_t sIOPMP_interrupt_type;
volatile uint8_t sIOPMP_permission_type, sIOPMP_device_id;

void handle_sIOPMP_violation() 
{
    // sbi_printf("sIOPMP: begin to handle sIOPMP violation\n");
    /* The violation address in the sIOPMP violation */ 
    u32 violated_addr = readl((void *)ADDR_INTER_IOPMP);
    u32 SID = readl((void *)SID_INDEX_INTER_IOPMP);
    /* The permission type in the IOPMP table */ 
    u8 permission_type = readb((void *)PERMISSION_TYPE_INTER_IOPMP);

    sbi_printf("sIOPMP: violated_addr %x SID %u permission_type %u\n", violated_addr, SID, permission_type);

    /* For Debug, record the interrupt info */
    sIOPMP_violated_addr = violated_addr;
    sIOPMP_SID = SID;
    sIOPMP_permission_type = permission_type;
    sIOPMP_interrupt_type = sIOPMP_VIOLATION;
}

void handle_cold_device_switching() 
{    

    /* Read the current device id for sIOPMP interrupt */ 
    u8 device_id = readb((void *)DEVICE_INT_IOPMP);

    // sbi_printf("sIOPMP: device_iopmp result %u\n", device_id);

    /* Read the SID, one device can have multiple SID, like one NIC has eight channels */  
    u32 CSTL = readl((void *)CSTL_IOPMP + device_id * 4);

    sbi_printf("sIOPMP: begin to handle cold device switching, deivceID is %d SID is %d\n", device_id, CSTL);

    /* Disable the interrupt from this device */
    writeb(0x1, (void *)CLEARINT_IOPMP + device_id * 1);

    // sbi_printf("sIOPMP: CSTL(SID) %u \n", CSTL);

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

    /* Set the current SID to the eSID regsiter, eSID is a per-device register */
    writel(CSTL, (void *)VSTL_IOPMP+device_id*4);
    /* Resume the interrupt for this device */
    writeb(0x0, (void *)CLEARINT_IOPMP + device_id * 1);

    /* For Debug, record the interrupt info */
    sIOPMP_device_id = device_id;
    sIOPMP_SID = CSTL;
    sIOPMP_interrupt_type = sIOPMP_DEVICE_SWITCHING;
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
				pmp_address = (iopmp_cfg_t.paddr | ((iopmp_cfg_t.size>>1)-1));
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
    writeq((u64)pmp_address, (void *)IOPMP_ADDR0 + iopmp_index*8);
    sbi_printf("sIOPMP: set IOPMP entry %d address %lx\n", iopmp_index, (u64) pmp_address);
}

void set_IOPMP_CFG(int iopmp_index, u8 value) {
    writeb(value, (void *)IOPMP_CFD0 + iopmp_index);
}

// /*
//  * SID: 64 (0~63)
//  * MD: 63 (0~62)
//  * IOPMP: 32 (0~31)
//  * eSID MD: index 62, the last MD is used for eSID
//  */
// void sIOPMP_setup() {
//     uint64_t SD0 = 0x00000001, SD1 = 0x00000001;
//     uint32_t MD0 = 0x0001, MD1 = 0x0002;
//     int i = 0;

//     // uint8_t read_dma, start_dma = 0xFF;
//     // uint32_t addr_iopmp = 0x89001FFFL;
//     // uint32_t addr_dma = 0x88000000L;
//     // uint16_t size_dma = 0x1000L;
//     /*
//      * Bit 7: Read
//      * Bit 6: write
//      * Bit 5: execution
//      * Bit 4,3: Mode
//      * Bit 2,1: res
//      * Bit 0: Lock
//      */

//     // IOPMP entry has no permission
//     // uint8_t CFG0 = 0x19, CFG1 = 0x19, CFG2 = 0x19, CFG3 = 0x19;

//     // Bind the SID0 to the MD0
//     sbi_printf("sIOPMP: sIOPMP setup\n");
//     for (i=0; i<16; i++) {
//         set_SRCMD(i, SD0);
//     }

//     // set the MD for dummy DMA (sid = 16)
//     set_SRCMD(16, SD1);
//     // Bind MD0 to the IOPMP0
//     set_MDCFG(0, MD0);
//     sbi_printf("sIOPMP: set the MDCFG0 MD0: %x\n", MD0);
//     // Bind the MD1 to the IOPMP1
//     set_MDCFG(1, MD1);

//     // struct iopmp_config_t iopmp_cfg_t_0 = {.paddr = 0x80000000, .size = 0x40000000, .perm = IOPMP_L|IOPMP_R|IOPMP_W|IOPMP_X, .mode = IOPMP_A_NAPOT};
//     struct iopmp_config_t iopmp_cfg_t_1 = {.paddr = 0x88000000L, .size = 0x4000, .perm = IOPMP_L, .mode = IOPMP_A_NAPOT};

//     set_IOPMP(0, iopmp_cfg_t_1);
//     set_IOPMP(1, iopmp_cfg_t_1);
//     set_IOPMP(2, iopmp_cfg_t_1);
//     set_IOPMP(3, iopmp_cfg_t_1);
//     sbi_printf("sIOPMP: setup is finished\n");

//     writel(0x0, (void *)STLEN_IOPMP);

//     // set eSID to the hardware register, currently 17 is a invalid SID
//     writel(17, (void *)VSTL_IOPMP);

//     // set the CAM table, map device id n to SID n
//     for (i = 0; i<17; i++) {
//         writel(0x00010001*i, (void *)SIDDEVICE_IOPMP);
//     }
// }

/*
 * SID: 64 (0~63)
 * MD: 63 (0~62)
 * IOPMP: 32 (0~31)
 * eSID MD: index 62, the last MD is used for eSID
 */
void sIOPMP_setup() {
    uint64_t SD0 = 0x00000001, SD1 = 0x00000002;
    uint32_t MD0 = 0x0001, MD1 = 0x0002, MD32 = 0x0020;
    int i = 0;


    sbi_printf("sIOPMP: sIOPMP setup\n");

    // Bind the SID0~15, 17~63 to the MD0
    for (i=0; i<16; i++) {
        set_SRCMD(i, SD0);
    }
    for (i=17; i<64; i++) {
        set_SRCMD(i, SD0);
    }
    
    // set the MD1 for dummy DMA (sid = 16)
    set_SRCMD(16, SD1);

    // Bind MD0 to the IOPMP0
    set_MDCFG(0, MD0);
    sbi_printf("sIOPMP: set the MDCFG0 MD0: %x\n", MD0);
    // Bind the MD1 to the IOPMP1
    set_MDCFG(1, MD1);

    // Bind the MD2~62 to the IOPMP2~31 (T=32), these domain will not be used in the initialization phase
    for (i=2; i<63; i++) {
        set_MDCFG(i, MD32);
    }

    /* TODO: the IOPMP protected size can not lager than 0x4000(?), otherwise, the IOPMP checker will be disable*/
    struct iopmp_config_t iopmp_cfg_t_0 = {.paddr = 0x80000000, .size = 0x40000000, .perm = IOPMP_L|IOPMP_R|IOPMP_W|IOPMP_X, .mode = IOPMP_A_NAPOT};
    struct iopmp_config_t iopmp_cfg_t_1 = {.paddr = 0x88000000, .size = 0x4000, .perm = IOPMP_L, .mode = IOPMP_A_NAPOT};

    set_IOPMP(0, iopmp_cfg_t_0);
    set_IOPMP(1, iopmp_cfg_t_1);
    // Following IOPMP entries will not be used in the initialization phase
    set_IOPMP(2, iopmp_cfg_t_1);
    set_IOPMP(3, iopmp_cfg_t_1);
    sbi_printf("sIOPMP: setup is finished\n");

    writel(0x0, (void *)STLEN_IOPMP);

    // set eSID to the hardware register, currently 17 is a invalid SID
    writel(17, (void *)VSTL_IOPMP);

    // set the CAM table, map device id n to SID n
    // Ignore device id 4~15 to test the cold device switching
    // SID 8 will be used for network
    for (i = 0; i<4; i++) {
        writel(0x00010001*i, (void *)SIDDEVICE_IOPMP);
    }

    // Map the device id (dummy DMA) 16 to the sid 16
    // We test dummy DMA in the test_DMA function
    for (i = 16; i<17; i++) {
        writel(0x00010001*i, (void *)SIDDEVICE_IOPMP);
    }
}

void test_DMA() {
    sbi_printf("sIOPMP: test DMA \n");
    uint32_t tmp = 0;
    uint32_t addr_dma = 0x88000000L;
    uint16_t size_dma = 0x1000L;
    uint8_t result_dma;
    writel(addr_dma, (void *)DMA_ADDR);
    writew(size_dma, (void *)DMA_SIZE);
    writel(0x01, (void *)DMA_COUNTER);
    result_dma = readb((void *)DMA_START);

    sbi_printf("sIOPMP: DMA result %u\n", result_dma);

    // Start DMA read
    writeb(0x1, (void *)DMA_START);

    for (int i = 0; i<10000; i++){
        tmp=tmp+1;
    }

    sbi_printf("sIOPMP: interrupt type %x\n", sIOPMP_interrupt_type);
}