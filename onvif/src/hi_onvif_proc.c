//#include "hi_platform.h"

//#if (HI_SUPPORT_PLATFORM && PLATFORM_ONVIF)

#include "hi_net_api.h"
#include "hi_platform_onvif.h"
#include "hi_onvif.h"
#include "hi_onvif_proc.h"
#include "hi_onvif_subscribe.h"
#include "hi_character_encode.h"
#include "hi_param_interface.h"
#include "openssl/sha.h"


#include "wsseapi.h"
#include "wsaapi.h"


#ifdef USE_DIGEST_AUTH
	#include "httpda.h"
#endif



HI_ONVIF_INFO_S gOnvifInfo;


#define SOAP_UNAUTH_ERROR			401 //SOAP_USER_ERROR 此返回值通常用于digest方式

#ifdef USE_DIGEST_AUTH
	static char authrealm[] = "gufkuiopipiopupupup";
#endif


int onvif_check_security(struct soap* soap)
{
	struct _wsse__Security* wsse__Security;
	struct _wsse__UsernameToken* UsernameToken;
	HI_DEV_USER_CFG_S stUserCfg;
	char nonce[256];
	char out[256];
	int noncelen = 0;
	int i;
	int orgin_admin_pass_changed = 1;

	//用户admin的密码还是admin时，不做密码校验。
	for(i = 0; i < HI_MAX_USR_NUM; i++)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, i,
		                     (char*)&stUserCfg, sizeof(HI_DEV_USER_CFG_S));

		if( (strcmp(stUserCfg.szUsrName, "admin") == 0)  &&
		        (strcmp(stUserCfg.szPsw, "admin") == 0) )
		{
			orgin_admin_pass_changed = 0;
			break;
		}
	}

	if( orgin_admin_pass_changed == 0)
	{
		onvif_clear_wsse_head(soap);
		return SOAP_OK;
	}

#ifndef USE_WSSE_AUTH
	onvif_clear_wsse_head(
	    soap);   //如果不清除wsse加密头的化，加密头将会返回客户端。造成客户端不识别的问题。
#endif
#ifdef USE_WSSE_AUTH
	const char* username = soap_wsse_get_Username(soap);
	const char* password;

	if(username == NULL)
	{
#ifdef USE_DIGEST_AUTH
		int ret;
		ret = onvif_check_digest_security(soap);
		return ret;
#else
		return SOAP_UNAUTH_ERROR;
#endif
	}

	if (!username)
	{
		soap_wsse_delete_Security(soap); // remove old security headers before returning!
		return soap->error; // no username: return FailedAuthentication (from soap_wsse_get_Username)
		//return SOAP_UNAUTH_ERROR;
	}

	for(i = 0; i < HI_MAX_USR_NUM; i++)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, i,
		                     (char*)&stUserCfg, sizeof(HI_DEV_USER_CFG_S));

		if(strncmp(stUserCfg.szUsrName, username,
		           sizeof(stUserCfg.szUsrName)) == 0)
		{
			break;
		}
	}

	password = stUserCfg.szPsw; // lookup password of the username provided
	//printf("password %s!\n", password);

	if(soap_wsse_verify_Password(soap, password))
	{
		soap_wsse_delete_Security(soap); // remove old security headers before returning!
		return soap->error; // no username: return FailedAuthentication (from soap_wsse_verify_Password)
		//return SOAP_UNAUTH_ERROR;
	}

	soap_wsse_delete_Security(soap);
#endif
	return SOAP_OK;

	if(soap->header == NULL)
	{
		return SOAP_UNAUTH_ERROR;
	}

	wsse__Security = soap->header->wsse__Security;

	if(wsse__Security == NULL)
	{
		return SOAP_UNAUTH_ERROR;
	}

	UsernameToken = wsse__Security->UsernameToken;

	if(UsernameToken == NULL)
	{
		return SOAP_UNAUTH_ERROR;
	}

	if(UsernameToken->Username == NULL || UsernameToken->Password == NULL)
	{
		return SOAP_UNAUTH_ERROR;
	}

	if(UsernameToken->Password->__item == NULL)
	{
		return SOAP_UNAUTH_ERROR;
	}

	if(UsernameToken->Nonce == NULL)
	{
		return SOAP_UNAUTH_ERROR;
	}

	if(UsernameToken->wsu__Created == NULL)
	{
		return SOAP_UNAUTH_ERROR;
	}

	hi_base64_decode((unsigned char*)UsernameToken->Nonce,
	                 strlen(UsernameToken->Nonce), nonce, &noncelen);
	memcpy(nonce + noncelen, UsernameToken->wsu__Created,
	       strlen(UsernameToken->wsu__Created));
	noncelen += strlen(UsernameToken->wsu__Created);

	for(i = 0; i < HI_MAX_USR_NUM; i++)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, i,
		                     (char*)&stUserCfg, sizeof(HI_DEV_USER_CFG_S));

		if(strncmp(stUserCfg.szUsrName, UsernameToken->Username,
		           sizeof(stUserCfg.szUsrName)) == 0)
		{
			break;
		}
	}

	if(i == HI_MAX_USR_NUM)
	{
		return SOAP_UNAUTH_ERROR;
	}

	memcpy(nonce + noncelen, stUserCfg.szPsw, strlen(stUserCfg.szPsw));
	noncelen += strlen(stUserCfg.szPsw);
	{
		SHA_CTX c;
		SHA1_Init(&c);
		SHA1_Update(&c, nonce, noncelen);
		SHA1_Final((unsigned char*)out, &c);
		memset(nonce, 0, sizeof(nonce));
		hi_base64_encode((unsigned char*)out, 20, nonce, &noncelen);

		if(strncmp(nonce, UsernameToken->Password->__item, noncelen) != 0)
		{
			return SOAP_UNAUTH_ERROR;
		}
	}
	return SOAP_OK;
}





int onvif_clear_wsse_head(struct soap* soap)
{
	//#ifdef USE_WSSE_AUTH
	const char* username = soap_wsse_get_Username(soap);

	if(username == NULL)
	{
		return SOAP_OK;
	}

	soap_wsse_delete_Security(soap);
	//#endif
	return SOAP_OK;
}



#ifdef USE_DIGEST_AUTH

int onvif_check_digest_security(struct soap* soap)
{
	char szUsrName[] = "admin";
	char szPsw[] = "admin";
	//printf("user1 %s %s\n", szUsrName, szPsw);
	//printf("user2 %s %s\n", soap->userid, soap->passwd);
	//printf("authrealm %s\n", soap->authrealm);

	if (soap->userid &&
	        soap->passwd) /* Basic authentication: we may want to reject this since the password was sent in the clear */
	{
		if (!strcmp(soap->userid, szUsrName)
		        && !strcmp(soap->passwd, szPsw))
		{
			return SOAP_OK;
		}
	}
	else if (soap->authrealm && soap->userid)
	{
		/* simulate database lookup on userid to find passwd */
		if (!strcmp(soap->authrealm, authrealm) && !strcmp(soap->userid, szUsrName))
		{
			char* passwd = szPsw;
			//printf("here1\n");

			if (!http_da_verify_post(soap, passwd))
			{
				return SOAP_OK;
			}
		}
	}

	soap->authrealm = authrealm;
	return 401; /* Not authorized, challenge digest authentication with httpda plugin */
}
#endif


int onvif_check_digest_security2(struct soap* soap)
{
#ifdef USE_DIGEST_AUTH
	int i;
	HI_DEV_USER_CFG_S stUserCfg;
	memset(&stUserCfg, 0x00, sizeof(stUserCfg));

	if( soap->userid )
	{
		for(i = 0; i < HI_MAX_USR_NUM; i++)
		{
			gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, i,
			                     (char*)&stUserCfg, sizeof(HI_DEV_USER_CFG_S));

			if(strncmp(stUserCfg.szUsrName, soap->userid,
			           sizeof(stUserCfg.szUsrName)) == 0)
			{
				break;
			}
		}
	}

	printf("user1 %s %s\n", stUserCfg.szUsrName, stUserCfg.szPsw);
	printf("user2 %s %s\n", soap->userid, soap->passwd);
	printf("authrealm %s\n", soap->authrealm);

	if (soap->userid &&
	        soap->passwd) /* Basic authentication: we may want to reject this since the password was sent in the clear */
	{
		if (!strcmp(soap->userid, stUserCfg.szUsrName)
		        && !strcmp(soap->passwd, stUserCfg.szPsw))
		{
			return SOAP_OK;
		}
	}
	else if (soap->authrealm && soap->userid)
	{
		/* simulate database lookup on userid to find passwd */
		if (!strcmp(soap->authrealm, authrealm) && !strcmp(soap->userid, stUserCfg.szUsrName))
		{
			char* passwd = stUserCfg.szPsw;
			printf("here1\n");

			if (!http_da_verify_post(soap, passwd))
			{
				return SOAP_OK;
			}
		}
	}

	soap->authrealm = authrealm;
	return 401; /* Not authorized, challenge digest authentication with httpda plugin */
#endif
	return SOAP_OK;
}



int onvif_fheader(struct soap* soap)
{
	return SOAP_OK;
}

char* onvif_get_services_xaddr(int nIpAddr)
{
	char ipAddr[64];
	hi_ip_n2a(nIpAddr, ipAddr, 64);
	sprintf (gOnvifInfo.device_xaddr, "http://%s:%d%s",
	         ipAddr, gOnvifInfo.HttpPort, ONVIF_SERVICES);
	return gOnvifInfo.device_xaddr;
}


char* onvif_get_device_xaddr(int nIpAddr)
{
	char ipAddr[64];
	hi_ip_n2a(nIpAddr, ipAddr, 64);
	sprintf (gOnvifInfo.device_xaddr,  "http://%s:%d%s",
	         ipAddr, gOnvifInfo.HttpPort, ONVIF_DEVICE_SERVICE);
	return gOnvifInfo.device_xaddr;
}

unsigned int onvif_get_ipaddr(struct soap* soap)
{
	struct      sockaddr_in serv;
	socklen_t 	serv_len = sizeof(serv);
	getsockname(soap->socket, (struct sockaddr*)&serv, &serv_len);
	return htonl(serv.sin_addr.s_addr);
}

int onvif_get_mac(int nIpAddr, char macaddr[])
{
	HI_DEV_NET_CFG_S stNetCfg;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));

	if(nIpAddr == stNetCfg.struEtherCfg[0].u32IpAddr)
	{
		memcpy(macaddr, stNetCfg.struEtherCfg[0].u8MacAddr, 6);
	}
	else if(nIpAddr == stNetCfg.struEtherCfg[1].u32IpAddr)
	{
		memcpy(macaddr, stNetCfg.struEtherCfg[1].u8MacAddr, 6);
	}

	return 0;
}

unsigned long onvif_ip_n2a(unsigned long ip, char* ourIp, int len)
{
	return hi_ip_n2a(ip, ourIp, len);
}

char* onvif_get_hwid(int nIpAddr)
{
	static char hwid[128];
	char macaddr[6];
	onvif_get_mac(nIpAddr, macaddr);
	sprintf(hwid, "urn:uuid:1419d68a-1dd2-11b2-a105-%02X%02X%02X%02X%02X%02X",
	        macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	return hwid;
}

//////////////////////////////////////////////////
static int onvif_add_scope(HI_ONVIF_SCOPE_S* scopes, char* item)
{
	int i;

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(scopes[i].scope[0] == 0)
		{
			scopes[i].isFixed = 1;
			strcpy(scopes[i].scope, item);
			return 0;
		}
	}

	return -1;
}


////////////////////////////////////////////////////
int onvif_get_scopes(char* str, int buflen)
{
	int i, len = 0;

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(gOnvifInfo.type_scope[i].scope[0])
		{
			len += strlen(gOnvifInfo.type_scope[i].scope);
		}

		if(gOnvifInfo.name_scope[i].scope[0])
		{
			len += strlen(gOnvifInfo.type_scope[i].scope);
		}

		if(gOnvifInfo.location_scope[i].scope[0])
		{
			len += strlen(gOnvifInfo.type_scope[i].scope);
		}

		if(gOnvifInfo.hardware_scope[i].scope[0])
		{
			len += strlen(gOnvifInfo.type_scope[i].scope);
		}
	}

	if((len + i) > buflen)
	{
		return -1;
	}

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(gOnvifInfo.type_scope[i].scope[0])
		{
			strcat(str, gOnvifInfo.type_scope[i].scope);
			strcat(str, " ");
		}
	}

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(gOnvifInfo.location_scope[i].scope[0])
		{
			strcat(str, gOnvifInfo.location_scope[i].scope);
			strcat(str, " ");
		}
	}

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(gOnvifInfo.hardware_scope[i].scope[0])
		{
			strcat(str, gOnvifInfo.hardware_scope[i].scope);
			strcat(str, " ");
		}
	}

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(gOnvifInfo.name_scope[i].scope[0])
		{
			strcat(str, gOnvifInfo.name_scope[i].scope);
			strcat(str, " ");
		}
	}

	return 0;
}
/////////////////////////////////////////
int onvif_probe_match_scopes(char* item)
{
#if 0
	char* pStrH, *pStrN;
	int len, TypeLen, LocatLen, hwLen, nameLen;
	int i;

	if(item == NULL)
	{
		printf("The item is null.\n");
		return 0;
	}

	printf("item: %s\n", item);

	if(item[0] == 0x0)
	{
		return 0;
	}

	printf("item: %s\n", item);
	TypeLen = strlen("onvif://www.onvif.org/type");
	LocatLen = strlen("onvif://www.onvif.org/location");
	hwLen = strlen("onvif://www.onvif.org/hardware");
	nameLen = strlen("onvif://www.onvif.org/name");
	pStrH = item;

	while(1)
	{
		pStrN = strchr(pStrH, ' ');

		if(pStrN == NULL)
		{
			len = strlen(pStrH);
		}
		else
		{
			len = pStrN - pStrH;
		}

		if(!strncmp(pStrH, "onvif://www.onvif.org/type", TypeLen))
		{
			if(TypeLen == len)
			{
				return 0;
			}
			else
			{
				for(i = 0; i < ONVIF_SCOPES_NUM; i++)
				{
					if(!gOnvifInfo.type_scope[i].scope[0])
					{
						continue;
					}

					if(!strncmp(pStrH, gOnvifInfo.type_scope[i].scope, len))
					{
						return 0;
					}
				}
			}
		}
		else if(!strncmp(pStrH, "onvif://www.onvif.org/location", LocatLen))
		{
			if(TypeLen == len)
			{
				return 0;
			}
			else
			{
				for(i = 0; i < ONVIF_SCOPES_NUM; i++)
				{
					if(!gOnvifInfo.location_scope[i].scope[0])
					{
						continue;
					}

					if(!strncmp(pStrH, gOnvifInfo.location_scope[i].scope, len))
					{
						return 0;
					}
				}
			}
		}
		else if(!strncmp(pStrH, "onvif://www.onvif.org/hardware", hwLen))
		{
			if(TypeLen == len)
			{
				return 0;
			}
			else
			{
				for(i = 0; i < ONVIF_SCOPES_NUM; i++)
				{
					if(!gOnvifInfo.hardware_scope[i].scope[0])
					{
						continue;
					}

					if(!strncmp(pStrH, gOnvifInfo.hardware_scope[i].scope, len))
					{
						return 0;
					}
				}
			}
		}
		else if(!strncmp(pStrH, "onvif://www.onvif.org/name", nameLen))
		{
			if(TypeLen == len)
			{
				return 0;
			}
			else
			{
				for(i = 0; i < ONVIF_SCOPES_NUM; i++)
				{
					if(!gOnvifInfo.name_scope[i].scope[0])
					{
						continue;
					}

					if(!strncmp(pStrH, gOnvifInfo.name_scope[i].scope, len))
					{
						return 0;
					}
				}
			}
		}

		if(pStrN == NULL)
		{
			break;
		}

		pStrH = pStrN + 1;
	}

#endif
	return -1;
}

int onvif_get_profile_from_token(int nIpAddr, char* Token,
                                 HI_ONVIF_PROFILE_S* pOnvifProfile)
{
	int i;

	if(Token == NULL)
	{
		__E("The token is null.\n");
		return -1;
	}

	for(i = 0; i < ONVIF_PROFILE_NUM; i++)
	{
		if(strcmp(gOnvifInfo.OnvifProfile[i].token, Token) == 0)
		{
			return onvif_get_profile(i, nIpAddr, pOnvifProfile);
		}
	}

	return -1;
}

int onvif_get_video_source_from_token(char* Token,
                                      HI_ONVIF_VIDEO_SOURCE_S* pVideoSource)
{
	int i;

	if(Token == NULL)
	{
		__E("The token is null.\n");
		return -1;
	}

	for(i = 0; i < ONVIF_VIDEO_SOURCE_NUM; i++)
	{
		if(strcmp(gOnvifInfo.OnvifVideoSource[i].token, Token) == 0)
		{
			return onvif_get_video_source(i, pVideoSource);
		}
	}

	return -1;
}

int onvif_get_audio_source_from_token(char* Token,
                                      HI_ONVIF_AUDIO_SOURCE_S* pAudioSource)
{
	int i;

	if(Token == NULL)
	{
		__E("The token is null.\n");
		return -1;
	}

	for(i = 0; i < ONVIF_AUDIO_SOURCE_NUM; i++)
	{
		if(strcmp(gOnvifInfo.OnvifAudioSource[i].token, Token) == 0)
		{
			return onvif_get_audio_source(i, pAudioSource);
		}
	}

	return -1;
}

int onvif_get_video_source_cfg_from_token(char* Token,
        HI_ONVIF_VIDEO_SOURCE_CFG_S* pVideoSourceCfg)
{
	int i;

	if(Token == NULL)
	{
		__E("The token is null.\n");
		return -1;
	}

	for(i = 0; i < ONVIF_VIDEO_SOURCE_CFG_NUM; i++)
	{
		if(strcmp(gOnvifInfo.OnvifVideoSourceCfg[i].token, Token) == 0)
		{
			return onvif_get_video_source_cfg(i, pVideoSourceCfg);
		}
	}

	return onvif_get_video_source_cfg(0, pVideoSourceCfg);
}

int onvif_get_video_encoder_cfg_from_token(char* Token,
        HI_ONVIF_VIDEO_ENCODER_CFG_S* pVideoEncoderCfg)
{
	int i;

	if(Token == NULL)
	{
		__E("The token is null.\n");
		return -1;
	}

	for(i = 0; i < ONVIF_VIDEO_ENCODER_CFG_NUM; i++)
	{
		if(strcmp(gOnvifInfo.OnvifVideoEncoderCfg[i].token, Token) == 0)
		{
			return onvif_get_video_encoder_cfg(i, pVideoEncoderCfg);
		}
	}

	return -1;
}

int onvif_get_audio_source_cfg_from_token(char* Token,
        HI_ONVIF_AUDIO_SOURCE_CFG_S* pAudioSourceCfg)
{
	int i;

	if(Token == NULL)
	{
		__E("The token is null.\n");
		return -1;
	}

	for(i = 0; i < ONVIF_AUDIO_SOURCE_CFG_NUM; i++)
	{
		if(strcmp(gOnvifInfo.OnvifAudioSourceCfg[i].token, Token) == 0)
		{
			return onvif_get_audio_source_cfg(0, pAudioSourceCfg);
		}
	}

	return -1;
}

int onvif_get_audio_encoder_cfg_from_token(char* Token,
        HI_ONVIF_AUDIO_ENCODER_CFG_S* pAudioEncoderCfg)
{
	int i;

	if(Token == NULL)
	{
		__E("The token is null.\n");
		return -1;
	}

	for(i = 0; i < ONVIF_AUDIO_ENCODER_CFG_NUM; i++)
	{
		if(strcmp(gOnvifInfo.OnvifAudioEncoderCfg[i].token, Token) == 0)
		{
			return onvif_get_audio_encoder_cfg(0, pAudioEncoderCfg);
		}
	}

	return -1;
}

int onvif_get_profile(int idx, int nIpAddr, HI_ONVIF_PROFILE_S* pOnvifProfile)
{
	if(idx >= ONVIF_PROFILE_NUM)
	{
		return -1;
	}

#if 1
	int tem = idx;

	if(idx == 1)
	{
		tem = 2;
	}

	snprintf(gOnvifInfo.OnvifProfile[idx].streamUri,
	         sizeof(gOnvifInfo.OnvifProfile[idx].streamUri),
	         "rtsp://%d.%d.%d.%d/%d",
	         (nIpAddr >> 24) & 0xff,
	         (nIpAddr >> 16) & 0xff,
	         (nIpAddr >> 8) & 0xff,
	         (nIpAddr >> 0) & 0xff,
	         tem);
#else
#endif
	memcpy(pOnvifProfile, &gOnvifInfo.OnvifProfile[idx],
	       sizeof(HI_ONVIF_PROFILE_S));
	return 0;
}
int onvif_get_video_source(int idx, HI_ONVIF_VIDEO_SOURCE_S* pVideoSource)
{
	HI_DEV_VENC_CFG_S stVencCfg;

	if(idx >= ONVIF_VIDEO_SOURCE_NUM)
	{
		return -1;
	}

	if(idx == 0)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMAIN_PARAM, 0,
		                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	}
	else
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VSUB_PARAM, 0,
		                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	}

	gOnvifInfo.OnvifVideoSource[idx].Resolution.Width = stVencCfg.u16Width;
	gOnvifInfo.OnvifVideoSource[idx].Resolution.Height = stVencCfg.u16Height;
	gOnvifInfo.OnvifVideoSource[idx].Framerate = stVencCfg.u8FrameRate;
	gOnvifInfo.OnvifVideoSource[idx].token = ONVIF_MEDIA_VIDEO_SOURCE_TOKEN;
	memcpy(pVideoSource, &gOnvifInfo.OnvifVideoSource[idx],
	       sizeof(HI_ONVIF_VIDEO_SOURCE_S));
	return 0;
}
int onvif_get_audio_source(int idx, HI_ONVIF_AUDIO_SOURCE_S* pAudioSource)
{
	if(idx >= ONVIF_AUDIO_SOURCE_NUM)
	{
		return -1;
	}

	memcpy(pAudioSource, &gOnvifInfo.OnvifAudioSource[idx],
	       sizeof(HI_ONVIF_AUDIO_SOURCE_S));
	return 0;
}
int onvif_get_video_source_cfg(int idx,
                               HI_ONVIF_VIDEO_SOURCE_CFG_S* pVideoSourceCfg)
{
	if(idx >= ONVIF_VIDEO_SOURCE_CFG_NUM)
	{
		return -1;
	}

	memcpy(pVideoSourceCfg, &gOnvifInfo.OnvifVideoSourceCfg[idx],
	       sizeof(HI_ONVIF_VIDEO_SOURCE_CFG_S));
	return 0;
}
int onvif_get_video_encoder_cfg(int idx,
                                HI_ONVIF_VIDEO_ENCODER_CFG_S* pVideoEncoderCfg)
{
	HI_DEV_VENC_CFG_S stVencCfg;

	if(idx >= ONVIF_VIDEO_ENCODER_CFG_NUM)
	{
		return -1;
	}

	if(idx == 0)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMAIN_PARAM, 0,
		                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
		gOnvifInfo.OnvifVideoEncoderCfg[idx].Quality = stVencCfg.u8PicQuilty;
		gOnvifInfo.OnvifVideoEncoderCfg[idx].Gop = stVencCfg.u8Gop;
		gOnvifInfo.OnvifVideoEncoderCfg[idx].FrameRate = stVencCfg.u8FrameRate;
		gOnvifInfo.OnvifVideoEncoderCfg[idx].Bitrate = stVencCfg.u32BitrateRate;
	}
	else
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VSUB_PARAM, 0,
		                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
		gOnvifInfo.OnvifVideoEncoderCfg[idx].Quality = stVencCfg.u8PicQuilty;
		gOnvifInfo.OnvifVideoEncoderCfg[idx].Gop = stVencCfg.u8Gop;
		gOnvifInfo.OnvifVideoEncoderCfg[idx].FrameRate = stVencCfg.u8FrameRate;
		gOnvifInfo.OnvifVideoEncoderCfg[idx].Bitrate = stVencCfg.u32BitrateRate;
	}

	gOnvifInfo.OnvifVideoEncoderCfg[idx].AdvanceEncType = stVencCfg.u8VideoEncType;
	memcpy(pVideoEncoderCfg, &gOnvifInfo.OnvifVideoEncoderCfg[idx],
	       sizeof(HI_ONVIF_VIDEO_ENCODER_CFG_S));
	return 0;
}
int onvif_get_audio_source_cfg(int idx,
                               HI_ONVIF_AUDIO_SOURCE_CFG_S* pAudioSourceCfg)
{
	if(idx >= ONVIF_AUDIO_SOURCE_CFG_NUM)
	{
		return -1;
	}

	memcpy(pAudioSourceCfg, &gOnvifInfo.OnvifAudioSourceCfg[idx],
	       sizeof(HI_ONVIF_AUDIO_SOURCE_CFG_S));
	return 0;
}
int onvif_get_audio_encoder_cfg(int idx,
                                HI_ONVIF_AUDIO_ENCODER_CFG_S* pAudioEncoderCfg)
{
	if(idx >= ONVIF_AUDIO_ENCODER_CFG_NUM)
	{
		return -1;
	}

	memcpy(pAudioEncoderCfg, &gOnvifInfo.OnvifAudioEncoderCfg[idx],
	       sizeof(HI_ONVIF_AUDIO_ENCODER_CFG_S));
	return 0;
}
int onvif_get_ptz_cfg(int idx, HI_ONVIF_PTZ_CFG_S* pPtzCfg)
{
	if(idx >= ONVIF_PTZ_CFG_NUM)
	{
		return -1;
	}

	memcpy(pPtzCfg, &gOnvifInfo.OnvifPTZCfg[idx], sizeof(HI_ONVIF_PTZ_CFG_S));
	return 0;
}
int onvif_set_ptz_cfg(int idx, HI_ONVIF_PTZ_CFG_S* pPtzCfg)
{
	if(idx >= ONVIF_PTZ_CFG_NUM)
	{
		return -1;
	}

	memcpy(&gOnvifInfo.OnvifPTZCfg[idx], pPtzCfg, sizeof(HI_ONVIF_PTZ_CFG_S));
	return 0;
}

static int onvif_scope_init()
{
	onvif_add_scope(gOnvifInfo.type_scope, ONVIF_SCOPE_0);
	onvif_add_scope(gOnvifInfo.type_scope, ONVIF_SCOPE_4);
	onvif_add_scope(gOnvifInfo.type_scope, ONVIF_SCOPE_5);
	onvif_add_scope(gOnvifInfo.type_scope, ONVIF_SCOPE_6);
	onvif_add_scope(gOnvifInfo.type_scope, ONVIF_SCOPE_7);
	onvif_add_scope(gOnvifInfo.type_scope, ONVIF_SCOPE_8);
	onvif_add_scope(gOnvifInfo.location_scope, ONVIF_SCOPE_1);
	onvif_add_scope(gOnvifInfo.hardware_scope, ONVIF_SCOPE_2);
	onvif_add_scope(gOnvifInfo.name_scope, ONVIF_SCOPE_3);
	return 0;
}


static int onvif_profile_init()
{
	// main profile initialize.
	gOnvifInfo.OnvifProfile[0].Name = ONVIF_MEDIA_MAIN_PROFILE_NAME;
	gOnvifInfo.OnvifProfile[0].token = ONVIF_MEDIA_MAIN_PROFILE_TOKEN;
	gOnvifInfo.OnvifProfile[0].fixed = 1;
	gOnvifInfo.OnvifProfile[0].videoSourceCfgIdx = 0;
	gOnvifInfo.OnvifProfile[0].videoEncoderCfgIdx = 0;
	gOnvifInfo.OnvifProfile[0].audioSourceCfgIdx = 0;
	gOnvifInfo.OnvifProfile[0].audioEncoderCfgIdx = 0;
	gOnvifInfo.OnvifProfile[0].ptzCfgIdx = 0;
	// sub profile initialize.
	gOnvifInfo.OnvifProfile[1].Name = ONVIF_MEDIA_SUB_PROFILE_NAME;
	gOnvifInfo.OnvifProfile[1].token = ONVIF_MEDIA_SUB_PROFILE_TOKEN;
	gOnvifInfo.OnvifProfile[1].fixed = 1;
	gOnvifInfo.OnvifProfile[1].videoSourceCfgIdx = 0;
	gOnvifInfo.OnvifProfile[1].videoEncoderCfgIdx = 1;
	gOnvifInfo.OnvifProfile[1].audioSourceCfgIdx = 0;
	gOnvifInfo.OnvifProfile[1].audioEncoderCfgIdx = 0;
	gOnvifInfo.OnvifProfile[1].ptzCfgIdx = 0;
	return 0;
}

static int onvif_video_source_init()
{
	HI_DEV_VENC_CFG_S stVencCfg;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMAIN_PARAM, 0,
	                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	gOnvifInfo.OnvifVideoSource[0].token = ONVIF_MEDIA_VIDEO_SOURCE_TOKEN;
	gOnvifInfo.OnvifVideoSource[0].Framerate = stVencCfg.u8FrameRate;
	gOnvifInfo.OnvifVideoSource[0].Resolution.Width = stVencCfg.u16Width;
	gOnvifInfo.OnvifVideoSource[0].Resolution.Height = stVencCfg.u16Width;
#if 0
	hi_platform_get_sub_venc_cfg(0, &stVencCfg);
	gOnvifInfo.OnvifVideoSource[1].token = ONVIF_MEDIA_VIDEO_SUB_SOURCE_TOKEN;
	gOnvifInfo.OnvifVideoSource[1].Framerate = stVencCfg.u8FrameRate;
	gOnvifInfo.OnvifVideoSource[1].Resolution.Width = stVencCfg.u16Width;
	gOnvifInfo.OnvifVideoSource[1].Resolution.Height = stVencCfg.u16Width;
#endif
	return 0;
}

static int onvif_audio_source_init()
{
	gOnvifInfo.OnvifAudioSource[0].token = ONVIF_MEDIA_AUDIO_SOURCE_TOKEN;
	gOnvifInfo.OnvifAudioSource[0].Channels = 1;
	return 0;
}

static int onvif_video_source_cfg_init()
{
	HI_DEV_VENC_CFG_S stVencCfg;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMAIN_PARAM, 0,
	                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	gOnvifInfo.OnvifVideoSourceCfg[0].Name =
	    ONVIF_MEDIA_VIDEO_SOURCE_TOKEN;//ONVIF_MEDIA_VIDEO_MAIN_CFG_NAME;
	gOnvifInfo.OnvifVideoSourceCfg[0].token =
	    ONVIF_MEDIA_VIDEO_MAIN_CFG_TOKEN;//ONVIF_MEDIA_VIDEO_MAIN_CFG_TOKEN;
	gOnvifInfo.OnvifVideoSourceCfg[0].UseCount = 1;
	gOnvifInfo.OnvifVideoSourceCfg[0].SourceToken = ONVIF_MEDIA_VIDEO_SOURCE_TOKEN;
	gOnvifInfo.OnvifVideoSourceCfg[0].Bounds.x = 0;
	gOnvifInfo.OnvifVideoSourceCfg[0].Bounds.y = 0;
	gOnvifInfo.OnvifVideoSourceCfg[0].Bounds.width = stVencCfg.u16Width;
	gOnvifInfo.OnvifVideoSourceCfg[0].Bounds.height = stVencCfg.u16Height;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VSUB_PARAM, 0,
	                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	gOnvifInfo.OnvifVideoSourceCfg[1].Name =
	    ONVIF_MEDIA_VIDEO_SOURCE_TOKEN;//ONVIF_MEDIA_VIDEO_SUB_CFG_NAME;
	gOnvifInfo.OnvifVideoSourceCfg[1].token =
	    ONVIF_MEDIA_VIDEO_MAIN_CFG_TOKEN;//ONVIF_MEDIA_VIDEO_SUB_CFG_TOKEN;
	gOnvifInfo.OnvifVideoSourceCfg[1].UseCount = 1;
	gOnvifInfo.OnvifVideoSourceCfg[1].SourceToken = ONVIF_MEDIA_VIDEO_SOURCE_TOKEN;
	gOnvifInfo.OnvifVideoSourceCfg[1].Bounds.x = 0;
	gOnvifInfo.OnvifVideoSourceCfg[1].Bounds.y = 0;
	gOnvifInfo.OnvifVideoSourceCfg[1].Bounds.width = stVencCfg.u16Width;
	gOnvifInfo.OnvifVideoSourceCfg[1].Bounds.height = stVencCfg.u16Height;
	return 0;
}

static int onvif_video_encoder_cfg_init()
{
	HI_DEV_VENC_CFG_S stVencCfg;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMAIN_PARAM, 0,
	                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	gOnvifInfo.OnvifVideoEncoderCfg[0].Name = ONVIF_MEDIA_VIDEO_MAIN_ENCODER_NAME;
	gOnvifInfo.OnvifVideoEncoderCfg[0].token = ONVIF_MEDIA_VIDEO_MAIN_ENCODER_TOKEN;
	gOnvifInfo.OnvifVideoEncoderCfg[0].UseCount = 1;
	gOnvifInfo.OnvifVideoEncoderCfg[0].Encoding = tt__VideoEncoding__H264;
	gOnvifInfo.OnvifVideoEncoderCfg[0].Resolution.Width = stVencCfg.u16Width;
	gOnvifInfo.OnvifVideoEncoderCfg[0].Resolution.Height = stVencCfg.u16Height;
	gOnvifInfo.OnvifVideoEncoderCfg[0].Quality = stVencCfg.u8PicQuilty;
	gOnvifInfo.OnvifVideoEncoderCfg[0].SessionTimeout = "PT5S";
	gOnvifInfo.OnvifVideoEncoderCfg[0].Gop = stVencCfg.u8Gop;
	gOnvifInfo.OnvifVideoEncoderCfg[0].FrameRate = stVencCfg.u8FrameRate;
	gOnvifInfo.OnvifVideoEncoderCfg[0].Bitrate = stVencCfg.u32BitrateRate;
	gOnvifInfo.MediaConfig.h264Options[0].Height = stVencCfg.u16Height;
	gOnvifInfo.MediaConfig.h264Options[0].Width = stVencCfg.u16Width;
	gOnvifInfo.MediaConfig.VideoResolutionconfig =
	    gOnvifInfo.MediaConfig.h264Options[0];
	gOnvifInfo.MediaConfig.VideoEncoderRateControl.BitrateLimit =
	    stVencCfg.u32BitrateRate;
	gOnvifInfo.MediaConfig.VideoEncoderRateControl.FrameRateLimit =
	    stVencCfg.u8FrameRate;
	gOnvifInfo.MediaConfig.VideoEncoderRateControl.EncodingInterval =
	    stVencCfg.u8Gop;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VSUB_PARAM, 0,
	                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	gOnvifInfo.OnvifVideoEncoderCfg[1].Name = ONVIF_MEDIA_VIDEO_SUB_ENCODER_NAME;
	gOnvifInfo.OnvifVideoEncoderCfg[1].token = ONVIF_MEDIA_VIDEO_SUB_ENCODER_TOKEN;
	gOnvifInfo.OnvifVideoEncoderCfg[1].UseCount = 1;
	gOnvifInfo.OnvifVideoEncoderCfg[1].Encoding = tt__VideoEncoding__H264;
	gOnvifInfo.OnvifVideoEncoderCfg[1].Resolution.Width = stVencCfg.u16Width;
	gOnvifInfo.OnvifVideoEncoderCfg[1].Resolution.Height = stVencCfg.u16Height;
	gOnvifInfo.OnvifVideoEncoderCfg[1].Quality = stVencCfg.u8PicQuilty;
	gOnvifInfo.OnvifVideoEncoderCfg[1].SessionTimeout = "PT5S";
	gOnvifInfo.OnvifVideoEncoderCfg[1].Gop = stVencCfg.u8Gop;
	gOnvifInfo.OnvifVideoEncoderCfg[1].FrameRate = stVencCfg.u8FrameRate;
	gOnvifInfo.OnvifVideoEncoderCfg[1].Bitrate = stVencCfg.u32BitrateRate;
	gOnvifInfo.MediaConfig.h264Options[1].Width = stVencCfg.u16Width;
	gOnvifInfo.MediaConfig.h264Options[1].Height = stVencCfg.u16Height;
	return 0;
}

static int onvif_audio_source_cfg_init()
{
	gOnvifInfo.OnvifAudioSourceCfg[0].Name = ONVIF_MEDIA_AUDIO_CFG_NAME;
	gOnvifInfo.OnvifAudioSourceCfg[0].token = ONVIF_MEDIA_AUDIO_CFG_TOKEN;
	gOnvifInfo.OnvifAudioSourceCfg[0].UseCount = 2;
	gOnvifInfo.OnvifAudioSourceCfg[0].SourceToken = ONVIF_MEDIA_AUDIO_SOURCE_TOKEN;
	return 0;
}

static int onvif_audio_encoder_cfg_init()
{
	gOnvifInfo.OnvifAudioEncoderCfg[0].Name = ONVIF_MEDIA_AUDIO_ENCODER_NAME;
	gOnvifInfo.OnvifAudioEncoderCfg[0].token = ONVIF_MEDIA_AUDIO_ENCODER_TOKEN;
	gOnvifInfo.OnvifAudioEncoderCfg[0].UseCount = 1;
	gOnvifInfo.OnvifAudioEncoderCfg[0].Encoding = tt__AudioEncoding__G711;
	gOnvifInfo.OnvifAudioEncoderCfg[0].Bitrate = 16000;
	gOnvifInfo.OnvifAudioEncoderCfg[0].SampleRate = 8000;
	gOnvifInfo.OnvifAudioEncoderCfg[0].SessionTimeout = "PT5S";
	return 0;
}

static int onvif_ptz_cfg_init()
{
	int i;

	for(i = 0; i < ONVIF_PTZ_PRESET_NUM; i++)
	{
		sprintf(gOnvifInfo.OnvifPTZCfg[0].presetCfg[i].Name, "%s_%d", ONVIF_PTZ_NAME,
		        i);
	}

	for(i = 0; i < ONVIF_PTZ_PRESET_NUM; i++)
	{
		sprintf(gOnvifInfo.OnvifPTZCfg[0].presetCfg[i].token, "%s_%d", ONVIF_PTZ_TOKEN,
		        i);
	}

	return 0;
}

int onvif_server_init(int nHttpPort)
{
	__FUN_BEGIN("\n");
	memset(&gOnvifInfo, 0, sizeof(gOnvifInfo));
	gOnvifInfo.HttpPort = nHttpPort;
#ifdef EXTERNAL_SVR_PORT
	__D("Use define port %d for onvif external port.\n", EXTERNAL_SVR_PORT);
	gOnvifInfo.HttpPort = EXTERNAL_SVR_PORT;
#endif
	onvif_scope_init();
	onvif_profile_init();
	onvif_video_source_init();
	onvif_audio_source_init();
	onvif_video_source_cfg_init();
	onvif_video_encoder_cfg_init();
	onvif_audio_source_cfg_init();
	onvif_audio_encoder_cfg_init();
	onvif_ptz_cfg_init();
	tev_Init();
	__FUN_END("\n");
	return 0;
}



static char* onvif_get_device_ip(int nIpAddr)
{
	static char ipAddr[64];
	hi_ip_n2a(nIpAddr, ipAddr, 64);
	return ipAddr;
}


/*****************************************************************************
 函 数 名  : onvif_net_get_media_url
 功能描述  : 获取访问流媒体的URL
 输入参数  :
             int channel
             char* rtsp_url
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
    修改内容   : 新生成函数

*****************************************************************************/
int onvif_net_get_media_url(int channel, char* rtsp_url            )
{
	HI_DEV_PLATFORM_CFG_S   stPfCfg = {0};
	HI_DEV_NET_CFG_S        stNetCfg = {0};
	HI_NETWORK_STATUS_S     stNetStatus = {0};
	char szIp[HI_IP_ADDR_LEN] = {0};
	int nRet = -1;
	int nLen = 0;
	int nStreamPort;
	//platform info
	nLen = sizeof(HI_DEV_PLATFORM_CFG_S);

	if ((nRet = gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_PF_PARAM, 0,
	                                 (char*)&stPfCfg, nLen)) != HI_SUCCESS)
	{
		__D("Media Url: Get PF Failed: %d\n", nRet);
		return HI_FAILURE;
	}

	//network info
	nLen = sizeof(HI_DEV_NET_CFG_S);
	//__D("Media Url: Get Network Info\n");

	if ((nRet = gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                                 (char*)&stNetCfg, nLen)) != HI_SUCCESS)
	{
		__D("Media Url: Get Network Info Failed: %d\n", nRet);
		return HI_FAILURE;
	}

	//__D("u32IpAddr %x  %s\n", stNetCfg.struEtherCfg[0].u32IpAddr,
	//onvif_get_device_ip(stNetCfg.struEtherCfg[0].u32IpAddr));
	strcpy(szIp, onvif_get_device_ip(stNetCfg.struEtherCfg[0].u32IpAddr));
	/* 解决连接上wifi且wifi和eth网卡ip不在同一个网段时，
	    * 客户端没有视频的问题: url的ip地址填充不正确
	    * modify 2012-10-11
	    * 完善url 填充: lym 2013-01-21
	    */
	//get current network
	nLen = sizeof(HI_NETWORK_STATUS_S);

	if ((nRet = gOnvifDevCb.pMsgProc(NULL, HI_GET_SYS_INFO, HI_NETWORK_STATUS, 0,
	                                 (char*)&stNetStatus, nLen)) != HI_SUCCESS)
	{
		__D("Media Url: Get Network Status Failed: %d\n", nRet);
		return HI_FAILURE;
	}

	nStreamPort = stNetCfg.u16StreamPort;
	//__D("Media Url: Get Network Info end5\n");
	sprintf(rtsp_url,
	        "rtsp://%s:%d/dev=%s/media=%d/channel=%d&level=%d",
	        szIp, nStreamPort,
	        stPfCfg.u8PfId,
	        0,
	        0,
	        channel
	       );
	//__D("Media Url: Get Network Info end6 %s\n",rtsp_url);
	return HI_SUCCESS;
}


//#endif

