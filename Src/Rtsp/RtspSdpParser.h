#ifndef SdpParser_H
#define SdpParser_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

// using namespace std;

class SdpMedia
{
public:
    int payloadType_;
    int samplerate_;
    int channel_;
    int index_;
    int port_;
    std::string trackType_;
    std::string codec_;
    std::string fmtp_;
    std::string control_;
    std::unordered_map<char, std::string> mapmedia_;
    std::unordered_map<std::string, std::string> mapAttr_;
};

class SdpTitle
{
public:
    std::unordered_map<char, std::string> mapTitle_;
    std::unordered_map<std::string, std::string> mapAttr_;
};

class RtspSdpParser {
public:
    void parse(const std::string& sdp);

public:
    
    std::string _sdp;
    std::shared_ptr<SdpTitle> _title;
    // index , SdpMedia
    std::vector<std::shared_ptr<SdpMedia>> _vecSdpMedia;
};



#endif //RtspSplitter_H
