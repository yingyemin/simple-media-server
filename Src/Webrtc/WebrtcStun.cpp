#include "WebrtcStun.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/CRC32.h"
#include "Util/MD5.h"
#include "Ssl/HmacSHA1.h"

#include <unordered_map>

#define MAGIC_COOKIE 0x2112A442

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

Buffer::Ptr WebrtcStun::getBuffer() const
{
    return _buffer;
}

std::string WebrtcStun::getRealm() const
{
    return _realm;
}

std::string WebrtcStun::getNonce() const
{
    return _nonce;
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

void WebrtcStun::setMappedAddress(const sockaddr* addr)
{
    _addr = (sockaddr*)addr;
}

void WebrtcStun::setMappedPort(const uint32_t& port)
{
    _mappedPort = port;
}

int32_t WebrtcStun::parse(const char* buf, const int32_t nb_buf)
{
    int32_t err = 0;
    int index = 0;
    uint8_t mask[16];

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
        memset(mask, 0, sizeof(mask));

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

            case MappedAddress: {
                parseMappedAddress(val, mask, &_mapAddr);
                break;
            }

            case MessageIntegrity: {
                char message_integrity_hex[41];

                for (int i = 0; i < 20; i++) {
                sprintf(message_integrity_hex + 2 * i, "%02x", (uint8_t)val[i]);
                }
                break;
            }

            case Realm: {
                _realm = val;
                break;
            }

            case Nonce: {
                _nonce = val;
                break;
            }

            case XorRelayedAddress: {
                *((uint32_t*)mask) = htonl(MAGIC_COOKIE);
                memcpy(mask + 4, _transcationId.c_str(), _transcationId.size());
                parseMappedAddress(val, mask, &_relayAddr);
                break;
            }

            case XorMappedAddress: {
                *((uint32_t*)mask) = htonl(MAGIC_COOKIE);
                memcpy(mask + 4, _transcationId.c_str(), _transcationId.size());
                parseMappedAddress(val, mask, &_mapAddr);
                break;
            }

            case Fingerprint: {
                logInfo << "fingerprint: " << val;
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

Address* WebrtcStun::getMappedAddr(int type)
{
    if (type == 1) {
        return &_mapAddr;
    }
    return &_relayAddr;
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
    if (_addr->sa_family == AF_INET) {
        buffer[4] = 0;
        buffer[5] = 1;
        _mappedPort = ntohs((reinterpret_cast<const sockaddr_in*>(_addr))->sin_port);
        _mappedAddress = ntohl((reinterpret_cast<const sockaddr_in*>(_addr))->sin_addr.s_addr);
        writeUint16BE((char*)buffer.data() + 6, _mappedPort ^ (kStunMagicCookie >> 16));
        writeUint32BE((char*)buffer.data() + 8, _mappedAddress ^ kStunMagicCookie);
    } else {
        buffer.resize(24);
        buffer[4] = 0;
        buffer[5] = 2;

        std::memcpy(
                      (char*)buffer.data() + 6,
                      &(reinterpret_cast<const sockaddr_in6*>(_addr))->sin6_port,
                      2);
                    buffer[6] ^= kStunMagicCookie >> 24;
                    buffer[7] ^= (kStunMagicCookie >> 16) & 0xff;
                    // Set address and XOR it.
                    std::memcpy(
                      (char*)buffer.data() + 8,
                      &(reinterpret_cast<const sockaddr_in6*>(_addr))->sin6_addr.s6_addr,
                      16);
                    buffer[8] ^= kStunMagicCookie >> 24;;
                    buffer[9] ^= (kStunMagicCookie >> 16) & 0xff;;
                    buffer[10] ^= (kStunMagicCookie >> 8) & 0xff;;
                    buffer[11] ^= (kStunMagicCookie) & 0xff;;
                    buffer[12] ^= _transcationId[0];
                    buffer[13] ^= _transcationId[1];
                    buffer[14] ^= _transcationId[2];
                    buffer[15] ^= _transcationId[3];
                    buffer[16] ^= _transcationId[4];
                    buffer[17] ^= _transcationId[5];
                    buffer[18] ^= _transcationId[6];
                    buffer[19] ^= _transcationId[7];
                    buffer[20] ^= _transcationId[8];
                    buffer[21] ^= _transcationId[9];
                    buffer[22] ^= _transcationId[10];
                    buffer[23] ^= _transcationId[11];
    }

    // stream->write_2bytes(XorMappedAddress);
    // stream->write_2bytes(8);
    // stream->write_1bytes(0); // ignore this bytes
    // stream->write_1bytes(1); // ipv4 family
    // stream->write_2bytes(_mappedPort ^ (kStunMagicCookie >> 16));
    // stream->write_4bytes(_mappedAddress ^ kStunMagicCookie);

    return buffer;
}

void WebrtcStun::parseMappedAddress(const std::string& val, uint8_t* mask, Address* addr)
{
    char* value = (char*)val.data();

    int i;
    char addr_string[INET6_ADDRSTRLEN];
    uint32_t* addr32 = (uint32_t*)&addr->getAddr4()->sin_addr;
    uint16_t* addr16 = (uint16_t*)&addr->getAddr6()->sin6_addr;
    uint8_t family = value[1];
    uint16_t port;

    switch (family) {
    case STUN_FAMILY_IPV6:
        addr->setFamily(AF_INET6);
        for (i = 0; i < 8; i++) {
            addr16[i] = (*(uint16_t*)(value + 4 + 2 * i) ^ *(uint16_t*)(mask + 2 * i));
        }
        break;
    case STUN_FAMILY_IPV4:
    default:
        addr->setFamily(AF_INET);
        *addr32 = (*(uint32_t*)(value + 4) ^ *(uint32_t*)mask);
        break;
    }

    port = ntohs(*(uint16_t*)(value + 2) ^ *(uint16_t*)mask);
    auto addrString = addr->toString();
    addr->setPort(port);

    logDebug << "XOR Mapped Address Family: " << family;
    logDebug << "XOR Mapped Address Port: " << addr->getPort() << " (Port XOR: " << port << ")";
    logDebug << "XOR Mapped Address IP: " << addr_string << " (IP XOR: "  << *addr32 << ")";
}

void WebrtcStun::createHeader()
{
    StunHeader header;
    header.type = htons(_messageType);
    header.length = 0;
    header.magic_cookie = htonl(kStunMagicCookie);
    header.transaction_id[0] = htonl(0x12345678);
    header.transaction_id[1] = htonl(0x90abcdef);
    header.transaction_id[2] = htonl(0x12345678);

    _size = sizeof(StunHeader);
    _buffer->append((char*)&header, sizeof(StunHeader));
}

void WebrtcStun::createAttribute(StunAttrType type, uint16_t length, char* value)
{
    StunHeader* header = (StunHeader*)_buffer->data();

    StunAttribute stun_attr;
    stun_attr.type = htons(type);
    stun_attr.length = htons(length);
    if (value)
        memcpy(stun_attr.value, value, length);

    length = 4 * ((length + 3) / 4);
    header->length = htons(ntohs(header->length) + sizeof(StunAttribute) + length);

    _size += length + sizeof(StunAttribute);

    _buffer->append((char*)&stun_attr, sizeof(StunAttribute));

    switch (type) {
    case STUN_ATTR_TYPE_REALM:
        _realm.assign(value, length);
        break;
    case STUN_ATTR_TYPE_NONCE:
        _nonce.assign(value, length);
        break;
    case STUN_ATTR_TYPE_USERNAME:
        _username.assign(value, length);
        break;
    default:
        break;
    }
}

void WebrtcStun::createAttribute(StunAttrType type, const std::string& value)
{
    createAttribute(type, value.size(), (char*)value.data());
}

void WebrtcStun::finish(StunCredential credential, std::string& password)
{
    StunHeader* header = (StunHeader*)_buffer->data();

    uint16_t header_length = ntohs(header->length);

    switch (credential) {
    case STUN_CREDENTIAL_LONG_TERM:
        password = MD5(_username + ":" + _realm + ":" + password).rawdigest();
        // password_len = 16;
        break;
    default:
        break;
    }

    //   stun_attr = (StunAttribute*)(msg->buf + msg->size);
    header->length = htons(header_length + 24); /* HMAC-SHA1 */
    // stun_attr.type = htons(STUN_ATTR_TYPE_MESSAGE_INTEGRITY);
    // stun_attr.length = htons(20);
    auto hmanBuf = HmacSHA1::hmac_sha1(_buffer->buffer(), password);
    string hmac = encode_hmac((char*)hmanBuf.data(), hmanBuf.size());
    _buffer->append(hmac);
    // utils_get_hmac_sha1((char*)_bu, msg->size, password, password_len, (unsigned char*)stun_attr->value);
    _size += sizeof(StunAttribute) + 20;
    // FINGERPRINT

    //   stun_attr1 = (StunAttribute*)(msg->buf + msg->size);
    header->length = htons(header_length + 24 /* HMAC-SHA1 */ + 8 /* FINGERPRINT */);
    // stun_attr1.type = htons(STUN_ATTR_TYPE_FINGERPRINT);
    // stun_attr1.length = htons(4);
    // stun_calculate_fingerprint((char*)msg->buf, msg->size, (uint32_t*)stun_attr1.value);

    uint32_t crc32 = CRC32::encode((unsigned char*)_buffer->data(), _buffer->size()) ^ 0x5354554E;

    string fingerprint = encode_fingerprint(crc32);

    _buffer->append(fingerprint);

    _size += sizeof(StunAttribute) + 4;
}