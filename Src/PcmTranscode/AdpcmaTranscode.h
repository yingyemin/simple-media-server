#ifndef _IMA_ADPCM_H_
#define _IMA_ADPCM_H_

#include "Common/Frame.h"

enum
{
    IMA_ADPCM_DVI4 = 0,
    IMA_ADPCM_VDVI = 1
};

/*!
    IMA (DVI/Intel) ADPCM conversion state descriptor. This defines the state of
    a single working instance of the IMA ADPCM converter. This is used for
    either linear to ADPCM or ADPCM to linear conversion.
*/
typedef struct
{
    int variant;
    int last;
    int step_index;
    uint16_t ima_byte;
    int bits;
} ima_adpcm_state_t;


class AdpcmTranscode
{
public: 
    void init(int variant);

    FrameBuffer::Ptr encode(const FrameBuffer::Ptr frame);

    FrameBuffer::Ptr decode(const FrameBuffer::Ptr frame);

private:
    std::shared_ptr<ima_adpcm_state_t> _state;
};


#endif
/*- End of file ------------------------------------------------------------*/

