#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#if defined(_WIN32)
#include "Util/Util.h"
#else
#include <arpa/inet.h>
#endif

#include "RtcpPacket.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;
