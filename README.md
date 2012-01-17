deyear
=====

Fix buggy year text frames for ID3 v2.4 tags.

Trivia
------

2.3 year tags:

 * TYER - **year**
 * TORY - original release year
 * TDAT - recording date
 * TIME - recording time
 * TRDA - recording dates

2.4 year tags:

 * TDRC - **recording time**  
 * TDRL - release time
 * TDOR - original release time

Changes:

 * TDAT,TIME,TRDA,TYER -> TDRC
 * TORY -> TDOR
 * air -> TDRL

You can notice a lot of ID3 v2.3 tags are not allowed for 2.4 tag.
Sometimes buggy software writes 2.3 year frames to the new 2.4 tag
so it become broken. Sometimes year is not displayed in your player.
You can check a state of your tags for sure with the eyeD3 python program.
Look [here](https://bugs.launchpad.net/ubuntu/+source/audacity/+bug/794308) for example.

deyear
-----

This little program tries to fix buggy 2.4 tags with some heuristics:

 * remove TYER and other 2.3-specific frames
 * removes empty ("empty" means empty string) frames
 * migrate old 2.3 frame to 2.4
 * copy TDRL tag to TDRC (maybe it isn't correct)
 * remove duplicated frames (standard allows only one frame of each type)

How to run
------

First compile with latest TagLib library.

<code>
	g++ deyear.cpp -ltag -o deyear
</code>

Checked with Linux and OS X.

Then launch:

<code>
	deyear [yes] mp3-file
</code>

Specify "yes" switch for actual writing of the tag.


References
-------

[ID3 v2.4 frames](http://www.id3.org/id3v2.4.0-frames)

[ID3 v2.3 frames](http://www.id3.org/d3v2.3.0)
