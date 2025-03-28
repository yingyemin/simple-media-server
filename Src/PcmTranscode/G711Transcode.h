#ifndef _IMA_G711_H_
#define _IMA_G711_H_

class G711Transcode
{
public:
    static void pcm_2_alaw(const short *src_16lepcm, unsigned char *dst_alaw, unsigned int sample_cnt);
    static void alaw_2_pcm(const unsigned char *src_alaw, short *dst_16lepcm, unsigned int sample_cnt);
    static void pcm_2_ulaw(const short *src_16lepcm, unsigned char *dst_ulaw, unsigned int sample_cnt);
    static void ulaw_2_pcm(const unsigned char *src_ulaw, short *dst_16lepcm, unsigned int sample_cnt);
    static void alaw_2_ulaw(const unsigned char *src_alaw, char *dst_ulaw, unsigned int sample_cnt);
    static void ulaw_2_alaw(const unsigned char *src_ulaw, char *dst_alaw, unsigned int sample_cnt);
};

#endif
/*- End of file ------------------------------------------------------------*/

