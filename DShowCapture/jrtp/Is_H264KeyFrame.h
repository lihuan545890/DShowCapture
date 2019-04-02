#ifndef IS_H264KEYFRAME_H
#define IS_H264KEYFRAME_H

#ifdef __cplusplus
extern "C"
{
#endif

//#define MAX_SPS_COUNT   32

typedef unsigned char I_BOOL;
typedef char I_BYTE;
typedef int  I_INT;
//typedef unsigned int I_UINT;
//typedef unsigned short I_USHORT;
typedef short   I_SHORT;

//#ifdef  WIN32
//typedef __int64     I_INT64;
//typedef unsigned __int64    I_UINT64;    
//#else
//typedef long long   I_INT64;     
//typedef unsigned long long I_UINT64;
//#endif


typedef struct SPS{
    int profile_idc;
    int constraint_set_flags;          //constraint_set[0-3]_flag
    int level_idc;

    int reserved_zero_4bits; 

    int seq_parameter_set_id;

    int chroma_format_idc;
    int residual_colour_transform_flag;
    int bit_depth_luma_minus8;
    int bit_depth_chroma_minus8;
    int qpprime_y_zero_transform_bypass_flag;
    int seq_scaling_matrix_present_flag;
    int seq_scaling_list_present_flag;
    int log2_max_frame_num_minus4;
    int pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;
    int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int num_ref_frames_in_pic_order_cnt_cycle;
    short offset_for_ref_frame[256];
    int num_ref_frames;
    int gaps_in_frame_num_value_allowed_flag;
    int pic_width_in_mbs_minus1;
    int pic_height_in_map_units_minus1;
    int frame_mbs_only_flag;
    int mb_adaptive_frame_field_flag;
    int direct_8x8_inference_flag;
    int frame_cropping_flag;
    
    int frame_crop_left_offset;
    int frame_crop_right_offset;
    int frame_crop_top_offset;
    int frame_crop_bottom_offset;

    int vui_parameters_present_flag;
}SPS;

int GetNALSize(I_BYTE *lpData , I_INT nSize , I_INT *pSize);
I_BOOL Is_H264KeyFrame(I_BYTE *pFrameData , I_INT nSize , I_INT *pWidth , I_INT *pHight);
void H264_Decode_SPS(I_BYTE *pFrameData , I_INT *PSize , SPS *sps );
I_INT get_ue_golomb(I_BYTE* *pFrameData , I_INT *pSize , I_INT *pFromByteBits);
I_INT get_se_golomb(I_BYTE* *pFrameData , I_INT *pSize , I_INT *pFromByteBits);
I_INT get_bits1(I_BYTE* *pFrameData , I_INT *pSize , I_INT *pFromByteBits);


typedef struct
{
	char	type;
	I_BYTE* pdata;		// not included 00 00 00 01.. head flag
	int		len;
}h264_nal;

int  GetNalSize( I_BYTE * lpData,int dataSize, h264_nal *pnal, int maxNal );

#ifdef __cplusplus
}
#endif


#endif