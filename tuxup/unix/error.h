/*
 * Tuxup - Error
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

#ifndef _ERROR_H_
#define _ERROR_H_

typedef enum
{
    E_TUXUP_NOERROR = 0,
    E_TUXUP_USAGE = 2,
    E_TUXUP_DONGLENOTFOUND = 3,
    E_TUXUP_DONGLEMANUALBOOTLOAD = 4,
    E_TUXUP_BADPROGFILE = 5,
    E_TUXUP_BOOTLOADINGFAILED = 6,
    E_TUXUP_USBERROR = 7,
    E_TUXUP_DFUPROGNOTFOUND = 8,
    E_TUXUP_PROGRAMMINGFAILED = 9,
    E_FUXUSB_VER_ERROR = 10,
    E_SERVER_CONNECTION = 11
} tuxup_error_t;

#endif /* _ERROR_ */
