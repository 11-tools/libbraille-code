#!/usr/bin/python

import braille

if __name__ == '__main__':
    if(braille.braille_init()):
	print "libbraille initialised correctly!"
    braille.braille_display("test")
