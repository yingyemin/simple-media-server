#ifndef SdpParser_H
#define SdpParser_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

using namespace std;

class SdpMedia
{
public:
    int payloadType_;
    int samplerate_;
    int channel_;
    int index_;
    string trackType_;
    string codec_;
    string fmtp_;
    string control_;
    unordered_map<char, string> mapmedia_;
    unordered_map<string, string> mapAttr_;
};

class SdpTitle
{
public:
    unordered_map<char, string> mapTitle_;
    unordered_map<string, string> mapAttr_;
};

class RtspSdpParser {
public:
    void parse(const string& sdp);

public:
    
    string _sdp;
    shared_ptr<SdpTitle> _title;
    // index , SdpMedia
    vector<shared_ptr<SdpMedia>> _vecSdpMedia;
};



#endif //RtspSplitter_H
