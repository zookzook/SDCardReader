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

Reading memory from the sd card is really simple. You can read 512 bytes at once. By specifing
the address of the memory block you will get the whole 512 bytes. Therefore 512 bytes of the ram is
needed for reading. But we need more information. Reading the data from the sd card is not enough. 
We need to understand the FAT(16/32) file system. The sd cards are formatted in FAT16 or FAT32 format. 
If you have a 2 gb sd card then it will be formatted with FAT16. To open a file and to read it's
content you have to read some sectors of the sd card to locate the data of the file:

* Find the root directory
* Find the first sector/cluster
* Read the data
* Locate the next sector/cluster
* Read the data until the end of file is reached



