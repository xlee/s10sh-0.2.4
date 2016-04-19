#!/bin/sh

if [ "$1" = "" ]; then
	echo usage: $0 \<release\>
	exit 1
fi

rm -rf s10sh-${1} s10sh-${1}.tar.gz
mkdir s10sh-${1} || exit 1
cp * s10sh-${1}
cp -a libusb-0.1.7 s10sh-${1}
cd s10sh-${1} || exit 1
rm -f *.JPG
rm -f ./s10sh_REMOVE_ME
make distclean
rm -f libusb
ln -s libusb-0.1.7 libusb
cd libusb
make distclean
cd ../../
tar cvzf s10sh-${1}.tar.gz s10sh-${1}
rm -rf s10sh-${1}
