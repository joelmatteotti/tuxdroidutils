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

/* $Id: tux-api.h 1854 2008-09-16 15:10:59Z Paul_R $ */

#ifndef TUX_API_H
#define TUX_API_H

/**
 * USB command types
 * This is the first byte of the USB packet
 */
#define LIBUSB_RF_HEADER        0
#define DONGLE_CMD_HDR          1
#define HID_I2C_HEADER          3
#define INFO_FUXUSB             6

#define FUXUSB_VERSION_CMD      200
#define MIN_VER_MINOR           5
#define MIN_VER_UPDATE          2
enum mem_type_t
{ FLASH, EEPROM };

#endif /* TUX_API_H */
