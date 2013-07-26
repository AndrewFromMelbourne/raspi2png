#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bcm_host.h"

#ifndef ALIGN_UP
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#endif

//-----------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int opt;
    char *pngname = "snapshot.png";
    int align_to = 16;
    int requested_width = 0;
    int requested_height = 0;
    bool verbose = false;

    //-------------------------------------------------------------------

    while ((opt = getopt(argc, argv, "h:p:vw:")) != -1)
    {
        switch (opt)
        {
        case 'h':

            requested_height = atoi(optarg);
            break;

        case 'p':

            pngname = optarg;
            break;

        case 'v':

            verbose = true;
            break;

        case 'w':

            requested_width = atoi(optarg);
            break;

        default:

            fprintf(stderr, "Usage: %s [-p pngname] [-v]", argv[0]);
            fprintf(stderr, " [-w <width>] [-h <height>]\n");

            fprintf(stderr, "    -p - name of png file to create\n");
            fprintf(stderr, "    -v - verbose\n");

            fprintf(stderr,
                    "    -h - image height (default is screen height)\n");
            fprintf(stderr,
                    "    -w - image width (default is screen width)\n");

            exit(EXIT_FAILURE);
            break;
        }
    }

    //-------------------------------------------------------------------

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T displayHandle = vc_dispmanx_display_open(0);

    DISPMANX_MODEINFO_T modeInfo;
    int getInfoReturned = vc_dispmanx_display_get_info(displayHandle,
                                                       &modeInfo);

    if (getInfoReturned != 0)
    {
        fprintf(stderr, "%s: unable to get display information\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = modeInfo.width;
    int height = modeInfo.height;

    if (requested_width > 0)
    {
        width = requested_width;
    }

    if (requested_height > 0)
    {
        height = requested_height;
    }
    else if (requested_width > 0)
    {
        height = (modeInfo.height * requested_width) / modeInfo.width;
    }

    int pitch = 3 * ALIGN_UP(width, align_to);

    if (verbose)
    {
        printf("screen width = %d\n", modeInfo.width);
        printf("screen height = %d\n", modeInfo.height);
        printf("requested width = %d\n", requested_width);
        printf("requested height = %d\n", requested_height);
        printf("image width = %d\n", width);
        printf("image height = %d\n", height);
        printf("pitch = %d\n", pitch);
    }

    uint32_t vcImagePtr;
    void *imagePtr = malloc(pitch * height);

    if (imagePtr == NULL)
    {
        fprintf(stderr, "%s: unable to allocated image buffer\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    resourceHandle = vc_dispmanx_resource_create(VC_IMAGE_RGB888,
                                                 width,
                                                 height,
                                                 &vcImagePtr);

    int result = 0;
    
    result = vc_dispmanx_snapshot(displayHandle, resourceHandle, 0);

    if (verbose)
    {
        printf("vc_dispmanx_snapshot() returned %d\n", result);
    }

    VC_RECT_T rect;
    result = vc_dispmanx_rect_set(&rect, 0, 0, width, height);

    if (verbose)
    {
        printf("vc_dispmanx_rect_set() returned %d\n", result);
    }

    result = vc_dispmanx_resource_read_data(resourceHandle,
                                            &rect,
                                            imagePtr,
                                            pitch);

    if (verbose)
    {
        printf("vc_dispmanx_resource_read_data() returned %d\n", result);
    }

    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);

    //-------------------------------------------------------------------

    png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL,
                                                 NULL,
                                                 NULL);

    if (pngPtr == NULL)
    {
        fprintf(stderr,
                "%s: unable to allocated PNG write structure\n",
                argv[0]);

        exit(EXIT_FAILURE);
    }

    png_infop infoPtr = png_create_info_struct(pngPtr);

    if (infoPtr == NULL)
    {
        fprintf(stderr,
                "%s: unable to allocated PNG info structure\n",
                argv[0]);

        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(pngPtr)))
    {
        fprintf(stderr, "%s: unable to create PNG\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *pngfp = fopen(pngname, "wb");

    if (pngfp == NULL)
    {
        fprintf(stderr,
                "%s: unable to create %s - %s\n",
                argv[0],
                pngname,
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

    int y;
    for (y = 0; y < height; y++)
    {
        png_write_row(pngPtr, imagePtr + (pitch * y));
    }

    png_write_end(pngPtr, NULL);
    png_destroy_write_struct(&pngPtr, &infoPtr);
    fclose(pngfp);

    //-------------------------------------------------------------------

    free(imagePtr);

    return 0;
}
