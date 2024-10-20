﻿#define MS_CLASS "RTC::SctpAssociation"
// #define MS_LOG_DEV_LEVEL 3

#include "SctpAssociation.h"
#include "Log/Logger.h"
#include <stdarg.h>
#include <cstdlib> // std::malloc(), std::free()
#include <cstring> // std::memset(), std::memcpy()
#include <string>
#include <memory>
#include <error.h>

using namespace std;

/* Static. */
static constexpr size_t SctpMtu{ 1200 };
static constexpr uint16_t MaxSctpStreams{ 65535 };

/* SCTP events to which we are subscribing. */
/* clang-format off */
static constexpr uint16_t EventTypes[] =
{
    SCTP_ADAPTATION_INDICATION,
    SCTP_ASSOC_CHANGE,
    SCTP_ASSOC_RESET_EVENT,
    SCTP_REMOTE_ERROR,
    SCTP_SHUTDOWN_EVENT,
    SCTP_SEND_FAILED_EVENT,
    SCTP_STREAM_RESET_EVENT,
    SCTP_STREAM_CHANGE_EVENT
};

/* clang-format on */
/* Static methods for usrsctp callbacks. */
inline static int onRecvSctpData(
  struct socket* /*sock*/,
  union sctp_sockstore /*addr*/,
  void* data,
  size_t len,
  struct sctp_rcvinfo rcv,
  int flags,
  void* ulpInfo)
{
    auto* sctpAssociation = static_cast<SctpAssociation*>(ulpInfo);

    if (sctpAssociation == nullptr)
    {
        std::free(data);

        return 0;
    }

    if (flags & MSG_NOTIFICATION)
    {
        sctpAssociation->OnUsrSctpReceiveSctpNotification(
          static_cast<union sctp_notification*>(data), len);
    }
    else
    {
        uint16_t streamId = rcv.rcv_sid;
        uint32_t ppid     = ntohl(rcv.rcv_ppid);
        uint16_t ssn      = rcv.rcv_ssn;

        fprintf(
          stdout,
          "data chunk received [length:%zu, streamId:%u" ", SSN:%u" ", TSN:%u"
          ", PPID:%u" ", context:%u" ", flags:%d]",
          len,
          rcv.rcv_sid,
          rcv.rcv_ssn,
          rcv.rcv_tsn,
          ntohl(rcv.rcv_ppid),
          rcv.rcv_context,
          flags);

        sctpAssociation->OnUsrSctpReceiveSctpData(
          streamId, ssn, ppid, flags, static_cast<uint8_t*>(data), len);
    }

    std::free(data);

    return 1;
}

/* Static methods for usrsctp global callbacks. */
inline static int onSendSctpData(void *addr, void *data, size_t len, uint8_t /*tos*/, uint8_t /*setDf*/) {
    auto* sctpAssociation = static_cast<SctpAssociation*>(addr);

    if (sctpAssociation == nullptr)
        return -1;

    sctpAssociation->OnUsrSctpSendSctpData(data, len);

    // NOTE: Must not free data, usrsctp lib does it.

    return 0;
}

// Static method for printing usrsctp debug.
inline static void sctpDebug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

class SctpEnv : public enable_shared_from_this<SctpEnv> {
public:
    ~SctpEnv();
    static SctpEnv &Instance();

private:
    SctpEnv();
};

SctpEnv& SctpEnv::Instance() {
    static SctpEnv env;
    return env;
}

SctpEnv::SctpEnv() {
    usrsctp_init(0, onSendSctpData, sctpDebug);
    // Disable explicit congestion notifications (ecn).
    usrsctp_sysctl_set_sctp_ecn_enable(0);

#ifdef SCTP_DEBUG
    usrsctp_sysctl_set_sctp_debug_on(SCTP_DEBUG_ALL);
#endif
}

SctpEnv::~SctpEnv() {
    usrsctp_finish();
}

////////////////////////////////////////////////////////////////////////////////////

/* Instance methods. */

#define MS_THROW_ERROR(format, err) do {printf(format, err); throw std::runtime_error("MS_THROW_ERROR");}while(false)
SctpAssociation::SctpAssociation(
    int localPort, int peerPort,
    uint16_t os, uint16_t mis, size_t maxSctpMessageSize, bool isDataChannel)
    : _os(os), _mis(mis), _localPort(localPort), _peerPort(peerPort), _maxSctpMessageSize(maxSctpMessageSize),
    _isDataChannel(isDataChannel)
{
    // MS_TRACE();
    _env = SctpEnv::Instance().shared_from_this();

    // Register ourselves in usrsctp.
    usrsctp_register_address(static_cast<void*>(this));

    int ret;

    this->_socket = usrsctp_socket(
        AF_CONN, SOCK_STREAM, IPPROTO_SCTP, onRecvSctpData, nullptr, 0, static_cast<void*>(this));

    if (this->_socket == nullptr)
        MS_THROW_ERROR("usrsctp_socket() failed: %s", strerror(errno));

    usrsctp_set_ulpinfo(this->_socket, static_cast<void*>(this));

    // Make the _socket non-blocking.
    ret = usrsctp_set_non_blocking(this->_socket, 1);

    if (ret < 0)
        MS_THROW_ERROR("usrsctp_set_non_blocking() failed: %s", strerror(errno));

    // Set SO_LINGER.
    // This ensures that the usrsctp close call deletes the association. This
    // prevents usrsctp from calling the global send callback with references to
    // this class as the address.
    struct linger lingerOpt; // NOLINT(cppcoreguidelines-pro-type-member-init)

    lingerOpt.l_onoff  = 1;
    lingerOpt.l_linger = 0;

    ret = usrsctp_setsockopt(this->_socket, SOL_SOCKET, SO_LINGER, &lingerOpt, sizeof(lingerOpt));

    if (ret < 0)
        MS_THROW_ERROR("usrsctp_setsockopt(SO_LINGER) failed: %s", std::strerror(errno));

    // Set SCTP_ENABLE_STREAM_RESET.
    struct sctp_assoc_value av; // NOLINT(cppcoreguidelines-pro-type-member-init)

    av.assoc_value =
        SCTP_ENABLE_RESET_STREAM_REQ | SCTP_ENABLE_RESET_ASSOC_REQ | SCTP_ENABLE_CHANGE_ASSOC_REQ;

    ret = usrsctp_setsockopt(this->_socket, IPPROTO_SCTP, SCTP_ENABLE_STREAM_RESET, &av, sizeof(av));

    if (ret < 0)
    {
        MS_THROW_ERROR("usrsctp_setsockopt(SCTP_ENABLE_STREAM_RESET) failed: %s", std::strerror(errno));
    }

    // Set SCTP_NODELAY.
    uint32_t noDelay = 1;

    ret = usrsctp_setsockopt(this->_socket, IPPROTO_SCTP, SCTP_NODELAY, &noDelay, sizeof(noDelay));

    if (ret < 0)
        MS_THROW_ERROR("usrsctp_setsockopt(SCTP_NODELAY) failed: %s", std::strerror(errno));

    // Enable events.
    struct sctp_event event; // NOLINT(cppcoreguidelines-pro-type-member-init)

    std::memset(&event, 0, sizeof(event));
    event.se_on = 1;

    for (size_t i{ 0 }; i < sizeof(EventTypes) / sizeof(uint16_t); ++i)
    {
        event.se_type = EventTypes[i];

        ret = usrsctp_setsockopt(this->_socket, IPPROTO_SCTP, SCTP_EVENT, &event, sizeof(event));

        if (ret < 0)
            MS_THROW_ERROR("usrsctp_setsockopt(SCTP_EVENT) failed: %s", std::strerror(errno));
    }

    // Init message.
    struct sctp_initmsg initmsg; // NOLINT(cppcoreguidelines-pro-type-member-init)

    std::memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_num_ostreams  = this->_os;
    initmsg.sinit_max_instreams = this->_mis;

    ret = usrsctp_setsockopt(this->_socket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg));

    if (ret < 0)
        MS_THROW_ERROR("usrsctp_setsockopt(SCTP_INITMSG) failed: %s", std::strerror(errno));

    // Server side.
    struct sockaddr_conn sconn; // NOLINT(cppcoreguidelines-pro-type-member-init)

    std::memset(&sconn, 0, sizeof(sconn));
    sconn.sconn_family = AF_CONN;
    sconn.sconn_port   = htons(_localPort);
    sconn.sconn_addr   = static_cast<void*>(this);
#ifdef HAVE_SCONN_LEN
    sconn.sconn_len = sizeof(sconn);
#endif

    ret = usrsctp_bind(this->_socket, reinterpret_cast<struct sockaddr*>(&sconn), sizeof(sconn));

    if (ret < 0)
        MS_THROW_ERROR("usrsctp_bind() failed: %s", std::strerror(errno));
}

SctpAssociation::~SctpAssociation()
{
    // MS_TRACE();

    usrsctp_set_ulpinfo(this->_socket, nullptr);
    usrsctp_close(this->_socket);

    // Deregister ourselves from usrsctp.
    usrsctp_deregister_address(static_cast<void*>(this));

    delete[] this->_messageBuffer;
}

void SctpAssociation::TransportConnected()
{
    // MS_TRACE();

    // Just run the SCTP stack if our _state is 'new'.
    if (this->_state != SctpState::NEW)
        return;

    try
    {
        int ret;
        struct sockaddr_conn rconn; // NOLINT(cppcoreguidelines-pro-type-member-init)

        std::memset(&rconn, 0, sizeof(rconn));
        rconn.sconn_family = AF_CONN;
        rconn.sconn_port   = htons(_peerPort);
        rconn.sconn_addr   = static_cast<void*>(this);
#ifdef HAVE_SCONN_LEN
        rconn.sconn_len = sizeof(rconn);
#endif

        ret = usrsctp_connect(this->_socket, reinterpret_cast<struct sockaddr*>(&rconn), sizeof(rconn));

        if (ret < 0 && errno != EINPROGRESS)
            MS_THROW_ERROR("usrsctp_connect() failed: %s", std::strerror(errno));

        // Disable MTU discovery.
        sctp_paddrparams peerAddrParams; // NOLINT(cppcoreguidelines-pro-type-member-init)

        std::memset(&peerAddrParams, 0, sizeof(peerAddrParams));
        std::memcpy(&peerAddrParams.spp_address, &rconn, sizeof(rconn));
        peerAddrParams.spp_flags = SPP_PMTUD_DISABLE;

        // The MTU value provided specifies the space available for chunks in the
        // packet, so let's subtract the SCTP header size.
        peerAddrParams.spp_pathmtu = SctpMtu - sizeof(peerAddrParams);

        ret = usrsctp_setsockopt(
            this->_socket, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &peerAddrParams, sizeof(peerAddrParams));

        if (ret < 0)
            MS_THROW_ERROR("usrsctp_setsockopt(SCTP_PEER_ADDR_PARAMS) failed: %s", std::strerror(errno));

        // Announce connecting _state.
        this->_state = SctpState::CONNECTING;
        this->onSctpAssociationConnecting(this);
    }
    catch (... /*error*/)
    {
        this->_state = SctpState::FAILED;
        this->onSctpAssociationFailed(this);
        throw;
    }
}

void SctpAssociation::ProcessSctpData(const uint8_t* data, size_t len)
{
    // MS_TRACE();

#if MS_LOG_DEV_LEVEL == 3
    MS_DUMP_DATA(data, len);
#endif

    usrsctp_conninput(static_cast<void*>(this), data, len, 0);
}

void SctpAssociation::SendSctpMessage(
    const SctpStreamParameters &parameters, uint32_t ppid, const uint8_t* msg, size_t len)
{
    // MS_TRACE();

    // This must be controlled by the DataConsumer.
    // MS_ASSERT(
    //   len <= this->_maxSctpMessageSize,
    //   "given message exceeds max allowed message size [message size:%zu, max message size:%zu]",
    //   len,
    //   this->_maxSctpMessageSize);
    if (len > _maxSctpMessageSize) {
        printf("len(%ld) > _maxSctpMessageSize(%ld)\n", len, _maxSctpMessageSize);
        throw;
    }

    // Fill stcp_sendv_spa.
    struct sctp_sendv_spa spa; // NOLINT(cppcoreguidelines-pro-type-member-init)

    std::memset(&spa, 0, sizeof(spa));
    spa.sendv_flags             = SCTP_SEND_SNDINFO_VALID;
    spa.sendv_sndinfo.snd_sid   = parameters.streamId;
    spa.sendv_sndinfo.snd_ppid  = htonl(ppid);
    spa.sendv_sndinfo.snd_flags = SCTP_EOR;

    // If ordered it must be reliable.
    if (parameters.ordered)
    {
        spa.sendv_prinfo.pr_policy = SCTP_PR_SCTP_NONE;
        spa.sendv_prinfo.pr_value  = 0;
    }
    // Configure reliability: https://tools.ietf.org/html/rfc3758
    else
    {
        spa.sendv_flags |= SCTP_SEND_PRINFO_VALID;
        spa.sendv_sndinfo.snd_flags |= SCTP_UNORDERED;

        if (parameters.maxPacketLifeTime != 0)
        {
            spa.sendv_prinfo.pr_policy = SCTP_PR_SCTP_TTL;
            spa.sendv_prinfo.pr_value  = parameters.maxPacketLifeTime;
        }
        else if (parameters.maxRetransmits != 0)
        {
            spa.sendv_prinfo.pr_policy = SCTP_PR_SCTP_RTX;
            spa.sendv_prinfo.pr_value  = parameters.maxRetransmits;
        }
    }

    int ret = usrsctp_sendv(
        this->_socket, msg, len, nullptr, 0, &spa, static_cast<socklen_t>(sizeof(spa)), SCTP_SENDV_SPA, 0);

    if (ret < 0)
    {
        fprintf(
          stdout,
          "error sending SCTP message [sid:%u" ", ppid:%u" ", message size:%zu]: %s",
          parameters.streamId,
          ppid,
          len,
          std::strerror(errno));
    }
}

void SctpAssociation::HandleDataConsumer(const SctpStreamParameters &params)
{
    // MS_TRACE();

    auto streamId = params.streamId;

    // We need more OS.
    if (streamId > this->_os - 1)
        AddOutgoingStreams(/*force*/ false);
}

void SctpAssociation::DataProducerClosed(const SctpStreamParameters &params)
{
    // MS_TRACE();

    auto streamId = params.streamId;

    // Send SCTP_RESET_STREAMS to the remote.
    // https://tools.ietf.org/html/draft-ietf-rtcweb-data-channel-13#section-6.7
    if (this->_isDataChannel)
        ResetSctpStream(streamId, StreamDirection::OUTGOING);
    else
        ResetSctpStream(streamId, StreamDirection::INCOMING);
}

void SctpAssociation::DataConsumerClosed(const SctpStreamParameters &params)
{
    // MS_TRACE();

    auto streamId = params.streamId;

    // Send SCTP_RESET_STREAMS to the remote.
    ResetSctpStream(streamId, StreamDirection::OUTGOING);
}

void SctpAssociation::ResetSctpStream(uint16_t streamId, StreamDirection direction)
{
    // MS_TRACE();

    // Do nothing if an outgoing stream that could not be allocated by us.
    if (direction == StreamDirection::OUTGOING && streamId > this->_os - 1)
        return;

    int ret;
    struct sctp_assoc_value av; // NOLINT(cppcoreguidelines-pro-type-member-init)
    socklen_t len = sizeof(av);

#ifndef SCTP_RECONFIG_SUPPORTED
#define SCTP_RECONFIG_SUPPORTED         0x00000029
#endif
    ret = usrsctp_getsockopt(this->_socket, IPPROTO_SCTP, SCTP_RECONFIG_SUPPORTED, &av, &len);

    if (ret == 0)
    {
        if (av.assoc_value != 1)
        {
            // MS_DEBUG_TAG(sctp, "stream reconfiguration not negotiated");

            return;
        }
    }
    else
    {
        fprintf(
          stdout,
          "could not retrieve whether stream reconfiguration has been negotiated: %s\n",
          std::strerror(errno));

        return;
    }

    // As per spec: https://tools.ietf.org/html/rfc6525#section-4.1
    len = sizeof(sctp_assoc_t) + (2 + 1) * sizeof(uint16_t);

    auto* srs = static_cast<struct sctp_reset_streams*>(std::malloc(len));

    switch (direction)
    {
        case StreamDirection::INCOMING:
            srs->srs_flags = SCTP_STREAM_RESET_INCOMING;
            break;

        case StreamDirection::OUTGOING:
            srs->srs_flags = SCTP_STREAM_RESET_OUTGOING;
            break;
    }

    srs->srs_number_streams = 1;
    srs->srs_stream_list[0] = streamId; // No need for htonl().

    ret = usrsctp_setsockopt(this->_socket, IPPROTO_SCTP, SCTP_RESET_STREAMS, srs, len);

    if (ret == 0)
    {
        // MS_DEBUG_TAG(sctp, "SCTP_RESET_STREAMS sent [streamId:%" PRIu16 "]", streamId);
    }
    else
    {
        fprintf(stdout, "usrsctp_setsockopt(SCTP_RESET_STREAMS) failed: %s", std::strerror(errno));
    }

    std::free(srs);
}

void SctpAssociation::AddOutgoingStreams(bool force)
{
    // MS_TRACE();

    uint16_t additionalOs{ 0 };

    if (MaxSctpStreams - this->_os >= 32)
        additionalOs = 32;
    else
        additionalOs = MaxSctpStreams - this->_os;

    if (additionalOs == 0)
    {
        // MS_WARN_TAG(sctp, "cannot add more outgoing streams [OS:%" PRIu16 "]", this->_os);

        return;
    }

    auto nextDesiredOs = this->_os + additionalOs;

    // Already in progress, ignore (unless forced).
    if (!force && nextDesiredOs == this->_desiredOs)
        return;

    // Update desired value.
    this->_desiredOs = nextDesiredOs;

    // If not connected, defer it.
    if (this->_state != SctpState::CONNECTED)
    {
        // MS_DEBUG_TAG(sctp, "SCTP not connected, deferring OS increase");

        return;
    }

    struct sctp_add_streams sas; // NOLINT(cppcoreguidelines-pro-type-member-init)

    std::memset(&sas, 0, sizeof(sas));
    sas.sas_instrms  = 0;
    sas.sas_outstrms = additionalOs;

    // MS_DEBUG_TAG(sctp, "adding %" PRIu16 " outgoing streams", additionalOs);

    int ret = usrsctp_setsockopt(
        this->_socket, IPPROTO_SCTP, SCTP_ADD_STREAMS, &sas, static_cast<socklen_t>(sizeof(sas)));

    // if (ret < 0)
    //     MS_WARN_TAG(sctp, "usrsctp_setsockopt(SCTP_ADD_STREAMS) failed: %s", std::strerror(errno));
}

void SctpAssociation::OnUsrSctpSendSctpData(void* buffer, size_t len)
{
    // MS_TRACE();

    const uint8_t* data = static_cast<uint8_t*>(buffer);

#if MS_LOG_DEV_LEVEL == 3
    MS_DUMP_DATA(data, len);
#endif

    this->onSctpAssociationSendData(this, data, len);
}

void SctpAssociation::OnUsrSctpReceiveSctpData(
    uint16_t streamId, uint16_t ssn, uint32_t ppid, int flags, const uint8_t* data, size_t len)
{
    // Ignore WebRTC DataChannel Control DATA chunks.
    if (ppid == 50)
    {
        // MS_WARN_TAG(sctp, "ignoring SCTP data with ppid:50 (WebRTC DataChannel Control)");

        return;
    }

    if (this->_messageBufferLen != 0 && ssn != this->_lastSsnReceived)
    {
        // MS_WARN_TAG(
        //   sctp,
        //   "message chunk received with different SSN while buffer not empty, buffer discarded [ssn:%" PRIu16
        //   ", last ssn received:%" PRIu16 "]",
        //   ssn,
        //   this->_lastSsnReceived);

        this->_messageBufferLen = 0;
    }

    // Update last SSN received.
    this->_lastSsnReceived = ssn;

    auto eor = static_cast<bool>(flags & MSG_EOR);

    if (this->_messageBufferLen + len > this->_maxSctpMessageSize)
    {
        // MS_WARN_TAG(
        //   sctp,
        //   "ongoing received message exceeds max allowed message size [message size:%zu, max message size:%zu, eor:%u]",
        //   this->_messageBufferLen + len,
        //   this->_maxSctpMessageSize,
        //   eor ? 1 : 0);

        this->_lastSsnReceived = 0;

        return;
    }

    // If end of message and there is no buffered data, notify it directly.
    if (eor && this->_messageBufferLen == 0)
    {
        // MS_DEBUG_DEV("directly notifying listener [eor:1, buffer len:0]");

        this->onSctpAssociationMessageReceived(this, streamId, ppid, data, len);
    }
    // If end of message and there is buffered data, append data and notify buffer.
    else if (eor && this->_messageBufferLen != 0)
    {
        std::memcpy(this->_messageBuffer + this->_messageBufferLen, data, len);
        this->_messageBufferLen += len;

        // MS_DEBUG_DEV("notifying listener [eor:1, buffer len:%zu]", this->_messageBufferLen);

        this->onSctpAssociationMessageReceived(
            this, streamId, ppid, this->_messageBuffer, this->_messageBufferLen);

        this->_messageBufferLen = 0;
    }
    // If non end of message, append data to the buffer.
    else if (!eor)
    {
        // Allocate the buffer if not already done.
        if (!this->_messageBuffer)
            this->_messageBuffer = new uint8_t[this->_maxSctpMessageSize];

        std::memcpy(this->_messageBuffer + this->_messageBufferLen, data, len);
        this->_messageBufferLen += len;

        // MS_DEBUG_DEV("data buffered [eor:0, buffer len:%zu]", this->_messageBufferLen);
    }
}

void SctpAssociation::OnUsrSctpReceiveSctpNotification(union sctp_notification* notification, size_t len)
{
    if (notification->sn_header.sn_length != (uint32_t)len)
        return;

    switch (notification->sn_header.sn_type)
    {
        case SCTP_ADAPTATION_INDICATION:
        {
            // MS_DEBUG_TAG(
            //   sctp,
            //   "SCTP adaptation indication [%x]",
            //   notification->sn_adaptation_event.sai_adaptation_ind);

            break;
        }

        case SCTP_ASSOC_CHANGE:
        {
            switch (notification->sn_assoc_change.sac_state)
            {
                case SCTP_COMM_UP:
                {
                    // MS_DEBUG_TAG(
                    //   sctp,
                    //   "SCTP association connected, streams [out:%" PRIu16 ", in:%" PRIu16 "]",
                    //   notification->sn_assoc_change.sac_outbound_streams,
                    //   notification->sn_assoc_change.sac_inbound_streams);

                    // Update our OS.
                    this->_os = notification->sn_assoc_change.sac_outbound_streams;

                    // Increase if requested before connected.
                    if (this->_desiredOs > this->_os)
                        AddOutgoingStreams(/*force*/ true);

                    if (this->_state != SctpState::CONNECTED)
                    {
                        this->_state = SctpState::CONNECTED;
                        this->onSctpAssociationConnected(this);
                    }

                    break;
                }

                case SCTP_COMM_LOST:
                {
                    if (notification->sn_header.sn_length > 0)
                    {
                        static const size_t BufferSize{ 1024 };
                        thread_local static char buffer[BufferSize];

                        uint32_t len = notification->sn_assoc_change.sac_length - sizeof(struct sctp_assoc_change);

                        for (uint32_t i{ 0 }; i < len; ++i)
                        {
                            std::snprintf(
                                buffer, BufferSize, " 0x%02x", notification->sn_assoc_change.sac_info[i]);
                        }

                        // MS_DEBUG_TAG(sctp, "SCTP communication lost [info:%s]", buffer);
                    }
                    else
                    {
                        // MS_DEBUG_TAG(sctp, "SCTP communication lost");
                    }

                    if (this->_state != SctpState::CLOSED)
                    {
                        this->_state = SctpState::CLOSED;
                        this->onSctpAssociationClosed(this);
                    }

                    break;
                }

                case SCTP_RESTART:
                {
                    // MS_DEBUG_TAG(
                    //   sctp,
                    //   "SCTP remote association restarted, streams [out:%" PRIu16 ", int:%" PRIu16 "]",
                    //   notification->sn_assoc_change.sac_outbound_streams,
                    //   notification->sn_assoc_change.sac_inbound_streams);

                    // Update our OS.
                    this->_os = notification->sn_assoc_change.sac_outbound_streams;

                    // Increase if requested before connected.
                    if (this->_desiredOs > this->_os)
                        AddOutgoingStreams(/*force*/ true);

                    if (this->_state != SctpState::CONNECTED)
                    {
                        this->_state = SctpState::CONNECTED;
                        this->onSctpAssociationConnected(this);
                    }

                    break;
                }

                case SCTP_SHUTDOWN_COMP:
                {
                    // MS_DEBUG_TAG(sctp, "SCTP association gracefully closed");

                    if (this->_state != SctpState::CLOSED)
                    {
                        this->_state = SctpState::CLOSED;
                        this->onSctpAssociationClosed(this);
                    }

                    break;
                }

                case SCTP_CANT_STR_ASSOC:
                {
                    if (notification->sn_header.sn_length > 0)
                    {
                        static const size_t BufferSize{ 1024 };
                        thread_local static char buffer[BufferSize];

                        uint32_t len = notification->sn_assoc_change.sac_length - sizeof(struct sctp_assoc_change);

                        for (uint32_t i{ 0 }; i < len; ++i)
                        {
                            std::snprintf(
                                buffer, BufferSize, " 0x%02x", notification->sn_assoc_change.sac_info[i]);
                        }

                        // MS_WARN_TAG(sctp, "SCTP setup failed: %s", buffer);
                    }

                    if (this->_state != SctpState::FAILED)
                    {
                        this->_state = SctpState::FAILED;
                        this->onSctpAssociationFailed(this);
                    }

                    break;
                }

                default:;
            }

            break;
        }

        // https://tools.ietf.org/html/rfc6525#section-6.1.2.
        case SCTP_ASSOC_RESET_EVENT:
        {
            // MS_DEBUG_TAG(sctp, "SCTP association reset event received");

            break;
        }

        // An Operation Error is not considered fatal in and of itself, but may be
        // used with an ABORT chunk to report a fatal condition.
        case SCTP_REMOTE_ERROR:
        {
            static const size_t BufferSize{ 1024 };
            thread_local static char buffer[BufferSize];

            uint32_t len = notification->sn_remote_error.sre_length - sizeof(struct sctp_remote_error);

            for (uint32_t i{ 0 }; i < len; i++)
            {
                std::snprintf(buffer, BufferSize, "0x%02x", notification->sn_remote_error.sre_data[i]);
            }

            // MS_WARN_TAG(
            //   sctp,
            //   "remote SCTP association error [type:0x%04x, data:%s]",
            //   notification->sn_remote_error.sre_error,
            //   buffer);

            break;
        }

        // When a peer sends a SHUTDOWN, SCTP delivers this notification to
        // inform the application that it should cease sending data.
        case SCTP_SHUTDOWN_EVENT:
        {
            // MS_DEBUG_TAG(sctp, "remote SCTP association shutdown");

            if (this->_state != SctpState::CLOSED)
            {
                this->_state = SctpState::CLOSED;
                this->onSctpAssociationClosed(this);
            }

            break;
        }

        case SCTP_SEND_FAILED_EVENT:
        {
            static const size_t BufferSize{ 1024 };
            thread_local static char buffer[BufferSize];

            uint32_t len =
                notification->sn_send_failed_event.ssfe_length - sizeof(struct sctp_send_failed_event);

            for (uint32_t i{ 0 }; i < len; ++i)
            {
                std::snprintf(buffer, BufferSize, "0x%02x", notification->sn_send_failed_event.ssfe_data[i]);
            }

            // MS_WARN_TAG(
            //   sctp,
            //   "SCTP message sent failure [streamId:%" PRIu16 ", ppid:%" PRIu32
            //   ", sent:%s, error:0x%08x, info:%s]",
            //   notification->sn_send_failed_event.ssfe_info.snd_sid,
            //   ntohl(notification->sn_send_failed_event.ssfe_info.snd_ppid),
            //   (notification->sn_send_failed_event.ssfe_flags & SCTP_DATA_SENT) ? "yes" : "no",
            //   notification->sn_send_failed_event.ssfe_error,
            //   buffer);

            break;
        }

        case SCTP_STREAM_RESET_EVENT:
        {
            bool incoming{ false };
            bool outgoing{ false };
            uint16_t numStreams =
                (notification->sn_strreset_event.strreset_length - sizeof(struct sctp_stream_reset_event)) /
                sizeof(uint16_t);

            if (notification->sn_strreset_event.strreset_flags & SCTP_STREAM_RESET_INCOMING_SSN)
                incoming = true;

            if (notification->sn_strreset_event.strreset_flags & SCTP_STREAM_RESET_OUTGOING_SSN)
                outgoing = true;

            //todo 打印sctp调试信息
            if (false /*MS_HAS_DEBUG_TAG(sctp)*/)
            {
                std::string streamIds;

                for (uint16_t i{ 0 }; i < numStreams; ++i)
                {
                    auto streamId = notification->sn_strreset_event.strreset_stream_list[i];

                    // Don't log more than 5 stream ids.
                    if (i > 4)
                    {
                        streamIds.append("...");

                        break;
                    }

                    if (i > 0)
                        streamIds.append(",");

                    streamIds.append(std::to_string(streamId));
                }

                // MS_DEBUG_TAG(
                //   sctp,
                //   "SCTP stream reset event [flags:%x, i|o:%s|%s, num streams:%" PRIu16 ", stream ids:%s]",
                //   notification->sn_strreset_event.strreset_flags,
                //   incoming ? "true" : "false",
                //   outgoing ? "true" : "false",
                //   numStreams,
                //   streamIds.c_str());
            }

            // Special case for WebRTC DataChannels in which we must also reset our
            // outgoing SCTP stream.
            if (incoming && !outgoing && this->_isDataChannel)
            {
                for (uint16_t i{ 0 }; i < numStreams; ++i)
                {
                    auto streamId = notification->sn_strreset_event.strreset_stream_list[i];

                    ResetSctpStream(streamId, StreamDirection::OUTGOING);
                }
            }

            break;
        }

        case SCTP_STREAM_CHANGE_EVENT:
        {
            if (notification->sn_strchange_event.strchange_flags == 0)
            {
                // MS_DEBUG_TAG(
                //   sctp,
                //   "SCTP stream changed, streams [out:%" PRIu16 ", in:%" PRIu16 ", flags:%x]",
                //   notification->sn_strchange_event.strchange_outstrms,
                //   notification->sn_strchange_event.strchange_instrms,
                //   notification->sn_strchange_event.strchange_flags);
            }
            else if (notification->sn_strchange_event.strchange_flags & SCTP_STREAM_RESET_DENIED)
            {
                // MS_WARN_TAG(
                //   sctp,
                //   "SCTP stream change denied, streams [out:%" PRIu16 ", in:%" PRIu16 ", flags:%x]",
                //   notification->sn_strchange_event.strchange_outstrms,
                //   notification->sn_strchange_event.strchange_instrms,
                //   notification->sn_strchange_event.strchange_flags);

                break;
            }
            else if (notification->sn_strchange_event.strchange_flags & SCTP_STREAM_RESET_FAILED)
            {
                // MS_WARN_TAG(
                //   sctp,
                //   "SCTP stream change failed, streams [out:%" PRIu16 ", in:%" PRIu16 ", flags:%x]",
                //   notification->sn_strchange_event.strchange_outstrms,
                //   notification->sn_strchange_event.strchange_instrms,
                //   notification->sn_strchange_event.strchange_flags);

                break;
            }

            // Update OS.
            this->_os = notification->sn_strchange_event.strchange_outstrms;

            break;
        }

        default:
        {
            // MS_WARN_TAG(
            //   sctp, "unhandled SCTP event received [type:%" PRIu16 "]", notification->sn_header.sn_type);
        }
    }
}

void SctpAssociation::setOnSctpAssociationConnecting(const function<void(SctpAssociation* sctpAssociation)>& cb)
{
    _onSctpAssociationConnecting = cb;
}

void SctpAssociation::setOnSctpAssociationConnected(const function<void(SctpAssociation* sctpAssociation)>& cb)
{
    _onSctpAssociationConnected = cb;
}

void SctpAssociation::setOnSctpAssociationFailed(const function<void(SctpAssociation* sctpAssociation)>& cb)
{
    _onSctpAssociationFailed = cb;
}

void SctpAssociation::setOnSctpAssociationClosed(const function<void(SctpAssociation* sctpAssociation)>& cb)
{
    _onSctpAssociationClosed = cb;
}

void SctpAssociation::setOnSctpAssociationSendData(const function<void(SctpAssociation* sctpAssociation, const uint8_t* data, size_t len)>& cb)
{
    _onSctpAssociationSendData = cb;
}

void SctpAssociation::setOnSctpAssociationMessageReceived(const function<void(
    SctpAssociation* sctpAssociation,
    uint16_t streamId,
    uint32_t ppid,
    const uint8_t* msg,
    size_t len)>& cb)
{
    _onSctpAssociationMessageReceived = cb;
}

void SctpAssociation::onSctpAssociationConnecting(SctpAssociation* sctpAssociation)
{
    if (_onSctpAssociationConnecting) {
        _onSctpAssociationConnecting(sctpAssociation);
    }
}

void SctpAssociation::onSctpAssociationConnected(SctpAssociation* sctpAssociation)
{
    if (_onSctpAssociationConnected) {
        _onSctpAssociationConnected(sctpAssociation);
    }
}

void SctpAssociation::onSctpAssociationFailed(SctpAssociation* sctpAssociation)
{
    if (_onSctpAssociationFailed) {
        _onSctpAssociationFailed(sctpAssociation);
    }
}

void SctpAssociation::onSctpAssociationClosed(SctpAssociation* sctpAssociation)
{
    if (_onSctpAssociationClosed) {
        _onSctpAssociationClosed(sctpAssociation);
    }
}

void SctpAssociation::onSctpAssociationSendData(SctpAssociation* sctpAssociation, const uint8_t* data, size_t len)
{
    if (_onSctpAssociationSendData) {
        _onSctpAssociationSendData(sctpAssociation, data, len);
    }
}

void SctpAssociation::onSctpAssociationMessageReceived(
    SctpAssociation* sctpAssociation,
    uint16_t streamId,
    uint32_t ppid,
    const uint8_t* msg,
    size_t len)
{
    if (_onSctpAssociationMessageReceived) {
        _onSctpAssociationMessageReceived(sctpAssociation, streamId, ppid, msg, len);
    }
}

////////////////////////////////////////////////////////////////////////////////////////

void SctpAssociationImp::OnUsrSctpSendSctpData(void* buffer, size_t len) {
    if (_poller->isCurrent()) {
        SctpAssociation::OnUsrSctpSendSctpData(buffer, len);
    } else {
        weak_ptr<SctpAssociationImp> weak_self = shared_from_this();
        string copy((const char *)buffer, len);
        _poller->async([weak_self, copy]() {
            auto strong_self = weak_self.lock();
            if (strong_self) {
                strong_self->SctpAssociation::OnUsrSctpSendSctpData((void *)copy.data(), copy.size());
            }
        }, true);
    }
}

void SctpAssociationImp::OnUsrSctpReceiveSctpData(uint16_t streamId, uint16_t ssn, uint32_t ppid, int flags, const uint8_t* data, size_t len) {
    if (_poller->isCurrent()) {
        SctpAssociation::OnUsrSctpReceiveSctpData(streamId, ssn, ppid, flags, data, len);
    } else {
        weak_ptr<SctpAssociationImp> weak_self = shared_from_this();
        string copy((const char *)data, len);
        _poller->async([weak_self, copy, streamId, ssn, ppid, flags]() {
            auto strong_self = weak_self.lock();
            if (strong_self) {
                strong_self->SctpAssociation::OnUsrSctpReceiveSctpData(streamId, ssn, ppid, flags, (const uint8_t* )copy.data(), copy.size());
            }
        }, true);
    }
}

void SctpAssociationImp::OnUsrSctpReceiveSctpNotification(union sctp_notification* notification, size_t len) {
    if (_poller->isCurrent()) {
        SctpAssociation::OnUsrSctpReceiveSctpNotification(notification, len);
    } else {
        weak_ptr<SctpAssociationImp> weak_self = shared_from_this();
        string copy((const char *)notification, len);
        _poller->async([weak_self, copy]() {
            auto strong_self = weak_self.lock();
            if (strong_self) {
                strong_self->SctpAssociation::OnUsrSctpReceiveSctpNotification((union sctp_notification *)copy.data(), copy.size());
            }
        },true);
    }
}
