/*
 * 
 *
 * 
 *
 * 
 * 
 * 
 */


#ifndef CTVSS_RTCSTUN_H
#define CTVSS_RTCSTUN_H

#include "RtcSrs.h"

class SrsBuffer
{
private:
    // current position at bytes.
    char* p;
    // the bytes data for stream to read or write.
    char* bytes;
    // the total number of bytes.
    int nb_bytes;
public:
    SrsBuffer();
    // Initialize buffer with data b and size nb_b.
    // @remark User must free the data b.
    SrsBuffer(char* b, int nb_b);
    virtual ~SrsBuffer();
// get the status of stream
public:
    /**
     * get data of stream, set by initialize.
     * current bytes = data() + pos()
     */
    inline char* data() { return bytes; }
    inline char* head() { return p; }
    /**
     * the total stream size, set by initialize.
     * left bytes = size() - pos().
     */
    virtual int size();
    /**
     * tell the current pos.
     */
    virtual int pos();
    // Left bytes in buffer, total size() minus the current pos().
    virtual int left();
    /**
     * whether stream is empty.
     * if empty, user should never read or write.
     */
    virtual bool empty();
    /**
     * whether required size is ok.
     * @return true if stream can read/write specified required_size bytes.
     * @remark assert required_size positive.
     */
    virtual bool require(int required_size);
    // to change stream.
public:
    /**
     * to skip some size.
     * @param size can be any value. positive to forward; nagetive to backward.
     * @remark to skip(pos()) to reset stream.
     * @remark assert initialized, the data() not NULL.
     */
    virtual void skip(int size);
public:
    /**
     * get 1bytes char from stream.
     */
    virtual int8_t read_1bytes();
    /**
     * get 2bytes int from stream.
     */
    virtual int16_t read_2bytes();
    /**
     * get 3bytes int from stream.
     */
    virtual int32_t read_3bytes();
    /**
     * get 4bytes int from stream.
     */
    virtual int32_t read_4bytes();
    /**
     * get 8bytes int from stream.
     */
    virtual int64_t read_8bytes();
    /**
     * get string from stream, length specifies by param len.
     */
    virtual std::string read_string(int len);
    /**
     * get bytes from stream, length specifies by param len.
     */
    virtual void read_bytes(char* data, int size);
public:
    /**
     * write 1bytes char to stream.
     */
    virtual void write_1bytes(int8_t value);
    /**
     * write 2bytes int to stream.
     */
    virtual void write_2bytes(int16_t value);
    /**
     * write 4bytes int to stream.
     */
    virtual void write_4bytes(int32_t value);
    /**
     * write 3bytes int to stream.
     */
    virtual void write_3bytes(int32_t value);
    /**
     * write 8bytes int to stream.
     */
    virtual void write_8bytes(int64_t value);
    /**
     * write string to stream
     */
    virtual void write_string(std::string value);
    /**
     * write bytes to stream
     */
    virtual void write_bytes(char* data, int size);
};

// @see: https://tools.ietf.org/html/rfc5389
// The magic cookie field MUST contain the fixed value 0x2112A442 in network byte order
const uint32_t kStunMagicCookie = 0x2112A442;

enum SrsStunMessageType
{
	// see @ https://tools.ietf.org/html/rfc3489#section-11.1	
    BindingRequest            = 0x0001,
    BindingResponse           = 0x0101,
    BindingErrorResponse      = 0x0111,
    SharedSecretRequest       = 0x0002,
    SharedSecretResponse      = 0x0102,
    SharedSecretErrorResponse = 0x0112,
};

enum SrsStunMessageAttribute
{
    // see @ https://tools.ietf.org/html/rfc3489#section-11.2
	MappedAddress     = 0x0001,
   	ResponseAddress   = 0x0002,
   	ChangeRequest     = 0x0003,
   	SourceAddress     = 0x0004,
   	ChangedAddress    = 0x0005,
   	Username          = 0x0006,
   	Password          = 0x0007,
   	MessageIntegrity  = 0x0008,
   	ErrorCode         = 0x0009,
   	UnknownAttributes = 0x000A,
   	ReflectedFrom     = 0x000B,

    // see @ https://tools.ietf.org/html/rfc5389#section-18.2
    Realm             = 0x0014,
    Nonce             = 0x0015,
    XorMappedAddress  = 0x0020,
    Software          = 0x8022,
    AlternateServer   = 0x8023,
    Fingerprint       = 0x8028,

    Priority          = 0x0024,
    UseCandidate      = 0x0025,
    IceControlled     = 0x8029,
    IceControlling    = 0x802A,
};

class SrsStunPacket 
{
private:
    uint16_t message_type;
    std::string username;
    std::string local_ufrag;
    std::string remote_ufrag;
    std::string transcation_id;
    uint32_t mapped_address;
    uint16_t mapped_port;
    bool use_candidate;
    bool ice_controlled;
    bool ice_controlling;
public:
    SrsStunPacket();
    virtual ~SrsStunPacket();
public:
    bool is_binding_request() const;
    bool is_binding_response() const;
    uint16_t get_message_type() const;
    std::string get_username() const;
    std::string get_local_ufrag() const;
    std::string get_remote_ufrag() const;
    std::string get_transcation_id() const;
    uint32_t get_mapped_address() const;
    uint16_t get_mapped_port() const;
    bool get_ice_controlled() const;
    bool get_ice_controlling() const;
    bool get_use_candidate() const;
    void set_message_type(const uint16_t& m);
    void set_local_ufrag(const std::string& u);
    void set_remote_ufrag(const std::string& u);
    void set_transcation_id(const std::string& t);
    void set_mapped_address(const uint32_t& addr);
    void set_mapped_port(const uint32_t& port);
    srs_error_t decode(const char* buf, const int nb_buf);
    srs_error_t encode(const std::string& pwd, SrsBuffer* stream);
private:
    srs_error_t encode_binding_response(const std::string& pwd, SrsBuffer* stream);
    std::string encode_username();
    std::string encode_mapped_address();
    std::string encode_hmac(char* hamc_buf, const int hmac_buf_len);
    std::string encode_fingerprint(uint32_t crc32);
};

#endif
