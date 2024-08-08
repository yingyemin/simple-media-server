//
// H264Deserialize.h
//
// Library: Codec
// Package: H265
// Module:  H265
// 

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "H265Common.h"
#include "H26xBinaryReader.h"

namespace Mmp
{
namespace Codec
{

class H265Deserialize
{
public:
    using ptr = std::shared_ptr<H265Deserialize>;
public:
    H265Deserialize();
    ~H265Deserialize() = default;
public:
    /**
     * @note for H264 Annex B type, common in network stream
     */
    bool DeserializeByteStreamNalUnit(H26xBinaryReader::ptr br, H265NalSyntax::ptr nal);
    bool DeserializeNalSyntax(H26xBinaryReader::ptr br, H265NalSyntax::ptr nal);
    bool DeserializeNalHeaderSyntax(H26xBinaryReader::ptr br, H265NalUnitHeaderSyntax::ptr nalHeader); 
    bool DeserializePpsSyntax(H26xBinaryReader::ptr br, H265PpsSyntax::ptr pps);
    bool DeserializeSpsSyntax(H26xBinaryReader::ptr br, H265SpsSyntax::ptr sps);
    bool DeserializeVPSSyntax(H26xBinaryReader::ptr br, H265VPSSyntax::ptr vps);
    bool DeserializeSeiMessageSyntax(H26xBinaryReader::ptr br, H265VPSSyntax::ptr vps, H265SpsSyntax::ptr sps, H265VuiSyntax::ptr vui, H265HrdSyntax::ptr hrd, H265SeiMessageSyntax::ptr sei);
    bool DeserializeSliceHeaderSyntax(H26xBinaryReader::ptr br, H265NalUnitHeaderSyntax::ptr nal, H265SliceHeaderSyntax::ptr slice);
private: /* pps */
    bool DeserializePps3dSyntax(H26xBinaryReader::ptr br, H265PpsSyntax::ptr pps, H265Pps3dSyntax::ptr pps3d);
    bool DeserializePpsRangeSyntax(H26xBinaryReader::ptr br, H265SpsSyntax::ptr sps, H265PpsSyntax::ptr pps, H265PpsRangeSyntax::ptr ppsRange);
    bool DeserializePpsSccSyntax(H26xBinaryReader::ptr br, H265PpsSccSyntax::ptr ppsScc);
private: /* sps */
    bool DeserializeSpsRangeSyntax(H26xBinaryReader::ptr br, H265SpsRangeSyntax::ptr spsRange);
    bool DeserializeSps3DSyntax(H26xBinaryReader::ptr br, H265Sps3DSyntax::ptr sps3d);
    bool DeserializeSpsSccSyntax(H26xBinaryReader::ptr br, H265SpsSyntax::ptr sps, H265SpsSccSyntax::ptr spsScc);
    bool DeserializeVuiSyntax(H26xBinaryReader::ptr br, H265SpsSyntax::ptr sps, H265VuiSyntax::ptr vui);
private: /* slice */
    bool DeserializeRefPicListsModificationSyntax(H26xBinaryReader::ptr br, H265SpsSyntax::ptr sps, H265PpsSyntax::ptr pps, H265SliceHeaderSyntax::ptr slice, H265RefPicListsModificationSyntax::ptr rplm);
    bool DeserializePredWeightTableSyntax(H26xBinaryReader::ptr br, H265SpsSyntax::ptr sps, H265SliceHeaderSyntax::ptr slice, H265PredWeightTableSyntax::ptr pwt);
private: /* sei */
    bool DeserializeSeiDecodedPictureHash(H26xBinaryReader::ptr br, H265SpsSyntax::ptr sps, H265SeiDecodedPictureHashSyntax::ptr dph);
    bool DeserializeSeiPicTimingSyntax(H26xBinaryReader::ptr br,  H265SpsSyntax::ptr sps, H265VuiSyntax::ptr vui, H265HrdSyntax::ptr hrd, H265SeiPicTimingSyntax::ptr pt);
    bool DeserializeSeiRecoveryPointSyntax(H26xBinaryReader::ptr br, H265SeiRecoveryPointSyntax::ptr rp);
    bool DeserializeSeiActiveParameterSetsSyntax(H26xBinaryReader::ptr br, H265VPSSyntax::ptr vps, H265SeiActiveParameterSetsSyntax::ptr aps);
    bool DeserializeSeiTimeCodeSyntax(H26xBinaryReader::ptr br, H265SeiTimeCodeSyntax::ptr tc);
    bool DeserializeSeiMasteringDisplayColourVolumeSyntax(H26xBinaryReader::ptr br, H265MasteringDisplayColourVolumeSyntax::ptr mpcv);
    bool DeserializeSeiContentLightLevelInformationSyntax(H26xBinaryReader::ptr br, H265ContentLightLevelInformationSyntax::ptr clli);
    bool DeserializeSeiContentColourVolumeSyntax(H26xBinaryReader::ptr br, H265ContentColourVolumeSyntax::ptr ccv);
private:
    bool DeserializeHrdSyntax(H26xBinaryReader::ptr br, uint8_t commonInfPresentFlag, uint32_t maxNumSubLayersMinus, H265HrdSyntax::ptr hrd);
    bool DeserializeSubLayerHrdSyntax(H26xBinaryReader::ptr br, uint32_t subLayerId, H265HrdSyntax::ptr hrd, H265SubLayerHrdSyntax::ptr slHrd);
    bool DeserializePTLSyntax(H26xBinaryReader::ptr br, uint8_t profilePresentFlag, uint32_t maxNumSubLayersMinus1, H265PTLSyntax::ptr ptl);
    bool DeserializeScalingListDataSyntax(H26xBinaryReader::ptr br, H265ScalingListDataSyntax::ptr sld);
    bool DeserializeStRefPicSetSyntax(H26xBinaryReader::ptr br, H265SpsSyntax::ptr sps, uint32_t stRpsIdx, H265StRefPicSetSyntax::ptr stps);
    bool DeserializeColourMappingTable(H26xBinaryReader::ptr br, H265ColourMappingTable::ptr cmt);
    bool DeserializePpsMultilayerSyntax(H26xBinaryReader::ptr br, H265PpsMultilayerSyntax::ptr ppsMultilayer);
    bool DeserializeDeltaDltSyntax(H26xBinaryReader::ptr br, H265Pps3dSyntax::ptr pps3d, H265DeltaDltSyntax::ptr dd);
private:
    H265ContextSyntax::ptr _contex;
};

} // namespace Codec
} // namespace Mmp