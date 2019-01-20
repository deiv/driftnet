
Driftnet
========

[![release](https://img.shields.io/github/release/deiv/driftnet.svg)](https://github.com/deiv/driftnet/releases)
[![version](https://img.shields.io/github/release-date/deiv/driftnet.svg)](https://github.com/deiv/driftnet/releases)
[![license](https://img.shields.io/github/license/deiv/driftnet.svg)](https://github.com/deiv/driftnet/blob/master/COPYING)
[![IDE](https://img.shields.io/badge/IDE-CLion-00AA00.svg)](https://www.jetbrains.com/clion/)
[![CircleCI](https://img.shields.io/circleci/project/github/deiv/driftnet/master.svg?colorB=CEC109)](https://circleci.com/gh/deiv/driftnet/tree/master)
[![codecov](https://codecov.io/gh/deiv/driftnet/branch/master/graph/badge.svg)](https://codecov.io/gh/deiv/driftnet)

Driftnet watches network traffic, and picks out and displays JPEG and GIF images for display. It is a horrific invasion of privacy and shouldn't be used by anyone anywhere. It can also extract MPEG audio data from the network and play it. If you live in a house with thick walls, this may be a useful way to find out about your neighbours' musical taste.

News
------------

    Added new, http and websockets based, display
    Added basic windows support (cygwin)
    Added option to list capture interfaces
    Added support for putting the interface in monitor mode


Support development
------------

If you interested in this software (or other FOSS activities I do), 
<a href="https://www.patreon.com/bePatron?u=13707009" data-patreon-widget-type="become-patron-button">get on board and become a Patron!</a>

Dependencies
------------


## Unix

You will need:
* [libpcap](https://sourceforge.net/projects/libpcap/)
* [libjpeg](http://libjpeg.sourceforge.net/)
* [libungif](http://directory.fsf.org/wiki/Libungif)
* [libpng](http://www.libpng.org/pub/png/libpng.html)
* [libwebsockets](https://libwebsockets.org/) (if you want the http server)

On most Linux distributions (APT based) these can be installed by executing `sudo apt-get install libpcap-dev libjpeg-dev libpng12-dev giflib-tools`. If you don't want a version of driftnet which will display images itself, but just want  to use it to gather images for some other application, you only need `libpcap`. See comments in the Makefile for more information. To play MPEG audio, you need an MPEG player. By default, driftnet will use [mpg123](http://www.mpg123.de/).

## Windows (on cygwin)

You will need:
* [libwinpcap](http://www.winpcap.org/devel.htm)
* ~~[libjpeg](http://libjpeg.sourceforge.net/)~~
* ~~[libungif](http://directory.fsf.org/wiki/Libungif)~~
* ~~[libpng](http://www.libpng.org/pub/png/libpng.html)~~

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

Driftnet is licensed under the GNU GPL. See the file COPYING in the distribution.
