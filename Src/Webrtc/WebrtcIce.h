#ifndef WebrtcIce_H
#define WebrtcIce_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "Net/Socket.h"
#include "Net/Address.h"
#include "WebrtcSdpParser.h"

enum IceCandidateType
{
  ICE_CANDIDATE_TYPE_HOST,
  ICE_CANDIDATE_TYPE_SRFLX, //stun
  ICE_CANDIDATE_TYPE_PRFLX,
  ICE_CANDIDATE_TYPE_RELAY, //turn
};

namespace std {
    template<>
    struct hash<IceCandidateType>
    {
        size_t operator()(IceCandidateType type) const
        {
            return std::hash<int>()(type);
        }
    };
}

class WebrtcIce : public std::enable_shared_from_this<WebrtcIce>
{
public:
    using Ptr = std::shared_ptr<WebrtcIce>;

    WebrtcIce(const std::vector<std::string>& urls);
    ~WebrtcIce();

public:
    void gatherNetwork(const std::string& ifacePrefix);
    std::vector<InterfaceInfo> getInterfaces();
    Address getCandidate(IceCandidateType type);
    void addCandidate(const CandidateInfo::Ptr& candidate);
    void check();
    void setSocketHost(const Socket::Ptr& socket);
    Socket::Ptr getSocketHost() {return _socketHost;}
    Socket::Ptr getSocketTurn() {return _socketTurn;}
    Socket::Ptr getSocketStun() {return _socketStun;}

private:
    void gatherStunAddr(const std::string& url);
    void gatherTurnAddr(const std::string& url, const std::string& username, const std::string& credential);
    void gatherTurnAddr_l(const StreamBuffer::Ptr& buffer, const std::string& username, const std::string& credential);

private:
    sockaddr_storage _stunAddr;
    Socket::Ptr _socketStun;
    sockaddr_storage _turnAddr;
    Socket::Ptr _socketTurn;
    Socket::Ptr _socketHost;
    sockaddr_storage _hostAddr;
    std::vector<std::string> _urls;
    std::vector<InterfaceInfo> _interfaces;
    std::unordered_map<IceCandidateType, Address*> _candidates;
    std::vector<CandidateInfo::Ptr> _peerHost;
    std::unordered_map<IceCandidateType, CandidateInfo::Ptr> _peerCandidates;
    std::unordered_map<IceCandidateType, pair<Address*, CandidateInfo::Ptr>> _pairAddr;
};

#endif //WebrtcIce_H
