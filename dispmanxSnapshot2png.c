#include <png.h>
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
    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s <snapshotName.png>\n", argv[0]);
        exit(EXIT_FAILURE);
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
	int pitch = ALIGN_UP(3 * width, 32);

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

    vc_dispmanx_snapshot(displayHandle, resourceHandle, 0);

    VC_RECT_T rect;
    vc_dispmanx_rect_set(&rect, 0, 0, width, height);
    vc_dispmanx_resource_read_data(resourceHandle,
                                   &rect,
                                   imagePtr,
                                   pitch);

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

    const char *pngname = "snapshot.png";

    if (argc > 1)
    {
        pngname = argv[1];
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
    fclose(pngfp);

    //-------------------------------------------------------------------

    free(imagePtr);

    return 0;
}
