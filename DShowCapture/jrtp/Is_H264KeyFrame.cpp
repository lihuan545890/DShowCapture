#include "Is_H264KeyFrame.h"
#include <stdio.h>
#include <math.h>

const I_SHORT bit[8] = {0x80 , 0x40 , 0x20 , 0x10 , 0x08 , 0x04 , 0x02 , 0x01};

//判断一帧H264数据是否是关键帧
//返回1:关键帧 pWidth返回视频的宽度 pHeight返回视频高度
//返回0:非关键帧 pWidth返回0  pHeihgt返回0
I_BOOL Is_H264KeyFrame(I_BYTE *pFrameData , I_INT nSize , I_INT *pWidth , I_INT *pHight)
{
    I_INT nNaluSize = 0;
    I_INT nKeyType;
    *pWidth = 0;
    *pHight = 0;
    if(nSize <= 4)
        return 0;
    nSize = nSize>1024?1024:nSize;
    while(nNaluSize < nSize)
    {
        while( *((I_INT*)pFrameData) != 0x01000000 && nNaluSize < nSize-3){
            pFrameData += 1;
            nNaluSize += 1;
        }
        if(nNaluSize < nSize-3 ){
            nKeyType = *(pFrameData+4)&0x1f;
            if(nKeyType == 07){
                SPS sps;
                I_INT SIZE = 0;
                GetNALSize(pFrameData , nSize-nNaluSize , &SIZE);
                SIZE -= 1;
                H264_Decode_SPS(pFrameData+5 , &SIZE , &sps);
                *pWidth = (sps.pic_width_in_mbs_minus1 + 1) * 16;
                *pHight = (sps.pic_height_in_map_units_minus1 + 1) * 16;
                return 1;
            }
            else{
                pFrameData += 4;
                nNaluSize += 4;
                continue;
//				return 0;
            }
        }
		else
		{
			return 0;
		}
    }
    return 0;
}




int isNalHead( char* lpData, int size )
{
	int i = 0;
	if(size >= 4 && lpData[i]==0&&lpData[i+1]==0 && lpData[i+2]==0 && lpData[i+3]==1)
		return 4;
	
	if(size >= 3 && lpData[i]==0&&lpData[i+1]==0 && lpData[i+2]==1 )
		return 3;
	
	return 0;
}

int isNalTail( char* lpData, int size )
{
	int i = 0;
	if(size >= 4 && lpData[i]==0&&lpData[i+1]==0 && lpData[i+2]==0 && lpData[i+3]==1)
		return 4;
	
	if(size >= 3 && lpData[i]==0&&lpData[i+1]==0 && lpData[i+2]==1 )
		return 3;
	
	if(size >= 3 && lpData[i]==0&&lpData[i+1]==0 && lpData[i+2]==0 )
		return 3;
	
	return 0;
}

//得到一个NALU单元中除去0x00000001后数据长度
int GetNALSize(I_BYTE *lpData , I_INT nSize , I_INT *pSize)
{
/*    I_INT i = 4;
    while((lpData[i]!=0 || lpData[i+1]!=0 || lpData[i+2]!=0
	|| lpData[i+3]!=1) )
    {
        i++;
        if(i == nSize-3)
            break;
    }
    if(i == nSize-3)
        *pSize = nSize-4;
    else
        *pSize = i-4;
		*/
	int nNALSignLen = isNalHead( lpData, nSize );
	int i = nNALSignLen;
	int nNALEndSignLen = 0;
	while( i < nSize && ( nNALEndSignLen=isNalTail(&lpData[i], nSize-i) ) == 0 )
	{
		i++;
	}
	if(nNALEndSignLen == 0)
		*pSize = nSize-nNALSignLen;
	else
		*pSize = i-nNALSignLen;

	return nNALSignLen;

}

//返回某1个字节中的某1位二进制位的值
I_INT get_bits1(I_BYTE* *pFrameData , I_INT *pSize , I_INT *pFromByteBits)
{
    I_INT nByteBits = *pFromByteBits;
    I_INT nBit_value = 0;
	if( *pSize < 0)
		return 0;
    nBit_value = (bit[nByteBits++] & **pFrameData) >> (7 - nByteBits);
    if(nByteBits == 8){
        nByteBits = 0;
        (*pFrameData)++;
        *pSize -= 1;
		if( *pSize <= 0 )
			return 0;
    }
    *pFromByteBits = nByteBits;
    return nBit_value;
}

//解析H264中有符号指数哥伦布码的码值
I_INT get_se_golomb(I_BYTE* *pFrameData , I_INT *pSize , I_INT *pFromByteBits)
{
    I_INT nByteBits = 0;
    I_INT nM_golombBits = 0;
    I_INT nM_golombINFO = 0;
    I_INT code_num = 0;
    I_INT i;
	if( *pSize < 0 ) 
		return 0;
    nByteBits = *pFromByteBits;
    while(!(bit[nByteBits++] & **pFrameData)){
        if(nByteBits==8){
            (*pFrameData)++;
            nByteBits = 0;
            *pSize -= 1;
			if( *pSize <= 0 )
				return 0;
        }
        nM_golombBits++;
    }
    code_num = (I_INT)pow(2.0f , nM_golombBits);
    for(i = 0; i < nM_golombBits; i++){
        if(nByteBits==8){
            (*pFrameData)++;
            nByteBits = 0;
            *pSize -= 1;
			if( *pSize <= 0 )
				return 0;
      }
        nM_golombINFO = (nM_golombINFO<<1)|( ( bit[nByteBits++] & **pFrameData) >> (7 - nByteBits) );
    }
    code_num = code_num + nM_golombINFO - 1;
    if(nByteBits == 8){
        (*pFrameData)++;
        nByteBits = 0;
        *pSize -= 1;
		if( *pSize <= 0 )
			return 0;
	}
    *pFromByteBits = nByteBits;
    return (I_INT)pow(-1.0f , code_num + 1) * ( (I_INT)(code_num / 2 + 0.5) );
}

//解析H264中无符号指数哥伦布码的码值
I_INT get_ue_golomb(I_BYTE* *pFrameData , I_INT *pSize , I_INT *pFromByteBits)
{
    I_INT nByteBits = 0;
    I_INT nM_golombBits = 0;
    I_INT nM_golombINFO = 0;
    I_INT code_num = 0;
    I_INT i;
	if( *pSize <= 0 )
		return 0;
    nByteBits = *pFromByteBits;
    while(!(bit[nByteBits++] & **pFrameData)){
        if(nByteBits==8){
            (*pFrameData)++;
            nByteBits = 0;
            *pSize -= 1;
			if( *pSize <= 0 )
				return 0;
        }
        nM_golombBits++;
    }
    code_num = (I_INT)pow(2.0f ,nM_golombBits);
    for(i = 0; i < nM_golombBits; i++){
        if(nByteBits==8){
            (*pFrameData)++;
            nByteBits = 0;
            *pSize -= 1;
			if( *pSize <= 0 )
				return 0;
        }
        nM_golombINFO = (nM_golombINFO<<1)|( ( bit[nByteBits++] & **pFrameData) >> (7 - nByteBits) );
    }
    code_num = code_num + nM_golombINFO - 1;
    if(nByteBits == 8){
        nByteBits = 0;
        (*pFrameData)++;
        *pSize -= 1;
		if( *pSize <= 0 )
			return 0;
    }
    *pFromByteBits = nByteBits;
    return code_num;
}

//解析H264数据的sps(序列参数集)信息
void H264_Decode_SPS(I_BYTE *pFrameData , I_INT *pSize , SPS *sps )
{
    I_INT FromByteBits = 0;
    I_INT i;
    I_INT bits;
    //码流对应的profile
    sps->profile_idc = *pFrameData;                             pFrameData++;    //8位
    sps->constraint_set_flags = (0xf0 & *pFrameData) >> 4;                       //4位constraint_set_flags
    sps->reserved_zero_4bits = (0x0f & *pFrameData);            pFrameData++;    //4位保留位
    sps->level_idc = *pFrameData;                               pFrameData++;    //8位
    *pSize -= 3;
    sps->seq_parameter_set_id = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
    if(sps->profile_idc == 83){
        //seq_parameter_set_svc_extension();
        ;
    }
    if(sps->profile_idc >= 100 || sps->profile_idc == 83){
        sps->chroma_format_idc = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
        if(sps->chroma_format_idc == 3){
            sps->residual_colour_transform_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
        }
        sps->bit_depth_luma_minus8   = get_ue_golomb(&pFrameData , pSize , &FromByteBits) + 8;
        sps->bit_depth_chroma_minus8 = get_ue_golomb(&pFrameData , pSize , &FromByteBits) + 8;
        sps->qpprime_y_zero_transform_bypass_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
        sps->seq_scaling_matrix_present_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
        if(sps->seq_scaling_matrix_present_flag){
            for(i = 0; i < 8; i++){
                bits = get_bits1(&pFrameData , pSize , &FromByteBits);
                sps->seq_scaling_list_present_flag = (sps->seq_scaling_list_present_flag << 1) | bits;
                if(bits){
                    if(i < 6){
                        //scaling_list( ScalingList4x4[ i ], 16, UseDefaultScalingMatrix4x4Flag[ i ]);
                        ;
                    }else{
                        //scaling_list( ScalingList8x8[ i – 6 ] , 64 , UseDefaultScalingMatrix8x8Flag[ i – 6 ] );
                        ;
                    }
                }
            }
        }
    }
    sps->log2_max_frame_num_minus4 = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
    sps->pic_order_cnt_type = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
    if(sps->pic_order_cnt_type == 0){
        sps->log2_max_pic_order_cnt_lsb_minus4 = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
    }
    else if(sps->pic_order_cnt_type == 1){
        sps->delta_pic_order_always_zero_flag = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
        sps->offset_for_non_ref_pic = get_se_golomb(&pFrameData , pSize , &FromByteBits);          
        sps->offset_for_top_to_bottom_field = get_se_golomb(&pFrameData , pSize , &FromByteBits);  
        sps->num_ref_frames_in_pic_order_cnt_cycle = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
        for(i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++){
            sps->offset_for_ref_frame[i] = get_se_golomb(&pFrameData , pSize , &FromByteBits);      
        }
    }
    sps->num_ref_frames = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
    sps->gaps_in_frame_num_value_allowed_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
    sps->pic_width_in_mbs_minus1 = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
    sps->pic_height_in_map_units_minus1 = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
    sps->frame_mbs_only_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
    if( !sps->frame_mbs_only_flag )
        sps->mb_adaptive_frame_field_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
    sps->direct_8x8_inference_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
    sps->frame_cropping_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
    if(sps->frame_cropping_flag){
        sps->frame_crop_left_offset = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
        sps->frame_crop_right_offset = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
        sps->frame_crop_top_offset = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
        sps->frame_crop_bottom_offset = get_ue_golomb(&pFrameData , pSize , &FromByteBits);
    }
    sps->vui_parameters_present_flag = get_bits1(&pFrameData , pSize , &FromByteBits);
    if(sps->vui_parameters_present_flag){
        //vui_parameters();
        //rbsp_trailing_bits();
        ;
    }
}




int  GetNalSize( I_BYTE * lpData,int dataSize, h264_nal *pnal, int maxNal )
{
	int nalCount = 0;

	while( dataSize && nalCount < maxNal )
	{
		int nNALSignLen ;
		I_INT size;
		char type;

		// search head 
		while( dataSize )
		{
			nNALSignLen = isNalHead( lpData, dataSize );
			if( nNALSignLen != 0 )
				break;
			lpData ++;
			dataSize --;
		}
		if( nNALSignLen == 0 )
			break;  // end ...

		// head ok, 
		nNALSignLen = GetNALSize( lpData , dataSize , &size );
		type = *(lpData+nNALSignLen);
		type = type&0x1f;

		// set the nal info
		pnal[nalCount].type = type;
		pnal[nalCount].pdata = lpData+nNALSignLen;
		pnal[nalCount].len = size;

		// offset 
		lpData += (nNALSignLen+size);
		dataSize -= (nNALSignLen+size);

		nalCount ++;
	}

	return nalCount;
}