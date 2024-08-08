/*
这个demo实现的是将h264裸流封装成ts文件(也可以是m3u8和ts文件)
由于我们的摄像头数据只含有以下帧类型
PPS  (0x00 00 00 01 68),
SPS  (0x00 00 00 01 67),
SEI  (0x00 00 00 01 66),
IDR  (0x00 00 00 01 65),
非IDR(0x00 00 00 01 41),
帧只有帧开始（0x00 00 00 01）没有帧中（0x00 00 01),
因此这个demo不会处理其他帧情况，如果输入文件含有其他帧会有问题。
重要：这个demo没有PCR信息，其他信息能省则省，毕竟规范文件内容太多。
测试1.——————————————————————————————————
输出结果.ts文件通过VLC播放器本地播放有跳帧的情况，VLC播放器给出的信息是:
main error: Invalid PCR value in ES_OUT_SET_(GROUP_)PCR !
......
main error: Invalid PCR value in ES_OUT_SET_(GROUP_)PCR !
direct3d9 error: SetThumbNailClip failed: 0x800706f4
avcodec info: Using DXVA2 (Intel(R) HD Graphics, vendor Intel(8086), device 402, revision 6) for hardware decoding
direct3d9 error: SetThumbNailClip failed: 0x800706f4
这应当是我们忽略了PCR导致的，不过使用android（版本8.1.0，手头没有其他版本的android机测试）和ios手机浏览器或视频播放器可以正常播放。
造成VLC播放异常的主要是PCR，这个PCR没怎么去了解。
测试2.——————————————————————————————————
.m3u8文件在本地貌似播放不了（试过一些播放器都不行）
然后用nginx的rtmp模块搭了一个hls点播服务器
1.使用pc端的VLC播放器打开网络串流，画面一闪而过，看似只有前面的1帧或几帧播放成功，后面都是黑屏
2.使用手机的浏览器打开能够正常播放
综述.——————————————————————————————————
由于是极简封装，存在一些发现或未发现的小问题，比如PCR
PC端VLC不能正常播放。
手机浏览器和视频播放器可以正常的播放。(貌似现在手机都兼容了safari浏览器？)
html5没有测试过。
—————————————————————————————————————
written by dyan1024
date:2019-1-2
*/

// #include "stdafx.h"    //在linux下注释掉这一行
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <math.h>
#ifdef _WIN32
#include <winsock.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include <arpa/inet.h>
#endif /*_WIN32*/
using namespace std;
FILE *bits = NULL;                //.h264输入文件
FILE *fw = NULL;                  //.ts输出文件
FILE *m3u8 = NULL;                  //.m3u8输出文件
static int FindStartCode2(unsigned char *Buf);//查找开始字符0x000001
static int FindStartCode3(unsigned char *Buf);//查找开始字符0x00000001

static int info2 = 0, info3 = 0;

#define BUF_SIZE 1024*1024*3
#define Program_map_PID 0x0003    //pmt节目pid，也会关联到pat表
#define Elementary_PID 0x0012    //pes流类型对应的pid,也会关联到pmt表
#define TS_PACKET_LEN 188    //ts包长度固定188字节
#define M3U8_TS_LEN 32    //m3u8文件每行最大长度
#define IDR_PER_TSFILE 10 //生成hls时每多少个关键帧一个.ts文件（ts包和.ts文件不同，.ts包含很多ts包，而一个ts包固定188字节）
/*用于存一帧h264数据*/
struct frame_buf {
    unsigned char * p_buf;
    int len;
    int max;
    frame_buf() {
        p_buf = new unsigned char[BUF_SIZE];//我们摄像头图像大小是640*480*3，就算不压缩，这个数组完全足够存一帧
        len = 0;
        max = BUF_SIZE;
    }
    ~frame_buf()
    {
        delete[] p_buf;
        p_buf = NULL;
    }
};
/*用于存一个ts包（188字节）*/
struct pes_packet {
    unsigned char *p_buf;
    int max;
    pes_packet() {
        p_buf = new unsigned char[TS_PACKET_LEN];
        max = TS_PACKET_LEN;
    }
    ~pes_packet()
    {
        delete[] p_buf;
        p_buf = NULL;
    }
};
/*用于存m3u8文件中标记.ts文件时长及文件名*/
struct m3u8_ts {
    float ts_time;
    char ts_name[M3U8_TS_LEN];
    m3u8_ts() {
        memset(&ts_name, 0, M3U8_TS_LEN);
    }
};
/*用于缓存m3u8文件内容*/
struct m3u8_text {
    char extm3u[M3U8_TS_LEN];
    char ext_x_version[M3U8_TS_LEN];
    char ext_x_allow_cache[M3U8_TS_LEN];
    char ext_x_targetduration[M3U8_TS_LEN];
    char ext_x_media_sequence[M3U8_TS_LEN];
    char ext_x_playlist_type[M3U8_TS_LEN];
    list<m3u8_ts> l_m3u8_ts;
    char extinf[M3U8_TS_LEN];
    char ext_x_endlist[M3U8_TS_LEN];
    m3u8_text() {
        memset(extm3u, 0, M3U8_TS_LEN);
        memcpy(extm3u, "#EXTM3U\n", strlen("#EXTM3U\n"));
        memset(ext_x_version, 0, M3U8_TS_LEN);
        memcpy(ext_x_version, "#EXT-X-VERSION:3\n", strlen("#EXT-X-VERSION:3\n"));
        memset(ext_x_allow_cache, 0, M3U8_TS_LEN);
        memcpy(ext_x_allow_cache, "#EXT-X-ALLOW-CACHE:YES\n", strlen("#EXT-X-ALLOW-CACHE:YES\n"));
        memset(ext_x_targetduration, 0, M3U8_TS_LEN);
        memcpy(ext_x_targetduration, "#EXT-X-TARGETDURATION:%d\n", strlen("#EXT-X-TARGETDURATION:%d\n"));
        memset(ext_x_media_sequence, 0, M3U8_TS_LEN);
        memcpy(ext_x_media_sequence, "#EXT-X-MEDIA-SEQUENCE:%d\n", strlen("#EXT-X-MEDIA-SEQUENCE:%d\n"));
        memset(ext_x_playlist_type, 0, M3U8_TS_LEN);
        memcpy(ext_x_playlist_type, "#EXT-X-PLAYLIST-TYPE:VOD\n", strlen("#EXT-X-PLAYLIST-TYPE:VOD\n"));    //点播
        memset(extinf, 0, M3U8_TS_LEN);
        memcpy(extinf, "#EXTINF:%f,\n", strlen("#EXTINF:%f,\n"));
        memset(ext_x_endlist, 0, M3U8_TS_LEN);
        memcpy(ext_x_endlist, "#EXT-X-ENDLIST\n", strlen("#EXT-X-ENDLIST\n"));
    }
};

frame_buf *p_pesframe = NULL;
unsigned long long pts = 1; //初始值似乎是随便定义的，第一帧最好不要为0，这个初始值pts+pts_per_frame不要等于0就行
unsigned int frame_rate = 90;    //视频帧率不太清楚，随便给个值在这里，只是影响播放时快慢的问题(在VLC播放器上还会影响播放效果，比如设置为20，播放时不是跳帧，而是从头到尾只有1帧，估计还是PCR的问题)
unsigned int pts_per_frame = 90000 / frame_rate;
unsigned long long pts_max = pow(2, 33);

unsigned int crc32Table[256] = { 0 };
int MakeTable(unsigned int *crc32_table)
{
    for (unsigned int i = 0; i < 256; i++) {
        unsigned int k = 0;
        for (unsigned int j = (i << 24) | 0x800000; j != 0x80000000; j <<= 1) {
            k = (k << 1) ^ (((k ^ j) & 0x80000000) ? 0x04c11db7 : 0);
        }

        crc32_table[i] = k;
    }
    return 0;
}
unsigned int Crc32Calculate(u_char *buffer, unsigned int size, unsigned int *crc32_table)
{
    unsigned int crc32_reg = 0xFFFFFFFF;
    for (unsigned int i = 0; i < size; i++) {
        crc32_reg = (crc32_reg << 8) ^ crc32_table[((crc32_reg >> 24) ^ *buffer++) & 0xFF];
    }
    return crc32_reg;
}

void OpenBitstreamFile(char *fn)
{
    if (NULL == (bits = fopen(fn, "rb")))
    {
        printf("open file error\n");
        exit(0);
    }
}
void CloseBitstreamFile() {
    fclose(bits);
    bits = NULL;
}
void OpenWriteFile(char *fn) {
    if (NULL == (fw = fopen(fn, "wb")))
    {
        printf("open write file error\n");
        exit(0);
    }
}
void CloseWriteFile() {
    fclose(fw);
    fw = NULL;
}

/*从文件中读取一帧h264裸流数据（I帧或P帧，没有B帧）
@Buf,    [out]已分配内存空间的指针，用于存储一帧图像
@len,    [out]返回Buf的有效长度
return，    -1（异常），0（IDR），1(非IDR)，2（最后一帧，不区分帧类型）*/
int GetOneFrame(unsigned char* Buf, int &len) {
    int StartCodeFound = 0;
    int rewind;
    int nal_unit_type;
    int forbidden_bit;
    int nal_reference_idc;
    len = 0;

    int startcodeprefix_len = 3;//初始化码流序列的开始字符为3个字节

    //从我们摄像头截取的h264数据获知只有帧开始(0x00000001)没有帧中（0x000001）,不处理帧中。
    if (4 != fread(Buf+len, 1, 4, bits))//从码流中读4个字节
    {
        return -1;
    }
    info3 = FindStartCode3(Buf+len);//判断是否为0x00000001
    if (info3 != 1)//如果不是，返回-1
    {
        return -1;
    }
    else {//如果是0x00000001,得到开始字符为4个字节
        len += 4;
        startcodeprefix_len = 4;
    }
    while (!StartCodeFound) {
        //到文件末尾，认为当前帧结束
        if (feof(bits)) {
            break;
        }
        //再读一个字节,知道找到下一个帧开始0x00000001
        Buf[len++] = fgetc(bits);
        info3 = FindStartCode3(&Buf[len - 4]);//判断是否为0x00000001
        //if (info3 != 1)
        //    info2 = FindStartCode2(&Buf[pos - 3]);//判断是否为0x000001
        StartCodeFound = (/*info2 == 1 || */info3 == 1);
        if (StartCodeFound) {
            //到文件末尾，认为当前帧结束
            if (feof(bits)) {
                break;
            }
            //再读一个字节判断帧类型
            Buf[len++] = fgetc(bits);
            //第1位h.264裸流肯定为0，不用管它
            forbidden_bit = Buf[len-1] & 0x80; //(nalu->buf[0]>>7) & 1;
            /*接下来2位，取值0~3，指示这个nalu的重要性，I帧、sps、pps通常取3，P帧通常取2，B帧通常取0.
            对于封装pes包帧重要性关系不大，因为我们实际情况的帧类型比较简单（只有IDR(关键帧),非IDR，SPS，PPS，SEI），主要还是看帧类型，因为我们不管什么帧只要在帧前加个pes头就行了*/
            nal_reference_idc = Buf[len-1] & 0x60; //(nalu->buf[0]>>5) & 3;
            /*最后5位，帧类型：IDR(关键帧),非IDR，SPS，PPS，SEI，分解符，片分区A/B/C，序列结束，码流结束，填充等。
            截取一段摄像头h264裸流分析后发现对于我们摄像头的实时h264裸流只有前5种。
            而封装pes包sps,pps,sei,IDR封装为一个pes包，各个非IDR分别一个pes包，因此检测到下一个IDR或非IDR才返回。*/
            nal_unit_type = (Buf[len-1]) & 0x1f;
            //只要不是非IDR或IDR（SEI，SPS，PPS),不要截断数据，继续读取
            if (1 != nal_unit_type && 5 != nal_unit_type) {
                StartCodeFound = 0;
                continue;
            }
            //检测到下一帧的开始，可以截取当前帧了
            else {
                rewind = (info3 == 1) ? -5 : -4;    //回退一个h264头（4字节（或3字节，帧中我们不考虑））和一个帧类型（1字节）
                if (0 != fseek(bits, rewind, SEEK_CUR))//把文件指针向后退开始字节的字节数
                {
                    printf("Cannot fseek in the bit stream file\n");
                    return -1;
                }
                len += rewind;//帧长减下一帧头0x00000001（或0x000001）的长度
                //区分一下IDR和非IDR，可能要用
                int front_nal_unit_type = Buf[startcodeprefix_len] & 0x1f;
                //非IDR返回1，IDR返回0
                if (1 == front_nal_unit_type) {
                    return 1;
                }
                else if (5 == front_nal_unit_type) {
                    return 0;
                }
            }
        }
    }
    return 2;
}

/*为一帧h264裸流加pes头(pts)
@p_pes,        [out]已提前分配存储空间的指针，用于存储加pes头的h264一帧数据
@Buf,        [int]一帧h264数据
@len,        [in]Buf数据的长度
return,    -1(异常)，0（成功）*/
int PesHead_p(frame_buf *p_pes, unsigned char* Buf, int len) {
    int temp_len = 0;
    p_pes->len = 0;
    memset(p_pes->p_buf, 0, BUF_SIZE);
    /*最后我们是要按字节写的，为了避免大小端的问题最好一个字节一个字节存。或者转为网络字节序
    比如pes start code存成*((unsigned int *)p_pes->p_buf) |= 0x00000100,看上去符合pes的开始标记 00 00 01 00,事实上写到本地后确是00 01 00 00,这是不对的
    */
    *(p_pes->p_buf + 2) = 0x01;    //pes start code,3字节 0x000001
    *(p_pes->p_buf + 3) = 0xe0;    //stream id,1字节 0xe0
                                //*(p_pes->p_buf + 4) = 0x00;
                                //*(p_pes->p_buf + 5) = 0x00;    //pes packet length，2字节 视频数据长度0x0000表示pes包不限制长度
    *(p_pes->p_buf + 6) = 0x80;    //通常取值0x80，表示数据不加密、无优先级、备份的数据等（自己封装怎么简单怎么来）
    *(p_pes->p_buf + 7) = 0x80;    //pts_dts_flags取值0x80（10 00 00 00）表示只含有pts(其中的前2bit(10))，取值0xc0（11 00 00 00）表示含有pts和dts,(前2bit（11）)这个序列1越多后面描述越长（自己封装怎么简单怎么来）。
    *(p_pes->p_buf + 8) = 0x05;    //因为我们只有pts，按协议就是5字节
    temp_len += 9;
    pts += pts_per_frame;    /*按协议pts/dts有效位33bit,不知道为什么33bit(看来需要长整形存储),就算我们只用33bit,可以长达约26.512（（2^33-1）/ 90000 / 3600）个小时*/
    pts %= pts_max;    /*如果超出33bit根据协议超过33bit记录后应该重新从0开始*/
    /*取33bit的pts值的前3位（本来应该右移30，但又需要给最后的maker_bit预留一个位子，所以最后右移29位和0x0e（00 00 11 10）与运算）*/
    char ptsn1_temp = 0x0e & pts >> 29;    //0x0e（00 00 11 10）
    /*我们前面pts_dts_flags是0x80(前2位为10)，则按协议这个字节前4位为0010，后跟pts的前3位，最后接一个maker_bit（1）*/
    char ptsn1 = 0x21 | ptsn1_temp;    //0x21(00 10 00 01)
    unsigned short ptsn2_temp = 0x0001 | (pts >> 14);    //按协议取接下来pts的15bit,后接一个位maker_bit(1),同上。额外注意的是这不是单字节数据类型，最后要转网络字节序
    unsigned short ptsn2 = htons(ptsn2_temp);    //转网络字节序
    unsigned short ptsn3_temp = 0x0001 | (pts << 1);    //按协议取最后pts的15bit,后接一个位maker_bit(1)，同上。额外注意的是这不是单字节数据类型，最后要转网络字节序
    unsigned short ptsn3 = htons(ptsn3_temp);    //转网络字节序
    //将pts写入
    *(p_pes->p_buf + temp_len) = ptsn1;
    temp_len++;
    *(unsigned short*)(p_pes->p_buf + temp_len) = ptsn2;
    temp_len += 2;
    *(unsigned short*)(p_pes->p_buf + temp_len) = ptsn3;
    temp_len += 2;
    //pes头结束，网上传言要在每个pes头后加个0x00000001 0x09(按h264格式这是B帧分解符？) 0x**(最后的**不是00就行)
    unsigned int h264_head = htonl(1);
    *(unsigned int*)(p_pes->p_buf + temp_len) = h264_head;
    temp_len += 4;
    *(p_pes->p_buf + temp_len) = 0x09;
    temp_len++;
    *(p_pes->p_buf + temp_len) = 0xff;
    temp_len++;
    //加上h264裸流,注意数组别越界
    if (p_pes->max < (temp_len + len)) {
        //TODO
        printf("pes buf size allocated is too small\n");
        return -1;
    }
    memcpy(p_pes->p_buf + temp_len, Buf, len);
    p_pes->len = len + temp_len;
    return 0;
}

/*为一帧h264裸流加pes头（pts和dts）
@p_pes,        [out]已提前分配存储空间的指针，用于存储加pes头的h264一帧数据
@Buf,        [int]一帧h264数据
@len,        [in]Buf数据的长度
return,    -1(异常)，0（成功）*/
int PesHead_pd(frame_buf *p_pes, unsigned char* Buf, int len) {
    int temp_len = 0;
    p_pes->len = 0;
    memset(p_pes->p_buf, 0, BUF_SIZE);
    /*最后我们是要按字节写的，为了避免大小端的问题最好一个字节一个字节存。或者转为网络字节序
    比如pes start code存成*((unsigned int *)p_pes->p_buf) |= 0x00000100,看上去符合pes的开始标记 00 00 01 00,事实上写到本地后确是00 01 00 00,这是不对的
    */
    *(p_pes->p_buf + 2) = 0x01;    //pes start code,3字节 0x000001
    *(p_pes->p_buf + 3) = 0xe0;    //stream id,1字节 0xe0
    //*(p_pes->p_buf + 4) = 0x00;
    //*(p_pes->p_buf + 5) = 0x00;    //pes packet length，2字节 视频数据长度0x0000表示pes包不限制长度
    *(p_pes->p_buf + 6) = 0x80;    //通常取值0x80，表示数据不加密、无优先级、备份的数据等（自己封装怎么简单怎么来）
    /*这里主要注意pts_dts_flag不同后面的pts和dts起始标识也会不同*/
    *(p_pes->p_buf + 7) = 0xc0;    //pts_dts_flags取值0xc0（11 00 00 00）表示含有pts和dts(其中前2bit（11）),取值0x80表示只含有pts(其中的前2bit(10))，(前2bit（11）)这个序列1越多后面描述越长（自己封装怎么简单怎么来，我们不要其他的信息了）。
    *(p_pes->p_buf + 8) = 0x0a;    //因为我们有pts和dts，按协议就是10字节,我们这里将pts和dts设置为一样就行
    temp_len += 9;
    pts += pts_per_frame;    /*按协议pts/dts有效位33bit,不知道为什么33bit(看来需要长整形存储),就算我们只用33bit,可以长达约26.512（（2^33-1）/ 90000 / 3600）个小时*/
    pts %= pts_max;    /*如果超出33bit根据协议超过33bit记录后应该重新从0开始*/
    /*取33bit的pts值的前3位（本来应该右移30，但又需要给最后的maker_bit预留一个位子，所以最后右移29位和0x0e（00 00 11 10）与运算）*/
    char ptsn1_temp = 0x0e & pts >> 29;    //0x0e（00 00 11 10）
    /*我们前面pts_dts_flags是0xc0(前2位为11)，则按协议这个字节前4位为0011，后跟pts的前3位，最后接一个maker_bit（1）*/
    char ptsn1 = 0x31 | ptsn1_temp;    //0x31(00 11 00 01)
    unsigned short ptsn2_temp = 0x0001 | (pts >> 14);    //取接下来的15bit,后接一个位maker_bit(1),同上。额外注意的是这不是单字节数据类型，最后要转网络字节序
    unsigned short ptsn2 = htons(ptsn2_temp);    //转网络字节序
    unsigned short ptsn3_temp = 0x0001 | (pts << 1);    //取最后的15bit,后接一个位maker_bit(1)，同上。额外注意的是这不是单字节数据类型，最后要转网络字节序
    unsigned short ptsn3 = htons(ptsn3_temp);    //转网络字节序
    //将pts写入
    *(p_pes->p_buf + temp_len) = ptsn1;
    temp_len++;
    *(unsigned short*)(p_pes->p_buf + temp_len) = ptsn2;
    temp_len += 2;
    *(unsigned short*)(p_pes->p_buf + temp_len) = ptsn3;
    temp_len += 2;
    /*将dts设置为与pts相同*/
    /*pts -= pts_per_frame;*/
    ptsn1 = 0x11 | pts >> 29;    /*取前2bit，我们前面pts_dts_flags是0xc0(前2位为11)，则按协议这个字节前4位为0001，后跟pts的前3位(由于我们是32位的unsigned int,最高位是没有的所以只取前2位，前一位置为0,应该右移30，但又需要给最后的maker_bit预留一个位子，所以最后右移29位)，最后接一个位maker_bit(1)
                                0x11(00 01 00 01)*/
    ptsn2_temp = 0x0001 | (pts >> 14);    //取接下来的15bit,后接一个位maker_bit(1),同上。额外注意的是这不是单字节数据类型，最后要转网络字节序
    ptsn2 = htons(ptsn2_temp);    //转网络字节序
    ptsn3_temp = 0x0001 | (pts << 1);    //取最后的15bit,后接一个位maker_bit(1)，同上。额外注意的是这不是单字节数据类型，最后要转网络字节序
    ptsn3 = htons(ptsn3_temp);    //转网络字节序
    //将dts写入
    *(p_pes->p_buf + temp_len) = ptsn1;
    temp_len++;
    *(unsigned short*)(p_pes->p_buf + temp_len) = ptsn2;
    temp_len += 2;
    *(unsigned short*)(p_pes->p_buf + temp_len) = ptsn3;
    temp_len += 2;

    //pes头结束，网上传言要在每个pes头后加个0x00000001 0x09(按h264格式这是B帧分解符？) 0x**(最后的**不是00就行)
    unsigned int h264_head = htonl(1);
    *(unsigned int*)(p_pes->p_buf + temp_len) = h264_head;
    temp_len += 4;
    *(p_pes->p_buf + temp_len) = 0x09;
    temp_len++;
    *(p_pes->p_buf + temp_len) = 0xff;
    temp_len++;
    //加上h264裸流,注意数组别越界
    if (p_pes->max < (temp_len + len)) {
        //TODO
        printf("pes buf size allocated is too small\n");
        return -1;
    }
    memcpy(p_pes->p_buf + temp_len, Buf, len);
    p_pes->len = len + temp_len;
    return 0;
}

/*将加了pes头的h264拆分成一个个ts包（每个pes包使用独立递增计数器）
@l_pes,        [out]list引用，元素存储空间函数内部分配，用完后需要在外部释放
@Buf,        [in]加pes头的h264数据
return，    -1（异常），0（成功）*/
int PesPacket(list<struct pes_packet*> &l_pes,frame_buf *Buf) {
    if (NULL == Buf || NULL == Buf->p_buf) {
        printf(" frame_buf or frame_buf->p_buf is NULL when pespacket\n");
        return -1;
    }
    unsigned char packet_pes = TS_PACKET_LEN - 4;    //每个ts包除包头后的长度
    unsigned char packet_remain = Buf->len % packet_pes;    //每个ts包长度规定188字节，再去掉ts头长度
    unsigned char packet_num = Buf->len / packet_pes;    //可以剩余可封装出的不含自适应区的包数

    while (!l_pes.empty()) {
        l_pes.pop_front();
    }
    unsigned short temp_1, temp_2, temp_3;
    unsigned int temp_len = 0;
    unsigned int pos = 0;    //pes数据偏移量
    unsigned char pes_count = 0;    //递增计数器（ts头最后4bit，0~f增加，到f后下一个为0）（这里定义一个局部变量，意思是每个pes包都重置递增计数器）

    pes_packet *p = new pes_packet;
    *(p->p_buf) = 0x47;
    temp_len++;
    temp_1 = 0x4000 & 0xe000;
    temp_2 = Elementary_PID & 0x1fff;
    temp_3 = htons(temp_1 | temp_2);
    *(unsigned short*)(p->p_buf + temp_len) = temp_3;
    temp_len += 2;

    /*如果pes包长度对184取余数大于0，需要填充，第一个包既含有自适应区又含有有效载荷*/
    if(packet_remain > 0){
        /*2bit加扰控制（00，无加密），2bit带自适应域（11，带自适应区和有效负载），4bit递增计数器（0000）*/
        *(p->p_buf + temp_len) = 0x30;    //00 11 00 00
        temp_len++;
        /**/
        unsigned char stuff_num = TS_PACKET_LEN - 4 - 1 - packet_remain;    //填充字节数 = ts包长（188字节） - ts头（4字节） - 自适应域长度（1字节） - 有效载荷长度
        *(p->p_buf + temp_len) = stuff_num;
        temp_len++;
        /*如果正好余下183字节，还只有一个调整位长度，前面我们已经设为0，接下来不要再填充*/
        if (stuff_num == 0) {
            //
        }
        else {
            *(p->p_buf + temp_len) = 0x00;    //8bit填充信息flag,我们不要额外的设定信息，填充字节全部0xff
            temp_len++;
            memset(p->p_buf + temp_len, 0xff, stuff_num - 1);    //填充字节数还要再减去填充信息flag的长度
            temp_len += (stuff_num - 1);
        }
        memcpy(p->p_buf + temp_len, Buf->p_buf + pos, packet_remain);
        pos += packet_remain;
    }
    /*第一个包仅含有效载荷.仅含有效载荷，没有自适应区和自适应长度，ts头后直接跟pes数据*/
    else {
        /*2bit加扰控制（00，无加密），2bit带自适应域（01，有效负载），4bit递增计数器（0000）*/
        *(p->p_buf + temp_len) = 0x10;    //00 01 00 00
        temp_len++;
        memcpy(p->p_buf + temp_len, Buf->p_buf + pos, packet_pes);
        pos += packet_pes;
        packet_num--;    //这个包不含自适应，剩余不含自适应区的包数量减一
    }
    /*将第一个包插入list链表*/
    l_pes.push_back(p);
    while (packet_num > 0) {
        pes_packet *p_temp = new pes_packet;
        unsigned int len = 0;
        /*因为第一个包用了0000，所以这里递增计数器先自增*/
        pes_count++;
        pes_count &= 0x0f;    //0~f增加，到f后下一个为0
        *(p_temp->p_buf) = 0x47;
        len++;
        temp_1 = 0x0000 & 0xe000;
        temp_2 = Elementary_PID & 0x1fff;
        temp_3 = htons(temp_1 | temp_2);
        *(unsigned short*)(p_temp->p_buf + len) = temp_3;
        len += 2;
        unsigned char c_temp = 0x10;    //00 01 00 00
        *(p_temp->p_buf + len) = c_temp | pes_count;
        len++;
        memcpy(p_temp->p_buf + len, Buf->p_buf + pos, packet_pes);
        pos += packet_pes;
        packet_num--;    //剩余不含自适应区的包数量减一
        l_pes.push_back(p_temp);
    }

    return 0;
}

/*ts头+PAT表
return,        加ts头的pat表，固定188字节，需要在外部释放*/
unsigned char* PatPacket() {
    int temp_len = 0;
    int pat_len = 0;
    int pat_before_len = 0;
    unsigned char *p_pat = new unsigned char[188];
    memset(p_pat, 0xff, 188);
    *p_pat = 0x47;    //ts包起始字节
    /*接下来1bit传输错误标识（0），1bit负载单元开始标识(1)，1bit传输优先级（1）,13bit为PID（pat表限定0）,共计2字节*/
    *(p_pat + 1) = 0x60;
    *(p_pat + 2) = 0x00;
    /*接下来2bit传输扰乱控制（00未加密），2bit是否包含自适应区（01只含有效载荷），4bit递增计数器（0000），共计1字节*/
    *(p_pat + 3) = 0x10;
    temp_len += 4;
    /*因为前面负载开始标识为1，这里有一个调整字节,一般这个字节为0x00*/
    *(p_pat + temp_len) = 0x00;
    temp_len++;
    pat_before_len = temp_len;
    /*接下来是PAT表*/
    /*table_id,对于PAT只能是0x00*/
    *(p_pat + temp_len) = 0x00;
    temp_len++;
    /*固定4bit（1011），2bit必定为00，后10bit表示后续长度(9+4*PMT表数，注意其中也包括了crc_32校验码的长度，我们的节目只有视频所以这个长度为13:0x000d)，共计2字节*/
    unsigned short temp_1 = 0xb000;    //10 11 00 00 00 00 00 00
    unsigned short temp_2 = 0x000d & 0x03ff; //10bit有效
    unsigned short temp_3 = temp_1 | temp_2;
    *(unsigned short*)(p_pat + temp_len) = htons(temp_3);
    temp_len += 2;
    *(unsigned short*)(p_pat + temp_len) = htons(0x0000);/*传输流ID,用户自定义（那就随便定义个0x0000）*/
    temp_len += 2;
    /*接下来2bit固定（11），5bit（00000,一旦PAT有变化，版本号加1),1bit(1,表示传送的PAT当前可以使用，若为0表示下一个表有效)*/
    *(p_pat + temp_len) = 0xc1;    //11 00 00 01
    temp_len++;
    /*接下来一个字节（0x00，我们只有一个视频节目，不存在分段)给出了该分段的数目。在PAT中的第一个分段的section_number为0x00，PAT中每一分段将加1。*/
    *(p_pat + temp_len) = 0x00;
    temp_len++;
    /*接下来一个字节（0x00，我们只有一个视频节目，不存在分段)给出了该分段的数目。该字段指出了最后一个分段号。在整个PAT中即分段的最大数目。*/
    *(p_pat + temp_len) = 0x00;
    temp_len++;

    /*开始循环*/
    /*接下来开始循环记录节目即PMT表（我们只有一个视频节目），每次循环4字节*/
    /*循环的前2个字节program_number。0x0001：这个为PMT。该字段指出了节目对于那个Program_map_PID是可以使用的。如果是0x0000，那么后面的PID是网络PID，否则其他值由用户定义。*/
    *(unsigned short*)(p_pat + temp_len) = htons(0x0001);
    temp_len += 2;
    /*循环的后2个字节，3bit为固定值（111），13bit为节目号对应内容的PID值(即Program_map_PID,反正只有这一个节目，在全局中随便定义一个id，等会PMT表还要用到)*/
    temp_1 = 0xe000;    //11 10 00 00 00 00 00 00
    temp_2 = Program_map_PID;
    temp_3 = temp_1 | temp_2;
    *(unsigned short*)(p_pat + temp_len) = htons(temp_3);
    temp_len += 2;
    /*循环到这里就结束了*/

    /*最后是4字节的PAT表crc_32校验码。这个校验码从PAT表（不含ts头及调整字节）开始到crc_32校验码前的数据*/
    //TODO 如果pat分表了，要考虑，我们这里不会分表不要考虑
    pat_len = temp_len - pat_before_len;
    unsigned int crc32_res = Crc32Calculate(p_pat + pat_before_len, pat_len, crc32Table);
    *(unsigned int *)(p_pat + temp_len) = htonl(crc32_res);
    temp_len += 4;
    /*后续用0xff填充，我们在初始化时已经初始化为0xff了*/
    return p_pat;
}

/*ts头+PMT表
return,        加ts头的pmt表，固定188字节，需要在外部释放*/
unsigned char* PmtPacket() {
    int temp_len = 0;
    int pmt_len = 0;
    int pmt_before_len = 0;
    unsigned short temp_1, temp_2, temp_3;
    unsigned char *p_pmt = new unsigned char[188];
    memset(p_pmt, 0xff, 188);
    *p_pmt = 0x47;    //ts包起始字节
    temp_len++;
    /*接下来1bit传输错误标识（0），1bit负载单元开始标识(1)，1bit传输优先级（1）,13bit为PID（用pat表中记录的Program_map_PID:0x0003）,共计2字节*/
    temp_1 = 0x6000 & 0xe000;
    temp_2 = 0x0003 & 0x1fff;
    temp_3 = htons(temp_1 | temp_2);
    *(unsigned short*)(p_pmt + temp_len) = temp_3;
    //*(p_pmt + 1) = 0x60;
    //*(p_pmt + 2) = 0x03;
    temp_len += 2;
    /*接下来2bit传输扰乱控制（00未加密），2bit是否包含自适应区（01只含有效载荷），4bit递增计数器（0000），共计1字节*/
    *(p_pmt + temp_len) = 0x10;    //00 01 00 00
    temp_len++;
    /*因为前面负载开始标识为1，这里有一个调整字节,一般这个字节为0x00*/
    *(p_pmt + temp_len) = 0x00;
    temp_len++;
    pmt_before_len = temp_len;
    /*接下来是PMT表*/
    /*table_id,对于PMT表固定为0x02*/
    *(p_pmt + temp_len) = 0x02;
    temp_len++;
    /*固定4bit（1011），2bit必定为00，后10bit表示后续长度(13+5*流类型数，我们的节目只有视频流所以这个长度为18:0x0012)，共计2字节*/
    temp_1 = 0xb000 & 0xf000;    //10 11 00 00 00 00 00
    temp_2 = 0x0012 & 0x03ff;    //00 00 00 00 01 00 10
    temp_3 = htons(temp_1 | temp_2);
    *(unsigned short*)(p_pmt + temp_len) = temp_3;
    temp_len += 2;
    /*对应PAT中的program_number(我们的PAT表中只有一个PMT的program_number:0x0001),共计2字节*/
    *(unsigned short*)(p_pmt + temp_len) = htons(0x0001);
    temp_len += 2;
    /*接下来2bit固定（11），5bit该字段指出了TS中program_map_section的版本号（00000，如果PAT有变化则版本号加1），1bit（1，当该字段置为1时，表示当前传送的program_map_section可用；当该字段置0时，表示当前传送的program_map_section不可用，下一个TS的program_map_section有效）*/
    *(p_pmt + temp_len) = 0xc1;    //11 00 00 01
    temp_len++;
    /*接下来section_number,该字段总是置为0x00*/
    *(p_pmt + temp_len) = 0x00;
    temp_len++;
    /*接下来last_section_number,该字段总是置为0x00*/
    *(p_pmt + temp_len) = 0x00;
    temp_len++;
    /*接下来3bit固定（111），13bitPCR(节目参考时钟)所在TS分组的PID（指定为视频PID），我们暂时不想要pcr(ISO/IEC-13818-1中2.4.4.9对于PMT表中PCR_PID有一段描述“若任何PCR均与专用流的节目定义无关，则此字段应取值0x1fff”，不是太懂PCR，但是我们貌似可以忽略它)，指定它为0x1fff,总计2字节*/
    temp_1 = 0xe000 & 0xe000;    //11 10 00 00 00 00 00 00
    temp_2 = 0x1fff & 0x1fff;    //00 01 11 11 11 11 11 11
    temp_3 = htons(temp_1 | temp_2);
    *(unsigned short*)(p_pmt + temp_len) = temp_3;
    temp_len += 2;
    /*接下来4bit固定（1111），12bit节目描述信息（指定为0x000表示没有），共计2字节*/
    temp_1 = 0xf000 & 0xf000;    //11 11 00 00 00 00 00 00
    temp_2 = 0x0000 & 0x0fff;    //00 00 00 00 00 00 00 00
    temp_3 = htons(temp_1 | temp_2);
    *(unsigned short*)(p_pmt + temp_len) = temp_3;
    temp_len += 2;

    /*开始循环*/
    /*接下来开始循环记录流类型、PID及描述信息，每次循环5字节*/
    /*流类型，8bit（0x1B:表示这个流时h264格式的，通俗点就是视频，我们只有h264裸流）*/
    *(p_pmt + temp_len) = 0x1b;
    temp_len++;
    /*3bit固定（111），13bit流类型对应的PID，表示该pid的ts包就是用来装该流类型的数据的（对应这里就是我们在pes封包中的为h264裸流指定的Elementary_PID）,总计2字节*/
    temp_1 = 0xe000 & 0xe000;
    temp_2 = Elementary_PID & 0x1fff;
    temp_3 = htons(temp_1 | temp_2);
    *(unsigned short*)(p_pmt + temp_len) = temp_3;
    temp_len += 2;
    /*4bit固定（1111），12bit节目描述信息，指定为0x000表示没有,共计2字节*/
    temp_1 = 0xf000 & 0xf000;
    temp_2 = 0x0000 & 0x0fff;
    temp_3 = htons(temp_1 | temp_2);
    *(unsigned short*)(p_pmt + temp_len) = temp_3;
    temp_len += 2;
    /*循环结束*/

    /*最后是4字节的PMT表crc_32校验码。这个校验码从PMT表（不含ts头及调整字节）开始到crc_32校验码前的数据*/
    pmt_len = temp_len - pmt_before_len;
    unsigned int crc32_res = Crc32Calculate(p_pmt + pmt_before_len, pmt_len, crc32Table);
    *(unsigned int *)(p_pmt + temp_len) = htonl(crc32_res);
    temp_len += 4;
    /*后续用0xff填充，我们在初始化时已经初始化为0xff了*/
    return p_pmt;
}

static int FindStartCode2(unsigned char *Buf)
{
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 1) return 0; //判断是否为0x000001,如果是返回1
    else return 1;
}

static int FindStartCode3(unsigned char *Buf)
{
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 0 || Buf[3] != 1) return 0;//判断是否为0x00000001,如果是返回1
    else return 1;
}

void write2file(unsigned char* buf, int len) {
    fwrite(buf, 1, len, fw);
}

void writeM3u8(m3u8_text text,int ts_time_max, int ts_start_num) {
    char file_m3u8[] = "stream.m3u8";
    OpenWriteFile(file_m3u8);
    char targetduration[M3U8_TS_LEN] = { 0 };
    memcpy(targetduration, text.ext_x_targetduration, M3U8_TS_LEN);
    char media_sequence[M3U8_TS_LEN] = { 0 };
    memcpy(media_sequence, text.ext_x_media_sequence, M3U8_TS_LEN);
    sprintf(text.ext_x_targetduration, targetduration, ts_time_max);
    sprintf(text.ext_x_media_sequence, media_sequence, ts_start_num);
    write2file((unsigned char*)text.extm3u, strlen(text.extm3u));
    write2file((unsigned char*)text.ext_x_version, strlen(text.ext_x_version));
    write2file((unsigned char*)text.ext_x_allow_cache, strlen(text.ext_x_allow_cache));
    write2file((unsigned char*)text.ext_x_targetduration, strlen(text.ext_x_targetduration));
    write2file((unsigned char*)text.ext_x_media_sequence, strlen(text.ext_x_media_sequence));
    write2file((unsigned char*)text.ext_x_playlist_type, strlen(text.ext_x_playlist_type));
    while (!text.l_m3u8_ts.empty()) {
        m3u8_ts temp = text.l_m3u8_ts.front();
        text.l_m3u8_ts.pop_front();
        char extinf[M3U8_TS_LEN] = { 0 };
        sprintf(extinf, text.extinf, temp.ts_time);
        write2file((unsigned char*)extinf, strlen(extinf));
        write2file((unsigned char*)temp.ts_name, strlen(temp.ts_name));
        write2file((unsigned char*)"\n", strlen("\n"));    //ts_name后是没有换行符的，这里要添加换行符
    }
    write2file((unsigned char*)text.ext_x_endlist, strlen(text.ext_x_endlist));
    CloseWriteFile();
}

/*输入h264裸流文件，输出ts文件*/
int H264ToTs()
{
    char file_in[] = "./stream.h264";
    OpenBitstreamFile(file_in);
    MakeTable(crc32Table);
    char file_out[] = "./stream.ts";
    OpenWriteFile(file_out);
    unsigned char* p_pat_res = PatPacket();
    write2file(p_pat_res, TS_PACKET_LEN);
    fflush(fw);
    unsigned char* p_pmt_res = PmtPacket();
    write2file(p_pmt_res, TS_PACKET_LEN);
    fflush(fw);

    unsigned char* temp = new unsigned char[BUF_SIZE];
    int temp_len = 0;
    frame_buf *p_frame = new frame_buf;
    list<pes_packet*> list_pes;
    int frame_count = 0;
    while (!feof(bits)) {
        int read_res = GetOneFrame(temp,temp_len);
        if (read_res < 0) {
            continue;
        }
        frame_count++;
        ////如果需要实时播放（直播）而不是每次都是都是从文件开始到结束，可以考虑在中间插入pat和pmt表，暂时我们也用不上
        //else if (read_res == 0) {
        //    write2file(p_pat_res, TS_PACKET_LEN);
        //    fflush(fw);
        //    write2file(p_pmt_res, TS_PACKET_LEN);
        //    fflush(fw);
        //}
        int res = PesHead_pd(p_frame, temp, temp_len);
        //int res = PesHead_pd_1(p_frame, temp, temp_len);
        if (res != 0) {
            printf("pesHead failed\n");
            return -1;
        }
        res = PesPacket(list_pes, p_frame);
        if (res != 0) {
            printf("pesPacket failed\n");
            return -1;
        }
        while (!list_pes.empty()) {
            pes_packet* p = list_pes.front();
            write2file(p->p_buf, TS_PACKET_LEN);
            fflush(fw);
            list_pes.pop_front();
            delete p;

        }
    }
    delete[] p_pat_res;
    delete[] p_pmt_res;
    delete[] temp;
    delete p_frame;
    fclose(bits);
    fclose(fw);
    return 0;
}

/*输入h264裸流文件，输出m3u8及多个ts文件*/
int H264ToM3u8()
{
    char file_in[] = "./stream.h264";
    OpenBitstreamFile(file_in);
    MakeTable(crc32Table);
    int ts_file_num = 100;
    int ts_start_num = ts_file_num;
    int ts_time_max = 0;
    char ts_file_temp[M3U8_TS_LEN] = "./stream%d.ts";
    char ts_file_current[M3U8_TS_LEN] = { 0 };
    sprintf(ts_file_current, ts_file_temp, ts_file_num);

    OpenWriteFile(ts_file_current);
    unsigned char* p_pat_res = PatPacket();
    write2file(p_pat_res, TS_PACKET_LEN);
    fflush(fw);
    unsigned char* p_pmt_res = PmtPacket();
    write2file(p_pmt_res, TS_PACKET_LEN);
    fflush(fw);

    unsigned char* temp_frame = new unsigned char[BUF_SIZE];
    int temp_frame_len = 0;
    frame_buf *p_frame = new frame_buf;
    list<pes_packet*> list_pes;
    int IDR_count = 0;
    int frame_count = 0;    //用来计算一个.ts文件的时间
    m3u8_text text;
    while (!feof(bits)) {
        int read_res = GetOneFrame(temp_frame, temp_frame_len);

        if (read_res < 0) {
            continue;
        }
        //将一个长ts文件分成多个ts短文件，我们尽量让第一个帧为关键帧（没试第一帧为非关键帧的情况）
        else if (0 == read_res) {
            IDR_count++;
            frame_count++;
            if (0 == (IDR_count%IDR_PER_TSFILE)) {
                CloseWriteFile();
                /*将这个ts文件插入到m3u8_text中*/
                m3u8_ts temp;
                temp.ts_time = 1.0 * frame_count / frame_rate;
                frame_count = 0;    //ts包计数清零
                memcpy(temp.ts_name, ts_file_current, M3U8_TS_LEN);
                text.l_m3u8_ts.push_back(temp);
                int temp_time = ceil(temp.ts_time);
                if (ts_time_max < temp_time) {
                    ts_time_max = temp_time;
                }
                /*开始下一个ts文件，文件号连续*/
                ts_file_num++;
                memset(ts_file_current, 0, M3U8_TS_LEN);
                sprintf(ts_file_current, ts_file_temp, ts_file_num);
                OpenWriteFile(ts_file_current);
                write2file(p_pat_res, TS_PACKET_LEN);
                fflush(fw);
                write2file(p_pmt_res, TS_PACKET_LEN);
                fflush(fw);
            }
        }
        else {
            frame_count++;
        }
        int res = PesHead_pd(p_frame, temp_frame, temp_frame_len);
        //int res = PesHead_pd_1(p_frame, temp, temp_len);
        if (res != 0) {
            printf("pesHead failed\n");
            return -1;
        }
        res = PesPacket(list_pes, p_frame);
        if (res != 0) {
            printf("pesPacket failed\n");
            return -1;
        }
        while (!list_pes.empty()) {
            pes_packet* p = list_pes.front();
            write2file(p->p_buf, TS_PACKET_LEN);
            fflush(fw);
            list_pes.pop_front();
            delete p;

        }
    }
    /*将最后一帧写入m3u8_text*/
    CloseWriteFile();
    m3u8_ts temp;
    temp.ts_time = 1.0 * frame_count / frame_rate;
    memcpy(temp.ts_name, ts_file_current, M3U8_TS_LEN);
    text.l_m3u8_ts.push_back(temp);
    int temp_time = ceil(temp.ts_time);
    if (ts_time_max < temp_time) {
        ts_time_max = temp_time;
    }
    /*使用m3u8_text完成最后的m3u8文件*/
    writeM3u8(text, ts_time_max, ts_start_num);
    /*释放内存*/
    delete[] p_pat_res;
    delete[] p_pmt_res;
    delete[] temp_frame;
    delete p_frame;
    CloseBitstreamFile();

    return 0;
}

int main() {
    char* test = "xyz";
    unsigned int crc32_res = Crc32Calculate((u_char*)test, 3, crc32Table);

    printf("crc32: %lld\n", crc32_res);
    //return H264ToM3u8();
    return H264ToTs();
}