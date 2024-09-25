#include "../../Src/Ffmpeg/TranscodeAudio.h"

int main(int argc,char *argv[])
{
	FILE* fpIn = fopen(argv[1], "rb");
	FILE* fpOut = fopen(argv[2], "wb");
	char buf[1024*1024] = {0};

	AudioEncodeOption option;
	
	option.codec_ = "libfdk_aac";
	option.codeId_ = AV_CODEC_ID_AAC;
	option.sampleRate_ = 44100;
	
	TranscodeAudio transcode(option, AV_CODEC_ID_PCM_ALAW);
	transcode.setOnPacket([fpOut](const StreamBuffer::Ptr &packet){
		fwrite(packet->data(), packet->size(), 1, fpOut);
	});
	transcode.init();

	while (true) {
        int size = fread(buf, 1, 4096, fpIn);
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