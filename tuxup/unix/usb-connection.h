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

/* $Id: usb-connection.h 1876 2008-09-17 13:17:12Z Paul_R $ */

#ifndef USB_CONNECTION_H
#define USB_CONNECTION_H

#include <usb.h>

/* USB general information */
#define TUX_VENDOR_ID       0x03EB /** USB Manufacturer ID (Vendor Id) */
#define TUX_PRODUCT_ID      0xFF07 /** USB Model Code (Product ID) */

/* USB interfaces */
#define USB_AUDIO_IN        0x01 /** audio input channel interface number */
#define USB_AUDIO_OUT       0x03 /** audio output channel interface number */
#define USB_COMMAND         0x03 /** command channel interface number */
#define USB_W_ENDPOINT      0x05 /** write endpoint of the command interface */
#define USB_W_TIMEOUT       5000 /** timeout in ms of usb_interrupt_write */
#define USB_R_ENDPOINT      0x84 /** read endpoint of the command interface */
#define USB_R_TIMEOUT       5000 /** timeout in ms of usb_interrupt_read */

// #define MSG_MAXL 1000
// #define USB_MSG_IN      USB_TYPE_CLASS
// #define USB_MSG_OUT     USB_TYPE_CLASS | 0x01
// #define TIME_OUT        1600

// #define URB_LENGTH   16

// #define USB_MODE 1
// #define CMD_NA       0
// #define CMD_SWITCH_USB   1
// #define CMD_SWITCH_PSTN  2
// #define CMD_QUIT 3
// #define CMD_RING 4

// #define URB_INIT     0
// #define URB_STATUS       1

// unsigned char URB_CMD[][URB_LENGTH]={
// INIT REGISTER
// { 0x8e,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x68 },
// GET STATUS
// { 0x03,0x02,0x12,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF1 },
// };

/* Prototypes */
usb_dev_handle *usb_open_tux(struct usb_device *dev);
struct usb_device *usb_find_tux();
int usb_check_tux_status(usb_dev_handle * dev_h);
int usb_send_commands(usb_dev_handle * dev_h, uint8_t * send_data, int size);
int usb_get_commands(usb_dev_handle * dev_h, uint8_t * receive_data, int size);

#endif /* USB_CONNECTION_H */
