//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2013 Andrew Duncan
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

#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bcm_host.h"

//-----------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

//-----------------------------------------------------------------------

const char* program = NULL;

//-----------------------------------------------------------------------

typedef struct
{
    const char* name;
    VC_IMAGE_TYPE_T type;
    int bytesPerPixel;
}
ImageInfo;

#define IMAGE_INFO_ENTRY(t, b) \
    { .name=(#t), .type=(VC_IMAGE_ ## t), .bytesPerPixel=(b) }

ImageInfo imageInfo[] =
{
    IMAGE_INFO_ENTRY(RGB565, 2),
    IMAGE_INFO_ENTRY(RGB888, 3),
    IMAGE_INFO_ENTRY(RGBA16, 2),
    IMAGE_INFO_ENTRY(RGBA32, 4)
};

static size_t imageEntries = sizeof(imageInfo)/sizeof(imageInfo[0]);

//-----------------------------------------------------------------------

void
pngWriteImageRGB565(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int rowLength = 3 * width;
    uint8_t *imageRow = malloc(rowLength);

    if (imageRow == NULL)
    {
        fprintf(stderr, "%s: unable to allocated row buffer\n", program);
        exit(EXIT_FAILURE);
    }

    int y = 0;
    for (y = 0; y < height; y++)
    {
        int x = 0;
        for (x = 0; x < width; x++)
        {
            uint16_t pixels = *(uint16_t*)(buffer + (y * pitch) + (x * 2));
            int index = x * 3;

            uint8_t r5 = (pixels >> 11) & 0x1F;
            uint8_t g6 = (pixels >> 5) & 0x3F;
            uint8_t b5 = (pixels) & 0x1F;

            imageRow[index] =  (r5 << 3) | (r5 >> 2);
            imageRow[index + 1] =  (g6 << 2) | (g6 >> 4);
            imageRow[index + 2] =  (b5 << 3) | (b5 >> 2);
        }
        png_write_row(pngPtr, imageRow);

    }

    free(imageRow);
}

//-----------------------------------------------------------------------

void
pngWriteImageRGB888(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int y = 0;
    for (y = 0; y < height; y++)
    {
        png_write_row(pngPtr, buffer + (pitch * y));
    }
}

//-----------------------------------------------------------------------

void
pngWriteImageRGBA16(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int rowLength = 4 * width;
    uint8_t *imageRow = malloc(rowLength);

    if (imageRow == NULL)
    {
        fprintf(stderr, "%s: unable to allocated row buffer\n", program);
        exit(EXIT_FAILURE);
    }

    int y = 0;
    for (y = 0; y < height; y++)
    {
        int x = 0;
        for (x = 0; x < width; x++)
        {
            uint16_t pixels = *(uint16_t*)(buffer + (y * pitch) + (x * 2));
            int index = x * 4;

            uint8_t r4 = (pixels >> 12) & 0xF;
            uint8_t g4 = (pixels >> 8) & 0xF;
            uint8_t b4 = (pixels >> 4) & 0xF;
            uint8_t a4 = pixels & 0xF;

            imageRow[index] =  (r4 << 4) | r4;
            imageRow[index + 1] =  (g4 << 4) | g4;
            imageRow[index + 2] =  (b4 << 4) | b4;
            imageRow[index + 3] =  (a4 << 4) | a4;
        }
        png_write_row(pngPtr, imageRow);

    }

    free(imageRow);
}

//-----------------------------------------------------------------------

void
pngWriteImageRGBA32(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int y = 0;
    for (y = 0; y < height; y++)
    {
        png_write_row(pngPtr, buffer + (pitch * y));
    }
}

//-----------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int opt = 0;

    char *pngName = "snapshot.png";
    int requestedWidth = 0;
    int requestedHeight = 0;
    bool verbose = false;
    int delay = 0;

    const char* imageTypeName = "RGB888";
    VC_IMAGE_TYPE_T imageType = VC_IMAGE_MIN;
    int bytesPerPixel  = 0;

    int result = 0;

    program = basename(argv[0]);

    //-------------------------------------------------------------------

    while ((opt = getopt(argc, argv, "d:h:p:t:vw:")) != -1)
    {
        switch (opt)
        {
        case 'd':

            delay = atoi(optarg);
            break;

        case 'h':

            requestedHeight = atoi(optarg);
            break;

        case 'p':

            pngName = optarg;
            break;

        case 't':

            imageTypeName = optarg;
            break;

        case 'v':

            verbose = true;
            break;

        case 'w':

            requestedWidth = atoi(optarg);
            break;

        default:

            fprintf(stderr, "Usage: %s [-p pngname] [-v]", program);
            fprintf(stderr, " [-w <width>] [-h <height>] [-t <type>]");
            fprintf(stderr, " [-d <delay>]\n");

            fprintf(stderr, "    -p - name of png file to create ");
            fprintf(stderr, "(default is %s)\n", pngName);
            fprintf(stderr, "    -v - verbose\n");

            fprintf(stderr,
                    "    -h - image height (default is screen height)\n");
            fprintf(stderr,
                    "    -w - image width (default is screen width)\n");

            fprintf(stderr, "    -t - type of image captured\n");
            fprintf(stderr, "         can be one of the following:");

            size_t entry = 0;
            for (entry = 0; entry < imageEntries; entry++)
            {
                fprintf(stderr, " %s", imageInfo[entry].name);
            }
            fprintf(stderr, "\n");
            fprintf(stderr, "    -d - delay in seconds (default 0)\n");
            fprintf(stderr, "\n");

            exit(EXIT_FAILURE);
            break;
        }
    }

    //-------------------------------------------------------------------

    size_t entry = 0;
    for (entry = 0; entry < imageEntries; entry++)
    {
        if (strcasecmp(imageTypeName, imageInfo[entry].name) == 0)
        {
            imageType = imageInfo[entry].type;
            bytesPerPixel =  imageInfo[entry].bytesPerPixel;
            break;
        }
    }

    if (imageType == VC_IMAGE_MIN)
    {
        fprintf(stderr,
                "%s: unknown image type %s\n",
                program,
                imageTypeName);

        exit(EXIT_FAILURE);
    }

    //-------------------------------------------------------------------

    bcm_host_init();

    //-------------------------------------------------------------------
    //
    // Calling vc_dispmanx_snapshot() fails when the display is rotate
    // either 90 or 270 degrees. It sometimes causes the program to hang.
    // check the config to make sure the screen is not rotated.
    //

    char response[1024];
    int display_rotate = 0;

    if (vc_gencmd(response, sizeof(response), "get_config int") == 0)
    {
        char *saveptr = NULL;
        char *token = strtok_r(response, "\n", &saveptr);

        while (token != NULL)
        {
            char setting[100];
            char value[100];

            if (sscanf(token, "%[^=]=%s", setting, value) == 2)
            {
                if (strcmp(setting, "display_rotate") == 0)
                {
                    display_rotate = strtod(value, NULL);
                }
            }

            token = strtok_r(NULL, "\n", &saveptr);
        }
    }

    // only need to check low bit of display_rotate (value of 1 or 3).

    if (display_rotate & 1)
    {
        fprintf(stderr,
                "%s: cannot create screenshot for rotated display\n",
                program);

        exit(EXIT_FAILURE);
    }

    //-------------------------------------------------------------------

    if (delay)
    {
        if (verbose)
        {
            printf("sleeping for %d seconds ...\n", delay);
        }

        sleep(delay);
    }

    //-------------------------------------------------------------------

    DISPMANX_DISPLAY_HANDLE_T displayHandle = vc_dispmanx_display_open(0);

    DISPMANX_MODEINFO_T modeInfo;
    result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

    if (result != 0)
    {
        fprintf(stderr, "%s: unable to get display information\n", program);
        exit(EXIT_FAILURE);
    }

    int width = modeInfo.width;
    int height = modeInfo.height;

    if (requestedWidth > 0)
    {
        width = requestedWidth;

        if (requestedHeight == 0)
        {
            double numerator = modeInfo.height * requestedWidth;
            double denominator = modeInfo.width;

            height = (int)ceil(numerator / denominator);
        }
    }

    if (requestedHeight > 0)
    {
        height = requestedHeight;

        if (requestedWidth == 0)
        {
            double numerator = modeInfo.width * requestedHeight;
            double denominator = modeInfo.height;

            width = (int)ceil(numerator / denominator);
        }
    }

    int pitch = bytesPerPixel * ALIGN_TO_16(width);

    if (verbose)
    {
        printf("screen width = %d\n", modeInfo.width);
        printf("screen height = %d\n", modeInfo.height);
        printf("requested width = %d\n", requestedWidth);
        printf("requested height = %d\n", requestedHeight);
        printf("image width = %d\n", width);
        printf("image height = %d\n", height);
        printf("image type = %s\n", imageTypeName);
        printf("bytes per pixel = %d\n", bytesPerPixel);
        printf("pitch = %d\n", pitch);
    }

    void *dmxImagePtr = malloc(pitch * height);

    if (dmxImagePtr == NULL)
    {
        fprintf(stderr, "%s: unable to allocated image buffer\n", program);
        exit(EXIT_FAILURE);
    }

    uint32_t vcImagePtr = 0;
    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    resourceHandle = vc_dispmanx_resource_create(imageType,
                                                 width,
                                                 height,
                                                 &vcImagePtr);

    result = vc_dispmanx_snapshot(displayHandle,
                                  resourceHandle,
                                  DISPMANX_NO_ROTATE);

    if (verbose)
    {
        printf("vc_dispmanx_snapshot() returned %d\n", result);
    }

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr, "%s: vc_dispmanx_snapshot() failed\n", program);
        exit(EXIT_FAILURE);
    }

    VC_RECT_T rect;
    result = vc_dispmanx_rect_set(&rect, 0, 0, width, height);

    if (verbose)
    {
        printf("vc_dispmanx_rect_set() returned %d\n", result);
        printf("rect = { %d, %d, %d, %d }\n",
               rect.x,
               rect.y,
               rect.width,
               rect.height);
    }

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
                                            pitch);


    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr,
                "%s: vc_dispmanx_resource_read_data() failed\n",
                program);

        exit(EXIT_FAILURE);
    }

    if (verbose)
    {
        printf("vc_dispmanx_resource_read_data() returned %d\n", result);
    }

    result = vc_dispmanx_resource_delete(resourceHandle);

    if (verbose)
    {
        printf("vc_dispmanx_resource_delete() returned %d\n", result);
    }

    result = vc_dispmanx_display_close(displayHandle);

    if (verbose)
    {
        printf("vc_dispmanx_display_close() returned %d\n", result);
    }

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

    FILE *pngfp = fopen(pngName, "wb");

    if (pngfp == NULL)
    {
        fprintf(stderr,
                "%s: unable to create %s - %s\n",
                program,
                pngName,
                strerror(errno));

        exit(EXIT_FAILURE);
    }

    png_init_io(pngPtr, pngfp);

    int png_color_type = PNG_COLOR_TYPE_RGB;

    if ((imageType == VC_IMAGE_RGBA16) || (imageType == VC_IMAGE_RGBA32))
    {
        png_color_type = PNG_COLOR_TYPE_RGBA;
    }

    png_set_IHDR(
        pngPtr,
        infoPtr,
        width,
        height,
        8,
        png_color_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);

    png_write_info(pngPtr, infoPtr);

    switch(imageType)
    {
    case VC_IMAGE_RGB565:

        pngWriteImageRGB565(width,
                            height,
                            pitch,
                            dmxImagePtr,
                            pngPtr,
                            infoPtr);
        break;

    case VC_IMAGE_RGB888:

        pngWriteImageRGB888(width,
                            height,
                            pitch,
                            dmxImagePtr,
                            pngPtr,
                            infoPtr);
        break;

    case VC_IMAGE_RGBA16:

        pngWriteImageRGBA16(width,
                            height,
                            pitch,
                            dmxImagePtr,
                            pngPtr,
                            infoPtr);
        break;

    case VC_IMAGE_RGBA32:

        pngWriteImageRGBA32(width,
                            height,
                            pitch,
                            dmxImagePtr,
                            pngPtr,
                            infoPtr);
        break;

    default:

        break;
    }

    png_write_end(pngPtr, NULL);
    png_destroy_write_struct(&pngPtr, &infoPtr);
    fclose(pngfp);

    //-------------------------------------------------------------------

    free(dmxImagePtr);

    return 0;
}

