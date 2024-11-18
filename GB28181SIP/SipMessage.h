/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Lixin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SIPMESSAGE_H
#define SIPMESSAGE_H

#include <string>
#include <vector>
#include <sstream>
#include <memory>

// SIP methods
#define SIP_METHOD_REGISTER       "REGISTER"
#define SIP_METHOD_MESSAGE        "MESSAGE"
#define SIP_METHOD_INVITE         "INVITE"
#define SIP_METHOD_ACK            "ACK"
#define SIP_METHOD_BYE            "BYE"

#define SIP_CALLID_REGISTER       "20000"
#define SIP_CALLID_KEEPALIVE      "20001"
#define SIP_CALLID_CATALOG        "20002"
#define SIP_CALLID_RECORDINFO     "20003"
#define SIP_CALLID_DEVICEINFO     "20004"
#define SIP_CALLID_RECORDSTATUS   "20005"
#define SIP_CALLID_DEVICESTATUS   "20006"

// SIP-Version
#define SIP_VERSION "SIP/2.0"
#define SIP_USER_AGENT "SimpleGb28181SipServer"

// std::vector<std::string> string_split(std::string str, std::string flag);
// std::string string_trim_start(std::string str, std::string trim_chars);
// std::string string_trim_end(std::string str, std::string trim_chars);

enum SipCmdType{
    SipCmdRequest=0,
    SipCmdRespone=1
};

class SipRequest
{
public:
    using Ptr = std::shared_ptr<SipRequest>;
    //sip header member
    std::string method;
    std::string uri;
    std::string version;
    std::string status;

    std::string via;
    std::string from;
    std::string to;
    std::string from_tag;
    std::string to_tag;
    std::string branch;
    
    std::string call_id;
    long seq;

    std::string contact;
    std::string user_agent;

    std::string content_type;
    std::string content;
    long content_length;

    long expires;
    int max_forwards;

    std::string www_authenticate;
    std::string authorization;

public:
    std::string serial;
    std::string realm;
    std::string sip_auth_id;
    std::string sip_auth_pwd;
    std::string sip_channel_id;
    std::string sip_username;
    std::string peer_ip;
    int peer_port;
    std::string host;
    int host_port;
    SipCmdType cmdtype;

public:
    bool _start = false;
    // DevChannel::Ptr _device;
    // SdpParser sdp;
public:
    SipRequest();
    virtual ~SipRequest();
public:
    virtual bool is_register();
    virtual bool is_invite();
    virtual bool is_message();
    virtual bool is_ack();
    virtual bool is_bye();
   
    virtual void copy(SipRequest* src);
public:
    virtual std::string get_cmdtype_str();
};

// The gb28181 sip protocol stack.
class SipStack
{
private:
    // The cached bytes buffer.
    std::vector<char> buf;
    long sn;
    std::string _role;
public:
    SipStack(const std::string& role = "server");
    virtual ~SipStack();
public:
    virtual int parse_request(std::shared_ptr<SipRequest>& preq, const char *recv_msg, int nb_buf);
protected:
    virtual int do_parse_request(std::shared_ptr<SipRequest> req, const char *recv_msg);

public:
    virtual void resp_status(std::stringstream& ss, std::shared_ptr<SipRequest> req);
    virtual void resp_keepalive(std::stringstream& ss, std::shared_ptr<SipRequest> req);
    virtual void resp_ack(std::stringstream& ss, std::shared_ptr<SipRequest> req);
    virtual void resp_401_unauthorized(std::stringstream& ss, std::shared_ptr<SipRequest> req);
    virtual void req_query_catalog(std::stringstream& ss, std::shared_ptr<SipRequest> req);
     
    virtual std::string req_invite(std::stringstream& ss, std::shared_ptr<SipRequest> req, std::string ip, int port, uint32_t ssrc);
	virtual void req_invite_playback(std::stringstream& ss, std::shared_ptr<SipRequest> req, std::string ip, 
        int port, uint32_t ssrc, std::string start_time, std::string end_time);
    virtual void req_bye(std::stringstream& ss, std::shared_ptr<SipRequest> req);
    virtual std::string req_record_info(std::stringstream& ss, std::shared_ptr<SipRequest> req, 
                    const std::string& deviceId, const std::string& startTime, const std::string& endTime);
    virtual void req_register(std::stringstream& ss, std::shared_ptr<SipRequest> req);
    virtual void req_registerWithAuth(std::stringstream& ss, std::shared_ptr<SipRequest> req);
    virtual void req_keepalive(std::stringstream& ss, std::shared_ptr<SipRequest> req);
    virtual void resp_invite(std::stringstream& ss, std::shared_ptr<SipRequest> req, const std::string& ssrc, bool isUdp);
    virtual std::string resp_catalog(std::stringstream& ss, std::shared_ptr<SipRequest> req, const std::string& strSN, int total, int num, uint64_t startId, const std::string& channelStartId);
    virtual void resp_deviceinfo(std::stringstream& ss, std::shared_ptr<SipRequest> req, const std::string& strSn, int num);
    virtual void resp_devicestatus(std::stringstream& ss, std::shared_ptr<SipRequest> req, const std::string& strSn);
};

#endif //SIPMESSAGE_H

