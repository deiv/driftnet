Driftnet
========
Driftnet watches network traffic, and picks out and displays JPEG and GIF images for display. It is a horrific invasion of privacy and shouldn't be used by anyone anywhere. It can also extract MPEG audio data from the network and play it. If you live in a house with thick walls, this may be a useful way to find out about your neighbours' musical taste.

Dependencies
------------
You will need:
* [libpcap](https://sourceforge.net/projects/libpcap/)
* [libjpeg](http://libjpeg.sourceforge.net/)
* [libungif](http://directory.fsf.org/wiki/Libungif)
* [libpng](http://www.libpng.org/pub/png/libpng.html)

On most Linux distributions (APT based) these can be installed by executing `sudo apt-get install libpcap-dev libjpeg-dev libpng12-dev giflib-tools`. If you don't want a version of driftnet which will display images itself, but just want  to use it to gather images for some other application, you only need `libpcap`. See comments in the Makefile for more information. To play MPEG audio, you need an MPEG player. By default, driftnet will use [mpg123](http://www.mpg123.de/).


Compilation
------------
To compile, generate the needed autotools files with `autoreconf -fi` (you probably need to install autotools) then run `./configure; make; make install`. Driftnet is at a very early stage of development and probably won't work for you at all.

Usage
-----
Driftnet needs to run with sufficient privilege to obtain raw packets from the network. On most systems, this means running it as root.

You can use Driftnet to sniff images passing over a wireless network. However, Driftnet does not understand the optional WEP encryption used with wireless ethernet. Instead, you can use [Kismet](http://www.kismetwireless.net/) to decrypt packets and pass them into a named pipe; the -f option can then be used to have Driftnet read the packets from the pipe. Thanks to Rob Timko and
Joshua Wright for pointing this out; [Rob's page](http://68.38.68.127:81/writings/driftnet.html) describes the process in greater detail.

If you find this program entertaining, you might want to help me develop it.
The TODO file contains a list of yet-to-be-done ideas.

Driftnet is licensed under the GNU GPL. See the file COPYING in the distribution.
