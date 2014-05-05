/*
 * Tux Droid - Hid interface (only for windows)
 * Copyright (C) 2008 C2ME Sa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _TUX_HID_H_
#define _TUX_HID_H_

#define HID_RW_TIMEOUT                  1000

extern bool tux_hid_capture(int vendor_id, int product_id);
extern void tux_hid_release(void);
extern bool tux_hid_write(int size, const unsigned char *buffer);
extern bool tux_hid_read(int size, unsigned char *buffer);

#endif /* _TUX_HID_H_ */

