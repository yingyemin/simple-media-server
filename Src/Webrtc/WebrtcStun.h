#ifndef WebrtcStun_h
#define WebrtcStun_h

#include <string>
#include <netinet/in.h>

#include "Net/Buffer.h"

using namespace std;

const uint32_t kStunMagicCookie = 0x2112A442;

enum YangStunMessageType
{
	// see @ https://tools.ietf.org/html/rfc3489#section-11.1	
    BindingRequest            = 0x0001,
    BindingResponse           = 0x0101,
    BindingErrorResponse      = 0x0111,
    SharedSecretRequest       = 0x0002,
    SharedSecretResponse      = 0x0102,
    SharedSecretErrorResponse = 0x0112,
};

enum YangStunMessageAttribute
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
    Fingerprint      = 0x8028,

    Priority          = 0x0024,
    UseCandidate      = 0x0025,
    IceControlled     = 0x8029,
    IceControlling    = 0x802A,
};


class WebrtcStun 
{
public:
    WebrtcStun();
    virtual ~WebrtcStun();

public:
    bool isBindingRequest() const;
    bool isBindingResponse() const;

    uint16_t getType() const;
    std::string getUsername() const;
    std::string getLocalUfrag() const;
    std::string getRemoteUfrag() const;
    std::string getTranscationId() const;
    uint32_t getMappedAddress() const;
    uint16_t getMappedPort() const;
    bool getIceControlled() const;
    bool getIceControlling() const;
    bool getUseCandidate() const;

    void setType(const uint16_t& type);
    void setLocalUfrag(const std::string& uflag);
    void setRemoteUfrag(const std::string& uflag);
    void setTranscationId(const std::string& transId);
    void setMappedAddress(const sockaddr* addr);
    void setMappedPort(const uint32_t& port);

    int32_t parse(const char* buf, const int32_t nb_buf);
    int32_t toBuffer(const std::string& pwd, StringBuffer::Ptr stream);
private:
    int32_t encodeBindingRequest(const std::string& pwd, StringBuffer::Ptr stream);
    int32_t encodeBindingResponse(const std::string& pwd, StringBuffer::Ptr stream);
    std::string encodeUsername();
    std::string encodeMappedAddress();
private:
    uint16_t _messageType;
    std::string _username;
    std::string _localUfrag;
    std::string _remoteUfrag;
    std::string _transcationId;
    sockaddr* _addr;
    uint32_t _mappedAddress;
    uint16_t _mappedPort;
    bool _useCandidate;
    bool _iceControlled;
    bool _iceControlling;
};

#endif //WebrtcStun_h