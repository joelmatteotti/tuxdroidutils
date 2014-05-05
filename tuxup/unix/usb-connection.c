/*
 * TUXUP - Firmware uploader for tuxdroid
 * Copyright (C) 2007 C2ME S.A. <tuxdroid@c2me.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* $Id: usb-connection.c 1876 2008-09-17 13:17:12Z Paul_R $ */

#include <stdint.h>
#include <stdio.h>
#include <usb.h>                /* libusb header */
#include <syslog.h>

#include "usb-connection.h"
#include "log.h"

#define PRINT_DATA 0

/**
 * \defgroup USB USB handling
 * \ingroup USB
 * \brief Internals of USB functions for communicating with the dongle
 *
 * @{
 */

/**
 * \brief Scan all USB busses to find Tux device
 * \return USB device of Tux
 */
struct usb_device *usb_find_tux()
{
    struct usb_bus *bus;
    struct usb_device *device;

    usb_init();
    usb_find_busses();
    usb_find_devices();

    for (bus = usb_busses; bus; bus = bus->next)
        for (device = bus->devices; device; device = device->next)
            if (device->descriptor.idVendor == TUX_VENDOR_ID
                && device->descriptor.idProduct == TUX_PRODUCT_ID)
                return device;

    return NULL;
}

/**
 * \brief Open USB interface
 * \param dev USB device
 * \return dev_h USB device handle
 */
usb_dev_handle *usb_open_tux(struct usb_device * dev)
{
    int err;
    usb_dev_handle *dev_h;

    if (!(dev_h = usb_open(dev)))
        return NULL;

    log_info("Device opened");

    err = usb_claim_interface(dev_h, USB_COMMAND);
    if (err != 0)
    {
        usb_detach_kernel_driver_np(dev_h, USB_COMMAND);
        err = usb_claim_interface(dev_h, USB_COMMAND);
        if (err != 0)
        {
            log_error("Claim interface failed: %s\n", usb_strerror());
            usb_close(dev_h);
            return NULL;
        }
    }

    log_info("USB interface claimed successfully \n");

    return dev_h;
}

/**
 * \brief Close Tux interface
 * \param dev_h USB device handle
 *
 * Releases and close the interface
 */
void usb_close_tux(usb_dev_handle * dev_h)
{
    usb_release_interface(dev_h, USB_COMMAND);
    usb_close(dev_h);
}

/**
 * \brief Send commands to Tux's dongle
 *
 * \param dev_h USB device handle
 * \return USB communication status
 *
 * \todo Need to define the format of the data array to send and pass
 *       it as parameter
 * \todo Need to add some doc or links about what this status is really
 */
int usb_send_commands(usb_dev_handle * dev_h, uint8_t * send_data, int size)
{
    int status;

    status = usb_interrupt_write(dev_h, USB_W_ENDPOINT, (char *)send_data, size,
                                 USB_W_TIMEOUT);
    if (status < 0)
    {
        /* error on usb_interrupt_write() */
        log_error("usb_interrupt_write error: status = %d :: %s \n",
                status, usb_strerror());
    }
    else
    {
        /* success */
#if (PRINT_DATA)
        printf("usb_interrupt_write: status =%d"
               " -> TX Buffer[%d]={%2X, %2X, %2X, %2X, %2X}\n",
               status, status, send_data[0], send_data[1], send_data[2],
               send_data[3], send_data[4]);
#endif
    }
    return status;
}

/**
 * \brief Get commands from Tux's dongle
 *
 * \param dev_h USB device handle
 * \return USB communication status
 *
 * \todo Need to define the format of the data array to send and pass
 *       it as parameter
 * \todo Need to add some doc or links about what this status is really
 */
int usb_get_commands(usb_dev_handle * dev_h, uint8_t * receive_data, int size)
{
    int status;

    status = usb_interrupt_read(dev_h, USB_R_ENDPOINT, (char *)receive_data,
                                size, USB_R_TIMEOUT);
    if (status < 0)
    {
        log_error("usb_interrupt_read error: status = %d :: %s \n",
                status, usb_strerror());
    }
    else
    {
        /* XXX this is only to test and display what has been received */
#if (PRINT_DATA)
        printf("usb_interrupt_read: status = %d"
               "-> TX Buffer[%d]={%hX, %hX, %hX, %hX, %hX}\n",
               status, status, receive_data[0], receive_data[1], receive_data[2],
               receive_data[3], receive_data[4]);
#endif
    }
    return status;
}

          /** @} *//* end of USB group */
