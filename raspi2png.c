//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2014 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#define _GNU_SOURCE

#include <getopt.h>
#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include <time.h>
#include <sys/stat.h>
#include "bcm_host.h"

//-----------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

//-----------------------------------------------------------------------

#define VERSION "1.1"
#define DEFAULT_DELAY 0
#define DEFAULT_DISPLAY_NUMBER 0
#define DEFAULT_PNG_COMPRESSION 2
#define DEFAULT_NAME_START "snapshot"
#define DEFAULT_NAME DEFAULT_NAME_START ".png"
#define MAX_NAME_SIZE 50


//-----------------------------------------------------------------------

const char* program = NULL;

//-----------------------------------------------------------------------

void
usage(void)
{
    fprintf(stderr, "%s (%s)\n", program, VERSION);
    fprintf(stderr, "Utility to take a snapshot of the raspberry pi screen and save it as a PNG file\n\n");

    fprintf(stderr, "Usage: %s [--pngname name]", program);
    fprintf(stderr, " [--width <width>] [--height <height>]");
    fprintf(stderr, " [--compression <level>]");
    fprintf(stderr, " [--delay <delay>] [--display <number>]");
    fprintf(stderr, " [--stdout] [--savepath path]");
    fprintf(stderr, " [--version] [--help]\n");

    fprintf(stderr, "\n");

    fprintf(stderr, "    --pngname,-p - name of png file to create ");
    fprintf(stderr, "(default is %s)\n", DEFAULT_NAME);

    fprintf(stderr, "    --height,-h - image height ");
    fprintf(stderr, "(default is screen height)\n");

    fprintf(stderr, "    --width,-w - image width ");
    fprintf(stderr, "(default is screen width)\n");

    fprintf(stderr, "    --compression,-c - PNG compression level ");
    fprintf(stderr, "(0 - 9)\n");

    fprintf(stderr, "    --delay,-d - delay in seconds ");
    fprintf(stderr, "(default %d)\n", DEFAULT_DELAY);

    fprintf(stderr, "    --display,-D - Raspberry Pi display number ");
    fprintf(stderr, "(default %d)\n", DEFAULT_DISPLAY_NUMBER);

    fprintf(stderr, "    --stdout,-s - write file to stdout\n");

    fprintf(stderr, "    --savepath,-a - save path\n");

    fprintf(stderr, "    --version,-v - print version number\n");

    fprintf(stderr, "    --help,-H - print this usage information\n");

    fprintf(stderr, "\n");
}

bool file_exists(char *filename) {
  struct stat FileStat;
  return (stat(filename, &FileStat) == 0);
}
//-----------------------------------------------------------------------

int
main(
    int argc,
    char *argv[])
{
    int opt = 0;

    bool writeToStdout = false;
    char pngNameWithTimestamp[MAX_NAME_SIZE+1];
    char *pngName = DEFAULT_NAME;
    bool replace = false;
    int32_t requestedWidth = 0;
    int32_t requestedHeight = 0;
    uint32_t displayNumber = DEFAULT_DISPLAY_NUMBER;
    int compression = DEFAULT_PNG_COMPRESSION;
    int delay = DEFAULT_DELAY;

    VC_IMAGE_TYPE_T imageType = VC_IMAGE_RGBA32;
    int8_t dmxBytesPerPixel  = 4;

    int result = 0;

    program = basename(argv[0]);

    //-------------------------------------------------------------------

    char *sopts = "c:d:D:Hh:p:w:s:a:v";

    struct option lopts[] =
    {
        { "compression", required_argument, NULL, 'c' },
        { "delay", required_argument, NULL, 'd' },
        { "display", required_argument, NULL, 'D' },
        { "height", required_argument, NULL, 'h' },
        { "help", no_argument, NULL, 'H' },
        { "version", no_argument, NULL, 'v' },
        { "pngname", required_argument, NULL, 'p' },
        { "width", required_argument, NULL, 'w' },
        { "stdout", no_argument, NULL, 's' },
        { "savepath", required_argument, NULL, 'a' },
        { NULL, no_argument, NULL, 0 }
    };

    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1)
    {
        switch (opt)
        {
        case 'a':

            chdir(optarg);
            break;

        case 'c':

            compression = atoi(optarg);

            if ((compression < 0) || (compression > 9))
            {
                compression = Z_DEFAULT_COMPRESSION;
            }

            break;

        case 'd':

            delay = atoi(optarg);
            break;

        case 'D':

            displayNumber = atoi(optarg);
            break;

        case 'h':

            requestedHeight = atoi(optarg);
            break;

        case 'p':

            pngName = optarg;
            replace = true;
            break;

        case 'w':

            requestedWidth = atoi(optarg);
            break;

        case 's':

            writeToStdout = true;
            break;

        case 'v':

            fprintf(stderr, "%s (%s)\n", program, VERSION);
            exit(EXIT_SUCCESS);

        case 'H':
        default:

            usage();

            if (opt == 'H')
            {
                exit(EXIT_SUCCESS);
            }
            else
            {
                exit(EXIT_FAILURE);
            }

            break;
        }
    }

    //-------------------------------------------------------------------

    bcm_host_init();

    //-------------------------------------------------------------------

    if (delay)
    {
        sleep(delay);
    }

    //-------------------------------------------------------------------

    DISPMANX_DISPLAY_HANDLE_T displayHandle
        = vc_dispmanx_display_open(displayNumber);

    if (displayHandle == 0)
    {
        fprintf(stderr,
                "%s: unable to open display %d\n",
                program,
                displayNumber);

        exit(EXIT_FAILURE);
    }

    DISPMANX_MODEINFO_T modeInfo;
    result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

    if (result != 0)
    {
        fprintf(stderr, "%s: unable to get display information\n", program);
        exit(EXIT_FAILURE);
    }

    int32_t pngWidth = modeInfo.width;
    int32_t pngHeight = modeInfo.height;

    if (requestedWidth > 0)
    {
        pngWidth = requestedWidth;

        if (requestedHeight == 0)
        {
            double numerator = modeInfo.height * requestedWidth;
            double denominator = modeInfo.width;

            pngHeight = (int32_t)ceil(numerator / denominator);
        }
    }

    if (requestedHeight > 0)
    {
        pngHeight = requestedHeight;

        if (requestedWidth == 0)
        {
            double numerator = modeInfo.width * requestedHeight;
            double denominator = modeInfo.height;

            pngWidth = (int32_t)ceil(numerator / denominator);
        }
    }

    //-------------------------------------------------------------------
    // only need to check low bit of modeInfo.transform (value of 1 or 3).
    // If the display is rotated either 90 or 270 degrees (value 1 or 3)
    // the width and height need to be transposed.

    int32_t dmxWidth = pngWidth;
    int32_t dmxHeight = pngHeight;

    if (modeInfo.transform & 1)
    {
        dmxWidth = pngHeight;
        dmxHeight = pngWidth;
    }

    int32_t dmxPitch = dmxBytesPerPixel * ALIGN_TO_16(dmxWidth);

    void *dmxImagePtr = malloc(dmxPitch * dmxHeight);

    if (dmxImagePtr == NULL)
    {
        fprintf(stderr, "%s: unable to allocated image buffer\n", program);
        exit(EXIT_FAILURE);
    }

    //-------------------------------------------------------------------

    uint32_t vcImagePtr = 0;
    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    resourceHandle = vc_dispmanx_resource_create(imageType,
                                                 dmxWidth,
                                                 dmxHeight,
                                                 &vcImagePtr);

    result = vc_dispmanx_snapshot(displayHandle,
                                  resourceHandle,
                                  DISPMANX_NO_ROTATE);

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr, "%s: vc_dispmanx_snapshot() failed\n", program);
        exit(EXIT_FAILURE);
    }

    VC_RECT_T rect;
    result = vc_dispmanx_rect_set(&rect, 0, 0, dmxWidth, dmxHeight);

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr, "%s: vc_dispmanx_rect_set() failed\n", program);
        exit(EXIT_FAILURE);
    }

    result = vc_dispmanx_resource_read_data(resourceHandle,
                                            &rect,
                                            dmxImagePtr,
                                            dmxPitch);


    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr,
                "%s: vc_dispmanx_resource_read_data() failed\n",
                program);

        exit(EXIT_FAILURE);
    }

    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);

    //-------------------------------------------------------------------
    // Convert from RGBA (32 bit) to RGB (24 bit)

    int8_t pngBytesPerPixel = 3;
    int32_t pngPitch = pngBytesPerPixel * pngWidth;
    void *pngImagePtr = malloc(pngPitch * pngHeight);

    int32_t j = 0;
    for (j = 0 ; j < pngHeight ; j++)
    {
        int32_t dmxXoffset = 0;
        int32_t dmxYoffset = 0;

        switch (modeInfo.transform & 3)
        {
        case 0: // 0 degrees

            if (modeInfo.transform & 0x20000) // flip vertical
            {
                dmxYoffset = (dmxHeight - j - 1) * dmxPitch;
            }
            else
            {
                dmxYoffset = j * dmxPitch;
            }

            break;

        case 1: // 90 degrees


            if (modeInfo.transform & 0x20000) // flip vertical
            {
                dmxXoffset = j * dmxBytesPerPixel;
            }
            else
            {
                dmxXoffset = (dmxWidth - j - 1) * dmxBytesPerPixel;
            }

            break;

        case 2: // 180 degrees

            if (modeInfo.transform & 0x20000) // flip vertical
            {
                dmxYoffset = j * dmxPitch;
            }
            else
            {
                dmxYoffset = (dmxHeight - j - 1) * dmxPitch;
            }

            break;

        case 3: // 270 degrees

            if (modeInfo.transform & 0x20000) // flip vertical
            {
                dmxXoffset = (dmxWidth - j - 1) * dmxBytesPerPixel;
            }
            else
            {
                dmxXoffset = j * dmxBytesPerPixel;
            }

            break;
        }

        int32_t i = 0;
        for (i = 0 ; i < pngWidth ; i++)
        {
            uint8_t *pngPixelPtr = pngImagePtr
                                 + (i * pngBytesPerPixel)
                                 + (j * pngPitch);

            switch (modeInfo.transform & 3)
            {
            case 0: // 0 degrees

                if (modeInfo.transform & 0x10000) // flip horizontal
                {
                    dmxXoffset = (dmxWidth - i - 1) * dmxBytesPerPixel;
                }
                else
                {
                    dmxXoffset = i * dmxBytesPerPixel;
                }

                break;

            case 1: // 90 degrees

                if (modeInfo.transform & 0x10000) // flip horizontal
                {
                    dmxYoffset = (dmxHeight - i - 1) * dmxPitch;
                }
                else
                {
                    dmxYoffset = i * dmxPitch;
                }

                break;

            case 2: // 180 degrees

                if (modeInfo.transform & 0x10000) // flip horizontal
                {
                    dmxXoffset = i * dmxBytesPerPixel;
                }
                else
                {
                    dmxXoffset = (dmxWidth - i - 1) * dmxBytesPerPixel;
                }

                break;

            case 3: // 270 degrees

                if (modeInfo.transform & 0x10000) // flip horizontal
                {
                    dmxYoffset = i * dmxPitch;
                }
                else
                {
                    dmxYoffset = (dmxHeight - i - 1) * dmxPitch;
                }

                break;
            }

            uint8_t *dmxPixelPtr = dmxImagePtr + dmxXoffset + dmxYoffset;

            memcpy(pngPixelPtr, dmxPixelPtr, 3);
        }
    }

    free(dmxImagePtr);
    dmxImagePtr = NULL;

    //-------------------------------------------------------------------

    png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL,
                                                 NULL,
                                                 NULL);

    if (pngPtr == NULL)
    {
        fprintf(stderr,
                "%s: unable to allocated PNG write structure\n",
                program);

        exit(EXIT_FAILURE);
    }

    png_infop infoPtr = png_create_info_struct(pngPtr);

    if (infoPtr == NULL)
    {
        fprintf(stderr,
                "%s: unable to allocated PNG info structure\n",
                program);

        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(pngPtr)))
    {
        fprintf(stderr, "%s: unable to create PNG\n", program);
        exit(EXIT_FAILURE);
    }

    FILE *pngfp = NULL;

    if (writeToStdout)
    {
        pngfp = stdout;
    }
    else
    {
        if (!replace && file_exists(pngName))
        {
          time_t rawtime;
          struct tm *ltime;
          time(&rawtime);
          ltime = localtime(&rawtime);
          int nLen = strlen(DEFAULT_NAME_START);

          strcpy(pngNameWithTimestamp, DEFAULT_NAME_START);
          strftime(&pngNameWithTimestamp[nLen], MAX_NAME_SIZE-nLen,
           "_%Y%m%d-%H%M%S.png",ltime);
          pngName = pngNameWithTimestamp;
        }

        pngfp = fopen(pngName, "wb");
        if (pngfp == NULL)
        {
            fprintf(stderr,
                    "%s: unable to create %s - %s\n",
                    program,
                    pngName,
                    strerror(errno));

            exit(EXIT_FAILURE);
        }
    }

    png_init_io(pngPtr, pngfp);

    png_set_IHDR(
        pngPtr,
        infoPtr,
        pngWidth,
        pngHeight,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);

    if (compression != Z_DEFAULT_COMPRESSION)
    {
        png_set_compression_level(pngPtr, compression);
    }

    png_write_info(pngPtr, infoPtr);

    int y = 0;
    for (y = 0; y < pngHeight; y++)
    {
        png_write_row(pngPtr, pngImagePtr + (pngPitch * y));
    }

    png_write_end(pngPtr, NULL);
    png_destroy_write_struct(&pngPtr, &infoPtr);

    if (pngfp != stdout)
    {
        fclose(pngfp);
    }

    //-------------------------------------------------------------------

    free(pngImagePtr);
    pngImagePtr = NULL;

    return 0;
}

