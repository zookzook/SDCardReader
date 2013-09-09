SDCardReader
============

Using the arduino sd library requires much memory space. This code shows how to read the contents of a file and needs only about 4k program space and 1k ram for buffering.

The idea
--------

I just want to make some sounds with the Arduino Uno. But not playing the some wave files, I want
to play mp3 files. The first solution was the mp3 trigger from Sparkfun. The breakboard is good and
it works but it is too expensive. I found another breakboard vom elv (http://www.elv.de) with the
same mp3 decoder from VLSI. You use the SD library to read 32 bytes from the mp3 file and send the
bytes to the mp3 decoder chip. This works really simple and good. Unfortunately using the SD
library needs a lot of flash. So I try to reduce the amount of memory by implementing only the code
which I need to read files. I don't need the api to create news files. 