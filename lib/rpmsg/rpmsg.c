/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 * Copyright (c) 2018 Linaro, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <openamp/rpmsg.h>
#include <metal/alloc.h>

#include "rpmsg_internal.h"

/**
 * @internal
 *
 * @brief rpmsg_get_address
 *
 * This function provides unique 32 bit address.
 *
 * @param bitmap	Bit map for addresses
 * @param size		Size of bitmap
 *
 * @return A unique address
 */
static uint32_t rpmsg_get_address(unsigned long *bitmap, int size)
{
	unsigned int addr = RPMSG_ADDR_ANY;
	unsigned int nextbit;

	nextbit = metal_bitmap_next_clear_bit(bitmap, 0, size);
	if (nextbit < (uint32_t)size) {
		addr = RPMSG_RESERVED_ADDRESSES + nextbit;
		metal_bitmap_set_bit(bitmap, nextbit);
	}

	return addr;
}

/**
 * @internal
 *
 * @brief Frees the given address.
 *
 * @param bitmap	Bit map for addresses
 * @param size		Size of bitmap
 * @param addr		Address to free
 */
static void rpmsg_release_address(unsigned long *bitmap, int size,
				  int addr)
{
	addr -= RPMSG_RESERVED_ADDRESSES;
	if (addr >= 0 && addr < size)
		metal_bitmap_clear_bit(bitmap, addr);
}

/**
 * @internal
 *
 * @brief Checks whether address is used or free.
 *
 * @param bitmap	Bit map for addresses
 * @param size		Size of bitmap
 * @param addr		Address to free
 *
 * @return TRUE/FALSE
 */
static int rpmsg_is_address_set(unsigned long *bitmap, int size, int addr)
{
	addr -= RPMSG_RESERVED_ADDRESSES;
	if (addr >= 0 && addr < size)
		return metal_bitmap_is_bit_set(bitmap, addr);
	else
		return RPMSG_ERR_PARAM;
}

/**
 * @internal
 *
 * @brief Marks the address as consumed.
 *
 * @param bitmap	Bit map for addresses
 * @param size		Size of bitmap
 * @param addr		Address to free
 *
 * @return 0 on success, otherwise error code
 */
static int rpmsg_set_address(unsigned long *bitmap, int size, int addr)
{
	addr -= RPMSG_RESERVED_ADDRESSES;
	if (addr >= 0 && addr < size) {
		metal_bitmap_set_bit(bitmap, addr);
		return RPMSG_SUCCESS;
	} else {
		return RPMSG_ERR_PARAM;
	}
}

int rpmsg_send_offchannel_raw(struct rpmsg_endpoint *ept, uint32_t src,
			      uint32_t dst, const void *data, int len,
			      int wait)
{
	struct rpmsg_device *rdev;

	if (!ept || !ept->rdev || !data || dst == RPMSG_ADDR_ANY || len < 0)
		return RPMSG_ERR_PARAM;

	rdev = ept->rdev;

	if (rdev->ops.send_offchannel_raw)
		return rdev->ops.send_offchannel_raw(rdev, src, dst, data,
						     len, wait);

	return RPMSG_ERR_PARAM;
}

int rpmsg_send_ns_message(struct rpmsg_endpoint *ept, unsigned long flags)
{
	struct rpmsg_ns_msg ns_msg;
	int ret;

	ns_msg.flags = flags;
	ns_msg.addr = ept->addr;
	strncpy(ns_msg.name, ept->name, sizeof(ns_msg.name));
	ret = rpmsg_send_offchannel_raw(ept, ept->addr,
					RPMSG_NS_EPT_ADDR,
					&ns_msg, sizeof(ns_msg), true);
	if (ret < 0)
		return ret;
	else
		return RPMSG_SUCCESS;
}

void rpmsg_hold_rx_buffer(struct rpmsg_endpoint *ept, void *rxbuf)
{
	struct rpmsg_device *rdev;

	if (!ept || !ept->rdev || !rxbuf)
		return;

	rdev = ept->rdev;

	if (rdev->ops.hold_rx_buffer)
		rdev->ops.hold_rx_buffer(rdev, rxbuf);
}

void rpmsg_release_rx_buffer(struct rpmsg_endpoint *ept, void *rxbuf)
{
	struct rpmsg_device *rdev;

	if (!ept || !ept->rdev || !rxbuf)
		return;

	rdev = ept->rdev;

	if (rdev->ops.release_rx_buffer)
		rdev->ops.release_rx_buffer(rdev, rxbuf);
}

int rpmsg_release_tx_buffer(struct rpmsg_endpoint *ept, void *buf)
{
	struct rpmsg_device *rdev;

	if (!ept || !ept->rdev || !buf)
		return RPMSG_ERR_PARAM;

	rdev = ept->rdev;

	if (rdev->ops.release_tx_buffer)
		return rdev->ops.release_tx_buffer(rdev, buf);

	return RPMSG_ERR_PERM;
}

void *rpmsg_get_tx_payload_buffer(struct rpmsg_endpoint *ept,
				  uint32_t *len, int wait)
{
	struct rpmsg_device *rdev;

	if (!ept || !ept->rdev || !len)
		return NULL;

	rdev = ept->rdev;

	if (rdev->ops.get_tx_payload_buffer)
		return rdev->ops.get_tx_payload_buffer(rdev, len, wait);

	return NULL;
}

int rpmsg_send_offchannel_nocopy(struct rpmsg_endpoint *ept, uint32_t src,
				 uint32_t dst, const void *data, int len)
{
	struct rpmsg_device *rdev;

	if (!ept || !ept->rdev || !data || dst == RPMSG_ADDR_ANY || len < 0)
		return RPMSG_ERR_PARAM;

	rdev = ept->rdev;

	if (rdev->ops.send_offchannel_nocopy)
		return rdev->ops.send_offchannel_nocopy(rdev, src, dst,
							data, len);

	return RPMSG_ERR_PARAM;
}

struct rpmsg_endpoint *rpmsg_get_endpoint(struct rpmsg_device *rdev,
					  const char *name, uint32_t addr,
					  uint32_t dest_addr)
{
	struct metal_list *node;
	struct rpmsg_endpoint *ept;

	metal_list_for_each(&rdev->endpoints, node) {
		int name_match = 0;

		ept = metal_container_of(node, struct rpmsg_endpoint, node);
		/* try to get by local address only */
		if (addr != RPMSG_ADDR_ANY && ept->addr == addr)
			return ept;
		/* else use name service and destination address */
		if (name)
			name_match = !strncmp(ept->name, name,
					      sizeof(ept->name));
		if (!name || !name_match)
			continue;
		/* destination address is known, equal to ept remote address */
		if (dest_addr != RPMSG_ADDR_ANY && ept->dest_addr == dest_addr)
			return ept;
		/* ept is registered but not associated to remote ept */
		if (addr == RPMSG_ADDR_ANY && ept->dest_addr == RPMSG_ADDR_ANY)
			return ept;
	}
	return NULL;
}

static void rpmsg_unregister_endpoint(struct rpmsg_endpoint *ept)
{
	struct rpmsg_device *rdev = ept->rdev;

	metal_mutex_acquire(&rdev->lock);
	if (ept->addr != RPMSG_ADDR_ANY)
		rpmsg_release_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE,
				      ept->addr);
	metal_list_del(&ept->node);
	ept->rdev = NULL;
	metal_mutex_release(&rdev->lock);
}

void rpmsg_register_endpoint(struct rpmsg_device *rdev,
			     struct rpmsg_endpoint *ept,
			     const char *name,
			     uint32_t src, uint32_t dest,
			     rpmsg_ept_cb cb,
			     rpmsg_ns_unbind_cb ns_unbind_cb)
{
	strncpy(ept->name, name ? name : "", sizeof(ept->name));
	ept->addr = src;
	ept->dest_addr = dest;
	ept->cb = cb;
	ept->ns_unbind_cb = ns_unbind_cb;
	ept->rdev = rdev;
	metal_list_add_tail(&rdev->endpoints, &ept->node);
}

int rpmsg_create_ept(struct rpmsg_endpoint *ept, struct rpmsg_device *rdev,
		     const char *name, uint32_t src, uint32_t dest,
		     rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb)
{
	int status = RPMSG_SUCCESS;
	uint32_t addr = src;

	if (!ept || !rdev || !cb){
		return RPMSG_ERR_PARAM;
	}
		

	metal_mutex_acquire(&rdev->lock);
	if (src == RPMSG_ADDR_ANY) {
		addr = rpmsg_get_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE);
		if (addr == RPMSG_ADDR_ANY) {
			status = RPMSG_ERR_ADDR;
			goto ret_status;
		}
	} else if (src >= RPMSG_RESERVED_ADDRESSES) {
		status = rpmsg_is_address_set(rdev->bitmap,
					      RPMSG_ADDR_BMP_SIZE, src);
		if (!status) {
			/* Mark the address as used in the address bitmap. */
			rpmsg_set_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE,
					  src);
		} else if (status > 0) {
			status = RPMSG_ERR_ADDR;
			goto ret_status;
		} else {
			goto ret_status;
		}
	} else {
		/* Skip check the address duplication in 0-1023:
		 * 1.Trust the author of predefined service
		 * 2.Simplify the tracking implementation
		 */
	}

	rpmsg_register_endpoint(rdev, ept, name, addr, dest, cb, unbind_cb);
	metal_mutex_release(&rdev->lock);

	/* Send NS announcement to remote processor */
	if (ept->name[0] && rdev->support_ns &&
	    ept->dest_addr == RPMSG_ADDR_ANY){
		status = rpmsg_send_ns_message(ept, RPMSG_NS_CREATE);
		if (status < 0) {
			status = RPMSG_ERR_INIT;
			goto ret_status;
		}

	}

	if (status)
		rpmsg_unregister_endpoint(ept);
	return status;

ret_status:
	metal_mutex_release(&rdev->lock);
	return status;
}

void rpmsg_destroy_ept(struct rpmsg_endpoint *ept)
{
	struct rpmsg_device *rdev;

	if (!ept || !ept->rdev)
		return;

	rdev = ept->rdev;

	if (ept->name[0] && rdev->support_ns &&
	    ept->addr >= RPMSG_RESERVED_ADDRESSES)
		(void)rpmsg_send_ns_message(ept, RPMSG_NS_DESTROY);
	rpmsg_unregister_endpoint(ept);
}
