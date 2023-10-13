/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_io.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi/riscv_asm.h>

#define PLIC_PRIORITY_BASE 0x0
#define PLIC_PENDING_BASE 0x1000
#define PLIC_ENABLE_BASE 0x2000
#define PLIC_ENABLE_STRIDE 0x80
#define PLIC_CONTEXT_BASE 0x200000
#define PLIC_CONTEXT_STRIDE 0x1000

/* Add by the sIOPMP */
#define PLIC_CONTEXT_CLAIM 0x200004
#define PLIC_BASE_ADDR 0x0C000000

/* Read interrupt id in the plic claim */
unsigned int PLIC_id_read(u32 cntxid)
{
    //read PIIL interrupt id
    volatile unsigned int id = readl((void *)PLIC_BASE_ADDR + PLIC_CONTEXT_CLAIM + cntxid*PLIC_CONTEXT_STRIDE);
    return id;
}

/* Clear interrupt id in the plic claim */
void PLIC_id_clr(unsigned int id, u32 cntxid)
{
    //clear PIIL interrupt id
    writel(id, (void *)PLIC_BASE_ADDR + PLIC_CONTEXT_CLAIM + cntxid*PLIC_CONTEXT_STRIDE);
}

static void plic_set_priority(struct plic_data *plic, u32 source, u32 val)
{
	volatile void *plic_priority = (void *)plic->addr +
			PLIC_PRIORITY_BASE + 4 * source;
	writel(val, plic_priority);
}

void plic_set_thresh(struct plic_data *plic, u32 cntxid, u32 val)
{
	volatile void *plic_thresh;

	if (!plic)
		return;

	plic_thresh = (void *)plic->addr +
		      PLIC_CONTEXT_BASE + PLIC_CONTEXT_STRIDE * cntxid;
	writel(val, plic_thresh);
}

void plic_set_ie(struct plic_data *plic, u32 cntxid, u32 word_index, u32 val)
{
	volatile void *plic_ie;

	if (!plic)
		return;

	plic_ie = (void *)plic->addr +
		   PLIC_ENABLE_BASE + PLIC_ENABLE_STRIDE * cntxid;
	writel(val, plic_ie + word_index * 4);
	sbi_printf("OPENSBI: enable interrupt id bitmap %u for PLIC address %lx\n", val, (unsigned long)plic_ie + word_index * 4);
}

int plic_warm_irqchip_init(struct plic_data *plic,
			   int m_cntx_id, int s_cntx_id)
{
	size_t i, ie_words;

	if (!plic)
		return SBI_EINVAL;

	ie_words = plic->num_src / 32 + 1;

	/* By default, disable all IRQs for M-mode of target HART */
	/* Enable the IOPMP interrupt 1 and 2 for m mode */
	if (m_cntx_id > -1) {
		for (i = 0; i < ie_words; i++) {
			if (i == 0)
				plic_set_ie(plic, m_cntx_id, i, (1<<sIOPMP_VIOLATION) | (1<<sIOPMP_DEVICE_SWITCHING));
			else
				plic_set_ie(plic, m_cntx_id, i, 0);
		}
	}
	sbi_printf("OPENSBI: addr %lx num_src %lu m_cntx_id %d ie_words %lu\n", plic->addr, plic->num_src, m_cntx_id, ie_words);
	// sbi_printf("OPENSBI: enable interrupt id %x \n", (1<<sIOPMP_VIOLATION) | (1<<sIOPMP_DEVICE_SWITCHING));

	/* By default, disable all IRQs for S-mode of target HART */
	if (s_cntx_id > -1) {
		for (i = 0; i < ie_words; i++)
			plic_set_ie(plic, s_cntx_id, i, 0);
	}
	sbi_printf("OPENSBI: s_cntx_id %d\n", s_cntx_id);

	/* By default, enable M-mode threshold to 1*/
	if (m_cntx_id > -1)
		plic_set_thresh(plic, m_cntx_id, 0x1);
	sbi_printf("OPENSBI: set threshold for m_cntx_id %d to 1\n", m_cntx_id);

	/* By default, disable S-mode threshold */
	if (s_cntx_id > -1)
		plic_set_thresh(plic, s_cntx_id, 0x7);

	// Enable MIE and MIE in mstatus
	csr_write(CSR_MIE, csr_read(CSR_MIE) | MIP_MEIP);
	csr_write(CSR_MSTATUS, csr_read(CSR_MSTATUS) | MSTATUS_MIE);

	return 0;
}

int plic_cold_irqchip_init(struct plic_data *plic)
{
	int i;

	if (!plic)
		return SBI_EINVAL;

	/* Configure default priorities of all IRQs */
	for (i = 1; i <= plic->num_src; i++)
		plic_set_priority(plic, i, 0);

	/* Set the sIOPMP interrupt priority */
	plic_set_priority(plic, sIOPMP_VIOLATION, 2);
	plic_set_priority(plic, sIOPMP_DEVICE_SWITCHING, 3);
	sbi_printf("OPENSBI: set interrupt %d priority to 2, set interrupt %d priority to 3\n", sIOPMP_VIOLATION, sIOPMP_DEVICE_SWITCHING);
	return 0;
}
