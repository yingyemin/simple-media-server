//
// H264Deserialize.h
//
// Library: Codec
// Package: H264
// Module:  H264
// 

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "H264Common.h"
#include "H26xBinaryReader.h"

namespace Mmp
{
namespace Codec
{

/**
 * @sa ISO 14496/10(2020) - 8.1 NAL unit decoding process
 */
class H264Deserialize
{
public:
    using ptr = std::shared_ptr<H264Deserialize>;
public:
    H264Deserialize();
    ~H264Deserialize();
public:
    /**
     * @note The format of NAL units for both packet-oriented transport and byte stream is identical except
     *       that each NAL unit can be preceded by a start code prefix and extra padding bytes in the byte stream format.
     */
    bool DeserializeByteStreamNalUnit(H26xBinaryReader::ptr br, H264NalSyntax::ptr nal);
    bool DeserializeNalSyntax(H26xBinaryReader::ptr br, H264NalSyntax::ptr nal);
    bool DeserializeHrdSyntax(H26xBinaryReader::ptr br, H264HrdSyntax::ptr hrd);
    bool DeserializeVuiSyntax(H26xBinaryReader::ptr br, H264VuiSyntax::ptr vui);
    bool DeserializeSeiSyntax(H26xBinaryReader::ptr br, H264SeiSyntax::ptr sei);
    bool DeserializeSpsSyntax(H26xBinaryReader::ptr br, H264SpsSyntax::ptr sps);
    bool DeserializeSliceHeaderSyntax(H26xBinaryReader::ptr br, H264NalSyntax::ptr nal, H264SliceHeaderSyntax::ptr slice);
    bool DeserializeDecodedReferencePictureMarkingSyntax(H26xBinaryReader::ptr br, H264NalSyntax::ptr nal, H264DecodedReferencePictureMarkingSyntax::ptr drpm);
    bool DeserializeSubSpsSyntax(H26xBinaryReader::ptr br, H264SpsSyntax::ptr sps, H264SubSpsSyntax::ptr subSps);
    bool DeserializeSpsMvcSyntax(H26xBinaryReader::ptr br, H264SpsMvcSyntax::ptr mvc);
    bool DeserializeMvcVuiSyntax(H26xBinaryReader::ptr br, H264MvcVuiSyntax::ptr mvcVui);
    bool DeserializePpsSyntax(H26xBinaryReader::ptr br, H264PpsSyntax::ptr pps);
private:
    bool DeserializeNalSvcSyntax(H26xBinaryReader::ptr br, H264NalSvcSyntax::ptr svc);
    bool DeserializeNal3dAvcSyntax(H26xBinaryReader::ptr br, H264Nal3dAvcSyntax::ptr avc);
    bool DeserializeNalMvcSyntax(H26xBinaryReader::ptr br, H264NalMvcSyntax::ptr mvc);
    bool DeserializeScalingListSyntax(H26xBinaryReader::ptr br, std::vector<int32_t>& scalingList, int32_t sizeOfScalingList, int32_t& useDefaultScalingMatrixFlag);
    bool DeserializeReferencePictureListModificationSyntax(H26xBinaryReader::ptr br, H264SliceHeaderSyntax::ptr slice, H264ReferencePictureListModificationSyntax::ptr rplm);
    bool DeserializePredictionWeightTableSyntax(H26xBinaryReader::ptr br, H264SpsSyntax::ptr sps, H264SliceHeaderSyntax::ptr slice, H264PredictionWeightTableSyntax::ptr pwt);
private: /* SEI */
    bool DeserializeSeiBufferPeriodSyntax(H26xBinaryReader::ptr br, H264SeiBufferPeriodSyntax::ptr bp);
    bool DeserializeSeiPictureTimingSyntax(H26xBinaryReader::ptr br, H264VuiSyntax::ptr vui, H264SeiPictureTimingSyntax::ptr pt);
    bool DeserializeSeiUserDataRegisteredSyntax(H26xBinaryReader::ptr br, uint32_t payloadSize, H264SeiUserDataRegisteredSyntax::ptr udr);
    bool DeserializeSeiUserDataUnregisteredSyntax(H26xBinaryReader::ptr br, uint32_t payloadSize, H264SeiUserDataUnregisteredSyntax::ptr udn);
    bool DeserializeSeiRecoveryPointSyntax(H26xBinaryReader::ptr br, H264SeiRecoveryPointSyntax::ptr pt);
    bool DeserializeSeiContentLigntLevelInfoSyntax(H26xBinaryReader::ptr br, H264SeiContentLigntLevelInfoSyntax::ptr clli);
    bool DeserializeSeiDisplayOrientationSyntax(H26xBinaryReader::ptr br, H264SeiDisplayOrientationSyntax::ptr dot);
    bool DeserializeSeiMasteringDisplayColourVolumeSyntax(H26xBinaryReader::ptr br, H264MasteringDisplayColourVolumeSyntax::ptr mdcv);
    bool DeserializeSeiFilmGrainSyntax(H26xBinaryReader::ptr br, H264SeiFilmGrainSyntax::ptr fg);
    bool DeserializeSeiFramePackingArrangementSyntax(H26xBinaryReader::ptr br, H264SeiFramePackingArrangementSyntax::ptr fpa);
    bool DeserializeSeiAlternativeTransferCharacteristicsSyntax(H26xBinaryReader::ptr br, H264SeiAlternativeTransferCharacteristicsSyntax::ptr atc);
    bool DeserializeAmbientViewingEnvironmentSyntax(H26xBinaryReader::ptr br, H264AmbientViewingEnvironmentSyntax::ptr awe);
private:
    H264ContextSyntax::ptr _contex;
};

} // namespace Codec
} // namespace Mmp