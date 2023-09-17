#include <stdint.h>
#include <sbi/sbi_types.h>

#define DMA_START 0x2000
#define DMA_ADDR 0x2010
#define DMA_SIZE 0x2030
#define DMA_COUNTER 0x2040
#define DMA_BURSTSIZE 0x2080
#define DMA_DUMMY_START 0x20000000
#define DMA_DUMMY_ADDR 0x20000010
#define DMA_DUMMY_SIZE 0x20000030
#define DMA_DUMMY_COUNTER 0x20000040
#define DMA_DUMMY_BURSTSIZE 0x20000080
#define IOPMP_SD0 0x10028000
#define IOPMP_SD1 0x10028008
#define IOPMP_MD0 0x10028200
#define IOPMP_MD1 0x10028204
#define IOPMP_CFD0 0x10028300
#define IOPMP_CFD1 0x10028301
#define IOPMP_CFD2 0x10028302
#define IOPMP_CFD3 0x10028303
#define IOPMP_ADDR0 0x10028800
#define IOPMP_ADDR1 0x10028804
#define IOPMP_ADDR2 0x10028808
#define IOPMP_ADDR3 0x1002880C


#define ADDR_INTER_IOPMP 0x10029800
#define SID_INDEX_INTER_IOPMP 0x10029804
#define PERMISSION_TYPE_INTER_IOPMP 0x10029808
#define STLEN_IOPMP 0x1002980C
#define STLSTAT_IOPMP 0x10029810
#define VSTL_IOPMP 0x10029850
#define CSTL_IOPMP 0x10029828
#define CLEARINT_IOPMP 0x10029848
#define DEVICE_INT_IOPMP 0x10029870
#define SIDDEVICE_IOPMP 0x10029874

#define IOPMP_R				_UL(0x80)
#define IOPMP_W				_UL(0x40)
#define IOPMP_X				_UL(0x20)
#define IOPMP_A				_UL(0x18) 
#define IOPMP_A_TOR			_UL(0x08)
#define IOPMP_A_NA4			_UL(0x10)
#define IOPMP_A_NAPOT			_UL(0x18)
#define IOPMP_L				_UL(0x1)

struct iopmp_config_t
{
  uintptr_t paddr;
  unsigned long size;
  u8 perm;
  u8 mode;
};

void handle_sIOPMP_violation();

void handle_cold_device_switching();

void sIOPMP_setup();