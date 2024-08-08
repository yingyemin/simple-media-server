#include "HttpUtil.h"
#include "Logger.h"

#include <unordered_map>

using namespace std;

static std::unordered_map<int, std::string> g_statusDesc = {
    {100,	"Continue"},
    {101,	"Switching Protocols"},	                     
    {200,	"OK"},	
    {201,	"Created"}	,
    {202,	"Accepted"	},
    {203,	"Non-Authoritative Information"},
    {204,	"No Content"	},
    {205,	"Reset Content"	},
    {206,	"Partial Content"	},
    {300,	"Multiple Choices"	},
    {301,	"Moved Permanently"	},
    {302,	"Found"	},
    {303,	"See Other"	},
    {304,	"Not Modified"	},
    {305,	"Use Proxy"	},
    {306,	"Unused"	},
    {307,	"Temporary Redirect"	},
    {400,	"Bad Request"	},
    {401,	"Unauthorized"	},
    {402,	"Payment Required"	},
    {403,	"Forbidden"	},
    {404,	"Not Found"	},
    {405,	"Method Not Allowed"	},
    {406,	"Not Acceptable"	},
    {407,	"Proxy Authentication Required"	},
    {408,	"Request Time-out"	},
    {409,	"Conflict"	},
    {410,	"Gone"	},
    {411,	"Length Required"},
    {412,	"Precondition Failed"	},
    {413,	"Request Entity Too Large"	},
    {414,	"Request-URI Too Large"	},
    {415,	"Unsupported Media Type"	},
    {416,	"Requested range not satisfiable"	},
    {417,	"Expectation Failed"	},
    {500,	"Internal Server Error"	},
    {501,	"Not Implemented	"},
    {502,	"Bad Gateway"	},
    {503,	"Service Unavailable"	},
    {504,	"Gateway Time-out"},
    {505,	"HTTP Version not supported"}
};

std::unordered_map<std::string, std::string> g_fileMimes = {
    {".apk", "application/vnd.android.package-archive"},
    {".avi", "video/x-msvideo"},
    {".buffer", "application/octet-stream"},
    {".cer", "application/pkix-cert"},
    {".chm", "application/vnd.ms-htmlhelp"},
    {".conf", "text/plain"},
    {".cpp","text/x-c"},
    {".crt", "application/x-x509-ca-cert"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".exe", "application/x-msdownload"},
    {".flac", "audio/x-flac"},
    {".flv", "video/x-flv"},
    {".gif", "image/gif"},
    {".h263", "video/h263"},
    {".h264", "video/h264"},
    {".htm","text/html"},
    {".html", "text/html"},
    {".ico", "image/x-icon"},
    {".ini", "text/plain"},
    {".ink", "application/inkml+xml"},
    {".iso", "application/x-iso9660-image"},
    {".jar", "application/java-archive"},
    {".java", "text/x-java-source"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".json5", "application/json5"},
    {".jsx", "text/jsx"},
    {".list", "text/plain"},
    {".lnk", "application/x-ms-shortcut"},
    {".log", "text/plain"},
    {".m3u8", "application/vnd.apple.mpegurl"},
    {".manifest", "text/cache-manifest"},
    {".map", "application/json"},
    {".markdown", "text/x-markdown"},
    {".md", "text/x-markdown"},
    {".mov", "video/quicktime"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".msi", "application/x-msdownload"},
    {".ogg", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".otf", "font/opentype"},
    {".pdf", "application/pdf"},
    {".png", "image/png"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".psd", "image/vnd.adobe.photoshop"},
    {".rar", "application/x-rar-compressed"},
    {".rm", "application/vnd.rn-realmedia"},
    {".rmvb", "application/vnd.rn-realmedia-vbr"},
    {".roff", "text/troff"},
    {".sass", "text/x-sass"},
    {".scss", "text/x-scss"},
    {".sh", "application/x-sh"},
    {".sql", "application/x-sql"},
    {".svg", "image/svg+xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tar", "application/x-tar"},
    {".text", "text/plain"},
    {".torrent", "application/x-bittorrent"},
    {".ttf", "application/x-font-ttf"},
    {".txt", "text/plain"},
    {".wav", "audio/x-wav"},
    {".webm", "video/webm"},
    {".wm", "video/x-ms-wm"},
    {".wma", "audio/x-ms-wma"},
    {".wmx", "video/x-ms-wmx"},
    {".woff", "application/font-woff"},
    {".woff2", "application/font-woff2"},
    {".wps", "application/vnd.ms-works"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "application/xml"},
    {".xz", "application/x-xz"},
    {".yaml", "text/yaml"},
    {".yml", "text/yaml"},
    {".zip", "application/zip"}
};


string HttpUtil::getStatusDesc(int status)
{
    auto it  = g_statusDesc.find(status);
        if(it != g_statusDesc.end()){
            return it->second;
        }
        return "Unknow";

}


string HttpUtil::getMimeType(const std::string& filename)
{
    size_t pos = filename.rfind('.');
    if(pos == std::string::npos) {
        return "application/octet-stream";
    }
    std::string mime = filename.substr(pos);
    auto it = g_fileMimes.find(mime);
    if(it != g_fileMimes.end()){
        return it->second;
    }
    return "application/octet-stream";
}