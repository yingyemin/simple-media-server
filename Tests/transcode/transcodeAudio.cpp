#include "../../Src/Ffmpeg/TranscodeAudio.h"
#include "../../Src/Codec/AacTrack.h"

int main(int argc,char *argv[])
{
	FILE* fpIn = fopen(argv[1], "rb");
	FILE* fpOut = fopen(argv[2], "wb");
	char buf[1024*1024] = {0};

	auto decodeTrack = make_shared<TrackInfo>();
	decodeTrack->channel_ = 1;
	decodeTrack->samplerate_ = 8000;
	decodeTrack->codec_ = "g711a";
	decodeTrack->trackType_ = "audio";

	auto decoder = make_shared<AudioDecoder>(decodeTrack);

	auto encodeTrack = make_shared<AacTrack>();
	encodeTrack->channel_ = 2;
	encodeTrack->samplerate_ = 44100;
	encodeTrack->codec_ = "aac";
	encodeTrack->trackType_ = "audio";
	encodeTrack->_bitrate = 0;

	auto encoder = make_shared<AudioEncoder>(encodeTrack);

	decoder->setOnDecode([encoder](const FFmpegFrame::Ptr & frame) {
		encoder->inputFrame(frame, true);
	});
	//_audio_enc->setOnEncode([this](const Frame::Ptr& frame) {
	encoder->setOnPacket([fpOut](const FrameBuffer::Ptr& frame) {
		fwrite(frame->data(), frame->size(), 1, fpOut);
	});

	uint64_t pts = 0;
	while (true) {
        int size = fread(buf, 1, 4000, fpIn);
        //cout << "get a frame. size is: " << size << endl;
        if (size <= 0) {
            break;
        }
		FrameBuffer::Ptr frame = make_shared<FrameBuffer>();
        frame->_buffer.assign(buf, size);
		frame->_dts = frame->_pts = pts;
		pts += 500; //采样率8000，所以4000个字节相当于500ms
        decoder->inputFrame(frame, true, true, false);
    }

	fclose(fpIn);
	fclose(fpOut);

	return 0;
}