#include <string>
#include <vector>
#include <sstream> // ostringstream
#include <filesystem> // directory_iterator
#include <fstream> // ifstream

using std::string;
using std::vector;
using std::to_string;
using std::stringstream;
using std::ifstream;

#define CARD_BASE_PATH "/proc/asound/card"


void getDeviceInfoPaths(int card, string subdir, vector<string> &infoPath_p, vector<string> &infoPath_c)
{
	// adapted from: https://stackoverflow.com/a/612176
	string cardPath = CARD_BASE_PATH+to_string(card);
	for(const auto &entry : std::__fs::filesystem::directory_iterator(cardPath))
	{
		string devicePath = entry.path();
		string deviceFolder = entry.path().filename();

		// all device folders end with a digit [part of the device num] and either 'p' [for playback] or 'c' [for capture]
		char last = deviceFolder.back();
		char secondLast = deviceFolder[deviceFolder.length()-2];
		if(isdigit(secondLast))
		{
			stringstream pathstream;
			pathstream << cardPath << "/" << deviceFolder << "/" << subdir; // assemble info file path
			if(last=='p')
				infoPath_p.push_back(pathstream.str());
			else if(last=='c')
				infoPath_c.push_back(pathstream.str());
			else // not a device folder, least likely case
				continue;
		}
	}
}

string getDeviceInfoPath(int card, int device, string subdir, bool is_playback)
{
    string dir = (is_playback) ? "p" : "c";
    return CARD_BASE_PATH+to_string(card)+"/pcm"+to_string(device)+dir+"/"+subdir;
}

int getDeviceInfo(string infoFilePath, string infoString, string &info)
{
	// open file
	ifstream infoFile;
	infoFile.open(infoFilePath);
	if( !infoFile.is_open() )
		return -1;

	string line;
	// let's look for infoString, line by line
	while(infoFile) 
	{
		getline(infoFile, line);
		size_t pos = line.find(infoString);
		if(pos!= string::npos)
		{
			info = line.substr(pos+infoString.length()); // remove substring	
			return 0;
		}
	}
	return -2;
}

size_t trimDeviceInfo(string &device_info)
{
    // make sure that there are no spaces in the name
	// get rid of everything after the first space
	size_t pos = device_info.find(" ");
	device_info = device_info.substr(0, pos);
    return pos;
}