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

/* $Id: bootloader.c 1876 2008-09-17 13:17:12Z Paul_R $ */

/**
 *
 *   @file   bootloader.c
 *
 *   @brief  Boot loader that sends data to the dongle which forwards it on I2C
 *   to reprogram all CPU's.
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "usb-connection.h"
#include "tux_hid_unix.h"
#include "tux-api.h"
#include "error.h"
#include "log.h"


bool HID;
static float step;
static float progress = 0;
static int hashes;

/* Debug commands */
//#define keybreak()      {puts("Press return to read feedback"); getchar();}
//#define keybreak()      {usleep(5000);}
#define keybreak()      {}
#define PRINT_DATA      0

#define TRUE    1
#define FALSE   0

#define USB_TIMEOUT 5
#define BOOT_INIT_ACK 255 
#define BOOT_EXIT_ACK 254 

/* USB bootloader commands */
#define BOOT_INIT 1
#define BOOT_FILLPAGE 2
#define BOOT_EXIT 3

static bool wait_status(unsigned char value, int timeout);
static void compute_progress_bar(const char *filename);
typedef uint32_t FILE_Addr_t;
typedef uint32_t FILE_SegmentLen_t;
typedef unsigned FILE_LineNum_t;
typedef uint8_t FILE_ParsedLen_t;

static uint8_t counter;

static enum mem_type_t mem_type;

typedef struct
{
    FILE_LineNum_t lineNum;     /* Line number of data record in ASCII file. */
    FILE_Addr_t addr;           /* Address that was parsed */
    FILE_ParsedLen_t dataLen;   /* Number of bytes of data */
    FILE_ParsedLen_t dataAlloc; /* Number of bytes allocated */
    uint8_t *data;              /* Data that was parsed */

} FILE_ParsedData_t;

typedef struct
{
    FILE_LineNum_t lineNum;     /* Tracks line number we're working on */
    FILE_ParsedData_t parsedData;       /* Last hunk of data parsed */
    FILE_Addr_t segAddr;        /* Address of the segment in memory */
    FILE_SegmentLen_t segLen;   /* Length of the segment in memory */
    int inSeg;                  /* Inside a segment */
    FILE_Addr_t currAddr;       /* Current address in memory being processed */
    uint8_t *segmentData;       /* Complete segment data */
    int segmentDataIdx;         /* Index used for filling in the segment data */
    usb_dev_handle *dev_handle; /* USB device handle for sending parsed data */
} Parser_t;

/**
 *   Clears any data stored in the parsed data
 */

static void ClearData(FILE_ParsedData_t * parsedData)
{
    if (parsedData->dataAlloc > 0)
    {
        memset(parsedData->data, 0, parsedData->dataAlloc);
    }

}                               // ClearData

/**
*   Releases any allocated data in the parsed data buffer.
*/

static void FreeData(FILE_ParsedData_t * parsedData)
{
    if (parsedData->dataAlloc > 0)
    {
        free(parsedData->data);
        parsedData->data = NULL;
        parsedData->dataAlloc = 0;
    }

}                               // FreeData

/**
 *   Allocates (or reallocates) the data buffer
 */

static uint8_t *AllocData(FILE_ParsedData_t * parsedData, FILE_ParsedLen_t reqLen)
{
    if (reqLen <= parsedData->dataAlloc)
    {
        return parsedData->data;
    }

    FreeData(parsedData);

    if ((parsedData->data = malloc(reqLen)) != NULL)
    {
        parsedData->dataAlloc = reqLen;

        ClearData(parsedData);
    }

    return parsedData->data;

}                               // AllocData

/**
 * Parses a single nibble from a string containing ASCII Hex characters.
 *
 * @param[in,out] parser  Parser object that we're reporting about
 * @param[in,out] s       Pointer to string; will be advanced.
 * @param[out]    b       Nibble that was parsed
 * @param[int]    label   Error string (used for reporting errors)
 *
 * @return  TRUE, if a nibble was parsed successfully, FALSE otherwise.
 */
static int GetNibble(Parser_t * parser, const char **s, unsigned char *b,
                     const char *label)
{
    char ch = **s;

    *s = *s + 1;

    if ((ch >= '0') && (ch <= '9'))
    {
        *b = ch - '0';
        return TRUE;
    }

    if ((ch >= 'A') && (ch <= 'F'))
    {
        *b = ch - 'A' + 10;
        return TRUE;
    }

    if ((ch >= 'a') && (ch <= 'f'))
    {
        *b = ch - 'a' + 10;
        return TRUE;
    }

    // Error(parser, "parsing %s, expecting hex digit, found '%c'", label, ch);
    return FALSE;

}                               // GetNibble

/**
 * Parses a single byte from a string containing ASCII Hex characters.
 *
 * @param[in,out] parser  Parser object that we're reporting about
 * @param[in,out] s       Pointer to string; will be advanced
 * @param[out]    b       Byte that was parsed
 * @param[in]     label   Error string (used for reporting errors)
 *
 * @return  TRUE, if a byte was parsed successfully, FALSE otherwise.
 */
static int GetByte(Parser_t * parser, const char **s, unsigned char *b,
                   const char *label)
{
    unsigned char b1, b2;

    if (GetNibble(parser, s, &b1, label) && GetNibble(parser, s, &b2, label))
    {
        *b = b1 << 4 | b2;
        return TRUE;
    }

    return FALSE;

}                               // GetByte

/**
 * Parses two bytes from a string containing ASCII Hex characters.
 *
 * @param[in,out] parser  Parser object that we're reporting about
 * @param[in,out] s       Pointer to string; will be advanced
 * @param[out]    b       Word that was parsed
 * @param[in]     label   Error string (used for reporting errors)
 *
 * @return  TRUE, if a byte was parsed successfully, FALSE otherwise.
 */
static int GetWord(Parser_t * parser, const char **s, unsigned short *b,
                   const char *label)
{
    unsigned char b1, b2;

    if (GetByte(parser, s, &b1, label) && GetByte(parser, s, &b2, label))
    {
        *b = (unsigned short)b1 << 8 | b2;
        return TRUE;
    }

    return FALSE;

}                               // GetWord

static int startSegment(Parser_t * parser)
{
    parser->segmentDataIdx = 0;

    /* set segment address */
    parser->segmentData[parser->segmentDataIdx++] =
        (uint8_t) (parser->segAddr >> 8);
    parser->segmentData[parser->segmentDataIdx++] = (uint8_t) parser->segAddr;

    /* fill with '0xFF' if necessary */
    while (parser->currAddr < parser->parsedData.addr)
    {
        parser->segmentData[parser->segmentDataIdx++] = 0xFF;
        parser->currAddr++;
    }

    return TRUE;
}

/**
 * Put one byte of data in the segmentData
 */
static int fillSegment(Parser_t * parser, unsigned char data)
{
    parser->segmentData[parser->segmentDataIdx++] = data;
    parser->currAddr++;
    return TRUE;
}

/**
 * Send the segment to the USB chip for I2C bootloading
 *
 * \todo remove the USB commands from here and put them in their own function
 */
int finishSegment(Parser_t * parser)
{
    int i, idx = 0;
    unsigned char data_buffer[64];
    int ret;

    /* Indicate that we completed a segment */
    parser->inSeg = FALSE;

#if (PRINT_DATA)
    /* XXX debug */
    printf("segment data: \n");
    for (i = 0; i < parser->segLen + 2; i++)
        printf("%02x", parser->segmentData[i]);
    printf("\n");
#endif

    data_buffer[0] = HID_I2C_HEADER;
    data_buffer[1] = BOOT_FILLPAGE;

    /*
     * Send first packet
     */
    for (i = 2; i < 36; i++)
        data_buffer[i] = parser->segmentData[idx++];
    /* EEPROM handling */
    if (mem_type == EEPROM)
    {
        /* try to solve the programming problem with some boards */
        usleep(200000);
         /* set the last bit to 1 to indicate eeprom type to the bootloader */
        data_buffer[2] |= 0x80;
    }
    if (HID)
    {
        ret = tux_hid_write(36, data_buffer);
    }
    else
    {
        ret = usb_send_commands(parser->dev_handle, data_buffer, 36);
    }
#if (PRINT_DATA)
    printf("Status of the first packet sent: %d\n", ret);
#endif
    if (HID)
    {
        if (!ret)
            return FALSE;
    }
    else
    {
        if (ret != 36)
            return FALSE;
    }

    /*
     * Send second packet
     */
    for (i = 2; i < 34; i++)
        data_buffer[i] = parser->segmentData[idx++];
    keybreak();
    if (HID)
    {
        ret = tux_hid_write(34, data_buffer);
    }
    else
    {
        ret = usb_send_commands(parser->dev_handle, data_buffer, 34);
    }
#if (PRINT_DATA)
    printf("Status of the second packet sent: %d\n", ret);
#endif
    if (HID)
    {
        if (!ret)
            return FALSE;
    }
    else
    {
        if (ret != 34)
            return FALSE;
    }

    keybreak();

    /*
     * Bootlader status command and result
     */
    if (HID)
    {
        ret = (wait_status(++counter, USB_TIMEOUT));
        ret = tux_hid_read(5, data_buffer);
    }
    else
    {
        ret = usb_get_commands(parser->dev_handle, data_buffer, 64);
        counter ++;
    }
#if (PRINT_DATA)
    printf("Status of feedback from bootloader: %x\n", ret);
#endif
    if (HID)
    {
        if ((ret) && (data_buffer[0] == 0xF0) && (data_buffer[1] == 0))
        {
            while (counter >= progress && hashes <= 60)
            {
                printf("#");
                progress += step;
                hashes++;
            }
            fflush (stdout);
            sleep(0.05);
            return TRUE;
        }
        else
        {
            log_error("Bootloading failed, program aborted at dongle reply.\n");
            exit(E_TUXUP_BOOTLOADINGFAILED);
        }
    }
    else
    {
        if ((ret == 64) && (data_buffer[0] == 0xF0) && (data_buffer[1] == 0))
        {
            while (counter >= progress && hashes <= 60)
            {
                printf("#");
                progress += step;
                hashes++;
            }
            fflush (stdout);
            return TRUE;
        }
        else
        {
            log_error("\nBootloading failed, program aborted at dongle reply.\n");
            exit(E_TUXUP_BOOTLOADINGFAILED);
        }
    }
}

/**
 * Called when a data line is parsed. This routine will,
 * in turn, call startSegment(), fillSegment(), and finishSegment() as
 * appropriate.
 *
 * @return TRUE if parsing should continue, FALSE if it should stop.
 */
static int ParsedData(Parser_t * parser, int lastSeg)
{
    FILE_ParsedData_t *parsedData = &parser->parsedData;

    /* Last segment, fill with '0xFF', send and return */
    if (lastSeg)
    {
#if (PRINT_DATA)
        printf("--last segment --\n");
#endif
        /* fill with '0' until the end of segment */
        while (parser->currAddr < (parser->segAddr + parser->segLen))
        {
            fillSegment(parser, 0xFF);
        }
        finishSegment(parser);
        return TRUE;
    }

    /* resynchronise currAddr and addr */
    while (parser->currAddr != parsedData->addr)
    {
#if (PRINT_DATA)
        printf("--synchronisation--\n");
#endif
        if (!parser->inSeg)
        {
            /* set current address at the segment start by zeroing lower bits */
            parser->currAddr = parsedData->addr & ~(parser->segLen - 1);
            break;
        }
        fillSegment(parser, 0xFF);

        /* end of segment ? */
        if (parser->currAddr == (parser->segAddr + parser->segLen))
        {
            finishSegment(parser);
        }
    }

    /* loop until all data has been processed */
    int dataIdx = 0;

    while (parser->currAddr < (parsedData->addr + parsedData->dataLen))
    {

#if (PRINT_DATA)
        printf("--loop--");
#endif
        /* start a new segment if needed */
        if (!parser->inSeg)
        {
#if (PRINT_DATA)
            printf("--new segment--\n");
#endif
            parser->inSeg = TRUE;
            /* set address of the segment start */
            parser->segAddr = parser->currAddr;
            startSegment(parser);
        }

        /* store data in segmentData */
        fillSegment(parser, parsedData->data[dataIdx]);
        dataIdx++;

        /* end of segment ? */
        if (parser->currAddr == (parser->segAddr + parser->segLen))
        {
#if (PRINT_DATA)
            printf("--end segment--");
#endif
            finishSegment(parser);
        }
    }

    return TRUE;

}                               // ParsedData

/**
 *   Parses a single line from an Intel Hex file.
 *
 *   The Intel Hex format looks like this:
 *
 *   : BC AAAA TT HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH CC <CR>
 *
 *   Note: Number of H's varies with data ..... this is example only.
 *   Note: Spaces added for clarity
 *
 *   :     Start of record character
 *   BC    Byte count
 *   AAAA  Address to load data.  (tells receiving device where to load)
 *   TT    Record type. (00=Data record, 01=End of file. No data in EOF record)
 *   HH    Each H is one ASCII hex digit.  (2 hex digits=1 byte)
 *   CC    Checksum of all bytes in record (BC+AAAA+TT+HH......HH+CC=0)
 *
 *   See: http://www.xess.com/faq/intelhex.pdf for the complete spec
 */

static int parseIHexLine(Parser_t * parser, const char *line)
{
    int i;
    FILE_ParsedData_t *parsedData = &parser->parsedData;
    uint8_t *data;

    ClearData(parsedData);

    if (line[0] != ':')
    {
        /* In Intel Hex format, lines which don't start with ':' are
           supposed to be ignored */
        return TRUE;
    }

    const char *s = &line[1];
    unsigned char dataLen;

    if (!GetByte(parser, &s, &dataLen, "count"))
    {
        return FALSE;
    }

    data = AllocData(parsedData, dataLen);

    unsigned short addr;

    if (!GetWord(parser, &s, &addr, "addr"))
    {
        return FALSE;
    }

    unsigned char recType;

    if (!GetByte(parser, &s, &recType, "recType"))
    {
        return FALSE;
    }

    unsigned char checksumCalc =
        dataLen + ((addr & 0xFF00) >> 8) + (addr & 0x00FF) + recType;

    for (i = 0; i < dataLen; i++)
    {
        if (!GetByte(parser, &s, &data[i], "data"))
        {
            return FALSE;
        }
        checksumCalc += data[i];
    }

    unsigned char checksumFound;

    if (!GetByte(parser, &s, &checksumFound, "checksum"))
    {
        return FALSE;
    }

    if ((unsigned char)(checksumCalc + checksumFound) != 0)
    {
        // Error(parser, "found checksum 0x%02x, expecting 0x%02x",
        // checksumFound, 0 - checksumCalc);
        return FALSE;
    }

    switch (recType)
    {
    case 0:                    // Data
        {
            parsedData->addr = addr;
            parsedData->dataLen = dataLen;

            ParsedData(parser, FALSE);
            break;
        }

    case 1:                    // EOF
        {
            ParsedData(parser, TRUE);   /* fill the last segment and send it */
            // Flush(parser); /* XXX what to do here? */
            break;
        }

    default:
        {
            // Error(parser, "Unrecognized record type: %d", recType);
            return FALSE;
        }
    }

    return TRUE;

}                               // parseIHexLine

/**
 *   Parses an intel hex file, calling the appropriate callbacks along the way
 */
static int FILE_ParseFile(usb_dev_handle * dev_h, const char *fileName)
{
    FILE *fs = NULL;
    Parser_t parser;
    char line[100];
    int rc = FALSE;
    memset(&parser, 0, sizeof(parser)); /* clear all parser elements */

    /* TODO get this value from either the file or the main program.
       What if we get an ATM168? */
    parser.segLen = 0x40;
    /* XXX this is not a good way to pass the handle up to
       finishSegment, any better idea? */
    parser.dev_handle = dev_h;

    if ((parser.segmentData = malloc(parser.segLen + 2)) == NULL)
    {
        log_error("Unable to allocate segmentData space");
        goto cleanup;
    }

    if ((fs = fopen(fileName, "rt")) == NULL)
    {
        log_error("Unable to open file '%s' for reading", fileName);
        goto cleanup;
    }

    /* Nb of line */
    compute_progress_bar(fileName);

    while (fgets(line, sizeof(line), fs) != NULL)
    {
        parser.lineNum++;
        if (!parseIHexLine(&parser, line))
        {
            goto cleanup;
        }
    }

    /* Everything went successfully */
    rc = TRUE;

  cleanup:

    if (fs != NULL)
    {
        free(parser.segmentData);
        fclose(fs);
    }

    return rc;

}                               // FILE_ParseFile

/**
 *   Bootloads a CPU with the provided hex file
 */
int bootload(usb_dev_handle * dev_h, uint8_t cpu_address, uint8_t mem_t,
             const char *filename)
{
    int rc = FALSE;
    unsigned char data_buffer[64];
    uint8_t page_size = 64;   /* XXX Should depend on CPU type */
    uint8_t packet_total = 2; /* XXX should depend on CPU type */
    int ret;

    
    /* Set global variable mem_type to the memory type */
    mem_type = mem_t;

    /* For *nix system, display the memory type and prepare the progress bar.
     * ex : FLASH   [                                              ]
     */
    /** \todo Find how works the escape sequences on windows */
    if (mem_type == EEPROM)
        printf("EEPROM [\033[s\033[61C]\033[u\033[1B"); 
    else
        printf("FLASH  [\033[s\033[61C]\033[u\033[1B"); 
    /* Bootloader initialization */
    data_buffer[0] = HID_I2C_HEADER;
    data_buffer[1] = BOOT_INIT;
    data_buffer[2] = cpu_address;
    data_buffer[3] = page_size;
    data_buffer[4] = packet_total;
    
    counter = 0;
    progress = 0;
    hashes = 0;

    if (HID)
    {
        sleep(0.5);
        ret = tux_hid_write(5, data_buffer);
        sleep(1);
    }
    else
    { 
        /* Send the command to active the bootloader */
        ret = usb_send_commands(dev_h, data_buffer, 5);
        /* ... and read the status to be ure that it's correctly initialized */
        ret = usb_get_commands(dev_h, data_buffer, 64);
    }
    if (HID)
    {
        if (!wait_status(BOOT_INIT_ACK, USB_TIMEOUT) || !ret)
        {
            log_error("\nInitialization failed\n");
            return FALSE;
        }
    }
    else
    {
#if (PRINT_DATA)
        printf("Boot init status: %x\n", ret);
#endif
        if ((ret != 64) && data_buffer[2] != BOOT_INIT_ACK)
        {
            log_error("\nInitialization failed\n");
            return FALSE;
        }
    }

    /* Bootloader: parse hex file and send data */
    if (FILE_ParseFile(dev_h, filename))
    {
        rc = TRUE;
    }

    /* Exit bootloader */
    data_buffer[0] = HID_I2C_HEADER;
    data_buffer[1] = BOOT_EXIT;
    data_buffer[2] = 0;
    data_buffer[3] = 0;
    data_buffer[4] = 0;

    progress = 0;
    
    if (HID)
    {  
        tux_hid_write(5, data_buffer);
        tux_hid_read(5, data_buffer);
        if (!wait_status(BOOT_EXIT_ACK, USB_TIMEOUT))
        {
            log_error("\nBootloader exit failed \n");
            return FALSE;
        }
    }
    else
    {
        ret = usb_send_commands(dev_h, data_buffer, 5);
        ret = usb_get_commands(dev_h, data_buffer, 64);
    }
    return rc;
}

/**
 * \brief Wait a specific ACK.
 * This function wait for a specific value of the second parameter of the
 * bootloader ACK.
 */
static bool wait_status(unsigned char value, int timeout)
{
    unsigned char data_buffer[64];
    uint8_t sttime = time(NULL);
    uint8_t edtime = 0;
    
    tux_hid_read(64, data_buffer);
    while ((data_buffer[2] != value) || (data_buffer[0] != 0xF0))
    {
        edtime = time(NULL);
        if (difftime(edtime, sttime) > timeout)
        {
            return 0;
        }
        tux_hid_read(64, data_buffer);
        usleep(5000);
    }
    return 1;
}

/**
 * \brief Compute the progress bar depending of the number of line in the file. 
 * One packet use 4 line. So, to know the number of packet to send, just divide
 * the line number per 4.
 * This progress bar display 60 "#"
 * \todo Find a better way to now the number of packet.
 */
static void compute_progress_bar(const char *filename)
{
    FILE *fs = NULL;
    char line[100];
    int char_cnt = 0;

    if ((fs = fopen(filename, "r")) == NULL)
        return;

    while (fgets(line, sizeof(line), fs) != NULL)
        /* 13 bytes are not data to be programmed */
        char_cnt += strlen(line) - 13;
    fclose(fs);

    /* 2 chars per byte, 64 bytes per frame, 60 hashes to print */
    step = (char_cnt / (2 * 64.0 * 60.0));
}
