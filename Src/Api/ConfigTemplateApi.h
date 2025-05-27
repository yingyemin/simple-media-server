#ifndef ConfigTemplateApi_h
#define ConfigTemplateApi_h

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"
#include "Common/json.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace std;
using json = nlohmann::json;

// 配置模板结构
struct ConfigTemplate {
    string id;              // 模板ID
    string name;            // 模板名称
    string description;     // 模板描述
    string category;        // 模板分类（rtmp, rtsp, webrtc等）
    string configData;      // 配置数据（JSON格式）
    string createTime;      // 创建时间
    string updateTime;      // 更新时间
    string author;          // 创建者
    bool isDefault;         // 是否为默认模板
};

// 配置模板管理器
class ConfigTemplateManager
{
public:
    static ConfigTemplateManager& instance();

    // 模板管理
    bool createTemplate(const ConfigTemplate& tmpl);
    bool updateTemplate(const string& id, const ConfigTemplate& tmpl);
    bool deleteTemplate(const string& id);
    ConfigTemplate* getTemplate(const string& id);
    vector<ConfigTemplate> getAllTemplates();
    vector<ConfigTemplate> getTemplatesByCategory(const string& category);

    // 模板应用
    bool applyTemplate(const string& templateId, const string& targetPath, const string& params);

    // 导入导出
    string exportTemplate(const string& id);
    string exportAllTemplates();
    bool importTemplate(const string& templateData);
    bool importTemplates(const string& templatesData);

    // 默认模板
    void loadDefaultTemplates();

private:
    ConfigTemplateManager() = default;
    unordered_map<string, ConfigTemplate> _templates;
    mutable mutex _mtx;

    string generateTemplateId();
    bool validateTemplate(const ConfigTemplate& tmpl, string& errorMsg);
};

class ConfigTemplateApi
{
public:
    static void initApi();

    // 模板CRUD操作
    static void createTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void getTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void updateTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void deleteTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void listTemplates(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);

    // 模板应用
    static void applyTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);

    // 导入导出
    static void exportTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void importTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);

    // 默认模板
    static void getDefaultTemplates(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc);

private:
    // 辅助方法
    static json templateToJson(const ConfigTemplate& tmpl);
    static ConfigTemplate jsonToTemplate(const json& j);
    static bool validateTemplateRequest(const json& requestData, string& errorMsg);
};

#endif //ConfigTemplateApi_h
