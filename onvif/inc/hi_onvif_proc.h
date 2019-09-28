#ifndef __HI_ONVIF_PROC_H__
#define __HI_ONVIF_PROC_H__

#include "soapStub.h"

#define MANUFACTURER_NAME						"IPNC"//"Octavision"
#define MODEL_NAME								"ONVIF"//"OV-IP20CR81H3A6-D"

#define ONVIF_SERVICES                    		"/onvif/services"

#define ONVIF_DEVICE_SERVICE                    "/onvif/device_service"
#define ONVIF_EVENTS_SERVICE                    "/onvif/events_service"
#define ONVIF_MEDIA_SERVICE                     "/onvif/media_service"
#define ONVIF_IMAGING_SERVICE                   "/onvif/imaging_service"
#define ONVIF_PTZ_SERVICE                       "/onvif/ptz_service"
#define ONVIF_ANALYTICS_SERVICE                 "/onvif/analytics_service"
#define ONVIF_DEVICEIO_SERVICE					"/onvif/deviceio_service"
#define ONVIF_RECORDING_CONTROL_SERVICE 		"/onvif/recordingcontrol_service"
#define ONVIF_RECORDING_SEARCH_SERVICE 			"/onvif/recordingsearch_service"
#define ONVIF_REPLAY_SERVICE 					"/onvif/replay_service"

#define ONVIF_MEDIA_MAIN_PROFILE_NAME			"MainStreamProfile"
#define ONVIF_MEDIA_MAIN_PROFILE_TOKEN			"MainStreamProfileToken"
#define ONVIF_MEDIA_SUB_PROFILE_NAME			"SubStreamProfile"
#define ONVIF_MEDIA_SUB_PROFILE_TOKEN			"SubStreamProfileToken"

/////////////20140619 zj//////////////
#define ONVIF_MEDIA_PRESETTOUR_TOKEN1			"presetTourToken_1"
#define ONVIF_MEDIA_PRESETTOUR_TOKEN2			"presetTourToken_2"
/////////////////END///////////////

#if 1
	#define ONVIF_MEDIA_VIDEO_SOURCE_NAME			"VideoSourceName"
	#define ONVIF_MEDIA_VIDEO_SOURCE_TOKEN			"VideoSourceToken"
#else
	#define ONVIF_MEDIA_VIDEO_MAIN_SOURCE_TOKEN		"VideoMainSourceToken"
	#define ONVIF_MEDIA_VIDEO_MAIN_SOURCE_NAME		"VideoMainSourceName"
	#define ONVIF_MEDIA_VIDEO_SUB_SOURCE_NAME		"VideoSubSourceName"
	#define ONVIF_MEDIA_VIDEO_SUB_SOURCE_TOKEN		"VideoSubSourceToken"
#endif

#define ONVIF_MEDIA_VIDEO_MAIN_CFG_NAME			"VideoSourceConfig"
#define ONVIF_MEDIA_VIDEO_MAIN_CFG_TOKEN		"VideoSourceConfigToken"
#define ONVIF_MEDIA_VIDEO_SUB_CFG_NAME			"VideoSourceConfig"
#define ONVIF_MEDIA_VIDEO_SUB_CFG_TOKEN			"VideoSourceConfigToken"

#define ONVIF_MEDIA_VIDEO_MAIN_ENCODER_NAME		"VideoEncodeConfig_m"
#define ONVIF_MEDIA_VIDEO_MAIN_ENCODER_TOKEN	"VideoEncodeConfigToken_m"
#define ONVIF_MEDIA_VIDEO_SUB_ENCODER_NAME		"VideoEncodeConfig_s"
#define ONVIF_MEDIA_VIDEO_SUB_ENCODER_TOKEN		"VideoEncodeConfigToken_s"

#define ONVIF_MEDIA_AUDIO_SOURCE_NAME			"AudioSourceName"
#define ONVIF_MEDIA_AUDIO_SOURCE_TOKEN			"AudioSourceToken"

#define ONVIF_MEDIA_AUDIO_CFG_NAME				"AudioSourceConfigureName"
#define ONVIF_MEDIA_AUDIO_CFG_TOKEN				"AudioSourceConfigureToken"

#define ONVIF_MEDIA_AUDIO_ENCODER_NAME			"G711"
#define ONVIF_MEDIA_AUDIO_ENCODER_TOKEN			"G711"

#define ONVIF_MEDIA_PTZ_NAME					"PTZName"
#define ONVIF_MEDIA_PTZ_TOKEN					"PTZToken"

#define ONVIF_RELAY_OUTPUT_NAME					"RelayOutputName"
#define ONVIF_RELAY_OUTPUT_TOKEN				"RelayOutputToken"


#define ONVIF_OSD_DATETIME_TOKEN				"OSDDateTimeToken"
#define ONVIF_OSD_CHN_TOKEN						"OSDChnToken"

#define ONVIF_VIDEO_ANALY_CFG_NAME				"VideoAnalyticsConfig"
#define ONVIF_VIDEO_ANALY_CFG_TOKEN				"VideoAnalyticsConfigToken"

#define ONVIF_IOINPUT_TOKEN0					"IOInputToken_0"
#define ONVIF_IOINPUT_TOKEN1					"IOInputToken_1"

#define ONVIF_SCOPES_NUM 		9
#define ONVIF_SCOPES_STR_LEN 	128
#define ONVIF_NAME_LEN			128
#define ONVIF_TOKEN_LEN		128
#define ONVIF_ADDRESS_LEN		128
#define ONVIF_DEVICE_SERVICE_ADDR_SIZE	80
#define ONVIF_PT_SPEED_MAX		63

#define ONVIF_SCOPE_0 "onvif://www.onvif.org/type/NetworkVideoTransmitter"
#define ONVIF_SCOPE_1 "onvif://www.onvif.org/location/sz/China"
#define ONVIF_SCOPE_2 "onvif://www.onvif.org/hardware/IPNC_V2.2"
#define ONVIF_SCOPE_3 "onvif://www.onvif.org/name/IPNC"
#define ONVIF_SCOPE_4 "onvif://www.onvif.org/Profile/Streaming"
#define ONVIF_SCOPE_5 "onvif://www.onvif.org/type/ptz"
#define ONVIF_SCOPE_6 "onvif://www.onvif.org/type/video_encoder"
#define ONVIF_SCOPE_7 "onvif://www.onvif.org/type/audio_encoder"
#define ONVIF_SCOPE_8 "onvif://www.onvif.org/type/video_analytics"



#define ONVIF_PROFILE_NUM			2
#define ONVIF_VIDEO_SOURCE_NUM		1
#define ONVIF_VIDEO_SOURCE_CFG_NUM	2
#define ONVIF_VIDEO_ENCODER_CFG_NUM	2

#define ONVIF_AUDIO_SOURCE_NUM		1
#define ONVIF_AUDIO_SOURCE_CFG_NUM	1
#define ONVIF_AUDIO_ENCODER_CFG_NUM	1

#define ONVIF_PTZ_CFG_NUM			1

#define ONVIF_PTZ_NAME				"PtzName"
#define ONVIF_PTZ_TOKEN				"PtzToken"
#define ONVIF_PTZ_PRESET_NUM	255
#define ONVIF_PTZ_HOME_FIXED	0

#define ONVIF_IMAGING_MAX	255
#define ONVIF_IMAGING_MIN	0

#define TRANSFORM_CHECK_BIT(a, col)		((a) & (0x01<<(DEF_COL - 1 - (col))))
#define TRANSFORM_CLR_BIT(a, col)		((a) &= ~(0x01<<(DEF_COL - 1 - (col))))
#define TRANSFORM_CHECK_ROW(a, mask)	(((a) & (mask)) == (mask))
#define TRANSFORM_CLR_ROW(a, mask)		((a) &= ~(mask))
#define DEF_ROW 18
#define DEF_COL 22
#define ALL_BITS_NUM 396    // col 22 * 18 row

/* WS-Discovery UDP Multicast Address & Port */
#define ONVIF_WSD_MC_ADDR               "239.255.255.250"

typedef enum _HI_ONVIF_NET_CARD_TYPE_S_
{
	HI_CARD_ETH0	= 0,
	HI_CARD_RA0		= 1,
} HI_ONVIF_NET_CARD_TYPE_S;

typedef struct _HI_ONVIF_SCOPE_S_
{
	int isFixed;
	char scope[ONVIF_SCOPES_STR_LEN];
} HI_ONVIF_SCOPE_S;

typedef struct _HI_ONVIF_VIDEO_SOURCE_S_
{
	char* token;	/* required attribute of type tt:ReferenceToken */
	float Framerate;	/* required element of type xsd:float */
	struct tt__VideoResolution
		Resolution;	/* required element of type tt:VideoResolution */
} HI_ONVIF_VIDEO_SOURCE_S;

typedef struct _HI_ONVIF_AUDIO_SOURCE_S_
{
	char* token;	/* required attribute of type tt:ReferenceToken */
	int Channels;	/* required element of type xsd:int */
} HI_ONVIF_AUDIO_SOURCE_S;

typedef struct _HI_ONVIF_PROFILE_S_
{
	char* Name;	/* required element of type tt:Name */
	char* token;	/* required attribute of type tt:ReferenceToken */
	int fixed;			/* optional attribute of type xsd:boolean */
	char streamUri[ONVIF_ADDRESS_LEN];

	int videoSourceCfgIdx;
	int videoEncoderCfgIdx;
	int audioSourceCfgIdx;
	int audioEncoderCfgIdx;
	int ptzCfgIdx;
} HI_ONVIF_PROFILE_S;

typedef struct _HI_ONVIF_VIDEO_SOURCE_CFG_S_
{
	char* Name;	/* required element of type tt:Name */
	int UseCount;	/* required element of type xsd:int */
	char* token;	/* required attribute of type tt:ReferenceToken */
	char* SourceToken;	/* required element of type tt:ReferenceToken */
	struct tt__IntRectangle Bounds;	/* required element of type tt:IntRectangle */
} HI_ONVIF_VIDEO_SOURCE_CFG_S;

typedef struct _HI_ONVIF_AUDIO_SOURCE_CFG_S_
{
	char* Name;	/* required element of type tt:Name */
	int UseCount;	/* required element of type xsd:int */
	char* token;	/* required attribute of type tt:ReferenceToken */
	char* SourceToken;	/* required element of type tt:ReferenceToken */
} HI_ONVIF_AUDIO_SOURCE_CFG_S;

typedef struct _HI_ONVIF_VIDEO_ENCODER_CFG_S_
{
	char* Name;	/* required element of type tt:Name */
	int UseCount;	/* required element of type xsd:int */
	char* token;	/* required attribute of type tt:ReferenceToken */
	enum tt__VideoEncoding Encoding;	/* required element of type tt:VideoEncoding */
	struct tt__VideoResolution
		Resolution;	/* required element of type tt:VideoResolution */
	float Quality;	/* required element of type xsd:float */
	char* SessionTimeout;	/* external */
	int Gop;
	int FrameRate;
	int Bitrate;
	int AdvanceEncType;  //视频编码格式0--h264 1--MJPEG 2--JPEG	3--MPEG4 4--h265
} HI_ONVIF_VIDEO_ENCODER_CFG_S;

typedef struct _HI_ONVIF_AUDIO_ENCODER_CFG_S_
{
	char* Name;	/* required element of type tt:Name */
	int UseCount;	/* required element of type xsd:int */
	char* token;	/* required attribute of type tt:ReferenceToken */
	enum tt__AudioEncoding Encoding;	/* required element of type tt:AudioEncoding */
	int Bitrate;	/* required element of type xsd:int */
	int SampleRate;	/* required element of type xsd:int */
	char* SessionTimeout;	/* external */
} HI_ONVIF_AUDIO_ENCODER_CFG_S;

typedef struct _HI_ONVIF_PTZ_PRESET_CFG_S_
{
	char Name[ONVIF_TOKEN_LEN];	/* required element of type tt:Name */
	char token[ONVIF_TOKEN_LEN];	/* required attribute of type tt:ReferenceToken */
} HI_ONVIF_PTZ_PRESET_CFG_S;

typedef struct _HI_ONVIF_PTZ_CFG_S_
{
	HI_ONVIF_PTZ_PRESET_CFG_S presetCfg[ONVIF_PTZ_PRESET_NUM];
} HI_ONVIF_PTZ_CFG_S;

typedef struct _HI_ONVIF_MEDIA_CFG_S_
{
	struct tt__VideoResolution h264Options[2];
	struct tt__VideoResolution VideoResolutionconfig;
	struct tt__VideoRateControl VideoEncoderRateControl;

} HI_ONVIF_MEDIA_CFG_S;

typedef struct _HI_ONVIF_INFO_S_
{
	int HttpPort;

	HI_ONVIF_PROFILE_S OnvifProfile[ONVIF_PROFILE_NUM];

	HI_ONVIF_VIDEO_SOURCE_S OnvifVideoSource[ONVIF_VIDEO_SOURCE_NUM];
	HI_ONVIF_AUDIO_SOURCE_S OnvifAudioSource[ONVIF_AUDIO_SOURCE_NUM];

	HI_ONVIF_VIDEO_SOURCE_CFG_S
	OnvifVideoSourceCfg[ONVIF_VIDEO_SOURCE_CFG_NUM];	/* optional element of type tt:VideoSourceConfiguration */
	HI_ONVIF_AUDIO_SOURCE_CFG_S
	OnvifAudioSourceCfg[ONVIF_AUDIO_SOURCE_CFG_NUM];	/* optional element of type tt:AudioSourceConfiguration */
	HI_ONVIF_VIDEO_ENCODER_CFG_S
	OnvifVideoEncoderCfg[ONVIF_VIDEO_ENCODER_CFG_NUM];	/* optional element of type tt:VideoEncoderConfiguration */
	HI_ONVIF_AUDIO_ENCODER_CFG_S
	OnvifAudioEncoderCfg[ONVIF_AUDIO_ENCODER_CFG_NUM];	/* optional element of type tt:AudioEncoderConfiguration */
	HI_ONVIF_PTZ_CFG_S OnvifPTZCfg[ONVIF_PTZ_CFG_NUM];



	//struct tt__Profile Profile[ONVIF_PROFILE_NUM];

	//struct tt__VideoSource VideoSource[ONVIF_VIDEO_SOURCE_NUM];
	//struct tt__AudioSource AudioSource[ONVIF_AUDIO_SOURCE_NUM];

	//struct tt__VideoSourceConfiguration VideoSourceConfiguration[ONVIF_VIDEO_SOURCE_CFG_NUM];	/* optional element of type tt:VideoSourceConfiguration */
	//struct tt__AudioSourceConfiguration AudioSourceConfiguration[ONVIF_AUDIO_SOURCE_CFG_NUM];	/* optional element of type tt:AudioSourceConfiguration */
	//struct tt__VideoEncoderConfiguration VideoEncoderConfiguration[ONVIF_VIDEO_ENCODER_CFG_NUM];	/* optional element of type tt:VideoEncoderConfiguration */
	//struct tt__AudioEncoderConfiguration AudioEncoderConfiguration[ONVIF_AUDIO_ENCODER_CFG_NUM];	/* optional element of type tt:AudioEncoderConfiguration */


	/* Service Addresses */
	char device_xaddr [ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char deviceio_xaddr[ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char events_xaddr [ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char media_xaddr [ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char imaging_xaddr [ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char ptz_xaddr [ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char analytics_xaddr [ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char recordingcontrol_xaddr[ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char recordingsearch_xaddr[ONVIF_DEVICE_SERVICE_ADDR_SIZE];
	char replay_xaddr[ONVIF_DEVICE_SERVICE_ADDR_SIZE];

	HI_ONVIF_SCOPE_S type_scope[ONVIF_SCOPES_NUM];
	HI_ONVIF_SCOPE_S name_scope[ONVIF_SCOPES_NUM];
	HI_ONVIF_SCOPE_S location_scope[ONVIF_SCOPES_NUM];
	HI_ONVIF_SCOPE_S hardware_scope[ONVIF_SCOPES_NUM];

	HI_ONVIF_MEDIA_CFG_S MediaConfig;

} HI_ONVIF_INFO_S;

extern HI_ONVIF_INFO_S gOnvifInfo;

int onvif_fheader(struct soap* soap);

char* onvif_get_services_xaddr(int nIpAddr);

char* onvif_get_device_xaddr(int nIpAddr);


unsigned int onvif_get_ipaddr(struct soap* soap);

unsigned long onvif_ip_n2a(unsigned long ip, char* ourIp, int len);

int onvif_get_mac(int nIpAddr, char macaddr[]);

char* onvif_get_hwid(int nIpAddr);

int onvif_get_scopes(char* str, int buflen);

int onvif_probe_match_scopes(char* item);

int onvif_get_profile_from_token(int nIpAddr, char* Token,
                                 HI_ONVIF_PROFILE_S* pOnvifProfile);
int onvif_get_video_source_from_token(char* Token,
                                      HI_ONVIF_VIDEO_SOURCE_S* pVideoSource);
int onvif_get_audio_source_from_token(char* Token,
                                      HI_ONVIF_AUDIO_SOURCE_S* pAudioSource);
int onvif_get_video_source_cfg_from_token(char* Token,
        HI_ONVIF_VIDEO_SOURCE_CFG_S* pVideoSourceCfg);
int onvif_get_video_encoder_cfg_from_token(char* Token,
        HI_ONVIF_VIDEO_ENCODER_CFG_S* pVideoEncoderCfg);
int onvif_get_audio_source_cfg_from_token(char* Token,
        HI_ONVIF_AUDIO_SOURCE_CFG_S* pAudioSourceCfg);
int onvif_get_audio_encoder_cfg_from_token(char* Token,
        HI_ONVIF_AUDIO_ENCODER_CFG_S* pAudioEncoderCfg);

int onvif_get_profile(int idx, int nIpAddr, HI_ONVIF_PROFILE_S* pOnvifProfile);
int onvif_get_video_source(int idx, HI_ONVIF_VIDEO_SOURCE_S* pVideoSource);
int onvif_get_audio_source(int idx, HI_ONVIF_AUDIO_SOURCE_S* pAudioSource);
int onvif_get_video_source_cfg(int idx,
                               HI_ONVIF_VIDEO_SOURCE_CFG_S* pVideoSourceCfg);
int onvif_get_video_encoder_cfg(int idx,
                                HI_ONVIF_VIDEO_ENCODER_CFG_S* pVideoEncoderCfg);
int onvif_get_audio_source_cfg(int idx,
                               HI_ONVIF_AUDIO_SOURCE_CFG_S* pAudioSourceCfg);
int onvif_get_audio_encoder_cfg(int idx,
                                HI_ONVIF_AUDIO_ENCODER_CFG_S* pAudioEncoderCfg);
int onvif_get_ptz_cfg(int idx, HI_ONVIF_PTZ_CFG_S* pPtzCfg);
int onvif_set_ptz_cfg(int idx, HI_ONVIF_PTZ_CFG_S* pPtzCfg);
int onvif_get_streamUri(char* uri, int streamType);

int onvif_server_init(int nHttpPort);

int onvif_check_security(struct soap* soap);
int onvif_clear_wsse_head(struct soap* soap);
int onvif_check_digest_security(struct soap* soap);

int onvif_net_get_media_url(int channel, char* rtsp_url            );




#endif

