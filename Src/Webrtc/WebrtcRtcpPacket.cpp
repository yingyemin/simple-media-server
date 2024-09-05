#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>
#include <bitset>
#include <netinet/in.h>

#include "WebrtcRtcpPacket.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtcpPacket::RtcpPacket(const StreamBuffer::Ptr& buffer, int pos)
    :_buffer(buffer)
    ,_pos(pos)
{
    _header = (RtcpHeader*)data();
    _length = ntohs(_header->length) * 4 + 4;
    _payloadLen = _length - sizeof(RtcpHeader);
}

void RtcpPacket::onRtcp(const RtcpPacket::Ptr& rtcp)
{
    if (_onRtcp) {
        _onRtcp(rtcp);
    }
}

void RtcpPacket::parse()
{
    auto len = size();
    auto rtcpBuf = data();
    int index = 0;
    shared_ptr<RtcpPacket> rtcp;
    while(index < len) {
        auto header = (RtcpHeader*)(rtcpBuf + index);
        switch (header->type)
        {
        case RtcpType_SR:
            rtcp = make_shared<RtcpSR>(_buffer, _pos + index);
            break;
        case RtcpType_RR:
            rtcp = make_shared<RtcpRR>(_buffer, _pos + index);
            break;
        case RtcpType_SDES:
            rtcp = make_shared<RtcpSdes>(_buffer, _pos + index);
            break;
        case RtcpType_BYE:
            rtcp = make_shared<RtcpBye>(_buffer, _pos + index);
            break;
        case RtcpType_APP:
            rtcp = make_shared<RtcpApp>(_buffer, _pos + index);
            break;
        case RtcpType_RTPFB:
            if(RtcpRtpFBFmt_NACK == header->rc) {
                //nack
                rtcp = make_shared<RtcpNack>(_buffer, _pos + index);
            } else if (RtcpRtpFBFmt_TWCC == header->rc) {
                //twcc
                rtcp = make_shared<RtcpTWCC>(_buffer, _pos + index);
            }
            break;
        case RtcpType_PSFB:
            if(1 == header->rc) {
                // pli
                rtcp = make_shared<RtcpPli>(_buffer, _pos + index);
            } else if(2 == header->rc) {
                //sli
                rtcp = make_shared<RtcpSli>(_buffer, _pos + index);
            } else if(3 == header->rc) {
                //rpsi
                rtcp = make_shared<RtcpRpsi>(_buffer, _pos + index);
            } else if (4 == header->rc) {
                //rpsi
                rtcp = make_shared<RtcpFir>(_buffer, _pos + index);
            } else if (15 == header->rc) {
                //rpsi
                rtcp = make_shared<RtcpRemb>(_buffer, _pos + index);
            } else {
                // common psfb
                rtcp = make_shared<RtcpPacket>(_buffer, _pos + index);
            }
            break;
        case RtcpType_XR:
            rtcp = make_shared<RtcpXR>(_buffer, _pos + index);
            
            break;
        
        default:
            break;
        }

        if (!rtcp) {
            return ;
        }
        index += rtcp->_length;

        rtcp->parse();
        onRtcp(rtcp);
    }
}

RtcpSR::RtcpSR(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpSR::parse()
{
    /* @doc: https://tools.ietf.org/html/rfc3550#section-6.4.1
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
header |V=2|P|    RC   |   PT=SR=200   |             length            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         SSRC of sender                        |
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
sender |              NTP timestamp, most significant word             |
info   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |             NTP timestamp, least significant word             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         RTP timestamp                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                     sender's packet count                     |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      sender's octet count                     |
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    */
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;
    _ntp = (uint64_t)readUint32BE(data + sizeof(RtcpHeader)) << 32 | readUint32BE(data + sizeof(RtcpHeader) + 4);
    _rtpTs = readUint32BE(data + sizeof(RtcpHeader) + 8);
    _sendPackets = readUint32BE(data + sizeof(RtcpHeader) + 12);
    _sendBytes = readUint32BE(data + sizeof(RtcpHeader) + 16);
    
    _payloadLen = _length - sizeof(RtcpHeader) - 20;
    _payload = data + sizeof(RtcpHeader) + 20;
}

RtcpRR::RtcpRR(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpRR::parse()
{
    /*
    @doc: https://tools.ietf.org/html/rfc3550#section-6.4.2

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
header |V=2|P|    RC   |   PT=RR=201   |             length            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                     SSRC of packet sender                     |
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
report |                 SSRC_1 (SSRC of first source)                 |
block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  1    | fraction lost |       cumulative number of packets lost       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |           extended highest sequence number received           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      interarrival jitter                      |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         last SR (LSR)                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                   delay since last SR (DLSR)                  |
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
report |                 SSRC_2 (SSRC of second source)                |
block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  2    :                               ...                             :
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
       |                  profile-specific extensions                  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;

    if (_header->rc == 0) {
        return ;
    }

    _payloadLen = _length - sizeof(RtcpHeader);
    _payload = data + sizeof(RtcpHeader);

    int rbLen = 0;
    while (rbLen + sizeof(RtcpRRBlock) <= _payloadLen) {
        RtcpRRBlock rb;
        rb._ssrc = readUint32BE(data + sizeof(RtcpHeader));
        rb._fractionLost = data[sizeof(RtcpHeader) + 4];
        rb._lostPackets = readUint24BE(data + sizeof(RtcpHeader) + 5);
        rb._highestSn = readUint32BE(data + sizeof(RtcpHeader) + 8);
        rb._jitter = readUint32BE(data + sizeof(RtcpHeader) + 12);
        rb._lsr = readUint32BE(data + sizeof(RtcpHeader) + 16);
        rb._dlsr = readUint32BE(data + sizeof(RtcpHeader) + 20);

        rbLen += sizeof(RtcpRRBlock);
        _reportBlocks.push_back(rb);
    }
}

RtcpXR::RtcpXR(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpXR::parse()
{
    /*
        0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|reserved |   PT=XR=207   |             length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                              SSRC                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     BT=4      |   reserved    |       block length = 2        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              NTP timestamp, most significant word             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |             NTP timestamp, least significant word             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

   /*
       0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|reserved |   PT=XR=207   |             length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                              SSRC                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     BT=5      |   reserved    |         block length          |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                 SSRC_1 (SSRC of first receiver)               | sub-
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
   |                         last RR (LRR)                         |   1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   delay since last RR (DLRR)                  |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                 SSRC_2 (SSRC of second receiver)              | sub-
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
   :                               ...                             :   2
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   */
    
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;
    auto blockType = data[0];

    _payloadLen = _length - sizeof(RtcpHeader);
    _payload = data + sizeof(RtcpHeader);

    if (blockType == 4) {
        _rrtBlock.ntp_ = (uint64_t)readUint32BE(data + sizeof(RtcpHeader) + 4) << 32 | readUint32BE(data + sizeof(RtcpHeader) + 8);
    } else if (blockType == 5) {
        
        int rbLen = 0;
        while (rbLen + sizeof(RtcpXrDlsrBlock) <= _payloadLen - 4) {
            RtcpXrDlsrBlock dlsr;
            dlsr.ssrc_ = readUint32BE(data + sizeof(RtcpHeader));
            dlsr.lrr_ = readUint32BE(data + sizeof(RtcpHeader) + 4);
            dlsr.dlrr_ = readUint32BE(data + sizeof(RtcpHeader) + 8);

            rbLen += sizeof(RtcpXrDlsrBlock);
            _dlsrBlocks.push_back(dlsr);
        }
    } 
}

RtcpSdes::RtcpSdes(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpSdes::parse()
{
    
}

RtcpApp::RtcpApp(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpApp::parse()
{
    /*
    @doc: https://tools.ietf.org/html/rfc3550#section-6.7
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P| subtype |   PT=APP=204  |             length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           SSRC/CSRC                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          name (ASCII)                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   application-dependent data                ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;
    _name.assign(data + sizeof(RtcpHeader), 4);
    _payload = data + sizeof(RtcpHeader) + 4;
    _payloadLen = _length - sizeof(RtcpHeader) - 4;
}

RtcpNack::RtcpNack(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpNack::parse()
{
    /*
     0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |V=2|P| FMT=1   |   PT=RTPFB    |             length            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                     SSRC of packet sender                     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                     SSRC of media source                      |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |          PID                  |            BLP                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;
    _ssrc = readUint32BE(data + sizeof(RtcpHeader));

    int index = sizeof(RtcpHeader) + 4;
    while (index + 4 < _length) {
        auto pid = readUint16BE(data + index);
        auto blp = readUint16BE(data + index + 2);
        _lossSn.push_back(pid);
        auto nack_blp = std::bitset<16>(blp);
        for(int i=0; i<16; i++) {
			if(nack_blp[i] == 1){
				_lossSn.emplace_back(pid+i+1);
			}
		}
    }
}

StreamBuffer::Ptr RtcpNack::encode()
{
    auto buffer = StreamBuffer::create();
    buffer->setCapacity(17);

    RtcpHeader header;
    header.version = 2;
    header.padding = 0;
    header.rc = RtcpRtpFBFmt_NACK;
    header.type = RtcpType_RTPFB;
    header.length = 3;
    header.ssrc = _ssrc;

    memcpy(buffer->data(), &header, 8);
    writeUint32BE(buffer->data() + 8, _ssrc);

    uint16_t pid = *_lossSn.begin();
    writeUint16BE(buffer->data() + 12, pid);
    uint16_t blp = 0;
    for (auto& sn : _lossSn) {
        if (sn - pid > 15) {
            break;
        }
        blp |= 1 << (15 - (sn - pid));
    }
    writeUint16BE(buffer->data() + 14, blp);

    return buffer;
}

RtcpTWCC::RtcpTWCC(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpTWCC::parse()
{
    /*
    @doc: https://tools.ietf.org/html/draft-holmer-rmcat-transport-wide-cc-extensions-01#section-3.1
            0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |V=2|P|  FMT=15 |    PT=205     |           length              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                     SSRC of packet sender                     |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      SSRC of media source                     |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      base sequence number     |      packet status count      |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                 reference time                | fb pkt. count |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |          packet chunk         |         packet chunk          |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       .                                                               .
       .                                                               .
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         packet chunk          |  recv delta   |  recv delta   |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       .                                                               .
       .                                                               .
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |           recv delta          |  recv delta   | zero padding  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    _payload = _buffer->data() + sizeof(RtcpHeader);
    _payloadLen = _length - sizeof(RtcpHeader);
    _ssrc = readUint32BE(_payload);
    _baseSeqNum = readUint16BE(_payload + 4);
    _pktStatusCnt = readUint16BE(_payload + 6);
    _referenceTime = readUint24BE(_payload + 8);
    _fbPktCnt = _payload[11];

    int pktCntParsed = 0;
    int index = 12;
    int curSn = _baseSeqNum;
    while (pktCntParsed < _pktStatusCnt) {
        uint16_t pktChunk = readUint16BE(_payload + index);
        index += 2;

        int type = pktChunk >> 15;
        bool isRunLengthChunk = false;
        if (type) {
            int symbolSize = (pktChunk >> 14) & 0b1;
            if (symbolSize) {
                // 2 bit
                for (int i = 6; i >= 0; --i) {
                    PacketChunkInfo info;
                    info.isRunLengthChunk = false;
                    info.seq = curSn + 6 - i;
                    info.status = (PacketChunkStatus)(pktChunk & (0b11 << (i*2)));

                    _pktChunks.push_back(info);
                }
                curSn += 7;
                pktCntParsed += 7;
            } else {
                // 1 bit
                for (int i = 13; i >= 0; --i) {
                    PacketChunkInfo info;
                    info.isRunLengthChunk = false;
                    info.seq = curSn + 13 - i;
                    info.status = (PacketChunkStatus)(pktChunk & (0b1 << i));

                    _pktChunks.push_back(info);
                }
                curSn += 14;
                pktCntParsed += 14;
            }
        } else {
            int pktStatusSymbol = (pktChunk >> 13) & 0b11;
            int length = (pktChunk << 3) >> 3;
            for (int i = 0; i < length; ++i) {
                PacketChunkInfo info;
                info.isRunLengthChunk = true;
                info.seq = curSn + i;
                info.status = (PacketChunkStatus)pktStatusSymbol;

                _pktChunks.push_back(info);
            }
            curSn += length;
            pktCntParsed += length;
        }
    }

    uint64_t curStamp = _referenceTime * 64 * 1000; // us
    for (int i = 0; i < _pktStatusCnt; ++i) {
        if (_pktChunks[i].status == PacketReceivedSmall) {
            uint8_t delta = _payload[index];
            index += 1;
            curStamp += delta * 250;
            _pktChunks[i].recvTime = curStamp;
        } else {
            auto delta = readUint16BE(_payload + index);
            index += 2;
            curStamp += delta * 250;
            _pktChunks[i].recvTime = curStamp;
        }
    }
}

// 这里图方便，全部用的StatusVectorChunk的PacketReceivedlarge类型
StringBuffer::Ptr RtcpTWCC::encode()
{
    StringBuffer::Ptr buffer = make_shared<StringBuffer>();

    RtcpHeader header;
    header.version = 2;
    header.padding = 0;
    header.rc = RtcpRtpFBFmt_TWCC;
    header.type = RtcpType_RTPFB;
    header.length = 3; // 这里先占个位
    header.ssrc = _ssrc;

    int length = 0;
    buffer->assign((char*)&header, 8);
    buffer->append((char*)&_ssrc, 4);
    buffer->append((char*)&_baseSeqNum, 2);
    buffer->append((char*)&_pktStatusCnt, 2);
    buffer->append((char*)&_referenceTime, 3);
    buffer->append((char*)&_fbPktCnt, 1);

    length = 20;

    uint16_t chunkBytes = 0b11 << 14;
    int index = 14;
    for (auto& chunk : _pktChunks) {
        if (!index) {
            buffer->append((char*)&chunkBytes, 2);
            index = 14;
            chunkBytes = 0b11 << 14;
        }
        index -= 2;
        chunkBytes |= chunk.status << index;
    }

    uint64_t preStamp = _referenceTime * 64 * 1000;
    for (auto& chunk : _pktChunks) {
        if (chunk.status == PacketReceivedSmall) {
            buffer->append((char*)&chunk.recvTime, 1);
        } else if (chunk.status == PacketReceivedlarge) {
            buffer->append((char*)&chunk.recvTime, 2);
        }
    }

    int padSize = 4 - (buffer->size() % 4);
    for (int i = 0; i <padSize; ++i) {
        buffer->push_back('0');
    }

    header.length = buffer->size() / 4;
    memcpy(buffer->data(), &header, 8);

    return buffer;
}

RtcpPli::RtcpPli(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpPli::parse()
{
    /*
    @doc: https://tools.ietf.org/html/rfc4585#section-6.1
        0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|   FMT   |       PT      |          length               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of packet sender                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of media source                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :            Feedback Control Information (FCI)                 :
   :                                                               :
   */
    // FCI 为空
    
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;
    _ssrc = readUint32BE(data + sizeof(RtcpHeader));
}

StreamBuffer::Ptr RtcpPli::encode(int ssrc)
{
    auto buffer = StreamBuffer::create();
    buffer->setCapacity(13);
    auto data = buffer->data();

    data[0] = 0x81;
    data[1] = RtcpType_PSFB;
    writeUint16BE(data + 2, 2);
    writeUint32BE(data + 4, ssrc);
    writeUint32BE(data + 8, ssrc);

    return buffer;
}

RtcpSli::RtcpSli(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpSli::parse()
{
    /*
    @doc: https://tools.ietf.org/html/rfc4585#section-6.1
        0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|   FMT   |       PT      |          length               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of packet sender                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of media source                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :            Feedback Control Information (FCI)                 :
   :                                                               :


    @doc: https://tools.ietf.org/html/rfc4585#section-6.3.2
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            First        |        Number           | PictureID |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
    
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;
    _ssrc = readUint32BE(data + sizeof(RtcpHeader));
}

RtcpRpsi::RtcpRpsi(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpRpsi::parse()
{
    /*
    @doc: https://tools.ietf.org/html/rfc4585#section-6.1
        0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|   FMT   |       PT      |          length               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of packet sender                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of media source                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :            Feedback Control Information (FCI)                 :
   :                                                               :


    @doc: https://tools.ietf.org/html/rfc4585#section-6.3.3
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      PB       |0| Payload Type|    Native RPSI bit string     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   defined per codec          ...                | Padding (0) |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
    
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;
    _ssrc = readUint32BE(data + sizeof(RtcpHeader));
}

RtcpBye::RtcpBye(const StreamBuffer::Ptr& buffer, int pos)
    :RtcpPacket(buffer, pos)
{}

void RtcpBye::parse()
{
    if (_header->length + _pos > _buffer->size()) {
        return ;
    }

    auto data = _buffer->data() + _pos;
    
    _payload = data + sizeof(RtcpHeader);
    _payloadLen = _length - sizeof(RtcpHeader);
    int index = 0;

    while (index + 4 <= _payloadLen) {
        auto ssrc = readUint32BE(_payload + index);
        _ssrcs.push_back(ssrc);

        index += 4;
    }

    uint8_t length = _payload[_payloadLen];
    _reason.assign(_payload + _payloadLen, length);
}