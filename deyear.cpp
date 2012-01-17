#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <taglib/taglib.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstringlist.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2header.h>
#include <taglib/textidentificationframe.h>

// Return frame of specified type or NULL
TagLib::ID3v2::Frame* frameOfType (const TagLib::String& s, const TagLib::ID3v2::FrameListMap& fmap)
{
	return fmap[s.data( TagLib::String::Latin1 ) ].isEmpty()
		? NULL
		: fmap[s.data( TagLib::String::Latin1 )].front();
}

// Checks for non-empty frame
bool isFrameOk (const TagLib::ID3v2::Frame* frame)
{
	return frame && !frame->toString().isEmpty();
}

// Find any existing frame from the list
bool isAnyOfFrames(const TagLib::StringList& possibleFrames, const TagLib::ID3v2::FrameListMap& fmap)
{
	for (TagLib::StringList::ConstIterator it = possibleFrames.begin(); it != possibleFrames.end(); it++)
		if (frameOfType( *it, fmap ))
			return true;
	return false;
}

// Locate among specified frame types non-empty frame with an year.
// If last argument is specified exclude it from the search
TagLib::ID3v2::Frame* findFrameWithYear (const TagLib::StringList& possibleFrames, const TagLib::ID3v2::FrameListMap& fmap,
			const TagLib::String exceptThisFrame)
{
	for (TagLib::StringList::ConstIterator it = possibleFrames.begin(); it != possibleFrames.end(); it++)
	{
		if (!(*it == exceptThisFrame))
		{
			TagLib::ID3v2::Frame* frame = frameOfType( *it, fmap );
			if (isFrameOk( frame ))
				return frame;
		}
	}
	return NULL;
}

// Add text frame T* to the tag
void addTextFrame (const TagLib::String& frameId, const TagLib::String year, TagLib::ID3v2::Tag* tag)
{
	TagLib::ID3v2::TextIdentificationFrame* newFrame = new TagLib::ID3v2::TextIdentificationFrame( frameId.data( TagLib::String::Latin1 ) );
	newFrame->setText( year );
	tag->addFrame( newFrame );
}

int main(int argc, char* argv[])
{
	if (argc == 1 || argc > 3)
	{
		std::cout << "Usage: deyear [yes] mp3-file" << std::endl;
		return 1;
	}

	const bool writeMode = argv[1] && !strcmp( argv[1], "yes" );
	const char* filename = writeMode ? argv[2] : argv[1];
	//if (writeMode)
	//	std::cout << "WRITE MODE" << std::endl;

	TagLib::MPEG::File file(filename);
	TagLib::ID3v2::Tag *tag = file.ID3v2Tag();

	if (!tag)
	{
		std::cout << "No tag here" << std::endl;
		return 1;
	}

	//std::cout << filename << std::endl;
	//std::cout << "ID3v2." << tag->header()->majorVersion() << "." << tag->header()->revisionNumber() << std::endl;
	
	const bool isTag24 = tag->header()->majorVersion() == 4;

	// Most popular frame must be first
	TagLib::StringList years23;
	years23.append( "TYER" );
	years23.append( "TORY" );
	years23.append( "TDAT" );
	years23.append( "TIME" );
	years23.append( "TRDA" );

	TagLib::StringList years24;
	years24.append( "TDRC" );
	years24.append( "TDRL" );
	years24.append( "TDOR" );

	assert(years24.front() == "TDRC");

	// Check whether there is a frame of any listed types
	bool isYear23 = isAnyOfFrames( years23, tag->frameListMap() );
	bool isYear24 = isAnyOfFrames( years24, tag->frameListMap() );

	bool modified = false;

	if (writeMode && false)
	{
		std::cout << "Before: " << std::endl;
		for (TagLib::ID3v2::FrameList::ConstIterator it = tag->frameList().begin(); it != tag->frameList().end(); it++)
			std::cout << "  " << (*it)->frameID() << " - \"" << (*it)->toString() << "\"" << std::endl;
	}

	if (!isTag24)
		std::cout << "Fail. Not 2.4 tag: " << filename << std::endl;
	else
	{
		bool shouldDrop23 = false;

		// Test for multiple T* frames.
		// According to 2.4 standard there must be only one T* frame of each kind
		for (TagLib::StringList::ConstIterator it = years24.begin(); it != years24.end(); it++)
		{
			const TagLib::ID3v2::FrameList& fl = tag->frameListMap()[it->data( TagLib::String::Latin1 ) ];
			if (fl.size() > 1)
			{
				TagLib::String year = fl.front()->toString();
				std::cout << "Damaged. Multiple " << *it << " frames found " << std::endl;
				std::cout << "Receipt. Remove all duplicated frames and replace with " << year << std::endl;
				
				TagLib::ID3v2::Frame* frame = NULL;
				do 
				{
					frame = frameOfType( *it, tag->frameListMap() );
					if (frame)
						tag->removeFrame( frame );
				} while (frame);
				addTextFrame( years24.front(), year, tag );

				modified = true;
			}
		}

		// Set 2.4 frames from existing 2.3 frames 
		if (isYear23 && !isYear24)
		{
			std::cout << "Damaged. 2.3 year exists but no 2.4 year" << std::endl;
			TagLib::ID3v2::Frame* frameWithYear = findFrameWithYear( years23, tag->frameListMap(), TagLib::String() );
			if (frameWithYear)
			{
				std::cout << "Receipt: set TDRC tag from existing: " << frameWithYear->toString() << std::endl;
				addTextFrame( years24.front(), frameWithYear->toString(), tag );
				modified = true;
				shouldDrop23 = true;
			}
		}

		// Drop 2.3 frames
		if (isYear23 && isYear24)
		{
			std::cout << "Damaged. both 2.3 and 2.4 years" << std::endl;
			shouldDrop23 = true;
		}

		if (shouldDrop23)
		{
			std::cout << "Receipt: remove 2.3 year tags " << std::endl;
			for (TagLib::StringList::ConstIterator it = years23.begin(); it != years23.end(); it++)
			{ 
				TagLib::ID3v2::Frame* frame = frameOfType( *it, tag->frameListMap() );
				if (frame)
					tag->removeFrame( frame );
			}
			modified = true;
		}

		// Sometimes TDRC is set empty but other 2.4 year tags are not empty
		// Set TDRC from them
		TagLib::ID3v2::Frame* frameTDRC = frameOfType( years24.front(), tag->frameListMap() );
		if (isYear24 && !isFrameOk( frameTDRC ))
		{
			if (frameTDRC)
			{
				std::cout << "Damaged. Main TDRC tag is empty" << std::endl;
				std::cout << "Receipt: remove empty TDRC " << std::endl;
				tag->removeFrame( frameTDRC );
				modified = true;
			}

			TagLib::ID3v2::Frame* frameWithYear = findFrameWithYear( years24, tag->frameListMap(), years24.front() );
			if (frameWithYear)
			{
				std::cout << "Damaged. TDRC is empty but others are not" << std::endl;
				std::cout << "Receipt: set TDRC tag from existing: " << frameWithYear->toString() << std::endl;
				addTextFrame( years24.front(), frameWithYear->toString(), tag );
				modified = true;
			}
		}
	}

	if (modified)
	{
		std::cout << "After: " << std::endl;
		for (TagLib::ID3v2::FrameList::ConstIterator it = tag->frameList().begin(); it != tag->frameList().end(); it++)
			std::cout << "  " << (*it)->frameID() << " - \"" << (*it)->toString() << "\"" << std::endl;

		// Actually write to file
		if (writeMode)
		{
			std::cout << "Writing to file: " << filename << std::endl;
			file.save( TagLib::MPEG::File::ID3v2 );
			std::cout << "Ok\n" << std::endl;
		}
	}

}

