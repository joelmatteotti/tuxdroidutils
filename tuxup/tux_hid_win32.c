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
 
#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <dbt.h>
#include <stdbool.h>
#include <stdio.h>

#include "tux_hid_win32.h"
#include "tux_misc.h"

static char device_symbolic_name[256] = "";
static HANDLE tux_device_hdl = NULL;
static COMMTIMEOUTS timeout;
/**
 * \brief Open the device.
 * \param vendor_id The vendor ID constant of the device.
 * \param product_id The product ID of the device.
 * This function search for a specified device and try to open the
 * communication.
 * \return True if the device has been correctly opened.
 */
bool LIBLOCAL
tux_hid_capture(int vendor_id, int product_id)
{
    GUID hid_guid;
    HANDLE h_dev_info;
    SP_DEVICE_INTERFACE_DATA dev_info_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detail_data = NULL;
    int member_index = 0;
    bool last_device = false;
    long result;
    unsigned long length = 0;
    ULONG required;
    HANDLE device_hdl = NULL;
    HIDD_ATTRIBUTES attributes;
    bool tux_found = false;;
    
    if (tux_device_hdl != NULL)
    {
        return false;
    }
    HidD_GetHidGuid(&hid_guid);
    
    h_dev_info = SetupDiGetClassDevs(&hid_guid, \
        NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    
    dev_info_data.cbSize = sizeof(dev_info_data);
    
    member_index = 0;
    
    do 
    {
        result = SetupDiEnumDeviceInterfaces(h_dev_info, 0,
            &hid_guid, member_index, &dev_info_data);
            
        if (result != 0)
        {
            result = SetupDiGetDeviceInterfaceDetail(h_dev_info,
                &dev_info_data, NULL, 0, &length, NULL);
                
            detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(length);
            detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            
            result = SetupDiGetDeviceInterfaceDetail(h_dev_info, 
				&dev_info_data, detail_data, length, &required, NULL);
                
            device_hdl = CreateFile(detail_data->DevicePath, 0, 
				FILE_SHARE_READ|FILE_SHARE_WRITE, (LPSECURITY_ATTRIBUTES)NULL,
				OPEN_EXISTING, 0, NULL);
                
            attributes.Size = sizeof(attributes);
			result = HidD_GetAttributes(device_hdl, &attributes);
            
            if ((attributes.VendorID == vendor_id) && 
                (attributes.ProductID == product_id))
            {
                printf(device_symbolic_name, "%s", detail_data->DevicePath);

                CloseHandle(device_hdl);
                
                tux_device_hdl = CreateFile(detail_data->DevicePath, 
                    GENERIC_WRITE|GENERIC_READ, 
                    FILE_SHARE_READ|FILE_SHARE_WRITE, 
                    (LPSECURITY_ATTRIBUTES)NULL,
                    OPEN_EXISTING, 0, NULL);
                    
                timeout.ReadTotalTimeoutConstant = HID_RW_TIMEOUT;
                timeout.WriteTotalTimeoutConstant = HID_RW_TIMEOUT;
                SetCommTimeouts(tux_device_hdl, &timeout);
				
				tux_found = true;
				
                break;
            }
            
            CloseHandle(device_hdl);
            free(detail_data);
        }
        else
        {
            last_device = true;
        }
        
        member_index++;
        
    } 
    while (last_device == false);
    
    if (tux_found)
    {
        return true;
    }
    else
    {
        return false;
    }
}
/**
 * \brief Release the actual device.
 * This function close the device.
 */
void LIBLOCAL
tux_hid_release(void)
{
    if (tux_device_hdl != NULL)
    {
        CloseHandle(tux_device_hdl);
        tux_device_hdl = NULL;
    }
}

/**
 * \brief Write a frame on the USB bus.
 * \param size The number of byte to write.
 * \param buffer A pointer to a buffer containing the data.
 * \return True if no error has been detected.
 * Write a frame buffer on the USB device.
 * For Tux, we don't use the report ID, so it's always null.
 */
bool LIBLOCAL
tux_hid_write(int size, const unsigned char *buffer)
{
    long wrt_count;
    char report[65] = { [0 ... 64] = 0 };
    long result;
    
    if (tux_device_hdl == NULL)
    {
        return false;
    }
    
    report[0] = 0;
    memcpy(&report[1], buffer, size);
    
    result = WriteFile(tux_device_hdl, report, 65, &wrt_count, NULL);

    if (!result)
    {
        return false;
    }
    else
    {
        return true;
    }
}

/**
 * \brief Read a byte on the USB bus.
 * \param size The number of data to read.
 * \param buffer Pointer to a buffer to write received data.
 * \return True if no error has been detected.
 * This function read 'size' bytes on the USB device. 
 * For Tux, we don't use the report ID, so the first byte of the received frame
 * is always ignored.
 * /!\ The report buffer size must be the same than the report of the device. 
 */
bool LIBLOCAL
tux_hid_read(int size, unsigned char *buffer)
{
    long rd_count;
    char report[65];
    long result;
    
    if (tux_device_hdl == NULL)
    {
        return false;
    }

    result = ReadFile(tux_device_hdl, report, 65, &rd_count, NULL);
    memcpy(buffer, &report[1], size);
    
    if (!result)
    {
        return false;
    }
    else
    {
        return true;
    }
}
