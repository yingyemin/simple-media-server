#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspSdpParser.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

void RtspSdpParser::parse(const string& sdp)
{
    _sdp = sdp;
    int parseTitle = 1;
    _title = make_shared<SdpTitle>();
    shared_ptr<SdpMedia> media;

    auto sdpLine = split(sdp, "\r\n");
    for (int i = 0; i < sdpLine.size(); ++i) {
        auto &line = sdpLine[i];
        if (line.size() < 2 || line[1] != '=') {
            continue;
        }
        auto key = line[0];
        // auto value = line.substr(2);
        switch(parseTitle) {
            case 1 : {
                if (key == 'm'){
                    parseTitle = 0;
                } else {
                    _title->mapTitle_.emplace(key, line.substr(2));
                    break;
                }
            } 
            case 0 :  {
                switch (key) {
                    case 'm' : {
                        media = make_shared<SdpMedia>();
                        auto vecM = split(line.substr(2), " ");
                        if (vecM.size() < 4) {
                            // THROW
                            return ;
                        }
                        media->trackType_ = vecM[0];
                        media->port_ = stoi(vecM[1]);
                        media->payloadType_ = stoi(vecM[3]);
                        media->index_ = _vecSdpMedia.size();
                        _vecSdpMedia.emplace_back(media);
                        break;
                    }
                    case 'a' : {
                        auto strA = line.substr(2);
                        auto pos = strA.find_first_of(":");
                        if (pos != string::npos) {
                            auto key = strA.substr(0, pos);
                            if (key == "rtpmap") {
                                auto value = strA.substr(pos + 1);
                                pos = value.find_first_of(" ");
                                if (pos == string::npos) {
                                    break;
                                }
                                auto pt = value.substr(0, pos);
                                if (pt != to_string(media->payloadType_)) {
                                    media->mapAttr_.emplace(key, value);
                                } else {
                                    auto codec = value.substr(pos + 1);
                                    auto vec = split(codec, "/");
                                    if (vec.size() < 2) {
                                        break;
                                    }
                                    media->codec_ = vec[0];
                                    media->samplerate_ = stoi(vec[1]);
                                    if (vec.size() == 3) {
                                        media->channel_ = stoi(vec[2]);
                                    }
                                }
                            } else if (key == "fmtp") {
                                auto value = strA.substr(pos + 1);
                                pos = value.find_first_of(" ");
                                if (pos == string::npos) {
                                    break;
                                }
                                auto pt = value.substr(0, pos);
                                if (pt != to_string(media->payloadType_)) {
                                    media->mapAttr_.emplace(key, value);
                                } else {
                                    media->fmtp_ = value.substr(pos + 1);
                                }
                            } else if (key == "control") {
                                media->control_ = strA.substr(pos + 1);
                            } else {
                                media->mapAttr_.emplace(key, strA.substr(pos + 1));
                            }
                        }
                    }
                    default : {
                        media->mapmedia_.emplace(key, line.substr(2));
                    }
                }
            }
        }
    }
}

// void RtspSdpParser::parse(const string& sdp)
// {
//     _sdp = sdp;
//     int stage = 1;
//     _title = make_shared<SdpTitle>();
//     shared_ptr<SdpMedia> media;

//     auto sdpLine = split(sdp, "\r\n");
//     for (int i = 0; i < sdpLine.size(); ++i) {
//         auto &line = sdpLine[i];
//         if (line.size() < 2 || line[1] != '=') {
//             continue;
//         }
//         auto key = line[0];
//         // auto value = line.substr(2);
//         if (stage == 1) {
//             switch (key) {
//                 case 'm' : {
//                     stage = 2;
//                     --i;
//                     break;
//                 }
//                 default : {
//                     _title->mapTitle_.emplace(key, line.substr(2));
//                 }
//             }
//         } else if (stage == 2) {
//             switch (key) {
//                 case 'm' : {
//                     // if (media) {
//                     //     media->index_ = _vecSdpMedia.size();
//                     //     _vecSdpMedia.emplace_back(media);
//                     // }
//                     media = make_shared<SdpMedia>();
//                     auto vecM = split(line.substr(2), " ");
//                     if (vecM.size() < 4) {
//                         // THROW
//                         return ;
//                     }
//                     media->trackType_ = vecM[0];
//                     media->port_ = stoi(vecM[1]);
//                     media->payloadType_ = stoi(vecM[3]);
//                     media->index_ = _vecSdpMedia.size();
//                     _vecSdpMedia.emplace_back(media);
//                     break;
//                 }
//                 case 'a' : {
//                     auto strA = line.substr(2);
//                     auto pos = strA.find_first_of(":");
//                     if (pos != string::npos) {
//                         auto key = strA.substr(0, pos);
//                         if (key == "rtpmap") {
//                             auto value = strA.substr(pos + 1);
//                             pos = value.find_first_of(" ");
//                             if (pos == string::npos) {
//                                 break;
//                             }
//                             auto pt = value.substr(0, pos);
//                             if (pt != to_string(media->payloadType_)) {
//                                 media->mapAttr_.emplace(key, value);
//                             } else {
//                                 auto codec = value.substr(pos + 1);
//                                 auto vec = split(codec, "/");
//                                 if (vec.size() < 2) {
//                                     break;
//                                 }
//                                 media->codec_ = vec[0];
//                                 media->samplerate_ = stoi(vec[1]);
//                                 if (vec.size() == 3) {
//                                     media->channel_ = stoi(vec[2]);
//                                 }
//                             }
//                         } else if (key == "fmtp") {
//                             auto value = strA.substr(pos + 1);
//                             pos = value.find_first_of(" ");
//                             if (pos == string::npos) {
//                                 break;
//                             }
//                             auto pt = value.substr(0, pos);
//                             if (pt != to_string(media->payloadType_)) {
//                                 media->mapAttr_.emplace(key, value);
//                             } else {
//                                 media->fmtp_ = value.substr(pos + 1);
//                             }
//                         } else if (key == "control") {
//                             media->control_ = strA.substr(pos + 1);
//                         } else {
//                             media->mapAttr_.emplace(key, strA.substr(pos + 1));
//                         }
//                     }
//                 }
//                 default : {
//                     media->mapmedia_.emplace(key, line.substr(2));
//                 }
//             }
//         }
//     }
//     // if (media) {
//     //     // if (media->codec_.empty()) {
//     //     //     if (media->payloadType_ == 97) {
//     //     //         media->codec_ = "mpeg4-generic";
//     //     //     } else {
//     //     //         return ;
//     //     //     }
//     //     // }
//     //     media->index_ = _vecSdpMedia.size();
//     //     _vecSdpMedia.emplace_back(media);
//     // }
// }
