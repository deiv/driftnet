
Driftnet
========

[![release](https://img.shields.io/github/release/deiv/driftnet.svg)](https://github.com/deiv/driftnet/releases)
[![version](https://img.shields.io/github/release-date/deiv/driftnet.svg)](https://github.com/deiv/driftnet/releases)
[![license](https://img.shields.io/github/license/deiv/driftnet.svg)](https://github.com/deiv/driftnet/blob/master/COPYING)
[![IDE](https://img.shields.io/badge/IDE-CLion-00AA00.svg)](https://www.jetbrains.com/clion/?from=driftnet)
[![Refactored](https://img.shields.io/badge/Refactored%20with-Structure101-8A2BE2.svg)](https://structure101.com/)
[![CircleCI](https://img.shields.io/circleci/project/github/deiv/driftnet/master.svg?colorB=CEC109)](https://circleci.com/gh/deiv/driftnet/tree/master)
[![codecov](https://codecov.io/gh/deiv/driftnet/branch/master/graph/badge.svg)](https://codecov.io/gh/deiv/driftnet)
[![jProfiler](https://img.shields.io/static/v1?label=profiler&message=JProfiler&color=0993e2)](https://www.ej-technologies.com/products/jprofiler/overview.html)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fdeiv%2Fdriftnet.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fdeiv%2Fdriftnet?ref=badge_shield)
[![Packaging status](https://repology.org/badge/tiny-repos/driftnet.svg)](https://repology.org/project/driftnet/versions)

Driftnet watches network traffic, and picks out and displays JPEG, GIF and other image formats for display. It is a horrific invasion of privacy and shouldn't be used by anyone anywhere. It can also extract MPEG audio data from the network and play it. If you live in a house with thick walls, this may be a useful way to find out about your neighbours' musical taste.

News
------------

    Added WebP image format support
    Added new, http and websockets based, display
    Added basic windows support (cygwin)
    Added option to list capture interfaces
    Added support for putting the interface in monitor mode

Dependencies
------------


## Unix

You will need:
* [libpcap](https://sourceforge.net/projects/libpcap/)
* [libjpeg](http://libjpeg.sourceforge.net/)
* [libungif](http://directory.fsf.org/wiki/Libungif)
* [libpng](http://www.libpng.org/pub/png/libpng.html)
* [libwebp](https://developers.google.com/speed/webp/)
* [libwebsockets](https://libwebsockets.org/) (if you want the http server)
* [libgtk](https://www.gtk.org/) (if you want the GTK display)

On most Linux distributions (APT based) these can be installed by executing `sudo apt-get install libpcap-dev libjpeg-dev libpng12-dev giflib-tools libwebp-dev`. If you don't want a version of driftnet which will display images itself, but just want  to use it to gather images for some other application, you only need `libpcap`. See comments in the Makefile for more information. To play MPEG audio, you need an MPEG player. By default, driftnet will use [mpg123](http://www.mpg123.de/).

## Windows (on cygwin)

You will need:
* [libwinpcap](http://www.winpcap.org/devel.htm)
* ~~[libjpeg](http://libjpeg.sourceforge.net/)~~
* ~~[libungif](http://directory.fsf.org/wiki/Libungif)~~
* ~~[libpng](http://www.libpng.org/pub/png/libpng.html)~~
* ~~[libwebp](https://developers.google.com/speed/webp/)~~

Only 32 bits builds are supported (libwinpcap did not provide a x64 library). You should pass a x86 compiler to configure:
`./configure --target=i686-pc-cygwin --host=i686-pc-cygwin`

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

## License

Driftnet is licensed under the GNU GPL. See the file COPYING in the distribution.

Thanks
------------
+ [Jetbrains](https://www.jetbrains.com) for his marvelous IDE [CLion](https://www.jetbrains.com/clion/)
+ **Headway Software Technologies Ltd** and their fantastic refactor tool [Structure101](https://structure101.com/)
+ All the [contributors](https://github.com/deiv/driftnet/graphs/contributors) 

Support development
------------

If you interested in this software (or other FOSS activities I do), 
<a href="https://www.patreon.com/bePatron?u=13707009" data-patreon-widget-type="become-patron-button">get on board and become a Patron!</a>
