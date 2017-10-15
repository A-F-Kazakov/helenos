/*
 * Copyright (c) 2017 Ondrej Hlavaty <aearsis@eideo.cz>
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
/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
 * An implementation of bus keeper for xHCI. The (physical) HC itself takes
 * care about addressing devices, so this implementation is actually simpler
 * than those of [OUE]HCI.
 */
#ifndef XHCI_BUS_H
#define XHCI_BUS_H

#include <adt/hash_table.h>
#include <usb/usb.h>
#include <usb/host/bus.h>

typedef struct xhci_hc xhci_hc_t;

/** Endpoint management structure */
typedef struct xhci_bus {
	bus_t base;		/**< Inheritance. Keep this first. */

	/** TODO: some mechanism to keep endpoints alive :)
	 * We may inspire in the usb2_bus, but keep in mind xHCI have much
	 * larger address space, thus simple array of lists for all available
	 * addresses can be just too big.
	 */

	hash_table_t devices;
} xhci_bus_t;

int xhci_bus_init(xhci_bus_t *);
void xhci_bus_fini(xhci_bus_t *);

int xhci_bus_enumerate_device(xhci_bus_t *, xhci_hc_t *, device_t *);

#endif
/**
 * @}
 */
