#ifndef WebrtcP2PManager_H
#define WebrtcP2PManager_H

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>

class WebrtcP2PManager : public std::enable_shared_from_this<WebrtcP2PManager>
{
public:
    using Ptr = std::shared_ptr<WebrtcP2PManager>;
    WebrtcP2PManager();
    ~WebrtcP2PManager();

public:
    static WebrtcP2PManager::Ptr getInstance();

    bool addP2PSdp(const std::string& roomId, const std::string& userId, const std::string& sdp);
    std::string getP2PSdp(const std::string& roomId, const std::string& userId);
    bool delP2PSdp(const std::string& roomId, const std::string& userId);
    bool delP2PManager(const std::string& roomId);

private:
    std::mutex _mtx;
    // roomId userId sdp
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> _p2pManagers;
};

#endif //WebrtcP2PManager_H
