#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Frame.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

FrameBuffer::FrameBuffer()
{

}

const char* FrameBuffer::findNextNalu(const char* p, size_t bytes, size_t& leading)
{
    size_t i, zeros;
    for (zeros = i = 0; i + 1 < bytes; i++)
    {
        if (0x01 == p[i] && zeros >= 2)
        {
            assert(i >= zeros);
            leading = (zeros + 1) > 4 ? 4 : (zeros + 1); // zeros + 0x01
            return p +i + 1 - leading;
        }

        zeros = 0x00 != p[i] ? 0 : (zeros + 1);
    }

    return nullptr;
}