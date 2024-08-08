/**
 * 
 *
 * 
 *
 * 
 * 
 * 
 **/


#include <assert.h>
#include <string.h>

#include <openssl/dh.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/ssl.h>

#include "RtcStun.h"

using namespace std;

#define srs_assert(expression) assert(expression)


SrsBuffer::SrsBuffer()
{
    p = bytes = NULL;
    nb_bytes = 0;
    
    // TODO: support both little and big endian.
    //srs_assert(srs_is_little_endian());
}

SrsBuffer::SrsBuffer(char* b, int nb_b)
{
    p = bytes = b;
    nb_bytes = nb_b;
    
    // TODO: support both little and big endian.
    //srs_assert(srs_is_little_endian());
}

SrsBuffer::~SrsBuffer()
{
}

int SrsBuffer::size()
{
    return nb_bytes;
}

int SrsBuffer::pos()
{
    return (int)(p - bytes);
}

int SrsBuffer::left()
{
    return nb_bytes - (int)(p - bytes);
}

bool SrsBuffer::empty()
{
    return !bytes || (p >= bytes + nb_bytes);
}

bool SrsBuffer::require(int required_size)
{
    srs_assert(required_size >= 0);
    
    return required_size <= nb_bytes - (p - bytes);
}

void SrsBuffer::skip(int size)
{
    srs_assert(p);
    srs_assert(p + size >= bytes);
    srs_assert(p + size <= bytes + nb_bytes);
    
    p += size;
}

int8_t SrsBuffer::read_1bytes()
{
    srs_assert(require(1));
    
    return (int8_t)*p++;
}

int16_t SrsBuffer::read_2bytes()
{
    srs_assert(require(2));
    
    int16_t value;
    char* pp = (char*)&value;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return value;
}

int32_t SrsBuffer::read_3bytes()
{
    srs_assert(require(3));
    
    int32_t value = 0x00;
    char* pp = (char*)&value;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return value;
}

int32_t SrsBuffer::read_4bytes()
{
    srs_assert(require(4));
    
    int32_t value;
    char* pp = (char*)&value;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return value;
}

int64_t SrsBuffer::read_8bytes()
{
    srs_assert(require(8));
    
    int64_t value;
    char* pp = (char*)&value;
    pp[7] = *p++;
    pp[6] = *p++;
    pp[5] = *p++;
    pp[4] = *p++;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return value;
}

string SrsBuffer::read_string(int len)
{
    srs_assert(require(len));
    
    std::string value;
    value.append(p, len);
    
    p += len;
    
    return value;
}

void SrsBuffer::read_bytes(char* data, int size)
{
    srs_assert(require(size));
    
    memcpy(data, p, size);
    
    p += size;
}

void SrsBuffer::write_1bytes(int8_t value)
{
    srs_assert(require(1));
    
    *p++ = value;
}

void SrsBuffer::write_2bytes(int16_t value)
{
    srs_assert(require(2));
    
    char* pp = (char*)&value;
    *p++ = pp[1];
    *p++ = pp[0];
}

void SrsBuffer::write_4bytes(int32_t value)
{
    srs_assert(require(4));
    
    char* pp = (char*)&value;
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
}

void SrsBuffer::write_3bytes(int32_t value)
{
    srs_assert(require(3));
    
    char* pp = (char*)&value;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
}

void SrsBuffer::write_8bytes(int64_t value)
{
    srs_assert(require(8));
    
    char* pp = (char*)&value;
    *p++ = pp[7];
    *p++ = pp[6];
    *p++ = pp[5];
    *p++ = pp[4];
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
}

void SrsBuffer::write_string(string value)
{
    srs_assert(require((int)value.length()));
    
    memcpy(p, value.data(), value.length());
    p += value.length();
}

void SrsBuffer::write_bytes(char* data, int size)
{
    srs_assert(require(size));
    
    memcpy(p, data, size);
    p += size;
}


static srs_error_t hmac_encode(const std::string& algo, const char* key, const int& key_length,  
        const char* input, const int input_length, char* output, unsigned int& output_length)
{
    srs_error_t err = srs_success;

    const EVP_MD* engine = EVP_get_digestbyname(algo.c_str());
	if (!engine) {
		logError << "unknown algo=" << algo;
		return ERROR_RTC_STUN;
	}

#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L
		HMAC_CTX ctx;
		HMAC_CTX_init(&ctx);
		HMAC_Init_ex(&ctx, key, key_length, engine, NULL);
		HMAC_Update(&ctx, (const unsigned char*)input, input_length);
		HMAC_Final(&ctx, (unsigned char*)output, &output_length);
		HMAC_CTX_cleanup(&ctx);
#else
		HMAC_CTX *ctx;
		ctx = HMAC_CTX_new();
		HMAC_Init_ex(ctx, key, key_length, engine, NULL);
		HMAC_Update(ctx, (const unsigned char*)input, input_length);
		HMAC_Final(ctx, (unsigned char*)output, &output_length);
		HMAC_CTX_free(ctx);
#endif
	
    return err;
}

// @see pycrc reflect at https://github.com/winlinvip/pycrc/blob/master/pycrc/algorithms.py#L107
uint64_t __crc32_reflect(uint64_t data, int width)
{
	uint64_t res = data & 0x01;
	
	for (int i = 0; i < (int)width - 1; i++) {
		data >>= 1;
		res = (res << 1) | (data & 0x01);
	}
	
	return res;
}
	
// @see pycrc gen_table at https://github.com/winlinvip/pycrc/blob/master/pycrc/algorithms.py#L178
void __crc32_make_table(uint32_t t[256], uint32_t poly, bool reflect_in)
{
	int width = 32; // 32bits checksum.
	uint64_t msb_mask = (uint32_t)(0x01 << (width - 1));
	uint64_t mask = (uint32_t)(((msb_mask - 1) << 1) | 1);
	
	int tbl_idx_width = 8; // table index size.
	int tbl_width = 0x01 << tbl_idx_width; // table size: 256
	
	for (int i = 0; i < (int)tbl_width; i++) {
		uint64_t reg = uint64_t(i);
		
		if (reflect_in) {
			reg = __crc32_reflect(reg, tbl_idx_width);
		}
		
		reg = reg << (width - tbl_idx_width);
		for (int j = 0; j < tbl_idx_width; j++) {
			if ((reg&msb_mask) != 0) {
				reg = (reg << 1) ^ poly;
			} else {
				reg = reg << 1;
			}
		}
		
		if (reflect_in) {
			reg = __crc32_reflect(reg, width);
		}
		
		t[i] = (uint32_t)(reg & mask);
	}
}
 
// @see pycrc table_driven at https://github.com/winlinvip/pycrc/blob/master/pycrc/algorithms.py#L207
uint32_t __crc32_table_driven(uint32_t* t, const void* buf, int size, uint32_t previous, bool reflect_in, uint32_t xor_in, bool reflect_out, uint32_t xor_out)
{
	int width = 32; // 32bits checksum.
	uint64_t msb_mask = (uint32_t)(0x01 << (width - 1));
	uint64_t mask = (uint32_t)(((msb_mask - 1) << 1) | 1);
	
	int tbl_idx_width = 8; // table index size.
	
	uint8_t* p = (uint8_t*)buf;
	uint64_t reg = 0;
	
	if (!reflect_in) {
		reg = xor_in;
		
		for (int i = 0; i < (int)size; i++) {
			uint8_t tblidx = (uint8_t)((reg >> (width - tbl_idx_width)) ^ p[i]);
			reg = t[tblidx] ^ (reg << tbl_idx_width);
		}
	} else {
		reg = previous ^ __crc32_reflect(xor_in, width);
		
		for (int i = 0; i < (int)size; i++) {
			uint8_t tblidx = (uint8_t)(reg ^ p[i]);
			reg = t[tblidx] ^ (reg >> tbl_idx_width);
		}
		
		reg = __crc32_reflect(reg, width);
	}
	
	if (reflect_out) {
		reg = __crc32_reflect(reg, width);
	}
	
	reg ^= xor_out;
	return (uint32_t)(reg & mask);
}
	
// @see pycrc https://github.com/winlinvip/pycrc/blob/master/pycrc/algorithms.py#L207
// IEEETable is the table for the IEEE polynomial.
static uint32_t __crc32_IEEE_table[256];
static bool __crc32_IEEE_table_initialized = false;

// @see pycrc https://github.com/winlinvip/pycrc/blob/master/pycrc/models.py#L220
//		crc32('123456789') = 0xcbf43926
// where it's defined as model:
//		'name': 		'crc-32',
//		'width':		 32,
//		'poly': 		 0x4c11db7,
//		'reflect_in':	 True,
//		'xor_in':		 0xffffffff,
//		'reflect_out':	 True,
//		'xor_out':		 0xffffffff,
//		'check':		 0xcbf43926,
uint32_t srs_crc32_ieee(const void* buf, int size, uint32_t previous)
{
	// @see golang IEEE of hash/crc32/crc32.go
	// IEEE is by far and away the most common CRC-32 polynomial.
	// Used by ethernet (IEEE 802.3), v.42, fddi, gzip, zip, png, ...
	// @remark The poly of CRC32 IEEE is 0x04C11DB7, its reverse is 0xEDB88320,
	//		please read https://en.wikipedia.org/wiki/Cyclic_redundancy_check
	uint32_t poly = 0x04C11DB7;
	
	bool reflect_in = true;
	uint32_t xor_in = 0xffffffff;
	bool reflect_out = true;
	uint32_t xor_out = 0xffffffff;
	
	if (!__crc32_IEEE_table_initialized) {
		__crc32_make_table(__crc32_IEEE_table, poly, reflect_in);
		__crc32_IEEE_table_initialized = true;
	}
	
	return __crc32_table_driven(__crc32_IEEE_table, buf, size, previous, reflect_in, xor_in, reflect_out, xor_out);
}


SrsStunPacket::SrsStunPacket()
{
    message_type = 0;
    local_ufrag = "";
    remote_ufrag = "";
    use_candidate = false;
    ice_controlled = false;
    ice_controlling = false;
}

SrsStunPacket::~SrsStunPacket()
{
}

bool SrsStunPacket::is_binding_request() const
{
    return message_type == BindingRequest;
}

bool SrsStunPacket::is_binding_response() const
{
    return message_type == BindingResponse;
}

uint16_t SrsStunPacket::get_message_type() const
{
    return message_type;
}

std::string SrsStunPacket::get_username() const
{
    return username;
}

std::string SrsStunPacket::get_local_ufrag() const
{
    return local_ufrag;
}

std::string SrsStunPacket::get_remote_ufrag() const
{
    return remote_ufrag;
}

std::string SrsStunPacket::get_transcation_id() const
{
    return transcation_id;
}

uint32_t SrsStunPacket::get_mapped_address() const
{
    return mapped_address;
}

uint16_t SrsStunPacket::get_mapped_port() const
{
    return mapped_port;
}

bool SrsStunPacket::get_ice_controlled() const
{
    return ice_controlled;
}

bool SrsStunPacket::get_ice_controlling() const
{
    return ice_controlling;
}

bool SrsStunPacket::get_use_candidate() const
{
    return use_candidate;
}

void SrsStunPacket::set_message_type(const uint16_t& m)
{
    message_type = m;
}

void SrsStunPacket::set_local_ufrag(const std::string& u)
{
    local_ufrag = u;
}

void SrsStunPacket::set_remote_ufrag(const std::string& u)
{
    remote_ufrag = u;
}

void SrsStunPacket::set_transcation_id(const std::string& t)
{
    transcation_id = t;
}

void SrsStunPacket::set_mapped_address(const uint32_t& addr)
{
    mapped_address = addr;
}

void SrsStunPacket::set_mapped_port(const uint32_t& port)
{
    mapped_port = port;
}

srs_error_t SrsStunPacket::decode(const char* buf, const int nb_buf)
{
    srs_error_t err = srs_success;

    SrsBuffer* stream = new SrsBuffer(const_cast<char*>(buf), nb_buf);
    SrsAutoFree(SrsBuffer, stream);

    if (stream->left() < 20) {
		logError << "invalid stun packet, size=" << stream->size();
		return ERROR_RTC_STUN;
    }

    message_type = stream->read_2bytes();
    uint16_t message_len = stream->read_2bytes();
    string magic_cookie = stream->read_string(4);
    transcation_id = stream->read_string(12);

    if (nb_buf != 20 + message_len) {
		logError << "invalid stun packet, message_len=" << message_len << ", nb_buf=" << nb_buf;
        return ERROR_RTC_STUN;
    }

    while (stream->left() >= 4) {
        uint16_t type = stream->read_2bytes();
        uint16_t len = stream->read_2bytes();

        if (stream->left() < len) {
			logError << "invalid stun packet";
            return ERROR_RTC_STUN;
        }

        string val = stream->read_string(len);
        // padding
        if (len % 4 != 0) {
            stream->read_string(4 - (len % 4));
        }

        switch (type) {
            case Username: {
                username = val;
                size_t p = val.find(":");
                if (p != string::npos) {
                    local_ufrag = val.substr(0, p);
                    remote_ufrag = val.substr(p + 1);
                }
                break;
            }

			case UseCandidate: {
                use_candidate = true;
                break;
            }

            // @see: https://tools.ietf.org/html/draft-ietf-ice-rfc5245bis-00#section-5.1.2
			// One agent full, one lite:  The full agent MUST take the controlling
            // role, and the lite agent MUST take the controlled role.  The full
            // agent will form check lists, run the ICE state machines, and
            // generate connectivity checks.
			case IceControlled: {
                ice_controlled = true;
                break;
            }

			case IceControlling: {
                ice_controlling = true;
                break;
            }
            
            default: {
                break;
            }
        }
    }

    return err;
}

srs_error_t SrsStunPacket::encode(const string& pwd, SrsBuffer* stream)
{
    if (is_binding_response()) {
        return encode_binding_response(pwd, stream);
    }

	logError << "unknown stun type=" << get_message_type();
    return ERROR_RTC_STUN;
}

// FIXME: make this function easy to read
srs_error_t SrsStunPacket::encode_binding_response(const string& pwd, SrsBuffer* stream)
{
    srs_error_t err = srs_success;

    string property_username = encode_username();
    string mapped_address = encode_mapped_address();

    stream->write_2bytes(BindingResponse);
    stream->write_2bytes(property_username.size() + mapped_address.size());
    stream->write_4bytes(kStunMagicCookie);
    stream->write_string(transcation_id);
    stream->write_string(property_username);
    stream->write_string(mapped_address);

    stream->data()[2] = ((stream->pos() - 20 + 20 + 4) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->pos() - 20 + 20 + 4) & 0x000000FF);

    char hmac_buf[20] = {0};
    unsigned int hmac_buf_len = 0;
    if ((err = hmac_encode("sha1", pwd.c_str(), pwd.size(), stream->data(), stream->pos(), hmac_buf, hmac_buf_len)) != srs_success) {
		logError << "hmac encode failed";
		return err;
    }

    string hmac = encode_hmac(hmac_buf, hmac_buf_len);

    stream->write_string(hmac);
    stream->data()[2] = ((stream->pos() - 20 + 8) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->pos() - 20 + 8) & 0x000000FF);

    uint32_t crc32 = srs_crc32_ieee(stream->data(), stream->pos(), 0) ^ 0x5354554E;

    string fingerprint = encode_fingerprint(crc32);

    stream->write_string(fingerprint);

    stream->data()[2] = ((stream->pos() - 20) & 0x0000FF00) >> 8;
    stream->data()[3] = ((stream->pos() - 20) & 0x000000FF);

    return err;
}

string SrsStunPacket::encode_username()
{
    char buf[1460];
    SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    SrsAutoFree(SrsBuffer, stream);

    string username = remote_ufrag + ":" + local_ufrag;

    stream->write_2bytes(Username);
    stream->write_2bytes(username.size());
    stream->write_string(username);

    if (stream->pos() % 4 != 0) {
        static char padding[4] = {0};
        stream->write_bytes(padding, 4 - (stream->pos() % 4));
    }

    return string(stream->data(), stream->pos());
}

string SrsStunPacket::encode_mapped_address()
{
    char buf[1460];
    SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    SrsAutoFree(SrsBuffer, stream);

    stream->write_2bytes(XorMappedAddress);
    stream->write_2bytes(8);
    stream->write_1bytes(0); // ignore this bytes
    stream->write_1bytes(1); // ipv4 family
    stream->write_2bytes(mapped_port ^ (kStunMagicCookie >> 16));
    stream->write_4bytes(mapped_address ^ kStunMagicCookie);

    return string(stream->data(), stream->pos());
}

string SrsStunPacket::encode_hmac(char* hmac_buf, const int hmac_buf_len)
{
    char buf[1460];
    SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    SrsAutoFree(SrsBuffer, stream);

    stream->write_2bytes(MessageIntegrity);
    stream->write_2bytes(hmac_buf_len);
    stream->write_bytes(hmac_buf, hmac_buf_len);

    return string(stream->data(), stream->pos());
}

string SrsStunPacket::encode_fingerprint(uint32_t crc32)
{
    char buf[1460];
    SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    SrsAutoFree(SrsBuffer, stream);

    stream->write_2bytes(Fingerprint);
    stream->write_2bytes(4);
    stream->write_4bytes(crc32);

    return string(stream->data(), stream->pos());
}
