/*
 * 
 *
 * 
 *
 * 
 * 
 * 
 */


#ifndef CTVSS_RTCSRS_H
#define CTVSS_RTCSRS_H

#include <stdarg.h>
#include "Log/Logger.h"

typedef int srs_error_t;

#define srs_success 0
#define ERROR_RTC_PORT                      5000
#define ERROR_RTP_PACKET_CREATE             5001
#define ERROR_OpenSslCreateSSL              5002
#define ERROR_OpenSslBIOReset               5003
#define ERROR_OpenSslBIOWrite               5004
#define ERROR_OpenSslBIONew                 5005
#define ERROR_RTC_RTP                       5006
#define ERROR_RTC_RTCP                      5007
#define ERROR_RTC_STUN                      5008
#define ERROR_RTC_DTLS                      5009
#define ERROR_RTC_UDP                       5010
#define ERROR_RTC_RTP_MUXER                 5011
#define ERROR_RTC_SDP_DECODE                5012
#define ERROR_RTC_SRTP_INIT                 5013
#define ERROR_RTC_SRTP_PROTECT              5014
#define ERROR_RTC_SRTP_UNPROTECT            5015
#define ERROR_RTC_RTCP_CHECK                5016
#define ERROR_RTC_SOURCE_CHECK              5017
#define ERROR_RTC_SDP_EXCHANGE              5018


#define SrsAutoFree(className, instance) \
impl_SrsAutoFree<className> _auto_free_##instance(&instance, false)
#define SrsAutoFreeA(className, instance) \
impl_SrsAutoFree<className> _auto_free_array_##instance(&instance, true)
template<class T>
class impl_SrsAutoFree
{
private:
    T** ptr;
    bool is_array;
public:
    impl_SrsAutoFree(T** p, bool array) {
        ptr = p;
        is_array = array;
    }
    
    virtual ~impl_SrsAutoFree() {
        if (ptr == NULL || *ptr == NULL) {
            return;
        }
        
        if (is_array) {
            delete[] *ptr;
        } else {
            delete *ptr;
        }
        
        *ptr = NULL;
    }
};

#endif
