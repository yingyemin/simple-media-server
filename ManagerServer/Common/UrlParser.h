#ifndef UrlParser_H
#define UrlParser_H

#include <string>
#include <unordered_map>

// using namespace std;

class UrlParser {
public:
    void parse(std::string url);
    static std::string urlEncode(const std::string& url, bool convert_space_to_plus = true);
    static std::string urlDecode(const std::string& url, bool convert_space_to_plus = true);

public:
    int port_ = 0;
    std::string protocol_;
    std::string host_;
    std::string path_;
    std::string param_;
    std::string url_;
    std::string vhost_ = "default";
    std::string type_ = "default";
    std::string username_;
    std::string password_;
    std::unordered_map<std::string, std::string> vecParam_;

private:
};



#endif //RtspSplitter_H
