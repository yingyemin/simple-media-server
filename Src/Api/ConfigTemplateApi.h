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

// using namespace std;
using json = nlohmann::json;

// 配置模板结构
struct ConfigTemplate {
    std::string id;              // 模板ID
    std::string name;            // 模板名称
    std::string description;     // 模板描述
    std::string category;        // 模板分类（rtmp, rtsp, webrtc等）
    std::string configData;      // 配置数据（JSON格式）
    std::string createTime;      // 创建时间
    std::string updateTime;      // 更新时间
    std::string author;          // 创建者
    bool isDefault;         // 是否为默认模板
};

// 配置模板管理器
class ConfigTemplateManager
{
public:
    static ConfigTemplateManager& instance();

    // 模板管理
    bool createTemplate(const ConfigTemplate& tmpl);
    bool updateTemplate(const std::string& id, const ConfigTemplate& tmpl);
    bool deleteTemplate(const std::string& id);
    ConfigTemplate* getTemplate(const std::string& id);
    std::vector<ConfigTemplate> getAllTemplates();
    std::vector<ConfigTemplate> getTemplatesByCategory(const std::string& category);

    // 模板应用
    bool applyTemplate(const std::string& templateId, const std::string& targetPath, const std::string& params);

    // 导入导出
    std::string exportTemplate(const std::string& id);
    std::string exportAllTemplates();
    bool importTemplate(const std::string& templateData);
    bool importTemplates(const std::string& templatesData);

    // 默认模板
    void loadDefaultTemplates();

private:
    ConfigTemplateManager() = default;
    std::unordered_map<std::string, ConfigTemplate> _templates;
    mutable std::mutex _mtx;

    std::string generateTemplateId();
    bool validateTemplate(const ConfigTemplate& tmpl, std::string& errorMsg);
};

class ConfigTemplateApi
{
public:
    static void initApi();

    // 模板CRUD操作
    static void createTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void getTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void updateTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void deleteTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void listTemplates(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    // 模板应用
    static void applyTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    // 导入导出
    static void exportTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void importTemplate(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    // 默认模板
    static void getDefaultTemplates(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

private:
    // 辅助方法
    static json templateToJson(const ConfigTemplate& tmpl);
    static ConfigTemplate jsonToTemplate(const json& j);
    static bool validateTemplateRequest(const json& requestData, std::string& errorMsg);
};

#endif //ConfigTemplateApi_h