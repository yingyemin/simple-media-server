#ifndef WebrtcStun_h
#define WebrtcStun_h

#include <string>
#include <netinet/in.h>

#include "Net/Buffer.h"
#include "Net/Address.h"

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

enum StunFamily {
    STUN_FAMILY_IPV4 = 0x01,
    STUN_FAMILY_IPV6 = 0x02,
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
    XorRelayedAddress = 0x0016,
    XorMappedAddress  = 0x0020,
    Software          = 0x8022,
    AlternateServer   = 0x8023,
    Fingerprint      = 0x8028,

    Priority          = 0x0024,
    UseCandidate      = 0x0025,
    IceControlled     = 0x8029,
    IceControlling    = 0x802A,
};

enum StunAttrType {

    STUN_ATTR_TYPE_MAPPED_ADDRESS = 0x0001,
    STUN_ATTR_TYPE_USERNAME = 0x0006,
    STUN_ATTR_TYPE_MESSAGE_INTEGRITY = 0x0008,
    STUN_ATTR_TYPE_LIFETIME = 0x000d,
    STUN_ATTR_TYPE_REALM = 0x0014,
    STUN_ATTR_TYPE_NONCE = 0x0015,
    STUN_ATTR_TYPE_XOR_RELAYED_ADDRESS = 0x0016,
    STUN_ATTR_TYPE_REQUESTED_TRANSPORT = 0x0019,
    STUN_ATTR_TYPE_XOR_MAPPED_ADDRESS = 0x0020,
    STUN_ATTR_TYPE_PRIORITY = 0x0024,
    STUN_ATTR_TYPE_USE_CANDIDATE = 0x0025,
    STUN_ATTR_TYPE_FINGERPRINT = 0x8028,
    STUN_ATTR_TYPE_ICE_CONTROLLED = 0x8029,
    STUN_ATTR_TYPE_ICE_CONTROLLING = 0x802a,
    STUN_ATTR_TYPE_SOFTWARE = 0x8022,
    // https://datatracker.ietf.org/doc/html/draft-thatcher-ice-network-cost-00
    STUN_ATTR_TYPE_NETWORK_COST = 0xc057,

};

enum StunCredential {

    STUN_CREDENTIAL_SHORT_TERM = 0x0001,
    STUN_CREDENTIAL_LONG_TERM = 0x0002,

};

struct StunHeader {
    uint16_t type;
    uint16_t length;
    uint32_t magic_cookie;
    uint32_t transaction_id[3];
};

struct StunAttribute {
    uint16_t type;
    uint16_t length;
    char value[0];
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
    Buffer::Ptr getBuffer() const;
    std::string getRealm() const;
    std::string getNonce() const;

    void setType(const uint16_t& type);
    void setLocalUfrag(const std::string& uflag);
    void setRemoteUfrag(const std::string& uflag);
    void setTranscationId(const std::string& transId);
    void setMappedAddress(const sockaddr* addr);
    void setMappedPort(const uint32_t& port);

    int32_t parse(const char* buf, const int32_t nb_buf);
    int32_t toBuffer(const std::string& pwd, StringBuffer::Ptr stream);
    Address* getMappedAddr(int type);

    void createHeader();
    void createAttribute(StunAttrType type, uint16_t length, char* value);
    void createAttribute(StunAttrType type, const std::string& value);
    void finish(StunCredential credential, std::string& password);

private:
    int32_t encodeBindingRequest(const std::string& pwd, StringBuffer::Ptr stream);
    int32_t encodeBindingResponse(const std::string& pwd, StringBuffer::Ptr stream);
    std::string encodeUsername();
    std::string encodeMappedAddress();
    void parseMappedAddress(const std::string& val, uint8_t* mask, Address* addr);

private:
    uint16_t _messageType;
    std::string _username;
    std::string _realm;
    std::string _nonce;
    std::string _localUfrag;
    std::string _remoteUfrag;
    std::string _transcationId;
    sockaddr* _addr;
    Address _mapAddr;
    Address _relayAddr;
    size_t _size;
    StringBuffer::Ptr _buffer;
    uint32_t _mappedAddress;
    uint16_t _mappedPort;
    bool _useCandidate;
    bool _iceControlled;
    bool _iceControlling;
};

#endif //WebrtcStun_h