# raspi2png

Utility to take a snapshot of the raspberry pi screen and save it as a PNG file

    Usage: raspi2png [--pngname name] [--width <width>] [--height <height>] [--compression <level>] [--delay <delay>] [--display <number>] [--stdout] [--help]

    --pngname,-p - name of png file to create (default is snapshot.png)
    --height,-h - image height (default is screen height)
    --width,-w - image width (default is screen width)
    --compression,-c - PNG compression level (0 - 9)
    --delay,-d - delay in seconds (default 0)
    --display,-D - Raspberry Pi display number (default 0)
	--stdout,-s - write file to stdout
    --help,-H - print this usage information

## Simple Install

Run this command through terminal or CLI screen.

curl -sL https://raw.githubusercontent.com/AndrewFromMelbourne/raspi2png/master/installer.sh | bash -

## Manual Building

You will need to install libpng before you build the program. On Raspbian / Raspberry Pi OS execute  
``sudo apt-get install libpng12-dev``  
Then just type 'make' and 'make install' in the raspi2png directory you cloned from github.

## Benchmark

raspi2png 1.1, 720p screen, Raspberry Pi 3 B, RAM-disk

| compression | seconds | size (KiB) |
|:-:|:----:|-----:|
| 0 | 0,23 | 2705 | 
| 1 | 0,42 |  676 |
| 2 | 0,42 |  654 |
| 3 | 0,53 |  624 |
| 4 | 0,57 |  606 |
| 5 | 0,71 |  593 |
| 6 | 1,10 |  575 |
| 7 | 1,46 |  569 |
| 8 | 3,04 |  564 |
| 9 | 5,30 |  560 |

optipng size:  ~553 KiB  


![Screenshot](https://github.com/GrazerComputerClub/raspi2png/raw/master/screenshot.png)


