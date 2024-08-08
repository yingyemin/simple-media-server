#ifndef UrlParser_H
#define UrlParser_H

#include <string>
#include <unordered_map>

using namespace std;

class UrlParser {
public:
    void parse(string url);
    static string urlEncode(const string& url, bool convert_space_to_plus = true);
    static std::string urlDecode(const std::string& url, bool convert_space_to_plus = true);

public:
    int port_ = 0;
    string protocol_;
    string host_;
    string path_;
    string param_;
    string url_;
    string vhost_ = "default";
    string type_ = "default";
    unordered_map<string, string> vecParam_;

private:
};



#endif //RtspSplitter_H
