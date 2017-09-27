#!/bin/bash

# Packages required on Debian:
# - libmagickcore-dev
# - libmagickcore-6-headers
# - libmagick++-dev
# - libmagick++-6-headers 
# - libmagickwand-6-headers
# - libmagickwand-dev
# - imagemagick-6-common
# - freeglut3-dev
# - libxmu-dev 
# - libxi-dev
# - Ygor


mconf="Magick++-config"
if [ -f "/usr/lib/x86_64-linux-gnu/ImageMagick-6.9.7/bin-q16/Magick++-config" ] ; then
    mconf="/usr/lib/x86_64-linux-gnu/ImageMagick-6.9.7/bin-q16/Magick++-config"
fi

g++ -ggdb -std=c++1y `$mconf --cppflags` \
    src/Mosaic.cc src/Structs.cc src/Text.cc src/Flashcard_Parsing.cc -o mosaic   \
    -L/usr/X11R6/lib `$mconf --ldflags --libs` \
    -lygor -lsqlite3 -lX11 -lXi -lXmu -lglut -lGL -lGLU -lm



