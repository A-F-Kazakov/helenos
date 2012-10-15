/*
 * Copyright (c) 2012 Jan Vesely
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief ARM926 on-chip UART (PrimeCell UART, PL011) driver.
 */

#ifndef KERN_ARM926_UART_H_
#define KERN_ARM926_UART_H_

#include <ddi/irq.h>
#include <console/chardev.h>
#include <typedefs.h>


/** ARM926 User Guide ch. 4.8.5 (p. 106 in the pdf) */
#define ARM926_UART0_BASE_ADDRESS   0x16000000
#define ARM926_UART1_BASE_ADDRESS   0x16000000

/** ARM926 User Guide ch. A.1 (p. 124 in the pdf) */
#define ARM926_UART0_IRQ   1
#define ARM926_UART1_IRQ   2

/** PrimeCell UART TRM ch. 3.3 (p. 49 in the pdf) */
typedef struct {
	/** UART data register */
	ioport32_t data;
#define ARM926_UART_DATA_DATA_MASK   0xff
#define ARM926_UART_DATA_FE_FLAG   (1 <<  7)
#define ARM926_UART_DATA_PE_FLAG   (1 <<  9)
#define ARM926_UART_DATA_BE_FLAG   (1 << 10)
#define ARM926_UART_DATA_OE_FLAG   (1 << 11)

	union {
		/* Same values that are in upper bits of data register*/
		const ioport32_t status;
#define ARM926_UART_STATUS_FE_FLAG   (1 << 0)
#define ARM926_UART_STATUS_PE_FLAG   (1 << 1)
#define ARM926_UART_STATUS_BE_FLAG   (1 << 2)
#define ARM926_UART_STATUS_OE_FLAG   (1 << 3)
		/* Writing anything clears all errors */
		ioport32_t error_clear;
	};
	uint32_t padd0_[4];

	const ioport32_t flag;
#define ARM926_UART_FLAG_CTS_FLAG    (1 << 0)
#define ARM926_UART_FLAG_DSR_FLAG    (1 << 1)
#define ARM926_UART_FLAG_DCD_FLAG    (1 << 2)
#define ARM926_UART_FLAG_BUSY_FLAG   (1 << 3)
#define ARM926_UART_FLAG_RXFE_FLAG   (1 << 4)
#define ARM926_UART_FLAG_TXFF_FLAG   (1 << 5)
#define ARM926_UART_FLAG_RXFF_FLAG   (1 << 6)
#define ARM926_UART_FLAG_TXFE_FLAG   (1 << 7)
#define ARM926_UART_FLAG_RI_FLAG     (1 << 8)
	uint32_t padd1_;

	ioport32_t irda_low_power;
#define ARM926_UART_IRDA_LOW_POWER_MASK   0xff

	ioport32_t int_baud_divisor;
#define ARM926_UART_INT_BAUD_DIVISOR_MASK   0xffff

	ioport32_t fract_baud_divisor;
#define ARM926_UART_FRACT_BAUD_DIVISOR_MASK   0x1f

	ioport32_t line_control_high;
#define ARM926_UART_CONTROLHI_BRK_FLAG    (1 << 0)
#define ARM926_UART_CONTROLHI_PEN_FLAG    (1 << 1)
#define ARM926_UART_CONTROLHI_EPS_FLAG    (1 << 2)
#define ARM926_UART_CONTROLHI_STP2_FLAG   (1 << 3)
#define ARM926_UART_CONTROLHI_FEN_FLAG    (1 << 4)
#define ARM926_UART_CONTROLHI_WLEN_MASK   0x3
#define ARM926_UART_CONTROLHI_WLEN_SHIFT    5
#define ARM926_UART_CONTROLHI_SPS_FLAG    (1 << 5)

	ioport32_t control;
#define ARM926_UART_CONTROL_UARTEN_FLAG   (1 << 0)
#define ARM926_UART_CONTROL_SIREN_FLAG    (1 << 1)
#define ARM926_UART_CONTROL_SIRLP_FLAG    (1 << 2)
#define ARM926_UART_CONTROL_LBE_FLAG      (1 << 7)
#define ARM926_UART_CONTROL_TXE_FLAG      (1 << 8)
#define ARM926_UART_CONTROL_RXE_FLAG      (1 << 9)
#define ARM926_UART_CONTROL_DTR_FLAG     (1 << 10)
#define ARM926_UART_CONTROL_RTS_FLAG     (1 << 11)
#define ARM926_UART_CONTROL_OUT1_FLAG    (1 << 12)
#define ARM926_UART_CONTROL_OUT2_FLAG    (1 << 13)
#define ARM926_UART_CONTROL_RTSE_FLAG    (1 << 14)
#define ARM926_UART_CONTROL_CTSE_FLAG    (1 << 15)

	ioport32_t interrupt_fifo;
#define ARM926_UART_INTERRUPTFIFO_TX_MASK   0x7
#define ARM926_UART_INTERRUPTFIFO_TX_SHIFT    0
#define ARM926_UART_INTERRUPTFIFO_RX_MASK   0x7
#define ARM926_UART_INTERRUPTFIFO_RX_SHIFT    3

	/** Interrupt mask register */
	ioport32_t interrupt_mask;
	/** Pending interrupts before applying the mask */
	const ioport32_t raw_interrupt_status;
	/** Pending interrupts after applying the mask */
	const ioport32_t masked_interrupt_status;
	/** Write 1s to clear pending interrupts */
	ioport32_t interrupt_clear;
#define ARM926_UART_INTERRUPT_RIM_FLAG    (1 << 0)
#define ARM926_UART_INTERRUPT_CTSM_FLAG   (1 << 1)
#define ARM926_UART_INTERRUPT_DCDM_FLAG   (1 << 2)
#define ARM926_UART_INTERRUPT_DSRM_FLAG   (1 << 3)
#define ARM926_UART_INTERRUPT_RX_FLAG     (1 << 4)
#define ARM926_UART_INTERRUPT_TX_FLAG     (1 << 5)
#define ARM926_UART_INTERRUPT_RT_FLAG     (1 << 6)
#define ARM926_UART_INTERRUPT_FE_FLAG     (1 << 7)
#define ARM926_UART_INTERRUPT_PE_FLAG     (1 << 8)
#define ARM926_UART_INTERRUPT_BE_FLAG     (1 << 9)
#define ARM926_UART_INTERRUPT_OE_FLAG    (1 << 10)
#define ARM926_UART_INTERRUPT_ALL           0x3ff

	ioport32_t dma_control;
#define ARM926_UART_DMACONTROL_RXDMAEN_FLAG    (1 << 0)
#define ARM926_UART_DMACONTROL_TXDMAEN_FLAG    (1 << 1)
#define ARM926_UART_DMACONTROL_DMAONERR_FLAG   (1 << 2)

	// TODO There is some reserved space here followed by
	// peripheral identification registers.
} arm926_uart_regs_t;

typedef struct {
	arm926_uart_regs_t *regs;
	indev_t *indev;
	outdev_t outdev;
	irq_t irq;
} arm926_uart_t;

bool arm926_uart_init(arm926_uart_t *, inr_t, uintptr_t, size_t);
void arm926_uart_input_wire(arm926_uart_t *, indev_t *);

#endif
/**
 * @}
 */
