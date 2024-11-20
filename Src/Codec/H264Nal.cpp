#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>

#include "Logger.h"
#include "Util/String.h"
#include "H264Nal.h"

using namespace std;
  
uint32_t Ue(unsigned char *pBuff, uint32_t nLen, uint32_t &nStartBit)
{  
    if (nStartBit == nLen * 8) {
        return 0;
    }
    //计算0bit的个数  
    uint32_t nZeroNum = 0;  
    while (nStartBit < nLen * 8)  
    {  
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余  
        {  
            break;  
        }  
        nZeroNum++;  
        nStartBit++;  
    }  
    nStartBit ++;  
  
  
    //计算结果  
    unsigned long dwRet = 0;  
    for (uint32_t i=0; i<nZeroNum; i++)  
    {  
        if (nStartBit >= nLen * 8) {
            break;
        }
        dwRet <<= 1;  
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))  
        {  
            dwRet += 1;  
        }  
        nStartBit++;  
    }  
    return (1 << nZeroNum) - 1 + dwRet;  
}  
  
  
int Se(unsigned char *pBuff, uint32_t nLen, uint32_t &nStartBit)
{  
    int UeVal=Ue(pBuff,nLen,nStartBit);  
    double k=UeVal;  
    int nValue=ceil(k/2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00  
    if (UeVal % 2==0)  
        nValue=-nValue;  
    return nValue;  
}  
  
  
unsigned long u(uint32_t BitCount,unsigned char * buf, uint32_t len, uint32_t &nStartBit)
{  
    unsigned long dwRet = 0;  
    len *= 8;
    for (uint32_t i=0; i<BitCount; i++)  
    {  
        if (nStartBit > len) {
            break;
        }
        dwRet <<= 1;  
        if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))  
        {  
            dwRet += 1;  
        }  
        nStartBit++;  
    }  
    return dwRet;  
}  
  
/** 
 * H264的NAL起始码防竞争机制 
 * 
 * @param buf SPS数据内容 
 * 
 * @无返回值 
 */  
void de_emulation_prevention(unsigned char* buf,unsigned int* buf_size)
{  
    int i=0,j=0;  
    unsigned char* tmp_ptr=NULL;  
    unsigned int tmp_buf_size=0;  
    int val=0;  
  
    tmp_ptr=buf;  
    tmp_buf_size=*buf_size;  
    for(i=0;i<(tmp_buf_size-2);i++)  
    {  
        //check for 0x000003  
        val=(tmp_ptr[i]^0x00) +(tmp_ptr[i+1]^0x00)+(tmp_ptr[i+2]^0x03);  
        if(val==0)  
        {  
            //kick out 0x03  
            for(j=i+2;j<tmp_buf_size-1;j++)  
                tmp_ptr[j]=tmp_ptr[j+1];  
  
            //and so we should devrease bufsize  
            (*buf_size)--;  
        }  
    }  
}  
  
/** 
 * 解码SPS,获取视频图像宽、高和帧率信息 
 * 
 * @param buf SPS数据内容 
 * @param nLen SPS数据的长度 
 * @param width 图像宽度 
 * @param height 图像高度 
 
 * @成功则返回true , 失败则返回false 
 */  
bool h264_decode_sps(unsigned char * buf,unsigned int nLen,int &width,int &height,int &fps)
{
    if (!buf || nLen == 0) {
        return false;
    }
    
    uint32_t StartBit=0;  
    // fps=0;  
    de_emulation_prevention(buf,&nLen);  
  
    int forbidden_zero_bit=u(1,buf, nLen, StartBit);  
    int nal_ref_idc=u(2,buf, nLen,StartBit);  
    int nal_unit_type=u(5,buf, nLen,StartBit);  
    if(nal_unit_type==7)  
    {  
        int profile_idc=u(8,buf, nLen,StartBit);  
        int constraint_set0_flag=u(1,buf, nLen,StartBit);//(buf[1] & 0x80)>>7;  
        int constraint_set1_flag=u(1,buf, nLen,StartBit);//(buf[1] & 0x40)>>6;  
        int constraint_set2_flag=u(1,buf, nLen,StartBit);//(buf[1] & 0x20)>>5;  
        int constraint_set3_flag=u(1,buf, nLen,StartBit);//(buf[1] & 0x10)>>4;  
        int reserved_zero_4bits=u(4,buf, nLen,StartBit);  
        int level_idc=u(8,buf, nLen,StartBit);  
  
        int seq_parameter_set_id=Ue(buf,nLen,StartBit);  
  
        if( profile_idc == 100 || profile_idc == 110 ||  
            profile_idc == 122 || profile_idc == 144 )  
        {  
            int chroma_format_idc=Ue(buf,nLen,StartBit);  
            if( chroma_format_idc == 3 )  
                int residual_colour_transform_flag=u(1,buf, nLen,StartBit);  
            int bit_depth_luma_minus8=Ue(buf,nLen,StartBit);  
            int bit_depth_chroma_minus8=Ue(buf,nLen,StartBit);  
            int qpprime_y_zero_transform_bypass_flag=u(1,buf, nLen,StartBit);  
            int seq_scaling_matrix_present_flag=u(1,buf, nLen,StartBit);  
  
            int seq_scaling_list_present_flag[8];  
            if( seq_scaling_matrix_present_flag )  
            {  
                for( int i = 0; i < 8; i++ ) {  
                    seq_scaling_list_present_flag[i]=u(1,buf, nLen,StartBit);  
                }  
            }  
        }  
        int log2_max_frame_num_minus4=Ue(buf,nLen,StartBit);  
        int pic_order_cnt_type=Ue(buf,nLen,StartBit);  
        if( pic_order_cnt_type == 0 )  
            int log2_max_pic_order_cnt_lsb_minus4=Ue(buf,nLen,StartBit);  
        else if( pic_order_cnt_type == 1 )  
        {  
            int delta_pic_order_always_zero_flag=u(1,buf, nLen,StartBit);  
            int offset_for_non_ref_pic=Se(buf,nLen,StartBit);  
            int offset_for_top_to_bottom_field=Se(buf,nLen,StartBit);  
            int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,StartBit);  
  
            int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];  
            for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )  
                offset_for_ref_frame[i]=Se(buf,nLen,StartBit);  
            delete [] offset_for_ref_frame;  
        }  
        int num_ref_frames=Ue(buf,nLen,StartBit);  
        int gaps_in_frame_num_value_allowed_flag=u(1,buf, nLen,StartBit);  
        int pic_width_in_mbs_minus1=Ue(buf,nLen,StartBit);  
        int pic_height_in_map_units_minus1=Ue(buf,nLen,StartBit);  

        printf("num_ref_frames:%d.\n", num_ref_frames);
        width=(pic_width_in_mbs_minus1+1)*16;  
        height=(pic_height_in_map_units_minus1+1)*16;  
  
        int frame_mbs_only_flag=u(1,buf, nLen,StartBit);  
        if(!frame_mbs_only_flag)  
            int mb_adaptive_frame_field_flag=u(1,buf, nLen,StartBit);  
  
        int direct_8x8_inference_flag=u(1,buf, nLen,StartBit);  
        int frame_cropping_flag=u(1,buf, nLen,StartBit);  
        if(frame_cropping_flag)  
        {  
            int frame_crop_left_offset=Ue(buf,nLen,StartBit);  
            int frame_crop_right_offset=Ue(buf,nLen,StartBit);  
            int frame_crop_top_offset=Ue(buf,nLen,StartBit);  
            int frame_crop_bottom_offset=Ue(buf,nLen,StartBit);  
        }  
        int vui_parameter_present_flag=u(1,buf, nLen,StartBit);  
        printf("vui_parameter_present_flag:%d.\n", vui_parameter_present_flag);
        if(vui_parameter_present_flag)  
        {  
            int aspect_ratio_info_present_flag=u(1,buf, nLen,StartBit);  
            if(aspect_ratio_info_present_flag)  
            {  
                int aspect_ratio_idc=u(8,buf, nLen,StartBit);  
                if(aspect_ratio_idc==255)  
                {  
                    int sar_width=u(16,buf, nLen,StartBit);  
                    int sar_height=u(16,buf, nLen,StartBit);  
                }  
            }  
            int overscan_info_present_flag=u(1,buf, nLen,StartBit);  
            if(overscan_info_present_flag)  
                int overscan_appropriate_flagu=u(1,buf, nLen,StartBit);  
            int video_signal_type_present_flag=u(1,buf, nLen,StartBit);  
            if(video_signal_type_present_flag)  
            {  
                int video_format=u(3,buf, nLen,StartBit);  
                int video_full_range_flag=u(1,buf, nLen,StartBit);  
                int colour_description_present_flag=u(1,buf, nLen,StartBit);  
                if(colour_description_present_flag)  
                {  
                    int colour_primaries=u(8,buf, nLen,StartBit);  
                    int transfer_characteristics=u(8,buf, nLen,StartBit);  
                    int matrix_coefficients=u(8,buf, nLen,StartBit);  
                }  
            }  
            int chroma_loc_info_present_flag=u(1,buf, nLen,StartBit);  
            if(chroma_loc_info_present_flag)  
            {  
                int chroma_sample_loc_type_top_field=Ue(buf,nLen,StartBit);  
                int chroma_sample_loc_type_bottom_field=Ue(buf,nLen,StartBit);  
            }  
            int timing_info_present_flag=u(1,buf, nLen,StartBit);  
  
            if(timing_info_present_flag)  
            {  
                int num_units_in_tick=u(32,buf, nLen,StartBit);  
                int time_scale=u(32,buf, nLen,StartBit);  
                fps=time_scale/num_units_in_tick;  
                int fixed_frame_rate_flag=u(1,buf, nLen,StartBit);  
                if(fixed_frame_rate_flag)  
                {  
                    fps=fps/2;  
                }  
            }  
        }  
        return true;  
    }  
    else  
        return false;  
}