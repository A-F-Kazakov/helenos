/*
 * Copyright (c) 2017 Michal Staruch
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The roothub structures abstraction.
 */

#ifndef XHCI_RH_H
#define XHCI_RH_H

#include <usb/host/usb_transfer_batch.h>
#include <usb/host/bus.h>

#include "hw_struct/regs.h"
#include "endpoint.h"

typedef struct xhci_hc xhci_hc_t;
typedef struct ddf_dev ddf_dev_t;

/**
 * xHCI lets the controller define speeds of ports it controls.
 */
typedef struct xhci_port_speed {
	char name [4];
	uint8_t major, minor;
	uint64_t rx_bps, tx_bps;
	usb_speed_t usb_speed;
} xhci_port_speed_t;

typedef struct hcd_roothub hcd_roothub_t;
typedef struct xhci_bus xhci_bus_t;

/* XHCI root hub instance */
typedef struct {
	/** Host controller */
	xhci_hc_t *hc;

	/* Root for the device tree */
	xhci_device_t device;

	/* We need this to attach children to */
	ddf_dev_t *hc_device;

	/** Interrupt transfer waiting for an actual interrupt to occur */
	usb_transfer_batch_t *unfinished_interrupt_transfer;

	/* Number of hub ports. */
	uint8_t max_ports;

	/* Device pointers connected to RH ports or NULL. (size is `max_ports`) */
	xhci_device_t **devices_by_port;
} xhci_rh_t;

int xhci_rh_init(xhci_rh_t *, xhci_hc_t *, ddf_dev_t *);
int xhci_rh_fini(xhci_rh_t *);
const xhci_port_speed_t *xhci_rh_get_port_speed(xhci_rh_t *, uint8_t);
int xhci_rh_reset_port(xhci_rh_t *, uint8_t);

int xhci_rh_handle_port_status_change_event(xhci_hc_t *, xhci_trb_t *);
void xhci_rh_handle_port_change(xhci_rh_t *);

int xhci_rh_address_device(xhci_rh_t *rh, device_t *dev, xhci_bus_t *bus);

#endif

/**
 * @}
 */
