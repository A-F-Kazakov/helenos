/*
 * Copyright (c) 2017 Petr Manek
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
 * @brief The host controller endpoint management.
 */

#ifndef XHCI_ENDPOINT_H
#define XHCI_ENDPOINT_H

#include <assert.h>

#include <usb/debug.h>
#include <usb/host/endpoint.h>
#include <usb/host/hcd.h>

#include "hc.h"
#include "transfers.h"

typedef struct xhci_device xhci_device_t;
typedef struct xhci_endpoint xhci_endpoint_t;
typedef struct xhci_bus xhci_bus_t;

enum {
	EP_TYPE_INVALID = 0,
	EP_TYPE_ISOCH_OUT = 1,
	EP_TYPE_BULK_OUT = 2,
	EP_TYPE_INTERRUPT_OUT = 3,
	EP_TYPE_CONTROL = 4,
	EP_TYPE_ISOCH_IN = 5,
	EP_TYPE_BULK_IN = 6,
	EP_TYPE_INTERRUPT_IN = 7
};

/** Connector structure linking endpoint context to the endpoint. */
typedef struct xhci_endpoint {
	endpoint_t base;	/**< Inheritance. Keep this first. */

	/** Main transfer ring (unused if streams are enabled) */
	xhci_trb_ring_t ring;

	/** There shall be only one transfer active on an endpoint. The
	 * synchronization is performed using the active flag in base
	 * endpoint_t */
	xhci_transfer_t active_transfer;

	/** Primary stream context array (or NULL if endpoint doesn't use streams) */
	xhci_stream_ctx_t *primary_stream_ctx_array;

	/** 2-log of maximum number of primary streams (0-16). Not to be used directly. */
	uint8_t max_streams;

	/** Maximum number of consecutive USB transactions (0-15) that should be executed per scheduling opportunity */
	uint8_t max_burst;

	/** Maximum number of bursts within an interval that this endpoint supports */
	uint8_t mult;
} xhci_endpoint_t;

typedef struct xhci_device {
	device_t base;		/**< Inheritance. Keep this first. */

	/** Slot ID assigned to the device by xHC. */
	uint32_t slot_id;

	/** Place to store virtual address for allocated context */
	xhci_device_ctx_t *dev_ctx;

	/** All endpoints of the device. Inactive ones are NULL */
	xhci_endpoint_t *endpoints[XHCI_EP_COUNT];

	/** Number of non-NULL endpoints. Reference count of sorts. */
	uint8_t active_endpoint_count;

	/** Need HC to schedule commands from bus callbacks. TODO: Move this elsewhere. */
	xhci_hc_t *hc;

	/** Flag indicating whether the device is USB3 (it's USB2 otherwise). */
	bool usb3;

	/** True if the device can add new endpoints and schedule transfers. */
	volatile bool online;
} xhci_device_t;

int xhci_endpoint_init(xhci_endpoint_t *, xhci_bus_t *);
void xhci_endpoint_fini(xhci_endpoint_t *);
int xhci_endpoint_alloc_transfer_ds(xhci_endpoint_t *);
int xhci_endpoint_free_transfer_ds(xhci_endpoint_t *);

uint8_t xhci_endpoint_dci(xhci_endpoint_t *);
uint8_t xhci_endpoint_index(xhci_endpoint_t *);

int xhci_device_add_endpoint(xhci_device_t *, xhci_endpoint_t *);
int xhci_device_remove_endpoint(xhci_device_t *, xhci_endpoint_t *);
xhci_endpoint_t * xhci_device_get_endpoint(xhci_device_t *, usb_endpoint_t);
int xhci_device_configure(xhci_device_t *, xhci_hc_t *);

static inline xhci_device_t * xhci_device_get(device_t *dev)
{
	assert(dev);
	return (xhci_device_t *) dev;
}

static inline xhci_endpoint_t * xhci_endpoint_get(endpoint_t *ep)
{
	assert(ep);
	return (xhci_endpoint_t *) ep;
}

static inline xhci_device_t * xhci_ep_to_dev(xhci_endpoint_t *ep)
{
	assert(ep);
	return xhci_device_get(ep->base.device);
}

#endif

/**
 * @}
 */
