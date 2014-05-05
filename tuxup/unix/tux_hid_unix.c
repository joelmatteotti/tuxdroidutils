/*
 * Tux Droid - Hid interface (only for unix)
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
 

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/hiddev.h>

#include <string.h>
#include <dirent.h>

#include "tux_hid_unix.h"

static int tux_device_hdl = -1;
static char tux_device_path[256] = "";
static struct hiddev_usage_ref uref_out;
static struct hiddev_report_info rinfo_out;

static bool
find_dongle_from_path(const char *path, int vendor_id, int product_id)
{
    DIR* dir;
    struct dirent *dinfo;
    int fd = -1;
    char device_path[256] = "";
    struct hiddev_devinfo device_info;
    int err;
    
    dir = opendir(path);
    if (dir != NULL)
    {
        while ((dinfo = readdir(dir)) != NULL)
        {
            if (strncmp(dinfo->d_name, "hiddev", 6) == 0)
            {
                sprintf(device_path, "%s/%s", path, dinfo->d_name);

                if ((fd = open(device_path, O_RDONLY)) >= 0)
                {
                    err = ioctl(fd, HIDIOCGDEVINFO, &device_info);
                    if ((device_info.vendor == vendor_id) &&
                        ((device_info.product & 0xFFFF) == product_id))
                    {
                        sprintf(tux_device_path, "%s", device_path);
                        tux_device_hdl = fd;
        
                        closedir(dir);
                        
                        return true;
                    }
                    else
                    {
                        close(fd);
                    }
                }
            }
        }
        
        closedir(dir);
    }

    return false;
}
    
bool LIBLOCAL
tux_hid_capture(int vendor_id, int product_id)
{
    /* Normal path to scan is /dev/usb */
    if (find_dongle_from_path("/dev/usb", vendor_id, product_id))
    {
        return true;
    }
    
    /* Other possible path to scan is /dev/usb */
    if (find_dongle_from_path("/dev", vendor_id, product_id))
    {
        return true;
    }
    
    /* dongle not found */
    return false;
}

void LIBLOCAL
tux_hid_release(void)
{
    if (tux_device_hdl != -1)
    {
        close(tux_device_hdl);
        tux_device_hdl = -1;
    }
}

bool LIBLOCAL
tux_hid_write(int size, const unsigned char *buffer)
{
    int i;
    int err;
    
    rinfo_out.report_type = HID_REPORT_TYPE_OUTPUT;
    rinfo_out.report_id = HID_REPORT_ID_FIRST;
    
    err = ioctl(tux_device_hdl, HIDIOCGREPORTINFO, &rinfo_out);
    if (err < 0)
    {
        return false;
    }
    
    for(i = 0; i < size; i++)
    {
        uref_out.report_type = HID_REPORT_TYPE_OUTPUT;
        uref_out.report_id   = HID_REPORT_ID_FIRST;
        uref_out.usage_index = i;
        uref_out.value = (unsigned char)buffer[i];
        
        err = ioctl(tux_device_hdl,HIDIOCSUSAGE, &uref_out);
        if (err < 0)
        {
            return false;
        }
    }
    
    err = ioctl(tux_device_hdl,HIDIOCSREPORT,&rinfo_out);
    if (err < 0)
    {
        return false;
    }
    
    return true;
}

bool LIBLOCAL
tux_hid_read(int size, unsigned char *buffer)
{
    int i;
    int err;
 
    rinfo_out.report_type = HID_REPORT_TYPE_INPUT;
    rinfo_out.report_id = HID_REPORT_ID_FIRST;
    
    err = ioctl(tux_device_hdl, HIDIOCGREPORT, &rinfo_out);
    if (err < 0)
    {
        return false;
    }

    for (i = 0; i < size; i++)
    {
        uref_out.report_type = HID_REPORT_TYPE_INPUT;
        uref_out.report_id   = HID_REPORT_ID_FIRST;
        uref_out.usage_index = i;
        
        err = ioctl(tux_device_hdl, HIDIOCGUCODE, &uref_out);
        if (err < 0)
        {
            return false;
        }
        
        err = ioctl(tux_device_hdl, HIDIOCGUSAGE, &uref_out); 
        if (err < 0)
        {
            return false;
        }
        buffer[i] = uref_out.value;
    }
    return true;
}

