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

/* $Id: main.c 3559 2009-02-04 15:55:02Z ks156 $ */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "tux-api.h"
#include "bootloader.h"
#include "version.h"
#include "common/defines.h"
#include "error.h"
#include "log.h"
#include "usb-connection.h"
#include "tux_hid_unix.h"
#include "http_request.h"
#define countof(X) ( (size_t) ( sizeof(X)/sizeof*(X) ) )

/* Messages. */
static char const *msg_old_fuxusb =
    "\n       Your dongle firmware is too old to use this version of Tuxup"
    "\n       Please update your dongle firmware with a version of fuxusb.hex"
    "\n       0.5.2 or higher.\n"
    "\n       Check http://wiki.tuxisalive.com for more details\n";

static char const *msg_old_firmware =
    "\n       Your dongle firmware is too old to support switching to bootloader"
    "\nmode automatically.\n"
    "\n  To enter bootloading mode manually, you need to unplug the dongle, press"
    "\n  with a small pin on the button located inside the hole on the belly and,"
    "\n  while keeping it pressed, plug the dongle."
    "\n  The eyes should not turn blue at this time, otherwise try again."
    "\n  Now, reprogram the dongle alone with the command:"
    "\n    'tuxup /opt/tuxdroid/hex/fusxusb.hex'"
    "\n  (You may use another path depending on the hex files"
    " you want to use).\n"
    "\nCheck http://www.tuxisalive.com/documentation/how-to/updating-the-firmware"
    "\nfor more details.\n";

static char const *msg_dfu_programmer_not_installed =
    "\nERROR: dfu-programmer is not installed or is not in your path, tuxup uses"
    "\nit to reprogram the USB cpu so installing dfu-programmer is mandatory.\n";

/* Programming modes. */
enum program_modes_t { NONE, ALL, MAIN, INPUTFILES };

/* The name of this program. */
static char const *program_name = "tuxup";

/* The version defined in version.h */
static char const *program_version = VERSION;

/* Whether to display verbose messages. */
int verbose = 0;

/* Pretend option. */
static int pretend = 0;

/* USB handle */
static struct usb_device *device;
static struct usb_dev_handle *dev_h;
static int usb_connected = 0;          /* Flag for usb connection status */

/*
 * Prints usage information for this program to STREAM (typically
 * stdout or stderr), and exit the program with EXIT_CODE. Does not return.
 */
static void usage(FILE *stream, int exit_code)
{
    fprintf(stream, "%s %s\n", program_name, program_version);
    fprintf(stream, "Usage: %s options [path|file ...]\n", program_name);
    fprintf(stream,
            " -m --main     Reprogram tuxcore and tuxaudio (flash and eeprom)\n"
            "               with hex files located in path.\n"
            " -a --all      Reprogram all cpu's with hex files located in path.\n"
            " -p --pretend  Don't do the programming, just simulate.\n"
            " -h --help     Display this usage information.\n"
            " -v --verbose  Print verbose messages.\n"
            " -d --debug    Print debug messages. \n"
            " -q --quiet    Silent mode. \n"
            " -V --version  Print the version number.\n" "\n"
            "Connection: connect the dongle alone, then press on tux's head\n"
            "  button while switching it on. Finally, connect the white cable\n"
            "  between tux and the dongle.\n" 
            "\n"
            "Notes:\n"
            "  * Options '-a' and -'m' can't be used simultaneously.\n"
            "  * Inputfiles can be specified only if the -a and -m options are\n"
            "    not selected.\n"
            "  * Any .hex or .eep files compiled for Tux Droid can be used.\n"
            "  * The eeprom file names should contain 'tuxcore' or 'tuxaudio'\n"
            "    in order to be identified. The usb hex file should contain\n"
            "    'fuxusb'.\n");
    exit(exit_code);
}

static void retrieve_version(int *ver_minor, int *ver_update)
{
    unsigned char data_buffer[64];
    uint8_t i;
    data_buffer[0] = DONGLE_CMD_HDR;
    data_buffer[1] = INFO_FUXUSB;
    
    if (HID)
    {
        tux_hid_write(64, data_buffer);
        sleep(1);
        tux_hid_read(64, data_buffer);
    }
    else
    {
        usb_send_commands(dev_h, data_buffer, 64);
        sleep(1);
        usb_get_commands(dev_h, data_buffer, 64);
    }
    for (i = 0; i < 64; i++) 
    {
        /* Parse the frame header */
        if (!((i) % 4))
        {
            if (data_buffer[i] == FUXUSB_VERSION_CMD)
            {
                *ver_minor = data_buffer[i+2];
                *ver_update = data_buffer[i+3];
                break;
            }
        }
    }
}

static int verify_version(void)
{
    int ver_minor=0, ver_update=0;
    uint8_t i;

    for (i = 0; i < 3; i++)
    {
        retrieve_version(&ver_minor, &ver_update);
        if (ver_minor != 0)
            break;
    }
    
    if (ver_minor <= MIN_VER_MINOR)
       if (ver_update < MIN_VER_UPDATE)
           return 1;

    return 0;
}


static void fux_connect(void)
{
    int wait = 5;
    if (usb_connected)
        return;

    /* First, try to found a HID device */
    if (!(tux_hid_capture(TUX_VENDOR_ID, TUX_PRODUCT_ID))) 
    {
        /* Unable to capture the device, try with the libusb */
        for (;;)
        {
            device = usb_find_tux();
            if (device != NULL || wait == 0)
                break;

            sleep(1);
            wait--;
        }

        if (device == NULL)
        {
            log_error("The dongle was not found, now exiting.\n");
            exit(E_TUXUP_DONGLENOTFOUND);
        }
        else
        {
            log_info("Libusb device");
            /* The dongle is a libusb device */
            HID = 0;
        }
    }
    else
    {
        log_info("HID device");
        /* The dongle is a HID device */
        HID = 1;

 
    }
    
    /* Verify if tuxhttpserver.pid exists. */
    if (stop_driver() > 0)
    {
        exit(E_SERVER_CONNECTION);
    }

    if (verbose)
        printf("The dongle was found.\n");

    /* Check if we have the old firmware that requires entering
     * bootloader mode manually, exits with a message that explains what
     * to do in such a case. */
    if (!HID)
    {
        if (device->descriptor.bcdDevice < 0x030)
        {
            log_error(msg_old_firmware);
            exit(E_TUXUP_DONGLEMANUALBOOTLOAD);
        }
    
        /* open USB device */
        if ((dev_h = usb_open_tux(device)) == NULL)
        {
            log_error("USB DEVICE INIT ERROR \n");
            exit(E_TUXUP_USBERROR);
        }
    }
    log_info("Interface configured \n");
    usb_connected = 1;
}


static void fux_disconnect(void)
{
    if (!usb_connected)
        return;
    log_info("Closing interface ...\n");
    if (HID)
    {
        tux_hid_release();
    }
    else
    {
        usb_release_interface(dev_h, USB_COMMAND);
        usb_close(dev_h);
    }
    log_info("     ... interface closed \n");
    usb_connected = 0;
}

static int hex_to_i(char *hex_nr)
{
    int nr;

    sscanf(hex_nr, "%x", &nr);
    return nr;
}

/*
 * Check if the hex file is valid and return the CPU number for which it has
 * been compiled. Fills the version structure with the information extracted
 * from the hex file and returns 0 if the hex file has a cpu and version
 * numbers, 1 otherwise.
 */
static int check_hex_file(char const *filename, version_bf_t * version)
{
    FILE *fs = NULL;
    char word[80];
    char hex_nr[3];

    hex_nr[2] = (char)'\0';
    if ((fs = fopen(filename, "r")) == NULL)
    {
        log_error("Unable to open file '%s' for reading\n", filename);
        exit(E_TUXUP_BADPROGFILE);
    }

    while (fscanf(fs, " %s", word) != EOF)
    {
        /* look for the address 0EF0 (RF) or 1EF0 (tuxcore and tuxaudio)
         * or just :0C (USB) and the C8 version command */
        if (!strncmp(word, ":0C0EF000C8", 11)
            || !strncmp(word, ":0C1DF000C8", 11)
            || (!strncmp(word, ":0C", 3) && !strncmp(word+7, "00C804", 6)))
        {
            /* get cpu number and version number */
            strncpy(hex_nr, &word[11], 2);
            version->cpu_nbr = (hex_to_i(hex_nr) & 0x7);        /* 3 lower bits */
            version->ver_major = (hex_to_i(hex_nr) >> 3);       /* 5 higher bits */
            strncpy(hex_nr, &word[13], 2);
            version->ver_minor = hex_to_i(hex_nr);      /* 5 higher bits */
            strncpy(hex_nr, &word[15], 2);
            version->ver_update = hex_to_i(hex_nr);     /* 5 higher bits */
            return 0;
        }
    }
    return 1;
}

static int prog_flash(char const *filename)
{
    version_bf_t version;
    uint8_t cpu_i2c_addr;
    int ret;

    /* Connect the dongle. */
    fux_connect();

    ret = verify_version();

    if (ret)
    {
        log_error(msg_old_fuxusb);
        exit(E_FUXUSB_VER_ERROR);
    }

    if (check_hex_file(filename, &version))
    {
        log_error("Programming of '%s' failed, this file is not a " \
               " hex file for that CPU\n\n", filename);
        return E_TUXUP_BADPROGFILE;
    }
    if (version.cpu_nbr == TUXCORE_CPU_NUM)
    {
        log_notice("\nProgramming %s in the tuxcore CPU", filename);
        cpu_i2c_addr = TUXCORE_BL_ADDR;
    }
    else if (version.cpu_nbr == TUXAUDIO_CPU_NUM)
    {
        log_notice("\nProgramming %s in the tuxaudio CPU", filename);
        cpu_i2c_addr = TUXAUDIO_BL_ADDR;
    }
    else if (version.cpu_nbr == TUXRF_CPU_NUM)
    {
        log_notice("\nProgramming %s in the tuxrf CPU", filename);
        cpu_i2c_addr = TUXRF_BL_ADDR;
    }
    else if (version.cpu_nbr == FUXRF_CPU_NUM)
    {
        log_notice("\nProgramming %s in the fuxrf CPU", filename);
        cpu_i2c_addr = FUXRF_BL_ADDR;
    }
    else
    {
        log_error("Unrecognized CPU number, %s doesn't appear to be compiled"
               " for a CPU of tuxdroid.\n", filename);
        return E_TUXUP_BADPROGFILE;
    }
    log_notice("Version %d.%d.%d\n", version.ver_major, version.ver_minor,
           version.ver_update);

    if (pretend)
        return E_TUXUP_NOERROR;
    if (bootload(dev_h, cpu_i2c_addr, FLASH, filename))
    { 
       printf("\033[2C[ \033[01;32mOK\033[00m ]\n");
       return E_TUXUP_NOERROR;
    }
    else
    {
        log_notice("\033[2C[\033[01;31mFAIL\033[00m]\n");
        return E_TUXUP_PROGRAMMINGFAILED;
    }
}

static int prog_eeprom(uint8_t cpu_nbr, char const *filename)
{
    uint8_t cpu_i2c_addr;
    int ret;

    /* Connect the dongle. */
    fux_connect();

    ret = verify_version();

    if (ret)
    {
        log_error(msg_old_fuxusb);
        exit(E_FUXUSB_VER_ERROR);
    }

    if (cpu_nbr == TUXCORE_CPU_NUM)
    {
        log_notice("Programming %s in tuxcore CPU\n", filename);
        cpu_i2c_addr = TUXCORE_BL_ADDR;
    }
    else if (cpu_nbr == TUXAUDIO_CPU_NUM)
    {
        log_notice("Programming %s in tuxaudio CPU\n", filename);
        cpu_i2c_addr = TUXAUDIO_BL_ADDR;
    }
    else
    {
        log_error("Wrong CPU number specified for the eeprom.\n");
        return E_TUXUP_BADPROGFILE;
    }

    if (pretend)
        return E_TUXUP_NOERROR;
    if (bootload(dev_h, cpu_i2c_addr, EEPROM, filename))
    {
        printf("\033[2C[ \033[01;32mOK\033[00m ]\n");
        return E_TUXUP_NOERROR;
    }
    else
    {
        log_notice("\033[2C[\033[01;31mFAIL\033[00m]\n");
        return E_TUXUP_PROGRAMMINGFAILED;
    }
}

static int prog_usb(char const *filename)
{
#define QUIET_CMD "1>/dev/null 2>&1"
    /* XXX include those as defines in commands.h */
    unsigned char send_data[5] = { 0x01, 0x01, 0x00, 0x00, 0xFF };
    char command_str[PATH_MAX];
    int ret;
    version_bf_t version;

    log_notice("Programming %s in the USB CPU\n", filename);

    /* Retrieving version number */
    if (check_hex_file(filename, &version))
    {
        log_error("Programming of '%s' failed, this file is not a correct"
               " hex file for that CPU\n\n", filename);
        return E_TUXUP_BADPROGFILE;
    }

    if (version.cpu_nbr != FUXUSB_CPU_NUM)
    {
        log_error("Unrecognized CPU number, %s doesn't appear to be compiled"
               " for a CPU of tuxdroid.\n", filename);
        return E_TUXUP_BADPROGFILE;
    }
    log_notice("Version %d.%d.%d\n", version.ver_major, version.ver_minor,
           version.ver_update);

    if (pretend)
        return E_TUXUP_NOERROR;

    /* Check if the dongle is already in bootloader mode */
    log_info("Testing if the dongle is already in bootloader mode. "\
             "If it is not, 'no device present' will be reported.");
    ret = system("dfu-programmer at89c5130 get bootloader-version 2> /dev/null ");
    /* If dfu-programmer didn't detect the dongle, we try to set it in
       bootloader mode with the specific command */
    if (ret)
    {
        /* Check if dfu-programmer is installed */
        /* XXX a better way to code that error, a define? */
        if (ret == 32512)
        {
            log_error(msg_dfu_programmer_not_installed);
            exit(E_TUXUP_DFUPROGNOTFOUND);
        }
           
        log_info("The dongle was not detected in bootloader mode, "
                 "now \ntrying to set it with a command.\n");
        fux_connect();
        /* Enter bootloader mode. */
        if (HID)
        {
            tux_hid_write(5, send_data);
            /* Windows needs more time than linux to enumerate the new dfu
             * device */
            sleep(5);
        }
        else
        {
            ret = usb_send_commands(dev_h, send_data, 5);
            if (ret == 5)
            {
                sleep(1);
                log_info("Switched to bootloader mode.\n");
                sleep(2);
            }

            else
            {
                log_error("Switching to bootloader mode failed.\n");
                return E_TUXUP_BOOTLOADINGFAILED;
            }
        }
    }
    else
    {
        log_info("Dongle in bootloader mode");
    }

    bool quiet = true;
    if (log_get_level() <= LOG_LEVEL_INFO)
        quiet = false;

    sprintf(command_str, "dfu-programmer at89c5130 erase %s", quiet ?
            QUIET_CMD : "");
    log_info(command_str);
    ret = system(command_str);

    if (ret)
    {
        log_error("Erasing the USB CPU failed.\n");
        log_notice("\033[2C[\033[01;31mFAIL\033[00m]\n");
        return E_TUXUP_PROGRAMMINGFAILED;
    }
    sprintf(command_str, "dfu-programmer at89c5130 flash %s %s", filename, quiet ?
            QUIET_CMD : "");
    log_info(command_str);
    ret = system(command_str);
    if (ret)
    {
        log_error("Flashing the USB CPU failed.\n");
        log_notice("\033[2C[\033[01;31mFAIL\033[00m]\n");
        return E_TUXUP_PROGRAMMINGFAILED;
    }
    sprintf(command_str, "dfu-programmer at89c5130 configure HSB 0x7b %s", quiet ?
                 QUIET_CMD : "");
    log_info(command_str);
    ret = system(command_str);
    if (ret)
    {
        log_error("Configuring the USB CPU failed.");
        log_notice("\033[2C[\033[01;31mFAIL\033[00m]\n");
        return E_TUXUP_PROGRAMMINGFAILED;
    }

    sprintf(command_str, "dfu-programmer at89c5130 start %s", quiet ? QUIET_CMD
            : "");
    log_info(command_str);
    ret = system(command_str);
    log_notice("\033[2C[ \033[01;32mOK\033[00m ]\n");

    fux_disconnect();
    /* Windows needs more time than linux to enumerate the device */
    /*sleep(5);*/
    /*fux_connect();*/
    return E_TUXUP_NOERROR;
}

/*
 * Programming function. Depending on the name, the flash or eeprom
 * programming will be selected. In case of eeprom, the correct CPU
 * will be choosen from the filename.
 */
static int program(char const *filename, char const *path)
{
    int ret = E_TUXUP_BADPROGFILE;
    size_t len;
    char *extension, filenamepath[PATH_MAX];

    if (path)
    {
        /* Checking path */
        len = strlen(path);
        strcpy(filenamepath, path);
        /* Append '/' at the end if not specified. */
        if (path[len - 1] != '/')
            strcat(filenamepath, "/");
        strcat(filenamepath, filename);
        filename = filenamepath;
    }
    log_info("Processing: %s\n", filename);

    extension = strrchr(filename, '.');
    /* check that an extension has been given */
    if (extension)
    {
        /* Check which program function to start. */
        if (!strcmp(extension, ".hex"))
        {
            int i, usb = 0;

            /* Check for 'fuxusb' hex file. */
            len = strlen(filename);
            for (i = 0; i < len - 7; i++)
            {
                if (!strncmp(filename + i, "fuxusb", 6))
                {
                    ret = prog_usb(filename);
                    usb = 1;
                    break;
                }
            }
            if (!usb)           /* AVR hex file. */
            {
                ret = prog_flash(filename);
            }
        }
        else if (!strcmp(extension, ".eep"))
        {
            int i;

            len = strlen(filename);
            for (i = 0; i < len - 7; i++)
            {
                if (!strncmp(filename + i, "tuxcore", 7))
                {
                    ret = prog_eeprom(TUXCORE_CPU_NUM, filename);
                    break;
                }
                else if (!strncmp(filename + i, "tuxaudio", 7))
                {
                    ret = prog_eeprom(TUXAUDIO_CPU_NUM, filename);
                    break;
                }
            }
        }
    }
    if (ret == E_TUXUP_BADPROGFILE)
        log_error("%s is not a valid programming file.\n", filename);
    return ret;
}

/*
 * Main application
 */
int main(int argc, char *argv[])
{
    char path[PATH_MAX];
    enum program_modes_t program_mode = NONE;
    time_t start_time, end_time;
    int ret = E_TUXUP_NOERROR;

    int next_option;

    /* A string listing valid short options letters.  */
    char const *const short_options = "maqphvdV";

    /* An array describing valid long options. */
    const struct option long_options[] = {
        {"main",    0, NULL, 'm'},
        {"all",     0, NULL, 'a'},
        {"quiet",   0, NULL, 'q'},
        {"pretend", 0, NULL, 'p'},
        {"help",    0, NULL, 'h'},
        {"verbose", 0, NULL, 'v'},
        {"debug",   0, NULL, 'd'},
        {"version", 0, NULL, 'V'},
        {NULL,      0, NULL, 0}      /* Required at end of array.  */
    };

    /* Remember the name of the program, to incorporate in messages.
     * The name is stored in argv[0]. */
    program_name = argv[0];

    /* Save the start time to measure the programming time */
    start_time = time(NULL);

    /* Flags to later select the correct log level */
    bool quiet = false, verbose = false, debug = false;


    do
    {
        next_option =
            getopt_long(argc, argv, short_options, long_options, NULL);
        switch (next_option)
        {
        case 'h':              /* -h or --help */
            /* User has requested usage information. Print it to standard
             * output, and exit with exit code zero (normal termination). */
            usage(stdout, E_TUXUP_NOERROR);
        case 'm':              /* -m or --main */
            if (program_mode == NONE)
                program_mode = MAIN;
            else
            {
                log_error("'-a' and '-m' options can't be used "\
                          "simultaneoulsy.");
                usage(stderr, E_TUXUP_USAGE);
            }
            break;
        case 'a':              /* -a or --all */
            if (program_mode == NONE)
                program_mode = ALL;
            else
            {
                log_error("'-a' and '-m' options can't be used "\
                          "simultaneoulsy.");
                usage(stderr, E_TUXUP_USAGE);
            }
            break;
        case 'q':              /* -q or --quiet */
            /* Prepare for logging */
            quiet = true;
            break;
        case 'p':              /* -a or --all */
            pretend = 1;
            break;
        case 'v':              /* -v or  --verbose */
            verbose = true;
            break;
            case 'd':              /* -d or  --debug */
            debug = true;
            break;
        case 'V':              /* -V or  --version */
            fprintf(stdout, "%s %s, an uploader program for tuxdroid.\n\n",
                    program_name, program_version);
            exit(E_TUXUP_NOERROR); /* version is already printed, so just exit. */
            break;
        case '?':              /* The user specified an invalid option. */
            /* Print usage information to standard error, and exit with exit
             * code one (indicating abnormal termination). */
            usage(stderr, E_TUXUP_NOERROR);
        case -1:               /* Done with options.  */
            break;
        default:               /* Something else: unexpected.  */
            abort();
        }
    }
    while (next_option != -1);
    /* Done with options. OPTIND points to first nonoption argument. */

    /* Set log level */
    if (quiet)
        log_set_level(LOG_LEVEL_NONE);
    else if (debug)
        log_set_level(LOG_LEVEL_DEBUG);
    else if (verbose)
        log_set_level(LOG_LEVEL_INFO);

    /* Print program name and version at program start */
    log_info("%s %s, an uploader program for tuxdroid.",
             program_name, program_version);

    /* If no program mode has been selected, choose INPUTFILES. */
    if (program_mode == NONE)
        program_mode = INPUTFILES;
    /* Check that if we have inputfiles, the correct program mode is selected */
    if (optind < argc)          /* Input files have been given. */
    {
        if (program_mode != INPUTFILES)
        {
            if (argc == optind + 1)
                strcpy(path, argv[optind]);
            else
            {
                log_error("Too many files or paths specified.");
                usage(stderr, E_TUXUP_USAGE);
            }
        }
    }
    else
    {
        log_error("No file nor path specified.");
        usage(stderr, E_TUXUP_USAGE);
    }

    /* Select which files to program */
    switch (program_mode)
    {
    case INPUTFILES:
        {
            int i;
            for (i = optind; i < argc && !ret ; ++i)
                ret = program(argv[i], NULL);
        }
        break;
    case MAIN:
        {
            char const *s[]={"tuxcore.hex", "tuxcore.eep", "tuxaudio.hex",
                "tuxaudio.eep"};
            char const **p;
            for (p = s; p < &s[countof(s)] && !ret ; p++)
                ret = program(*p, path);
        }
        break;
    case ALL:
        {
            char const *s[]={"fuxusb.hex", "tuxcore.hex", "tuxcore.eep",
                "tuxaudio.hex", "tuxaudio.eep", "fuxrf.hex", "tuxrf.hex"};
            char const **p;
            for (p = s; p < &s[countof(s)] && !ret ; p++)
                ret = program(*p, path);
        }
        break;
    default:
        abort();
    }

    fux_disconnect();
   
    if (start_driver() > 0)
    {
        exit(E_SERVER_CONNECTION); 
    }

    /* Print time elapsed for programming. */
    end_time = time(NULL);
    if (!pretend)
        log_notice("Time elapsed: %2.0f seconds.", difftime(end_time, start_time));
    return ret;
}
