/*
 * Copyright (c) 2011 Vojtech Horky, Jan Vesely
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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver hc interface implementation
 */
#include <ddf/driver.h>
#include <errno.h>

#include <usb/debug.h>
#include <usb/host/endpoint.h>

#include "iface.h"
#include "hc.h"

static inline int setup_batch(
    ddf_fun_t *fun, usb_target_t target, usb_direction_t direction,
    void *data, size_t size, void * setup_data, size_t setup_size,
    usbhc_iface_transfer_in_callback_t in,
    usbhc_iface_transfer_out_callback_t out, void *arg, const char* name,
    hc_t **hc, usb_transfer_batch_t **batch)
{
	assert(hc);
	assert(batch);
	assert(fun);
	*hc = fun_to_hc(fun);
	assert(*hc);

	size_t res_bw;
	endpoint_t *ep = hc_get_endpoint(*hc,
	    target.address, target.endpoint, direction, &res_bw);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for %s.\n",
		    target.address, target.endpoint, name);
		return ENOENT;
	}

	usb_log_debug("%s %d:%d %zu(%zu).\n",
	    name, target.address, target.endpoint, size, ep->max_packet_size);

	const size_t bw = bandwidth_count_usb11(
	    ep->speed, ep->transfer_type, size, ep->max_packet_size);
	if (res_bw < bw) {
		usb_log_error("Endpoint(%d:%d) %s needs %zu bw "
		    "but only %zu is reserved.\n",
		    target.address, target.endpoint, name, bw, res_bw);
		return ENOSPC;
	}

	*batch = batch_get(
	    fun, ep, data, size, setup_data, setup_size, in, out, arg);
	if (!*batch)
		return ENOMEM;
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Request address interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] speed Speed to associate with the new default address.
 * @param[out] address Place to write a new address.
 * @return Error code.
 */
static int request_address(
    ddf_fun_t *fun, usb_speed_t speed, usb_address_t *address)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	assert(address);

	usb_log_debug("Address request with speed %d.\n", speed);
	*address = device_keeper_get_free_address(&hc->manager, speed);
	usb_log_debug("Address request with result: %d.\n", *address);
	if (*address <= 0)
		return *address;
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Bind address interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] address Address of the device
 * @param[in] handle Devman handle of the device driver.
 * @return Error code.
 */
static int bind_address(
  ddf_fun_t *fun, usb_address_t address, devman_handle_t handle)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Address bind %d-%" PRIun ".\n", address, handle);
	usb_device_keeper_bind(&hc->manager, address, handle);
	return EOK;
}


/** Find device handle by address interface function.
 *
 * @param[in] fun DDF function that was called.
 * @param[in] address Address in question.
 * @param[out] handle Where to store device handle if found.
 * @return Error code.
 */
static int find_by_address(ddf_fun_t *fun, usb_address_t address,
    devman_handle_t *handle)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	bool found =
	    usb_device_keeper_find_by_address(&hc->manager, address, handle);
	return found ? EOK : ENOENT;
}

/*----------------------------------------------------------------------------*/
/** Release address interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] address USB address to be released.
 * @return Error code.
 */
static int release_address(ddf_fun_t *fun, usb_address_t address)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Address release %d.\n", address);
	usb_device_keeper_release(&hc->manager, address);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Register endpoint for bandwidth reservation.
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] address USB address of the device.
 * @param[in] ep_speed Endpoint speed (invalid means to use device one).
 * @param[in] endpoint Endpoint number.
 * @param[in] transfer_type USB transfer type.
 * @param[in] direction Endpoint data direction.
 * @param[in] max_packet_size Max packet size of the endpoint.
 * @param[in] interval Polling interval.
 * @return Error code.
 */
static int register_endpoint(ddf_fun_t *fun,
    usb_address_t address, usb_speed_t ep_speed, usb_endpoint_t endpoint,
    usb_transfer_type_t transfer_type, usb_direction_t direction,
    size_t max_packet_size, unsigned int interval)
{
	hc_t *hc = fun_to_hc(fun);
	assert(hc);

	usb_speed_t speed = usb_device_keeper_get_speed(&hc->manager, address);
	if (speed >= USB_SPEED_MAX) {
		speed = ep_speed;
	}
	const size_t size = max_packet_size;

	usb_log_debug("Register endpoint %d:%d %s %s(%d) %zu(%zu) %u.\n",
	    address, endpoint, usb_str_transfer_type(transfer_type),
	    usb_str_speed(speed), direction, size, max_packet_size, interval);

	return hc_add_endpoint(hc, address, endpoint, speed, transfer_type,
	    direction, max_packet_size, size, interval);
}
/*----------------------------------------------------------------------------*/
static int unregister_endpoint(
    ddf_fun_t *fun, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction)
{
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Unregister endpoint %d:%d %d.\n",
	    address, endpoint, direction);
	return hc_remove_endpoint(hc, address, endpoint, direction);
}
/*----------------------------------------------------------------------------*/
/** Schedule interrupt out transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] data Data to be sent (in USB endianess, allocated and deallocated
 *	by the caller).
 * @param[in] size Size of the @p data buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int interrupt_out(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	usb_transfer_batch_t *batch = NULL;
	hc_t *hc = NULL;
	int ret = setup_batch(fun, target, USB_DIRECTION_OUT, data, size,
	    NULL, 0, NULL, callback, arg, "Interrupt OUT", &hc, &batch);
	if (ret != EOK)
		return ret;
	batch_interrupt_out(batch);
	ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		usb_transfer_batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule interrupt in transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] data Buffer where to store the data (in USB endianess,
 *	allocated and deallocated by the caller).
 * @param[in] size Size of the @p data buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int interrupt_in(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	usb_transfer_batch_t *batch = NULL;
	hc_t *hc = NULL;
	int ret = setup_batch(fun, target, USB_DIRECTION_IN, data, size,
	    NULL, 0, callback, NULL, arg, "Interrupt IN", &hc, &batch);
	if (ret != EOK)
		return ret;
	batch_interrupt_in(batch);
	ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		usb_transfer_batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule bulk out transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] data Data to be sent (in USB endianess, allocated and deallocated
 *	by the caller).
 * @param[in] size Size of the @p data buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int bulk_out(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	usb_transfer_batch_t *batch = NULL;
	hc_t *hc = NULL;
	int ret = setup_batch(fun, target, USB_DIRECTION_OUT, data, size,
	    NULL, 0, NULL, callback, arg, "Bulk OUT", &hc, &batch);
	if (ret != EOK)
		return ret;
	batch_bulk_out(batch);
	ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		usb_transfer_batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule bulk in transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] data Buffer where to store the data (in USB endianess,
 *	allocated and deallocated by the caller).
 * @param[in] size Size of the @p data buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int bulk_in(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	usb_transfer_batch_t *batch = NULL;
	hc_t *hc = NULL;
	int ret = setup_batch(fun, target, USB_DIRECTION_IN, data, size,
	    NULL, 0, callback, NULL, arg, "Bulk IN", &hc, &batch);
	if (ret != EOK)
		return ret;
	batch_bulk_in(batch);
	ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		usb_transfer_batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule control write transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] setup_packet Setup packet buffer (in USB endianess, allocated
 *	and deallocated by the caller).
 * @param[in] setup_size Size of @p setup_packet buffer in bytes.
 * @param[in] data_buffer Data buffer (in USB endianess, allocated and
 *	deallocated by the caller).
 * @param[in] size Size of @p data_buffer buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int control_write(
    ddf_fun_t *fun, usb_target_t target,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	usb_transfer_batch_t *batch = NULL;
	hc_t *hc = NULL;
	int ret = setup_batch(fun, target, USB_DIRECTION_BOTH, data, size,
	    setup_data, setup_size, NULL, callback, arg, "Control WRITE",
	    &hc, &batch);
	if (ret != EOK)
		return ret;
	usb_endpoint_manager_reset_if_need(&hc->ep_manager, target, setup_data);
	batch_control_write(batch);
	ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		usb_transfer_batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule control read transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] setup_packet Setup packet buffer (in USB endianess, allocated
 *	and deallocated by the caller).
 * @param[in] setup_size Size of @p setup_packet buffer in bytes.
 * @param[in] data_buffer Buffer where to store the data (in USB endianess,
 *	allocated and deallocated by the caller).
 * @param[in] size Size of @p data_buffer buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int control_read(
    ddf_fun_t *fun, usb_target_t target,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	usb_transfer_batch_t *batch = NULL;
	hc_t *hc = NULL;
	int ret = setup_batch(fun, target, USB_DIRECTION_BOTH, data, size,
	    setup_data, setup_size, callback, NULL, arg, "Control READ",
	    &hc, &batch);
	if (ret != EOK)
		return ret;
	batch_control_read(batch);
	ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		usb_transfer_batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
usbhc_iface_t hc_iface = {
	.request_address = request_address,
	.bind_address = bind_address,
	.find_by_address = find_by_address,
	.release_address = release_address,

	.register_endpoint = register_endpoint,
	.unregister_endpoint = unregister_endpoint,

	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,

	.bulk_out = bulk_out,
	.bulk_in = bulk_in,

	.control_write = control_write,
	.control_read = control_read,
};
/**
 * @}
 */
