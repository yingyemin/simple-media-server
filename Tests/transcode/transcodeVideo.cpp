#include "../../Src/Ffmpeg/TranscodeVideo.h"

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

	exit(0);
}

// h265 转 h264
int main(int argc,char *argv[])
{
	FILE* fpIn = fopen(argv[1], "rb");
	FILE* fpOut = fopen(argv[2], "wb");
	char buf[1024*1024] = {0};

	VideoEncodeOption option;
	// 如果要264转265，这里改成libx265
	option.codec_ = "libx264";
	// 如果要264转265，这里改成AV_CODEC_ID_H264
	TranscodeVideo transcode(option, AV_CODEC_ID_H265);
	transcode.setOnPacket([fpOut](const StreamBuffer::Ptr &packet){
		fwrite(packet->data(), packet->size(), 1, fpOut);
	});
	transcode.initDecode();

	while (true) {
        int size = getNextNalu(fpIn, (unsigned char*)buf);
        //cout << "get a frame. size is: " << size << endl;
        if (size <= 0) {
            break;
        }
		FrameBuffer::Ptr frame = make_shared<FrameBuffer>();
        frame->_buffer.assign(buf, size);
        transcode.inputFrame(frame);
    }

	fclose(fpIn);
	fclose(fpOut);

	return 0;
}