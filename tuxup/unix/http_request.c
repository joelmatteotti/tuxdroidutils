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

/* $Id: http_request.c 2008 2008-09-25 09:45:16Z ks156 $ */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "http_request.h"
#include "error.h"

static int send_command(char *action)
{   
    int sock;
    struct sockaddr_in dest;
    char buff[100];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return E_SERVER_CONNECTION;
    }

    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(270); 
    if (inet_addr("localhost") == 0)//, &dest.sin_addr.s_addr) == 0)
    {
        return E_SERVER_CONNECTION;
    }

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0)
    {
        // Server isn't running
        return 0;
    }
    sprintf(buff, "GET %s HTTP/1.0\n\n", action);
    
    send(sock, buff, strlen(buff), 0); 
    close(sock);
    return 0;
}
/**
 * Send a command to stop the driver.
 */
int stop_driver(void)
{
    return send_command("/0/tuxup/stop_driver?");
}

/**
 * Send a command to start the driver.
 */
int start_driver(void)
{
    return send_command("/0/tuxup/start_driver?");
}
