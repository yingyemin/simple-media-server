#include <iostream>

#include "CustomInput.h"

using namespace std;

static int findStartCode(unsigned char *buf, int zeros_in_startcode)
{
    int info;
    int i;
 
    info = 1;
    for (i = 0; i < zeros_in_startcode; i++)
        if (buf[i] != 0)
            info = 0;
 
    if (buf[i] != 1)
        info = 0;
    return info;
}

static int getNextNalu(FILE* inpf, unsigned char* buf)
{
    int pos = 0;
    int startCodeFound = 0;
    int info2 = 0;
    int info3 = 0;
 
    while (!feof(inpf) && (buf[pos++] = fgetc(inpf)) == 0);
 
    while (!startCodeFound)
    {
        if (feof(inpf))
        {
            return pos - 1;
        }
        buf[pos++] = fgetc(inpf);
        info3 = findStartCode(&buf[pos - 4], 3);
        startCodeFound = (info3 == 1);
        if (info3 != 1)
            info2 = findStartCode(&buf[pos - 3], 2);
        startCodeFound = (info2 == 1 || info3 == 1);
    }
    if (info2)
    {
        fseek(inpf, -3, SEEK_CUR);
        return pos - 3;
    }
    if (info3)
    {
        fseek(inpf, -4, SEEK_CUR);
        return pos - 4;
    }

	return -1;
}

int main(int argc, char** argv)
{
    auto customInput = make_shared<CustomInput>(argv[1]);
    if (!customInput->init()) {
        return -1;
    }

    customInput->addVideoTrack("h265");
    customInput->addAudioTrack("g711a", 8000, 1, 8);

    std::string videoName = "video.h265";
    FILE* videoFp = fopen(videoName.c_str(), "rb");
    if (!videoFp) {
        return -1;
    }

    std::string audioName = "audio.g711a";
    FILE* audioFp = fopen(audioName.c_str(), "rb");
    if (!audioFp) {
        return -1;
    }

    TimeClock timeClock;
    int dts = 0;

    unsigned char videoData[1024*1024];
    char audioData[320];

    while (true)
    {
        auto timeCost = timeClock.startToNow();
        if (dts < timeCost) {
            dts += 40;
            auto size = getNextNalu(videoFp, videoData);
            if (size < 0) {
                break;
            }
            logTrace << "onframe video";
            customInput->onFrame((uint8_t*)videoData, size, dts, dts, 1);

            size = fread(audioData, 1, 320, audioFp);
            if (size != 320) {
                break;
            }
            logTrace << "onframe audio";
            customInput->onFrame((uint8_t*)audioData, size, dts, dts, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    
    return 0;
}
