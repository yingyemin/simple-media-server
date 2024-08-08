#ifndef H264Nal_H
#define H264Nal_H

uint32_t Ue(unsigned char *pBuff, uint32_t nLen, uint32_t &nStartBit);
int Se(unsigned char *pBuff, uint32_t nLen, uint32_t &nStartBit);
unsigned long u(uint32_t BitCount,unsigned char * buf,uint32_t &nStartBit);
void de_emulation_prevention(unsigned char* buf,unsigned int* buf_size);
bool h264_decode_sps(unsigned char * buf,unsigned int nLen,int &width,int &height,int &fps);

#endif //H264Nal_H
