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

static const char* imageTypeNames[] =
{
    "RGB565",
    "RGB888",
    "RGBA32"
};

static const VC_IMAGE_TYPE_T imageTypes[] =
{
    VC_IMAGE_RGB565,
    VC_IMAGE_RGB888,
    VC_IMAGE_RGBA32
};

static int imageBytesPerPixel[] =
{
    2,
    3,
    4
};

static size_t imageEntries = sizeof(imageTypeNames)/sizeof(imageTypeNames[0]);
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

            imageRow[index] =  ((pixels >> 11) * 0xFF) / 0x1F;
            imageRow[index + 1] =  (((pixels >> 5) & 0x3F) * 0xFF) / 0x3F;
            imageRow[index + 2] =  ((pixels & 0x1F) * 0xFF) / 0x1F;
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
pngWriteImageRGBA32(
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
            uint8_t *pixels = (uint8_t*)(buffer + (y * pitch) + (x * 4));
            int index = x * 3;

            imageRow[index] =  pixels[0];
            imageRow[index + 1] =  pixels[1];
            imageRow[index + 2] =  pixels[2];
        }
        png_write_row(pngPtr, imageRow);

    }

    free(imageRow);
}

//-----------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int opt = 0;

    char *pngName = "snapshot.png";
    int requestedWidth = 0;
    int requestedHeight = 0;
    bool verbose = false;

    const char* imageTypeName = "RGB888";
    VC_IMAGE_TYPE_T imageType = VC_IMAGE_MIN;
    int bytesPerPixel  = 0;

    int result = 0;

    program = argv[0];

    //-------------------------------------------------------------------

    while ((opt = getopt(argc, argv, "h:p:t:vw:")) != -1)
    {
        switch (opt)
        {
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
            fprintf(stderr, " [-w <width>] [-h <height>] [-t <type>]\n");

            fprintf(stderr, "    -p - name of png file to create\n");
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
                fprintf(stderr, " %s", imageTypeNames[entry]);
            }
            fprintf(stderr, "\n");

            exit(EXIT_FAILURE);
            break;
        }
    }

    //-------------------------------------------------------------------

    size_t entry = 0;
    for (entry = 0; entry < imageEntries; entry++)
    {
        if (strcmp(imageTypeName, imageTypeNames[entry]) == 0)
        {
            imageType = imageTypes[entry];
            bytesPerPixel =  imageBytesPerPixel[entry];
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
    }

    if (requestedHeight > 0)
    {
        height = requestedHeight;
    }
    else if (requestedWidth > 0)
    {
		double numerator = modeInfo.height * requestedWidth;
		double denominator = modeInfo.width;

		height = (int)ceil(numerator/denominator);
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

    result = vc_dispmanx_snapshot(displayHandle, resourceHandle, 0);

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

    png_set_IHDR(
        pngPtr,
        infoPtr,
        width,
        height,
        8,
        PNG_COLOR_TYPE_RGB,
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
