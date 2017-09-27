#!/bin/sh

# Use this way if you need to sort by date or something.
#
# NOTE: will split cards up into separate instances if there are a 
#       lot (~1000 - depends on filenames and Bash).
#
#find ~/Dropbox/Project\ -\ Flashcards/ -iname '*fcard' -newermt 'oct 9' -exec ./mosaic -r -f '{}' \+
#find ~/Dropbox/Project\ -\ Flashcards/ -iname '*fcard' -exec ./mosaic -r -f '{}' \+

# Use this way for the fastest startup. Bash does not expand all the filenames, but they are all 
# recursively found by the program itself.
#
# NOTE: Sorting by date is not easily possible using this method. It also requires all
#       flashcards to have the suffix '.fcard', which may or may not be possible.
./mosaic -S ~/Flashcards/Residency_General/


exit
exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit
exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit
exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit
exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit
exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit
exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit
exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit exit


# Get a raw listing of the solutions. This is helpful for 'familiarizing' yourself with foreign words
# and rapidly looking things up.

find ~/Dropbox/Project\ -\ Flashcards/ -type f -iname '*fcard' -exec ~/Dropbox/Project\ -\ Mosaic/Dump_Solutions.sh '{}' \; | sort -u 



# Generate a bunch of pdfs

find ~/Flashcards/ -type f -iname '*pdf' -delete
./mosaic -r -f -S ~/Flashcards/    # to spit out the HTML. Make sure it is compiled to do so (in Flashcard_Parsing.cc)
find ~/Flashcards/ -iname '*html' -exec  htmldoc --webpage --color -t pdf14 --size a4 '{}' -f '{}'.pdf \;

find ~/Flashcards/ -type f -iname '*fcard' | sort --random-sort > /tmp/allfcards
while read fcard ; do
    echo "${fcard}1.appended.html.pdf" >> /tmp/allhtmls
    echo "${fcard}2.appended.html.pdf" >> /tmp/allhtmls
done < /tmp/allfcards

# Make a simple title page using one of the above output as a template (to get congruent page size).
# inkscape .......pdf
# ... and then append /home/hal/Title.pdf to the *beginning* of /tmp/allhtmls

pdftk $(< /tmp/allhtmls) cat output ~/Cards.pdf compress

gs -dNOPAUSE -dQUIET -dBATCH -dNumRenderingThreads=1 -dCompatibilityLevel=1.4 -sDEVICE=pdfwrite -dDownsampleColorImages=true -dDownsampleGrayImages=true -dDownsampleMonoImages=true -dColorImageResolution=150 -dGrayImageResolution=150 -dMonoImageResolution=150 -o Cards2.pdf Cards.pdf

