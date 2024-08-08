#include "WebrtcStun.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/CRC32.h"
#include "Ssl/HmacSHA1.h"

#include <unordered_map>

using namespace std;

string encode_hmac(char* hmac_buf, const int32_t hmac_buf_len)
{
    char buf[1460];
    // YangBuffer* stream = new YangBuffer(buf, sizeof(buf));
    // YangAutoFree(YangBuffer, stream);

    string buffer;
    buffer.resize(4);
    writeUint16BE((char*)buffer.data(), MessageIntegrity);
    writeUint16BE((char*)buffer.data() + 2, hmac_buf_len);
    buffer.append(hmac_buf, hmac_buf_len);

    return buffer;

    // stream->write_2bytes(MessageIntegrity);
    // stream->write_2bytes(hmac_buf_len);
    // stream->write_bytes(hmac_buf, hmac_buf_len);

    // return string(stream->data(), stream->pos());
}

string encode_fingerprint(uint32_t crc32)
{
    string buffer;
    buffer.resize(8);
    writeUint16BE((char*)buffer.data(), Fingerprint);
    writeUint16BE((char*)buffer.data() + 2, 4);
    writeUint32BE((char*)buffer.data() + 4, crc32);

    return buffer;

//     char buf[1460];
//     YangBuffer* stream = new YangBuffer(buf, sizeof(buf));
//     YangAutoFree(YangBuffer, stream);

//     stream->write_2bytes(Fingerprint);
//     stream->write_2bytes(4);
//     stream->write_4bytes(crc32);

//     return string(stream->data(), stream->pos());
}

WebrtcStun::WebrtcStun()
{
    _messageType = 0;
    _localUfrag = "";
    _remoteUfrag = "";
    _useCandidate = false;
    _iceControlled = false;
    _iceControlling = false;
    _mappedPort = 0;
    _mappedAddress = 0;
}

WebrtcStun::~WebrtcStun()
{
}

bool WebrtcStun::isBindingRequest() const
{
    return _messageType == BindingRequest;
}

bool WebrtcStun::isBindingResponse() const
{
    return _messageType == BindingResponse;
}

uint16_t WebrtcStun::getType() const
{
    return _messageType;
}

std::string WebrtcStun::getUsername() const
{
    return _username;
}

std::string WebrtcStun::getLocalUfrag() const
{
    return _localUfrag;
}

std::string WebrtcStun::getRemoteUfrag() const
{
    return _remoteUfrag;
}

std::string WebrtcStun::getTranscationId() const
{
    return _transcationId;
}

uint32_t WebrtcStun::getMappedAddress() const
{
    return _mappedAddress;
}

uint16_t WebrtcStun::getMappedPort() const
{
    return _mappedPort;
}

bool WebrtcStun::getIceControlled() const
{
    return _iceControlled;
}

bool WebrtcStun::getIceControlling() const
{
    return _iceControlling;
}

bool WebrtcStun::getUseCandidate() const
{
    return _useCandidate;
}

void WebrtcStun::setType(const uint16_t& m)
{
    _messageType = m;
}

void WebrtcStun::setLocalUfrag(const std::string& u)
{
    _localUfrag = u;
}

void WebrtcStun::setRemoteUfrag(const std::string& u)
{
    _remoteUfrag = u;
}

void WebrtcStun::setTranscationId(const std::string& t)
{
    _transcationId = t;
}

void WebrtcStun::setMappedAddress(const uint32_t& addr)
{
    _mappedAddress = addr;
}

void WebrtcStun::setMappedPort(const uint32_t& port)
{
    _mappedPort = port;
}

int32_t WebrtcStun::parse(const char* buf, const int32_t nb_buf)
{
    int32_t err = 0;
    int index = 0;

    // YangBuffer* stream = new YangBuffer(const_cast<char*>(buf), nb_buf);
    // YangAutoFree(YangBuffer, stream);

    if (nb_buf < 20) {
        return -1;
    }

    _messageType = readUint16BE(buf);
    index += 2;
    uint16_t message_len = readUint16BE(buf + index);
    index += 2;
    string magic_cookie(buf + index, 4);
    index += 4;
    _transcationId.assign(buf + index, 12);// = stream->read_string(12);
    index += 12;

    if (nb_buf != 20 + message_len) {
        return -1;
    }

    while (nb_buf - index >= 4) {
        uint16_t type = readUint16BE(buf + index);
        index += 2;
        uint16_t len = readUint16BE(buf + index);
        index += 2;

        if (nb_buf - index < len) {
            return -1;
        }

        string val(buf + index, len);// = stream->read_string(len);
        index += len;
        // padding
        if (len % 4 != 0) {
            // stream->read_string(4 - (len % 4));
            index += 4 - (len % 4);
        }

        switch (type) {
            case Username: {
                _username = val;
                size_t p = val.find(":");
                if (p != string::npos) {
                    _localUfrag = val.substr(0, p);
                    _remoteUfrag = val.substr(p + 1);
                  //  yang_trace("stun packet local_ufrag=%s, remote_ufrag=%s", local_ufrag.c_str(), remote_ufrag.c_str());
                }
                break;
            }

			case UseCandidate: {
                _useCandidate = true;
                logTrace << ("stun use-candidate");
                break;
            }

            // @see: https://tools.ietf.org/html/draft-ietf-ice-rfc5245bis-00#section-5.1.2
			// One agent full, one lite:  The full agent MUST take the controlling
            // role, and the lite agent MUST take the controlled role.  The full
            // agent will form check lists, run the ICE state machines, and
            // generate connectivity checks.
			case IceControlled: {
                _iceControlled = true;
                logTrace << ("stun ice-controlled");
                break;
            }

			case IceControlling: {
                _iceControlling = true;
                logTrace << ("stun ice-controlling");
                break;
            }
            
            default: {
              //  yang_trace("stun type=%u, no process", type);
                break;
            }
        }
    }

    return err;
}

int32_t WebrtcStun::toBuffer(const string& pwd, StringBuffer::Ptr stream)
{
    if (isBindingResponse()) {
        return encodeBindingResponse(pwd, stream);
    } else if (isBindingRequest()) {
        return encodeBindingRequest(pwd, stream);
    }

    //  yang_error("ERROR_RTC_STUN unknown stun type=%d", get_message_type());
     return 1;
}

// FIXME: make this function easy to read
int32_t WebrtcStun::encodeBindingResponse(const string& pwd, StringBuffer::Ptr stream)
{
    int32_t err = 0;

    string property_username = encodeUsername();
    string mapped_address = encodeMappedAddress();

    stream->resize(8);

    writeUint16BE(stream->data(), BindingResponse);
    writeUint16BE(stream->data() + 2, property_username.size() + mapped_address.size());
    writeUint32BE(stream->data() + 4, kStunMagicCookie);
    stream->append(_transcationId);
    stream->append(property_username);
    stream->append(mapped_address);

    // stream->write_2bytes(BindingResponse);
    // stream->write_2bytes(property_username.size() + mapped_address.size());
    // stream->write_4bytes(kStunMagicCookie);
    // stream->write_string(_transcationId);
    // stream->write_string(property_username);
    // stream->write_string(mapped_address);

    stream->data()[2] = ((stream->size() - 20 + 20 + 4) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->size() - 20 + 20 + 4) & 0x000000FF);

    // char hmac_buf[20] = {0};
    // uint32_t  hmac_buf_len = 0;
    // if ((err = hmac_encode("sha1", pwd.c_str(), pwd.size(), stream->data(), stream->size(), hmac_buf, hmac_buf_len)) != 0) {
    //     return -1;
    // }

    auto hmanBuf = HmacSHA1::hmac_sha1(stream->buffer(), pwd);

    string hmac = encode_hmac((char*)hmanBuf.data(), hmanBuf.size());

    stream->append(hmac);
    stream->data()[2] = ((stream->size() - 20 + 8) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->size() - 20 + 8) & 0x000000FF);

    uint32_t crc32 = CRC32::encode((unsigned char*)stream->data(), stream->size()) ^ 0x5354554E;

    string fingerprint = encode_fingerprint(crc32);

    stream->append(fingerprint);

    stream->data()[2] = ((stream->size() - 20) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->size() - 20) & 0x000000FF);

    return err;
}

int32_t WebrtcStun::encodeBindingRequest(const string& pwd, StringBuffer::Ptr stream)
{
    int32_t err = 0;

    string property_username = encodeUsername();
    string mapped_address = encodeMappedAddress();

    stream->resize(8);
    writeUint16BE(stream->data(), BindingRequest);
    writeUint16BE(stream->data() + 2, property_username.size() + mapped_address.size());
    writeUint32BE(stream->data() + 4, kStunMagicCookie);
    stream->append(_transcationId);
    stream->append(property_username);
    stream->append(mapped_address);

    stream->data()[2] = ((stream->size() - 20 + 20 + 4) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->size() - 20 + 20 + 4) & 0x000000FF);

    // char hmac_buf[20] = {0};
    // uint32_t  hmac_buf_len = 0;
    // if ((err = hmac_encode("sha1", pwd.c_str(), pwd.size(), stream->data(), stream->size(), hmac_buf, hmac_buf_len)) != Yang_Ok) {
    //     return -1;
    // }

    auto hmanBuf = HmacSHA1::hmac_sha1(stream->buffer(), pwd);
    string hmac = encode_hmac((char*)hmanBuf.data(), hmanBuf.size());

    stream->append(hmac);
    stream->data()[2] = ((stream->size() - 20 + 8) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->size() - 20 + 8) & 0x000000FF);

    uint32_t crc32 = CRC32::encode((unsigned char*)stream->data(), stream->size()) ^ 0x5354554E;

    string fingerprint = encode_fingerprint(crc32);

    stream->append(fingerprint);

    stream->data()[2] = ((stream->size() - 20) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->size() - 20) & 0x000000FF);

    return err;
}

string WebrtcStun::encodeUsername()
{
    // char buf[1460];
    // YangBuffer* stream = new YangBuffer(buf, sizeof(buf));
    // YangAutoFree(YangBuffer, stream);

    string buffer;
    buffer.resize(4);

    string username = _remoteUfrag + ":" + _localUfrag;

    writeUint16BE((char*)buffer.data(), Username);
    writeUint16BE((char*)buffer.data() + 2, username.size());
    buffer.append(username);
    // stream->write_2bytes(Username);
    // stream->write_2bytes(username.size());
    // stream->write_string(username);

    if (buffer.size() % 4 != 0) {
        static char padding[4] = {0};
        buffer.append(padding, 4 - (buffer.size() % 4));
    }

    return buffer;
}

string WebrtcStun::encodeMappedAddress()
{
    // char buf[1460];
    // YangBuffer* stream = new YangBuffer(buf, sizeof(buf));
    // YangAutoFree(YangBuffer, stream);
    string buffer;
    buffer.resize(12);

    writeUint16BE((char*)buffer.data(), XorMappedAddress);
    writeUint16BE((char*)buffer.data() + 2, 8);
    buffer[4] = 0;
    buffer[5] = 1;
    writeUint16BE((char*)buffer.data() + 6, _mappedPort ^ (kStunMagicCookie >> 16));
    writeUint32BE((char*)buffer.data() + 8, _mappedAddress ^ kStunMagicCookie);

    // stream->write_2bytes(XorMappedAddress);
    // stream->write_2bytes(8);
    // stream->write_1bytes(0); // ignore this bytes
    // stream->write_1bytes(1); // ipv4 family
    // stream->write_2bytes(_mappedPort ^ (kStunMagicCookie >> 16));
    // stream->write_4bytes(_mappedAddress ^ kStunMagicCookie);

    return buffer;
}

