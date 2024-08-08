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

#include "SipMessage.h"
#include "tinyxml.h"
#include "Common/config.h"
#include "Log/Logger.h"
#include "Util/String.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>  
#include <map>
#include <string_view>
#include <any>

using namespace std;
using namespace tinyxml2;

// #include <srs_protocol_io.hpp>
// #include <srs_kernel_stream.hpp>
// #include <srs_kernel_error.hpp>
// #include <srs_kernel_log.hpp>
// #include <srs_kernel_consts.hpp>
// #include <srs_core_autofree.hpp>
// #include <srs_kernel_utility.hpp>
// #include <srs_kernel_buffer.hpp>
// #include <srs_kernel_codec.hpp>
// #include <srs_rtsp_stack.hpp>

#define RTSP_CRLF "\r\n" // 0x0D0A
#define RTSP_CRLFCRLF "\r\n\r\n" // 0x0D0A0D0A

unsigned int sip_random(int min,int max)  
{  
    //srand(int(time(0)));
    //return  rand() % (max - min + 1) + min;
    static uint64_t rand = 0;
    return  rand++ % (max - min + 1) + min;
} 

std::string  sip_get_form_to_uri(std::string  msg)
{
    //<sip:34020000002000000001@3402000000>;tag=536961166
    //sip:34020000002000000001@3402000000 

    size_t pos = msg.find("<");
    if (pos == string::npos) {
        return msg;
    }

    std::string_view msgView = msg.substr(pos+1);

    size_t pos2 = msgView.find(">");
    if (pos2 == string::npos) {
        return msgView.data();
    }

    msgView = msgView.substr(0, pos2);
    return msgView.data();
}

std::string sip_get_param(std::string msg, std::string param)
{
    std::vector<std::string>  vec_params = split(msg, ";");

    for (auto it = vec_params.begin(); it != vec_params.end(); ++it) {
        string  value = *it;
        
        size_t pos = value.find(param);
        if (pos == string::npos) {
            continue;
        }

        auto v_pram = split(value, "=");
        
        if (v_pram.size() > 0) {
            return v_pram.at(1);
        }
    }
    return "";
}

auto gettest()
{
    return make_pair(1,"test");
}

SipRequest::SipRequest()
{
    int i;
    string s;
    tie(seq, method) = gettest();
    any k = i;
    auto ss = any_cast<string>(k);
    if (int j = 0; j == 0) {

    }
    seq = 0;
    content_length = 0;
    // sdp = NULL;
    // transport = NULL;

    method = "";
    uri = "";;
    version = "";;
    seq = 0;
    content_type = "";
    content_length = 0;
    call_id = "";
    from = "";
    to = "";
    via = "";
    from_tag = "";
    to_tag = "";
    contact = "";
    user_agent = "";
    branch = "";
    status = "";
    expires = 3600;
    max_forwards = 70;
    www_authenticate = "";
    authorization = "";
    cmdtype = SipCmdRequest;

    host = "127.0.0.1";;
    host_port = 5060;

    serial = "";;
    realm = "";;

    sip_auth_id = "";
    sip_auth_pwd = "";
    sip_username = "";
    peer_ip = "";
    peer_port = 0;
}

SipRequest::~SipRequest()
{
    // srs_freep(sdp);
    // srs_freep(transport);
}

bool SipRequest::is_register()
{
    return method == SIP_METHOD_REGISTER;
}

bool SipRequest::is_invite()
{
    return method == SIP_METHOD_INVITE;
}

bool SipRequest::is_ack()
{
    return method == SIP_METHOD_ACK;
}

bool SipRequest::is_message()
{
    return method == SIP_METHOD_MESSAGE;
}

bool SipRequest::is_bye()
{
    return method == SIP_METHOD_BYE;
}

std::string SipRequest::get_cmdtype_str()
{
    switch(cmdtype) {
        case SipCmdRequest:
            return "request";
        case SipCmdRespone:
            return "responce";
    }

    return "";
}

void SipRequest::copy(SipRequest* src)
{
     if (!src){
         return;
     }
     
     method = src->method;
     uri = src->uri;
     version = src->version;
     seq = src->seq;
     content_type = src->content_type;
     content_length = src->content_length;
     call_id = src->call_id;
     from = src->from;
     to = src->to;
     via = src->via;
     from_tag = src->from_tag;
     to_tag = src->to_tag;
     contact = src->contact;
     user_agent = src->user_agent;
     branch = src->branch;
     status = src->status;
     expires = src->expires;
     max_forwards = src->max_forwards;
     www_authenticate = src->www_authenticate;
     authorization = src->authorization;
     cmdtype = src->cmdtype;

     host = src->host;
     host_port = src->host_port;

     serial = src->serial;
     realm = src->realm;
     
     sip_auth_id = src->sip_auth_id;
     sip_auth_pwd = src->sip_auth_pwd;
     sip_username = src->sip_username;
     peer_ip = src->peer_ip;
     peer_port = src->peer_port;
}

SipStack::SipStack()
{
    //buf = new SrsSimpleStream();
    sn = 0;
}

SipStack::~SipStack()
{
    //srs_freep(buf);
}

int SipStack::parse_request(shared_ptr<SipRequest>& req, const char* recv_msg, int nb_len)
{
    int err = 0;
    
    req.reset(new SipRequest());
    if ((err = do_parse_request(req, recv_msg)) != 0) {
        return err;
    }
    
    return err;
}

int SipStack::do_parse_request(shared_ptr<SipRequest> req, const char* recv_msg)
{
    int err = 0;

    std::vector<std::string> header_body = split(recv_msg, RTSP_CRLFCRLF);
    std::string header = header_body.at(0) + RTSP_CRLFCRLF;
    std::string body = "";

    if (header_body.size() > 1){
       body =  header_body.at(1);
    }
    req->content = body;

    // InfoL << "header is: " << header << endl;
    // InfoL << "body is: " << body << endl;

    // parse one by one.
    char* start = (char*)header.c_str();
    char* end = start + header.size();
    char* p = start;
    char* newline_start = start;
    std::string firstline = "";
    while (p < end) {
        if (p[0] == '\r' && p[1] == '\n'){
            p +=2;
            int linelen = (int)(p - newline_start);
            std::string oneline(newline_start, linelen);
            newline_start = p;

            //InfoL << "oneline is: " << oneline << endl;

            if (firstline == ""){
                firstline = oneline;
                //srs_info("sip: first line=%s", firstline.c_str());
            } else {
                size_t pos = oneline.find(":");
                if (pos != string::npos && pos != 0) {
                    //ex: CSeq: 100 MESSAGE  header is 'CSeq:',content is '100 MESSAGE'
                    std::string head = oneline.substr(0, pos+1);
                    std::string content = oneline.substr(pos+1, oneline.length()-pos-1);
                    content = string_replace(content, "\r\n", "");
                    content = trim(content, " ");
                    char *phead = (char*)head.c_str();

                    //InfoL << "head is: " << head << ", value is: " << content << endl;
                    
                    if (!strcasecmp(phead, "call-id:")) {
                        std::vector<std::string> vec_callid = split(content, " ");
                        req->call_id = vec_callid.at(0);
                    } 
                    else if (!strcasecmp(phead, "contact:")) {
                        req->contact = content;
                    } 
                    else if (!strcasecmp(phead, "content-encoding:")) {
                        //srs_trace("sip: message head %s content=%s", phead, content.c_str());
                    } 
                    else if (!strcasecmp(phead, "content-length:")) {
                        req->content_length = strtoul(content.c_str(), NULL, 10);
                    } 
                    else if (!strcasecmp(phead, "content-type:")) {
                        req->content_type = content;
                    } 
                    else if (!strcasecmp(phead, "cseq:")) {
                        std::vector<std::string> vec_seq = split(content, " ");
                        req->seq =  strtoul(vec_seq.at(0).c_str(), NULL, 10);
                        req->method = vec_seq.at(1);
                    } 
                    else if (!strcasecmp(phead, "from:")) {
                        content = string_replace(content, "sip:", "");
                        req->from = sip_get_form_to_uri(content.c_str());
                        if (string_contains(content, "tag")) {
                            req->from_tag = sip_get_param(content.c_str(), "tag");
                        }
                    } 
                    else if (!strcasecmp(phead, "to:")) {
                        content = string_replace(content, "sip:", "");
                        req->to = sip_get_form_to_uri(content.c_str());
                        if (string_contains(content, "tag")) {
                            req->to_tag = sip_get_param(content.c_str(), "tag");
                        }
                    } 
                    else if (!strcasecmp(phead, "via:")) {
                        std::vector<std::string> vec_seq = split(content, ";");
                        req->via = content;
                        req->branch = sip_get_param(content.c_str(), "branch");
                    } 
                    else if (!strcasecmp(phead, "expires:")){
                        req->expires = strtoul(content.c_str(), NULL, 10);
                    }
                    else if (!strcasecmp(phead, "user-agent:")){
                        req->user_agent = content;
                    } 
                    else if (!strcasecmp(phead, "max-forwards:")){
                        req->max_forwards = strtoul(content.c_str(), NULL, 10);
                    }
                    else if (!strcasecmp(phead, "www-authenticate:")){
                        req->www_authenticate = content;
                    } 
                    else if (!strcasecmp(phead, "authorization:")){
                        req->authorization = content;
                    } 
                    else {
                        //TODO: fixme
                        //srs_trace("sip: unkonw message head %s content=%s", phead, content.c_str());
                    }
                }
            }
        } else {
            p++;
        }
    }
   
    std::vector<std::string>  method_uri_ver = split(firstline, " ");
    //respone first line text:SIP/2.0 200 OK
    if (!strcasecmp(method_uri_ver.at(0).c_str(), "sip/2.0")) {
        req->cmdtype = SipCmdRespone;
        //req->method= vec_seq.at(1);
        req->status = method_uri_ver.at(1);
        req->version = method_uri_ver.at(0);
        req->uri = req->from;

        string authLine;
        if (_role == "server") {
            authLine = req->to;
        } else {
            authLine = req->from;
        }
        vector<string> str = split(authLine, "@");
        req->sip_auth_id = string_replace(str.at(0), "sip:", "");
  
    }else {//request first line text :MESSAGE sip:34020000002000000001@3402000000 SIP/2.0
        req->cmdtype = SipCmdRequest;
        req->method= method_uri_ver.at(0);
        req->uri = method_uri_ver.at(1);
        req->version = method_uri_ver.at(2);

        string authLine;
        if (_role == "server") {
            authLine = req->from;
        } else {
            authLine = req->to;
        }

        vector<string> str = split(authLine, "@");
        req->sip_auth_id = string_replace(str.at(0), "sip:", "");
    }

    req->sip_username =  req->sip_auth_id;
   
    // srs_info("sip: method=%s uri=%s version=%s cmdtype=%s", 
    //        req->method.c_str(), req->uri.c_str(), req->version.c_str(), req->get_cmdtype_str().c_str());
    // srs_info("via=%s", req->via.c_str());
    // srs_info("via_branch=%s", req->branch.c_str());
    // srs_info("cseq=%d", req->seq);
    // srs_info("contact=%s", req->contact.c_str());
    // srs_info("from=%s",  req->from.c_str());
    // srs_info("to=%s",  req->to.c_str());
    // srs_info("callid=%s", req->call_id.c_str());
    // srs_info("status=%s", req->status.c_str());
    // srs_info("from_tag=%s", req->from_tag.c_str());
    // srs_info("to_tag=%s", req->to_tag.c_str());
    // srs_info("sip_auth_id=%s", req->sip_auth_id.c_str());

    return err;
}

void SipStack::resp_keepalive(std::stringstream& ss, shared_ptr<SipRequest> req){
    ss << SIP_VERSION <<" 200 OK" << RTSP_CRLF
    << "Via: " << SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";branch=" << req->branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">;tag=" << req->from_tag << RTSP_CRLF
    << "To: <sip:"<< req->to << ">\r\n"
    << "Call-ID: " << req->call_id << RTSP_CRLF
    << "CSeq: " << req->seq << " " << req->method << RTSP_CRLF
    << "Contact: "<< req->contact << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "User-Agent: "<< SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: 0" << RTSP_CRLFCRLF;
}

void SipStack::resp_status(stringstream& ss, shared_ptr<SipRequest> req)
{
    if (req->method == "REGISTER"){
        /* 
        //request:  sip-agent-----REGISTER------->sip-server
        REGISTER sip:34020000002000000001@3402000000 SIP/2.0
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1371463273
        From: <sip:34020000001320000003@3402000000>;tag=2043466181
        To: <sip:34020000001320000003@3402000000>
        Call-ID: 1011047669
        CSeq: 1 REGISTER
        Contact: <sip:34020000001320000003@192.168.137.11:5060>
        Max-Forwards: 70
        User-Agent: IP Camera
        Expires: 3600
        Content-Length: 0
        
        //response:  sip-agent<-----200 OK--------sip-server
        SIP/2.0 200 OK
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1371463273
        From: <sip:34020000001320000003@3402000000>
        To: <sip:34020000001320000003@3402000000>
        CSeq: 1 REGISTER
        Call-ID: 1011047669
        Contact: <sip:34020000001320000003@192.168.137.11:5060>
        User-Agent: SRS/4.0.4(Leo)
        Expires: 3600
        Content-Length: 0

        */
        if (req->authorization.empty()){
            //TODO: fixme supoort 401
            //return req_401_unauthorized(ss, req);
        }

        ss << SIP_VERSION <<" 200 OK" << RTSP_CRLF
        << "Via: " << req->via << RTSP_CRLF
        << "From: <sip:"<< req->from << ">" << RTSP_CRLF
        << "To: <sip:"<< req->to << ">" << RTSP_CRLF
        << "CSeq: "<< req->seq << " " << req->method <<  RTSP_CRLF
        << "Call-ID: " << req->call_id << RTSP_CRLF
        << "Contact: " << req->contact << RTSP_CRLF
        << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
        << "Expires: " << req->expires << RTSP_CRLF
        << "Content-Length: 0" << RTSP_CRLFCRLF;
    }else{
        /*
        //request: sip-agnet-------MESSAGE------->sip-server
        MESSAGE sip:34020000002000000001@3402000000 SIP/2.0
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1066375804
        From: <sip:34020000001320000003@3402000000>;tag=1925919231
        To: <sip:34020000002000000001@3402000000>
        Call-ID: 1185236415
        CSeq: 20 MESSAGE
        Content-Type: Application/MANSCDP+xml
        Max-Forwards: 70
        User-Agent: IP Camera
        Content-Length:   175

        <?xml version="1.0" encoding="UTF-8"?>
        <Notify>
        <CmdType>Keepalive</CmdType>
        <SN>1</SN>
        <DeviceID>34020000001320000003</DeviceID>
        <Status>OK</Status>
        <Info>
        </Info>
        </Notify>
        //response: sip-agent------200 OK --------> sip-server
        SIP/2.0 200 OK
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1066375804
        From: <sip:34020000001320000003@3402000000>
        To: <sip:34020000002000000001@3402000000>
        CSeq: 20 MESSAGE
        Call-ID: 1185236415
        User-Agent: SRS/4.0.4(Leo)
        Content-Length: 0
        
        */

        ss << SIP_VERSION <<" 200 OK" << RTSP_CRLF
        << "Via: " << req->via << RTSP_CRLF
        << "From: <sip:"<< req->from << ">" << RTSP_CRLF
        << "To: <sip:"<< req->to << ">" << RTSP_CRLF
        << "CSeq: "<< req->seq << " " << req->method <<  RTSP_CRLF
        << "Call-ID: " << req->call_id << RTSP_CRLF
        << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
        << "Content-Length: 0" << RTSP_CRLFCRLF;
    }
   
}

void SipStack::req_invite(stringstream& ss, shared_ptr<SipRequest> req, string ip, int port, uint32_t ssrc)
{
    /* 
    //request: sip-agent <-------INVITE------ sip-server
    INVITE sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    Content-Type: Application/SDP
    Contact: <sip:34020000001320000003@3402000000>
    Max-Forwards: 70 
    User-Agent: SRS/4.0.4(Leo)
    Subject: 34020000001320000003:630886,34020000002000000001:0
    Content-Length: 164

    v=0
    o=34020000001320000003 0 0 IN IP4 39.100.155.146
    s=Play
    c=IN IP4 39.100.155.146
    t=0 0
    m=video 9000 RTP/AVP 96
    a=recvonly
    a=rtpmap:96 PS/90000
    y=630886
    //response: sip-agent --------100 Trying--------> sip-server
    SIP/2.0 100 Trying
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    User-Agent: IP Camera
    Content-Length: 0

    //response: sip-agent -------200 OK--------> sip-server 
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 20 INVITE
    Contact: <sip:34020000001320000003@192.168.137.11:5060>
    Content-Type: application/sdp
    User-Agent: IP Camera
    Content-Length:   263

    v=0
    o=34020000001320000003 1073 1073 IN IP4 192.168.137.11
    s=Play
    c=IN IP4 192.168.137.11
    t=0 0
    m=video 15060 RTP/AVP 96
    a=setup:active
    a=sendonly
    a=rtpmap:96 PS/90000
    a=username:34020000001320000003
    a=password:12345678
    a=filesize:0
    y=0000630886
    f=
    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0
    */

    std::stringstream sdp;
    sdp << "v=0" << RTSP_CRLF
    << "o=" << req->sip_auth_id << " 0 0 IN IP4 " << ip << RTSP_CRLF
    << "s=Play" << RTSP_CRLF
    << "c=IN IP4 " << ip << RTSP_CRLF
    << "t=0 0" << RTSP_CRLF
    //TODO 97 98 99 current no support
    //<< "m=video " << port <<" RTP/AVP 96 97 98 99" << RTSP_CRLF
    << "m=video " << port <<" RTP/AVP 96" << RTSP_CRLF
    << "a=recvonly" << RTSP_CRLF
    << "a=rtpmap:96 PS/90000" << RTSP_CRLF
    //TODO: current no support
    //<< "a=rtpmap:97 MPEG4/90000" << SRS_RTSP_CRLF
    //<< "a=rtpmap:98 H264/90000" << SRS_RTSP_CRLF
    //<< "a=rtpmap:99 H265/90000" << SRS_RTSP_CRLF
    //<< "a=streamMode:MAIN\r\n"
    //<< "a=filesize:0\r\n"
    << "y=" << ssrc << RTSP_CRLF;

    
    int rand = sip_random(1000, 9999);
    std::stringstream from, to, uri, branch, from_tag;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->host << ":"  << req->host_port;
    to << req->sip_auth_id <<  "@" << req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    branch << "z9hG4bK3420" << rand;
    from_tag << "51235" << rand;
    req->branch = branch.str();
    req->from_tag = from_tag.str();

    ss << "INVITE " << req->uri << " " << SIP_VERSION << RTSP_CRLF
    << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">;tag=" << req->from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << RTSP_CRLF
    << "Call-ID: 20000" << rand <<RTSP_CRLF
    << "CSeq: 20 INVITE" << RTSP_CRLF
    << "Content-Type: Application/SDP" << RTSP_CRLF
    << "Contact: <sip:" << req->to << ">" << RTSP_CRLF
    << "Max-Forwards: 70" << " \r\n"
    << "User-Agent: " << SIP_USER_AGENT <<RTSP_CRLF
    << "Subject: "<< req->sip_auth_id << ":" << ssrc << "," << req->serial << ":0" << RTSP_CRLF
    << "Content-Length: " << sdp.str().length() << RTSP_CRLFCRLF
    << sdp.str();
}

void SipStack::req_invite_playback(std::stringstream& ss, shared_ptr<SipRequest> req, std::string ip, 
        int port, uint32_t ssrc, std::string start_time, std::string end_time)
{
    /* 
    //request: sip-agent <-------INVITE------ sip-server
    INVITE sip:34020000001320000002@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 192.168.30.173:5060;rport;branch=z9hG4bK932888095
    From: <sip:34020000002000000001@3402000000>;tag=71888095
    To: <sip:34020000001320000002@3402000000>
    Call-ID: 935887989
    CSeq: 177 INVITE
    Content-Type: APPLICATION/SDP
    Contact: <sip:34020000002000000001@192.168.30.173:5060>
    Max-Forwards: 70
    User-Agent: LiveGBS v200529
    Subject: 34020000001320000002:1200000013,34020000002020000001:0
    Content-Length: 272

    v=0
    o=34020000002000000001 0 0 IN IP4 192.168.30.173
    s=Playback
    u=34020000001320000002:0
    c=IN IP4 192.168.30.173
    t=1591083900 1591084602
    m=video 30004 RTP/AVP 96 97 98
    a=recvonly
    a=rtpmap:96 PS/90000
    a=rtpmap:97 MPEG4/90000
    a=rtpmap:98 H264/90000
    y=1200000013


    //response: sip-agent --------100 Trying--------> sip-server
    SIP/2.0 100 Trying
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    User-Agent: IP Camera
    Content-Length: 0

    //response: sip-agent -------200 OK--------> sip-server 
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 192.168.30.173:5060;rport=5060;branch=z9hG4bK932888095
    From: <sip:34020000002000000001@3402000000>;tag=71888095
    To: <sip:34020000001320000002@3402000000>;tag=979281030
    Call-ID: 935887989
    CSeq: 177 INVITE
    Contact: <sip:34020000001320000002@192.168.30.228:5060>
    Content-Type: application/SDP
    User-Agent: Embedded Net DVR/NVR/DVS
    Content-Length:   264

    v=0
    o=34020000001110000001 0 0 IN IP4 192.168.30.228
    s=EZVIZ X5S
    c=IN IP4 192.168.30.228
    t=1591112700 1591113402
    m=video 62022 RTP/AVP 96
    a=sendonly
    a=rtpmap:96 PS/90000
    a=username:34020000001110000001
    a=password:12345678
    a=filesize:1
    y=1200000013
    f=

    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0
    */
    /*char _ssrc[11];
    sprintf(_ssrc, "%010d", ssrc);
  
    std::stringstream sdp;
    sdp << "v=0" << SRS_RTSP_CRLF
    << "o=" << req->serial << " 0 0 IN IP4 " << ip << SRS_RTSP_CRLF
    << "s=Download" << SRS_RTSP_CRLF
    //<< "s=Playback" << SRS_RTSP_CRLF
    << "u=" << req->chid << ":0" << SRS_RTSP_CRLF
    << "c=IN IP4 " << ip << SRS_RTSP_CRLF
    << "t=" << start_time << " " <<  end_time << SRS_RTSP_CRLF
    //TODO 97 98 99 current no support
    //<< "m=video " << port <<" RTP/AVP 96 97 98 99" << SRS_RTSP_CRLF
    << "m=video " << port <<" RTP/AVP 96" << SRS_RTSP_CRLF
    << "a=recvonly" << SRS_RTSP_CRLF
    << "a=rtpmap:96 PS/90000" << SRS_RTSP_CRLF
    << "a=downloadspeed:100" << SRS_RTSP_CRLF
    //TODO: current no support
    //<< "a=rtpmap:97 MPEG4/90000" << SRS_RTSP_CRLF
    //<< "a=rtpmap:98 H264/90000" << SRS_RTSP_CRLF
    //<< "a=rtpmap:99 H265/90000" << SRS_RTSP_CRLF
    //<< "a=streamMode:MAIN\r\n"
    //<< "a=filesize:0\r\n"
    << "y=" << _ssrc << SRS_RTSP_CRLF;

    
    std::stringstream from, to, uri;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->chid << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->realm;
    to <<  req->chid <<  "@" << req->realm;
   
    req->from = from.str();
    req->to = to.str();

    if (!req->to_realm.empty()){
        req->to  =  req->chid + "@" + req->to_realm;
    }

    if (!req->from_realm.empty()){
        req->from  =  req->serial + "@" + req->from_realm;
    }

    req->uri  = uri.str();

    req->call_id = srs_sip_generate_call_id();
    req->branch = srs_sip_generate_branch();
    req->from_tag = srs_sip_generate_from_tag();

    ss << "INVITE " << req->uri << " " << SRS_SIP_VERSION << SRS_RTSP_CRLF
    << "Via: " << SRS_SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SRS_RTSP_CRLF
    << "From: " << get_sip_from(req) << SRS_RTSP_CRLF
    << "To: " << get_sip_to(req) << SRS_RTSP_CRLF
    << "Call-ID: " << req->call_id <<SRS_RTSP_CRLF
    << "CSeq: " << req->seq << " INVITE" << SRS_RTSP_CRLF
    << "Content-Type: Application/SDP" << SRS_RTSP_CRLF
    << "Contact: <sip:" << req->from << ">" << SRS_RTSP_CRLF
    << "Max-Forwards: 70" << SRS_RTSP_CRLF
    << "User-Agent: " << SRS_SIP_USER_AGENT <<SRS_RTSP_CRLF
    << "Subject: "<< req->chid << ":" << _ssrc << "," << req->serial << ":0" << SRS_RTSP_CRLF
    << "Content-Length: " << sdp.str().length() << SRS_RTSP_CRLFCRLF
    << sdp.str();*/
}

void SipStack::req_401_unauthorized(std::stringstream& ss, shared_ptr<SipRequest> req)
{
    /* sip-agent <-----401 Unauthorized ------ sip-server
    SIP/2.0 401 Unauthorized
    Via: SIP/2.0/UDP 192.168.137.92:5061;rport=61378;received=192.168.1.13;branch=z9hG4bK802519080
    From: <sip:34020000001320000004@192.168.137.92:5061>;tag=611442989
    To: <sip:34020000001320000004@192.168.137.92:5061>;tag=102092689
    CSeq: 1 REGISTER
    Call-ID: 1650345118
    User-Agent: LiveGBS v200228
    Contact: <sip:34020000002000000001@192.168.1.23:15060>
    Content-Length: 0
    WWW-Authenticate: Digest realm="3402000000",qop="auth",nonce="f1da98bd160f3e2efe954c6eedf5f75a"
    */

    ss << SIP_VERSION <<" 401 Unauthorized" << RTSP_CRLF
    //<< "Via: " << req->via << SRS_RTSP_CRLF
    << "Via: " << req->via << ";rport=" << req->peer_port << ";received=" << req->peer_ip << ";branch=" << req->branch << RTSP_CRLF
    << "From: <sip:"<< req->from << ">" << RTSP_CRLF
    << "To: <sip:"<< req->to << ">" << RTSP_CRLF
    << "CSeq: "<< req->seq << " " << req->method << RTSP_CRLF
    << "Call-ID: " << req->call_id << RTSP_CRLF
    << "Contact: " << req->contact << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: 0" << RTSP_CRLF
    << "WWW-Authenticate: Digest realm=\"3402000000\",qop=\"auth\",nonce=\"f1da98bd160f3e2efe954c6eedf5f75a\"" << RTSP_CRLFCRLF;
    return;
}

void SipStack::resp_ack(std::stringstream& ss, shared_ptr<SipRequest> req){
    /*
    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0
    */
  
    ss << "ACK " << "sip:" <<  req->sip_auth_id << "@" << req->realm << " "<< SIP_VERSION << RTSP_CRLF
    << "Via: " << SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";rport;branch=" << req->branch << RTSP_CRLF
    << "From: <sip:" << req->serial << "@" << req->host + ":" << req->host_port << ">;tag=" << req->from_tag << RTSP_CRLF
    << "To: <sip:"<< req->sip_auth_id <<  "@" << req->realm << ">\r\n"
    << "Call-ID: " << req->call_id << RTSP_CRLF
    << "CSeq: " << req->seq << " ACK"<< RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "User-Agent: "<< SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: 0" << RTSP_CRLFCRLF;
}

void SipStack::req_bye(std::stringstream& ss, shared_ptr<SipRequest> req)
{
    /*
    //request: sip-agent <------BYE------ sip-server
    BYE sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@3402000000>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 79 BYE
    Max-Forwards: 70
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0

    //response: sip-agent ------200 OK ------> sip-server
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@3402000000>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 79 BYE
    User-Agent: IP Camera
    Content-Length: 0

    */

    std::stringstream from, to, uri;
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    from << req->serial << "@"  << req->realm;
    to << req->sip_auth_id <<  "@" <<  req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    string to_tag, from_tag, branch;

    if (req->branch.empty()){
        branch = "";
    }else {
        branch = ";branch=" + req->branch;
    }

    if (req->from_tag.empty()){
        from_tag = "";
    }else {
        from_tag = ";tag=" + req->from_tag;
    }
    
    if (req->to_tag.empty()){
        to_tag = "";
    }else {
        to_tag = ";tag=" + req->to_tag;
    }

    int seq = sip_random(22, 99);
    ss << "BYE " << req->uri << " "<< SIP_VERSION << RTSP_CRLF
    << "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << to_tag << RTSP_CRLF
    << "Call-ID: " << req->call_id << RTSP_CRLF
    << "CSeq: "<< seq <<" BYE" << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: 0" << RTSP_CRLFCRLF;
   
}

std::string SipStack::req_record_info(std::stringstream& ss, shared_ptr<SipRequest> req, 
            const std::string& deviceId, const std::string& startTime, const std::string& endTime)
{
    /*
    //request: sip-agent <------RecordInfo------ sip-server
    MESSAGE sip:34020000001320000002@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 192.168.30.173:5060;rport;branch=z9hG4bK686887582
    From: <sip:34020000002000000001@3402000000>;tag=734887582
    To: <sip:34020000001320000002@3402000000>
    Call-ID: 984887582
    [Generated Call-ID: 984887582]
    CSeq: 175 MESSAGE
    Content-Type: Application/MANSCDP+xml
    Max-Forwards: 70
    User-Agent: LiveGBS v200529
    Content-Length: 279
    
    <?xml version="1.0" encoding="UTF-8"?>\r\n
    <Query>\r\n
        <CmdType>RecordInfo</CmdType>\r\n
        <SN>840887582</SN>\r\n
        <DeviceID>34020000001320000002</DeviceID>\r\n
        <StartTime>2020-06-02T12:00:00</StartTime>\r\n
        <EndTime>2020-06-02T15:59:00</EndTime>\r\n
        <Type>all</Type>\r\n
    </Query>\r\n

    //response: sip-agent -------200 OK--------> sip-server
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 192.168.30.173:5060;rport=5060;branch=z9hG4bK686887582
    From: <sip:34020000002000000001@3402000000>;tag=734887582
    To: <sip:34020000001320000002@3402000000>;tag=815147343
    Call-ID: 984887582
    [Generated Call-ID: 984887582]
    CSeq: 175 MESSAGE
    User-Agent: Embedded Net DVR/NVR/DVS
    Content-Length: 0

    //response: sip-agent -------recordinfo--------> sip-server
    MESSAGE sip:34020000002000000001@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 192.168.30.228:5060;rport;branch=z9hG4bK242789517
    From: <sip:34020000001110000001@3402000000>;tag=1702343125
    To: <sip:34020000002000000001@3402000000>
    Call-ID: 1189630420
    [Generated Call-ID: 1189630420]
    CSeq: 20 MESSAGE
    Content-Type: Application/MANSCDP+xml
    Max-Forwards: 70
    User-Agent: Embedded Net DVR/NVR/DVS
    Content-Length:  2689
    
    <?xml version="1.0" encoding="gb2312"?>\r\n
    <Response>\n
        <CmdType>RecordInfo</CmdType>\n
        <SN>840887582</SN>\n
        <DeviceID>34020000001320000002</DeviceID>\n
        <Name>Camera 01</Name>\n
        <SumNum>9</SumNum>\n
        <RecordList Num="9">\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591069694_1591071812</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T11:48:14</StartTime>\n
                <EndTime>2020-06-02T12:23:32</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591071812_1591073930</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T12:23:32</StartTime>\n
                <EndTime>2020-06-02T12:58:50</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591073930_1591076048</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T12:58:50</StartTime>\n
                <EndTime>2020-06-02T13:34:08</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591076048_1591078166</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T13:34:08</StartTime>\n
                <EndTime>2020-06-02T14:09:26</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591078166_1591080284</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T14:09:26</StartTime>\n
                <EndTime>2020-06-02T14:44:44</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591080284_1591081343</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T14:44:44</StartTime>\n
                <EndTime>2020-06-02T15:02:23</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591081425_1591082484</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T15:03:45</StartTime>\n
                <EndTime>2020-06-02T15:21:24</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591082484_1591084602</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T15:21:24</StartTime>\n
                <EndTime>2020-06-02T15:56:42</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
            <Item>\n
                <DeviceID>34020000001320000002</DeviceID>\n
                <Name>Camera 01</Name>\n
                <FilePath>1591084602_1591084721</FilePath>\n
                <Address>Address 1</Address>\n
                <StartTime>2020-06-02T15:56:42</StartTime>\n
                <EndTime>2020-06-02T15:58:41</EndTime>\n
                <Secrecy>0</Secrecy>\n
                <Type>time</Type>\n
            </Item>\n
        </RecordList>\n
    </Response>\n

    //agent<------200 ok--------server
    SIP/2.0 200 OK
    Message Header
        Via: SIP/2.0/UDP 192.168.30.228:5060;rport=5060;received=192.168.30.228;branch=z9hG4bK242789517
        From: <sip:34020000001110000001@3402000000>;tag=1702343125
        To: <sip:34020000002000000001@3402000000>;tag=405887934
        CSeq: 20 MESSAGE
        Call-ID: 1189630420
        [Generated Call-ID: 1189630420]
        User-Agent: LiveGBS v200529
        Content-Length: 0

    */

    std::stringstream recordinfo;
    //recordinfo << "v=0" << SRS_RTSP_CRLF
    recordinfo << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
    << "<Query>\r\n"
    <<     "<CmdType>RecordInfo</CmdType>\r\n"
    <<     "<SN>" << ++sn << "</SN>\r\n"
    <<     "<DeviceID>" << deviceId << "</DeviceID>\r\n"
    <<     "<StartTime>" << startTime << "</StartTime>\r\n"
    <<     "<EndTime>" << endTime << "</EndTime>\r\n"
    <<     "<Type>all</Type>\r\n"
    << "</Query>\r\n";

    
    int rand = sip_random(1000, 9999);
    string realm = deviceId.substr(0, 10);
    std::stringstream from, to, branch, from_tag;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->host << ":"  << req->host_port;
    to << deviceId <<  "@" << realm;

    req->from = from.str();
    req->to   = to.str();

    branch << "z9hG4bK3420" << rand;
    from_tag << "51235" << rand;
    req->branch = branch.str();
    req->from_tag = from_tag.str();

    /*
    MESSAGE sip:34020000001320000002@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 192.168.30.173:5060;rport;branch=z9hG4bK686887582
    From: <sip:34020000002000000001@3402000000>;tag=734887582
    To: <sip:34020000001320000002@3402000000>
    Call-ID: 984887582
    [Generated Call-ID: 984887582]
    CSeq: 175 MESSAGE
    Content-Type: Application/MANSCDP+xml
    Max-Forwards: 70
    User-Agent: LiveGBS v200529
    Content-Length: 279
    */
    ss << "MESSAGE sip:" << req->to << " SIP/2.0\r\n"
    << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">;tag=" << req->from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << RTSP_CRLF
    << "Call-ID: 20000" << rand <<RTSP_CRLF
    << "CSeq: 175 MESSAGE" << RTSP_CRLF
    << "Content-Type: Application/MANSCDP+xml" << RTSP_CRLF
    //<< "Contact: <sip:" << req->to << ">" << SRS_RTSP_CRLF
    << "Max-Forwards: 70" << "\r\n"
    << "User-Agent: " << SIP_USER_AGENT <<RTSP_CRLF
    << "Content-Length: " << recordinfo.str().length() << RTSP_CRLFCRLF
    << recordinfo.str();
   
    return to_string(sn);
}

void SipStack::req_register(std::stringstream& ss, shared_ptr<SipRequest> req)
{
    /* 
    //request:  sip-agent-----REGISTER------->sip-server
    REGISTER sip:34020000002000000001@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1371463273
    From: <sip:34020000001320000003@3402000000>;tag=2043466181
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 1011047669
    CSeq: 1 REGISTER
    Contact: <sip:34020000001320000003@192.168.137.11:5060>
    Max-Forwards: 70
    User-Agent: IP Camera
    Expires: 3600
    Content-Length: 0
    
    //response:  sip-agent<-----200 OK--------sip-server
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1371463273
    From: <sip:34020000001320000003@3402000000>
    To: <sip:34020000001320000003@3402000000>
    CSeq: 1 REGISTER
    Call-ID: 1011047669
    Contact: <sip:34020000001320000003@192.168.137.11:5060>
    User-Agent: SRS/4.0.4(Leo)
    Expires: 3600
    Content-Length: 0

    */

    if (req->authorization != "") {
        req_registerWithAuth(ss, req);
        return ;
    }

    std::stringstream from, to, uri;
    uri << "sip:" <<  req->serial << "@" << req->realm;
    from << req->sip_auth_id << "@"  << req->realm;
    to << req->sip_auth_id <<  "@" <<  req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    int rand = sip_random(1000, 9999);
    //branch << "z9hG4bK3420" << rand;
    //from_tag << "51235" << rand;
    string from_tag = ";tag=51235" + to_string(rand);
    string branch = ";branch=z9hG4bK3420" + to_string(rand);

    //int seq = sip_random(22, 99);
    int seq = 1;
    ss << "REGISTER " << req->uri << " "<< SIP_VERSION << RTSP_CRLF
    << "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << RTSP_CRLF
    << "Call-ID: " << SIP_CALLID_REGISTER << rand << RTSP_CRLF
    << "CSeq: "<< seq <<" REGISTER" << RTSP_CRLF
    << "Contact: <sip:"<< req->sip_auth_id << "@" << req->host << ":" << req->host_port << ">" << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO" << RTSP_CRLF
    << "Expires: " << 3600 << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: 0" << RTSP_CRLFCRLF;
   
}

void SipStack::req_registerWithAuth(std::stringstream& ss, shared_ptr<SipRequest> req)
{
    std::stringstream from, to, uri;
    uri << "sip:" <<  req->serial << "@" << req->realm;
    from << req->sip_auth_id << "@"  << req->realm;
    to << req->sip_auth_id <<  "@" <<  req->realm;


    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    int rand = sip_random(1000, 9999);
    //branch << "z9hG4bK3420" << rand;
    //from_tag << "51235" << rand;
    string from_tag = ";tag=" + req->from_tag;
    string branch = ";branch=z9hG4bK3420" + to_string(rand);

    //int seq = sip_random(22, 99);
    int seq = 2;
    ss << "REGISTER " << req->uri << " "<< SIP_VERSION << RTSP_CRLF
    << "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << RTSP_CRLF
    << "Call-ID: " << req->call_id << RTSP_CRLF
    << "CSeq: "<< seq <<" REGISTER" << RTSP_CRLF
    << "Contact: <sip:"<< req->sip_auth_id << "@" << req->host << ":" << req->host_port << ">" << RTSP_CRLF
    << "Authorization: " << req->authorization << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO" << RTSP_CRLF
    << "Expires: " << 3600 << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: 0" << RTSP_CRLFCRLF;
   
}

void SipStack::req_keepalive(std::stringstream& ss, shared_ptr<SipRequest> req)
{
    /*
    //request: sip-agnet-------MESSAGE------->sip-server
    MESSAGE sip:34020000002000000001@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1066375804
    From: <sip:34020000001320000003@3402000000>;tag=1925919231
    To: <sip:34020000002000000001@3402000000>
    Call-ID: 1185236415
    CSeq: 20 MESSAGE
    Content-Type: Application/MANSCDP+xml
    Max-Forwards: 70
    User-Agent: IP Camera
    Content-Length:   175

    <?xml version="1.0" encoding="UTF-8"?>
    <Notify>
    <CmdType>Keepalive</CmdType>
    <SN>1</SN>
    <DeviceID>34020000001320000003</DeviceID>
    <Status>OK</Status>
    <Info>
    </Info>
    </Notify>
    //response: sip-agent------200 OK --------> sip-server
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1066375804
    From: <sip:34020000001320000003@3402000000>
    To: <sip:34020000002000000001@3402000000>
    CSeq: 20 MESSAGE
    Call-ID: 1185236415
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0
    
    */

    std::stringstream from, to, uri;
    uri << "sip:" <<  req->serial << "@" << req->realm;
    from << req->sip_auth_id << "@"  << req->realm;
    to << req->serial <<  "@" <<  req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    int rand = sip_random(1000, 9999);
    //branch << "z9hG4bK3420" << rand;
    //from_tag << "51235" << rand;
    string from_tag = ";tag=51235" + to_string(rand);
    string branch = ";branch=z9hG4bK3420" + to_string(rand);

    const char* declaration = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    XMLDocument doc;
    doc.Parse(declaration);
    XMLElement *root = doc.NewElement("Notify");
    //root->SetText();
    doc.InsertEndChild(root);

    XMLElement *cmdType = doc.NewElement("CmdType");
    cmdType->InsertEndChild(doc.NewText("Keepalive"));
    root->InsertEndChild(cmdType);

    XMLElement *SN = doc.NewElement("SN");
    SN->InsertEndChild(doc.NewText(to_string(++sn).c_str()));
    root->InsertEndChild(SN);

    XMLElement *DeviceID = doc.NewElement("DeviceID");
    DeviceID->InsertEndChild(doc.NewText(req->sip_auth_id.c_str()));
    root->InsertEndChild(DeviceID);

    XMLElement *Status = doc.NewElement("Status");
    Status->InsertEndChild(doc.NewText("OK"));
    root->InsertEndChild(Status);

    XMLElement *Info = doc.NewElement("Info");
    root->InsertEndChild(Info);

    XMLPrinter printer;
	doc.Print(&printer);
	string content = printer.CStr();

    int seq = sip_random(22, 99);
    ss << "MESSAGE " << req->uri << " "<< SIP_VERSION << RTSP_CRLF
    << "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << RTSP_CRLF
    << "Call-ID: " << SIP_CALLID_KEEPALIVE << rand << RTSP_CRLF
    << "CSeq: "<< seq <<" MESSAGE" << RTSP_CRLF
    //<< "Contact: <sip:"<< req->sip_auth_id << "@" << req->host << ":" << req->host_port << ">" << RTSP_CRLF
    << "Content-Type: Application/MANSCDP+xml" << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: " << content.size() << RTSP_CRLFCRLF
    << content;
   
}

void SipStack::resp_invite(std::stringstream& ss, shared_ptr<SipRequest> req, const string& ssrc)
{
    /* 
    //request: sip-agent <-------INVITE------ sip-server
    INVITE sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    Content-Type: Application/SDP
    Contact: <sip:34020000001320000003@3402000000>
    Max-Forwards: 70 
    User-Agent: SRS/4.0.4(Leo)
    Subject: 34020000001320000003:630886,34020000002000000001:0
    Content-Length: 164

    v=0
    o=34020000001320000003 0 0 IN IP4 39.100.155.146
    s=Play
    c=IN IP4 39.100.155.146
    t=0 0
    m=video 9000 RTP/AVP 96
    a=recvonly
    a=rtpmap:96 PS/90000
    y=630886
    //response: sip-agent --------100 Trying--------> sip-server
    SIP/2.0 100 Trying
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    User-Agent: IP Camera
    Content-Length: 0

    //response: sip-agent -------200 OK--------> sip-server 
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 20 INVITE
    Contact: <sip:34020000001320000003@192.168.137.11:5060>
    Content-Type: application/sdp
    User-Agent: IP Camera
    Content-Length:   263

    v=0
    o=34020000001320000003 1073 1073 IN IP4 192.168.137.11
    s=Play
    c=IN IP4 192.168.137.11
    t=0 0
    m=video 15060 RTP/AVP 96
    a=setup:active
    a=sendonly
    a=rtpmap:96 PS/90000
    a=username:34020000001320000003
    a=password:12345678
    a=filesize:0
    y=0000630886
    f=
    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0
    */

    // std::stringstream from, to, uri;
    // uri << "sip:" <<  req->serial << "@" << req->realm;
    // from << req->sip_auth_id << "@"  << req->realm;
    // to << req->serial <<  "@" <<  req->realm;

    // req->from = from.str();
    // req->to   = to.str();
    // req->uri  = uri.str();

    int rand = sip_random(1000, 9999);
    //branch << "z9hG4bK3420" << rand;
    //from_tag << "51235" << rand;
    string from_tag = ";tag=" + req->from_tag;
    string branch = ";branch=" + req->branch;
    string to_tag = ";tag=51236" + to_string(rand);

    std::stringstream sdp;
    sdp << "v=0" << RTSP_CRLF
    << "o=" << req->sip_channel_id << " 0 0 IN IP4 " << req->host << RTSP_CRLF
    << "s=Play" << RTSP_CRLF
    << "c=IN IP4 " << req->host << RTSP_CRLF
    << "t=0 0" << RTSP_CRLF
    //TODO 97 98 99 current no support
    //<< "m=video " << port <<" RTP/AVP 96 97 98 99" << RTSP_CRLF
    << "m=video " << req->_device->_port <<" RTP/AVP 96" << RTSP_CRLF
    << "a=sendonly" << RTSP_CRLF
    << "a=rtpmap:96 PS/90000" << RTSP_CRLF
    << "a=username:" << req->sip_channel_id << RTSP_CRLF
    << "a=password:" << req->sip_auth_pwd << RTSP_CRLF
    << "a=filesize:1" << RTSP_CRLF
    //TODO: current no support
    //<< "a=rtpmap:97 MPEG4/90000" << SRS_RTSP_CRLF
    //<< "a=rtpmap:98 H264/90000" << SRS_RTSP_CRLF
    //<< "a=rtpmap:99 H265/90000" << SRS_RTSP_CRLF
    //<< "a=streamMode:MAIN\r\n"
    //<< "a=filesize:0\r\n"
    << "y=" << ssrc << RTSP_CRLF;

    int seq = sip_random(22, 99);
    ss << "SIP/2.0 200 OK" << RTSP_CRLF
    << "Via: "<< SIP_VERSION << "/UDP "<< req->peer_ip << ":" << req->peer_port << ";rport=" << req->peer_port << branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << to_tag << RTSP_CRLF
    << "Call-ID: " << req->call_id << RTSP_CRLF
    << "CSeq: "<< req->seq <<" INVITE" << RTSP_CRLF
    << "Contact: <sip:"<< req->sip_channel_id << "@" << req->host << ":" << req->host_port << ">" << RTSP_CRLF
    << "Content-Type: Application/SDP" << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: " << sdp.str().size() << RTSP_CRLFCRLF
    << sdp.str();
   
}

string SipStack::resp_catalog(std::stringstream& ss, shared_ptr<SipRequest> req, const string& strSn, int total, int num, uint64_t startId, const std::string& channelStartId)
{
    /*
    //request: sip-agent <----MESSAGE Query Catalog--- sip-server
    MESSAGE sip:34020000001110000001@192.168.1.21:5060 SIP/2.0
    Via: SIP/2.0/UDP 192.168.1.17:5060;rport;branch=z9hG4bK563315752
    From: <sip:34020000001110000001@3402000000>;tag=387315752
    To: <sip:34020000001110000001@192.168.1.21:5060>
    Call-ID: 728315752
    CSeq: 32 MESSAGE
    Content-Type: Application/MANSCDP+xml
    Max-Forwards: 70
    User-Agent: SRS/4.0.20(Leo)
    Content-Length: 162

    <?xml version="1.0" encoding="UTF-8"?>
    <Query>
        <CmdType>Catalog</CmdType>
        <SN>419315752</SN>
        <DeviceID>34020000001110000001</DeviceID>
    </Query>
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 192.168.1.17:5060;rport=5060;branch=z9hG4bK563315752
    From: <sip:34020000001110000001@3402000000>;tag=387315752
    To: <sip:34020000001110000001@192.168.1.21:5060>;tag=1420696981
    Call-ID: 728315752
    CSeq: 32 MESSAGE
    User-Agent: Embedded Net DVR/NVR/DVS
    Content-Length: 0

    //response: sip-agent ----MESSAGE Query Catalog---> sip-server
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 192.168.1.17:5060;rport=5060;received=192.168.1.17;branch=z9hG4bK563315752
    From: <sip:34020000001110000001@3402000000>;tag=387315752
    To: <sip:34020000001110000001@192.168.1.21:5060>;tag=1420696981
    CSeq: 32 MESSAGE
    Call-ID: 728315752
    User-Agent: SRS/4.0.20(Leo)
    Content-Length: 0

    //request: sip-agent ----MESSAGE Response Catalog---> sip-server
    MESSAGE sip:34020000001110000001@3402000000.spvmn.cn SIP/2.0
    Via: SIP/2.0/UDP 192.168.1.21:5060;rport;branch=z9hG4bK1681502633
    From: <sip:34020000001110000001@3402000000.spvmn.cn>;tag=1194168247
    To: <sip:34020000001110000001@3402000000.spvmn.cn>
    Call-ID: 685380150
    CSeq: 20 MESSAGE
    Content-Type: Application/MANSCDP+xml
    Max-Forwards: 70
    User-Agent: Embedded Net DVR/NVR/DVS
    Content-Length:   909

    <?xml version="1.0" encoding="gb2312"?>
    <Response>
    <CmdType>Catalog</CmdType>
    <SN>419315752</SN>
    <DeviceID>34020000001110000001</DeviceID>
    <SumNum>8</SumNum>
    <DeviceList Num="2">
    <Item>
    <DeviceID>34020000001320000001</DeviceID>
    <Name>Camera 01</Name>
    <Manufacturer>Manufacturer</Manufacturer>
    <Model>Camera</Model>
    <Owner>Owner</Owner>
    <CivilCode>CivilCode</CivilCode>
    <Address>192.168.254.18</Address>
    <Parental>0</Parental>
    <SafetyWay>0</SafetyWay>
    <RegisterWay>1</RegisterWay>
    <Secrecy>0</Secrecy>
    <Status>ON</Status>
    </Item>
    <Item>
    <DeviceID>34020000001320000002</DeviceID>
    <Name>IPCamera 02</Name>
    <Manufacturer>Manufacturer</Manufacturer>
    <Model>Camera</Model>
    <Owner>Owner</Owner>
    <CivilCode>CivilCode</CivilCode>
    <Address>192.168.254.14</Address>
    <Parental>0</Parental>
    <SafetyWay>0</SafetyWay>
    <RegisterWay>1</RegisterWay>
    <Secrecy>0</Secrecy>
    <Status>OFF</Status>
    </Item>
    </DeviceList>
    </Response>

    */

    std::stringstream from, to, uri;
    uri << "sip:" <<  req->serial << "@" << req->realm;
    from << req->sip_auth_id << "@" << req->realm;
    to << req->serial <<  "@" <<  req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    int rand = sip_random(1000, 9999);
    //branch << "z9hG4bK3420" << rand;
    //from_tag << "51235" << rand;
    string from_tag = ";tag=51235" + to_string(rand);
    string branch = ";branch=z9hG4bK3420" + to_string(rand);

    stringstream ssBody;

    ssBody << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << RTSP_CRLF
           << "<Response>" << RTSP_CRLF
           <<     "<CmdType>Catalog</CmdType>" << RTSP_CRLF
           <<     "<SN>" << strSn << "</SN>" << RTSP_CRLF
           <<     "<DeviceID>" << req->sip_auth_id << "</DeviceID>" << RTSP_CRLF
           <<     "<SumNum>" << total << "</SumNum>" << RTSP_CRLF
           <<     "<DeviceList Num=\"" << num << "\">" << RTSP_CRLF;
    for (int i = 0; i < num; ++i) {
        string deviceId = channelStartId + to_string(startId + i);
    ssBody <<         "<Item>" << RTSP_CRLF
           <<             "<DeviceID>" << deviceId << "</DeviceID>" << RTSP_CRLF
           <<             "<Name>" << deviceId << "</Name>" << RTSP_CRLF
           <<             "<Manufacturer>Manufacturer</Manufacturer>" << RTSP_CRLF
           <<             "<Model>ipc</Model>" << RTSP_CRLF
           <<             "<Owner>Owner</Owner>" << RTSP_CRLF
           <<             "<CivilCode>CivilCode</CivilCode>" << RTSP_CRLF
           <<             "<Address>" << req->host << "</Address>" << RTSP_CRLF
           <<             "<Parental>0</Parental>" << RTSP_CRLF
           <<             "<SafetyWay>0</SafetyWay>" << RTSP_CRLF
           <<             "<RegisterWay>1</RegisterWay>" << RTSP_CRLF
           <<             "<Secrecy>0</Secrecy>" << RTSP_CRLF
           <<             "<Status>ON</Status>" << RTSP_CRLF
           <<         "</Item>" << RTSP_CRLF;
    }
    ssBody <<     "</DeviceList>" << RTSP_CRLF
           << "</Response>" << RTSP_CRLF;

    string content = ssBody.str();
    int seq = sip_random(22, 99);
    ss << "MESSAGE " << req->uri << " "<< SIP_VERSION << RTSP_CRLF
    << "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << RTSP_CRLF
    << "Call-ID: " << SIP_CALLID_CATALOG << rand << RTSP_CRLF
    << "CSeq: "<< seq <<" MESSAGE" << RTSP_CRLF
    //<< "Contact: <sip:"<< req->sip_auth_id << "@" << req->host << ":" << req->host_port << ">" << RTSP_CRLF
    << "Content-Type: Application/MANSCDP+xml" << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: " << content.size() << RTSP_CRLFCRLF
    << content;

    return SIP_CALLID_CATALOG + to_string(rand);
}

void SipStack::resp_deviceinfo(std::stringstream& ss, shared_ptr<SipRequest> req, const string& strSn, int num)
{
    std::stringstream from, to, uri;
    uri << "sip:" <<  req->serial << "@" << req->realm;
    from << req->sip_auth_id << "@" << req->realm;
    to << req->serial <<  "@" <<  req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    int rand = sip_random(1000, 9999);
    //branch << "z9hG4bK3420" << rand;
    //from_tag << "51235" << rand;
    string from_tag = ";tag=51235" + to_string(rand);
    string branch = ";branch=z9hG4bK3420" + to_string(rand);

    stringstream ssBody;

    ssBody << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << RTSP_CRLF
           << "<Response>" << RTSP_CRLF
           <<     "<CmdType>DeviceInfo</CmdType>" << RTSP_CRLF
           <<     "<SN>" << strSn << "</SN>" << RTSP_CRLF
           <<     "<DeviceID>" << req->sip_auth_id << "</DeviceID>" << RTSP_CRLF
           <<     "<DeviceName>" << req->sip_auth_id << "</DeviceName>" << RTSP_CRLF
           <<     "<Result>OK</Result>" << RTSP_CRLF
           <<     "<Manufacturer>Happytimesoft</Manufacturer>" << RTSP_CRLF
           <<     "<Model>ipc</Model>" << RTSP_CRLF
           <<     "<Firmware>V1.0</Firmware>" << RTSP_CRLF
           <<     "<Channel>" << num << "</Channel>" << RTSP_CRLF
           << "</Response>" << RTSP_CRLF;

    string content = ssBody.str();
    int seq = sip_random(22, 99);
    ss << "MESSAGE " << req->uri << " "<< SIP_VERSION << RTSP_CRLF
    << "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << RTSP_CRLF
    << "Call-ID: " << SIP_CALLID_DEVICEINFO << rand << RTSP_CRLF
    << "CSeq: "<< seq <<" MESSAGE" << RTSP_CRLF
    //<< "Contact: <sip:"<< req->sip_auth_id << "@" << req->host << ":" << req->host_port << ">" << RTSP_CRLF
    << "Content-Type: Application/MANSCDP+xml" << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: " << content.size() << RTSP_CRLFCRLF
    << content;
}

void SipStack::resp_devicestatus(std::stringstream& ss, shared_ptr<SipRequest> req, const string& strSn)
{
    std::stringstream from, to, uri;
    uri << "sip:" <<  req->serial << "@" << req->realm;
    from << req->sip_auth_id << "@" << req->realm;
    to << req->serial <<  "@" <<  req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    int rand = sip_random(1000, 9999);
    //branch << "z9hG4bK3420" << rand;
    //from_tag << "51235" << rand;
    string from_tag = ";tag=51235" + to_string(rand);
    string branch = ";branch=z9hG4bK3420" + to_string(rand);

    stringstream ssBody;

    ssBody << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << RTSP_CRLF
           << "<Response>" << RTSP_CRLF
           <<     "<CmdType>DeviceStatus</CmdType>" << RTSP_CRLF
           <<     "<SN>" << strSn << "</SN>" << RTSP_CRLF
           <<     "<DeviceID>" << req->sip_auth_id << "</DeviceID>" << RTSP_CRLF
           <<     "<Result>OK</Result>" << RTSP_CRLF
           <<     "<Online>ONLINE</Online>" << RTSP_CRLF
           <<     "<Status>ON</Status>" << RTSP_CRLF
           <<     "<Encode>OFF</Encode>" << RTSP_CRLF
           <<     "<Record>ON</Record>" << RTSP_CRLF
           <<     "<DeviceTime>" << getTimeStr("%Y-%m-%d") << "T" << getTimeStr("%H:%M:%S") << "</DeviceTime>" << RTSP_CRLF
           << "</Response>" << RTSP_CRLF;

    string content = ssBody.str();
    int seq = sip_random(22, 99);
    ss << "MESSAGE " << req->uri << " "<< SIP_VERSION << RTSP_CRLF
    << "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << RTSP_CRLF
    << "To: <sip:" << req->to << ">" << RTSP_CRLF
    << "Call-ID: " << SIP_CALLID_DEVICESTATUS << rand << RTSP_CRLF
    << "CSeq: "<< seq <<" MESSAGE" << RTSP_CRLF
    //<< "Contact: <sip:"<< req->sip_auth_id << "@" << req->host << ":" << req->host_port << ">" << RTSP_CRLF
    << "Content-Type: Application/MANSCDP+xml" << RTSP_CRLF
    << "Max-Forwards: 70" << RTSP_CRLF
    << "User-Agent: " << SIP_USER_AGENT << RTSP_CRLF
    << "Content-Length: " << content.size() << RTSP_CRLFCRLF
    << content;
}

