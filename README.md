raspi2png
=========

Utility to take a snapshot of the raspberry pi screen and save it as a PNG file

    Usage: raspi2png [--pngname name] [--verbose] [--width <width>] [--height <height>] [--type <type>] [--delay <delay>] [--help]

    --pngname - name of png file to create (default is snapshot.png)
    --verbose - print verbose/debug information
    --height - image height (default is screen height)
    --width - image width (default is screen width)
    --type - type of image captured
             can be one of the following: RGB565 RGB888 RGBA16 RGBA32
    --delay - delay in seconds (default 0)
    --help - print this usage information

building
--------

You will need to install libpng before you build the program. On Raspbian

sudo apt-get install libpng12-dev

Then just type 'make' in the raspi2png directory you cloned from github.
