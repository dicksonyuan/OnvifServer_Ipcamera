//#if (HI_SUPPORT_PLATFORM && PLATFORM_ONVIF)
#include <sys/types.h>
#include <sys/wait.h>
#include <net/route.h>

#include "soapH.h"
#include "soapStub.h"



#include "hi_net_api.h"
#include "hi_platform_onvif.h"
#include "hi_onvif_proc.h"
#include "hi_onvif.h"
#include "onvifface.h"
#include "hi_onvif_server.h"
#include "hi_param_interface.h"
#include "hi_onvif_subscribe.h"
#include "hi_timer.h"
#include "base64.h"
#include "packbits.h"
#include "hi_character_conv.h"


#define TRACE printf

extern int g_tev_TopicNum;




#define ALLOC_STRUCT(val, type) {\
       val = (type *) soap_malloc(soap, sizeof(type)); \
	if(val == NULL) \
	{\
		printf("malloc err\r\n");\
		return SOAP_FAULT;\
	}\
	memset(val, 0, sizeof(type)); \
}


#define ALLOC_STRUCT_INT(val, type, ret)  {\
       val = (type *) soap_malloc(soap, sizeof(type)); \
	if(val == NULL) \
	{\
		printf("malloc err\r\n");\
		return SOAP_FAULT;\
	}\
	*val = ret;\
}

#define ALLOC_STRUCT_NUM(val, type, num) {\
       val = (type *) soap_malloc(soap, sizeof(type)*num); \
	if(val == NULL) \
	{\
		printf("malloc err\r\n");\
		return SOAP_FAULT;\
	}\
	memset(val, 0, sizeof(type)*num); \
}

#define ALLOC_TOKEN(val, token) {\
	val = (char *) soap_malloc(soap, sizeof(char)*ONVIF_TOKEN_LEN); \
	if(val == NULL) \
	{\
		printf("malloc err\r\n");\
		return SOAP_FAULT;\
	}\
	memset(val, 0, sizeof(char)*ONVIF_TOKEN_LEN); \
	strcpy(val, token); \
	}

#define CHECK_USER { \
int ret = onvif_check_security(soap); \
if(ret != SOAP_OK) \
	return ret; \
}

#define CLEAR_WSSE_HEAD { \
int ret = onvif_clear_wsse_head(soap); \
if(ret != SOAP_OK) \
	return ret; \
}


char* topic_set[] =
{
#if 1
	//md alarm
	"<tns1:RuleEngine wstop:topic=\"true\">"
	"<tns1:CellMotionDetector wstop:topic=\"true\">"
	"<tns1:Motion wstop:topic=\"true\">"
	"<tt:MessageDescription IsProperty=\"true\">"
	"<tt:Source>"
	"<tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\"/>"
	"<tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\"/>"
	"<tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\"/>"
	"</tt:Source>"
	"<tt:Data>"
	"<tt:SimpleItemDescription Name=\"IsMotion\" Type=\"xs:boolean\"/>"
	"</tt:Data>"
	"</tt:MessageDescription>"
	"</tns1:Motion>"
	"</tns1:CellMotionDetector>"
	"</tns1:RuleEngine>",
#endif
#if 0
	//od alarm
	"<tns1:RuleEngine wstop:topic=\"true\">"
	"<tns1:TamperDetector wstop:topic=\"true\">"
	"<tns1:Tamper wstop:topic=\"true\">"
	"<tt:MessageDescription IsProperty=\"true\">"
	"<tt:Source>"
	"<tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\"/>"
	"<tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\"/>"
	"<tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\"/>"
	"</tt:Source>"
	"<tt:Data>"
	"<tt:SimpleItemDescription Name=\"IsTamper\" Type=\"xs:boolean\"/>"
	"</tt:Data>"
	"</tt:MessageDescription>"
	"</tns1:Tamper>"
	"</tns1:TamperDetector>"
	"</tns1:RuleEngine>",
#endif

#if 1
	//io alarm
	"<tns1:Device wstop:topic=\"true\">"
	"<tns1:Trigger wstop:topic=\"true\">"
	"<tns1:DigitalInput wstop:topic=\"true\">"
	"<tt:MessageDescription IsProperty=\"true\">"
	"<tt:Source>"
	"<tt:SimpleItemDescription Name=\"IOInputToken\" Type=\"tt:ReferenceToken\"/>"
	"</tt:Source>"
	"<tt:Data>"
	"<tt:SimpleItemDescription Name=\"LogicalState\" Type=\"xs:boolean\"/>"
	"</tt:Data>"
	"</tt:MessageDescription>"
	"</tns1:DigitalInput>"
	"</tns1:Trigger>"
	"</tns1:Device>"
#endif
};





int put_soap_header_wsa(struct soap* soap)
{
	time_t time_n;
	struct tm* tm_t, tm;
	char msgid[LARGE_INFO_LENGTH];
	char macaddr[MACH_ADDR_LENGTH];
	//soap->header->wsa__To = "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
	ALLOC_TOKEN(soap->header->wsa__To,
	            "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous");
	ALLOC_TOKEN(soap->header->wsa__Action,
	            "http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches");
	ALLOC_STRUCT(soap->header->wsa__RelatesTo, struct wsa__Relationship);
	ALLOC_TOKEN(soap->header->wsa__RelatesTo->__item, soap->header->wsa__MessageID);
	soap->header->wsa__RelatesTo->RelationshipType = NULL;
	soap->header->wsa__RelatesTo->__anyAttribute = NULL;
	unsigned long ip;
	ip = onvif_get_ipaddr(soap);
	onvif_get_mac(ip, macaddr);
	time_n = time(NULL);
	tm_t = localtime_r(&time_n, &tm);
	sprintf(msgid, "uuid:1319d68a-%d%d%d-%d%d-%d%d-%02X%02X%02X%02X%02X%02X",
	        tm_t->tm_wday, tm_t->tm_mday, tm_t->tm_mon,
	        tm_t->tm_year, tm_t->tm_hour, tm_t->tm_min,
	        tm_t->tm_sec, macaddr[0], macaddr[1], macaddr[2],
	        macaddr[3], macaddr[4], macaddr[5]);
	ALLOC_TOKEN(soap->header->wsa__MessageID, msgid)
	return 0;
}



int  SOAP_ENV__Fault(struct soap* soap, char* faultcode, char* faultstring,
                     char* faultactor, struct SOAP_ENV__Detail* detail,
                     struct SOAP_ENV__Code* SOAP_ENV__Code,
                     struct SOAP_ENV__Reason* SOAP_ENV__Reason, char* SOAP_ENV__Node,
                     char* SOAP_ENV__Role, struct SOAP_ENV__Detail* SOAP_ENV__Detail)
{
	return SOAP_FAULT;
}

int  __wsdd__Hello(struct soap* soap, struct wsdd__HelloType* wsdd__Hello)
{
	return SOAP_FAULT;
}

int  __wsdd__Bye(struct soap* soap, struct wsdd__ByeType* wsdd__Bye)
{
	return SOAP_FAULT;
}

/////////////////////zj 增加路由来进行跨网段搜索//////////////////////////
static int set_routeinfo(struct sockaddr* pNetInfo, char* pAddr, int flag)
{
	if(pNetInfo == NULL)
	{
		return -1;
	}

	struct sockaddr_in* pRTAddr = (struct sockaddr_in*)pNetInfo;

	memset(pRTAddr, 0, sizeof(struct sockaddr_in));

	pRTAddr->sin_family = AF_INET;

	if(pAddr == NULL)
	{
		//printf("%s:%d\r\n", __func__, __LINE__);
		return -1;
	}

	if(flag)
	{
		pRTAddr->sin_addr.s_addr = inet_addr(pAddr);
	}

	return 0;
}
static int netAddDelRoute(struct soap* soap, char* gateway, char* subnetmask,
                          char* dev, int method)
{
	int fd, posi = 0;
	struct rtentry rt;
	char pdst[16] = {0}, *pstr = NULL, local_ip[16] = {0}, client_ip[16] = {0};
	static int addrouteflag = 0;
	HI_DEV_NET_CFG_S stNetCfg;
	memset(&stNetCfg, '\0', sizeof(HI_DEV_NET_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0, (char*)&stNetCfg,
	                     sizeof(HI_DEV_NET_CFG_S));
	sprintf(local_ip, "%d.%d.%d", (stNetCfg.struEtherCfg[0].u32IpAddr >> 24) & 0xff,
	        (stNetCfg.struEtherCfg[0].u32IpAddr >> 16) & 0xff,
	        (stNetCfg.struEtherCfg[0].u32IpAddr >> 8) & 0xff);
	sprintf(client_ip, "%d.%d.%d", (soap->ip >> 24) & 0xFF, (soap->ip >> 16) & 0xFF,
	        (soap->ip >> 8) & 0xFF);

	if((strcmp(client_ip, "0.0.0") != 0) && (strcmp(local_ip, client_ip) != 0))
	{
		sprintf(client_ip, "%d.%d.%d.%d", (soap->ip >> 24) & 0xFF,
		        (soap->ip >> 16) & 0xFF, (soap->ip >> 8) & 0xFF, (soap->ip) & 0xFF);
		pstr = client_ip;
getsegment:

		if((pstr = strchr(pstr, '.')) != NULL)
		{
			posi++;

			if(posi != 3)
			{
				pstr += 1;
				goto getsegment;
			}
		}
		else if(posi != 3)
		{
			printf("%s:%d -- get setment err\r\n", __func__, __LINE__);
			return -1;
		}

		memcpy(pdst, client_ip, pstr - client_ip + 1);
		strcat(pdst, "0");
		//printf("pdst:%s\r\n", pdst);
		memset(&rt, 0, sizeof(struct rtentry));
		set_routeinfo(&rt.rt_dst, pdst, 1);
		set_routeinfo(&rt.rt_gateway, gateway, 0);
		set_routeinfo(&rt.rt_genmask, subnetmask, 1);
		rt.rt_flags = RTF_UP;
		rt.rt_dev = dev;
		fd = socket(AF_INET, SOCK_DGRAM, 0);

		if(fd < 0)
		{
			perror("socket");
			return -1;
		}

		if(method)
		{
			addrouteflag = 1;

			if (ioctl(fd, SIOCADDRT, (char*)&rt) < 0)
			{
				perror("ioctl add");
				close(fd);
				return -1;
			}

			//printf("add route ok!");
		}
		else
		{
			if(!addrouteflag)
			{
				return 0;
			}

			addrouteflag = 0;

			if (ioctl(fd, SIOCDELRT, (char*)&rt) < 0)
			{
				perror("ioctl del");
				close(fd);
				return -1;
			}

			//printf("del route ok!");
		}

		close(fd);
	}

	return 0;
}

//////////////////////////////////////////////END/////////////////////////////////////////////////

int  __wsdd__Probe(struct soap* soap, struct wsdd__ProbeType* wsdd__Probe)
{
#if 0
	char macaddr[6];
	char _HwId[1024];
	HI_DEV_NET_CFG_S stNetCfg;
	wsdd__ProbeMatchesType ProbeMatches;
	hi_platform_get_net_cfg(0, &stNetCfg);
	soap->master = stOnvifSocket.eth0Socket;
	memcpy(macaddr, stNetCfg.struEtherCfg[0].u8MacAddr, sizeof(macaddr));
	sprintf(_HwId, "urn:uuid:2419d68a-2dd2-21b2-a205-%02X%02X%02X%02X%02X%02X",
	        macaddr[0],
	        macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	ProbeMatches.__sizeProbeMatch = 1;
	ALLOC_STRUCT(ProbeMatches.ProbeMatch, struct wsdd__ProbeMatchType);
	ALLOC_TOKEN(ProbeMatches.ProbeMatch->wsa__EndpointReference.Address, _HwId);
#if 1
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties = NULL;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters = NULL;
#else
	ALLOC_STRUCT(
	    ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties,
	    struct wsa__ReferencePropertiesType);
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties->__size = 0;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties->__any =
	    NULL;
	ALLOC_STRUCT(
	    ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters,
	    struct wsa__ReferenceParametersType);
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters->__size = 0;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters->__any =
	    NULL;
#endif
	ProbeMatches.ProbeMatch->wsa__EndpointReference.PortType = (char**)soap_malloc(
	            soap, sizeof(char*) * 1);
	ALLOC_TOKEN(ProbeMatches.ProbeMatch->wsa__EndpointReference.PortType[0], "ttl");
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName = NULL;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.__size = 0;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.__any = NULL;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.__anyAttribute = NULL;
	ALLOC_TOKEN(ProbeMatches.ProbeMatch->Types, wsdd__Probe->Types);
	ALLOC_STRUCT(ProbeMatches.ProbeMatch->Scopes, struct wsdd__ScopesType);
	ProbeMatches.ProbeMatch->Scopes->__item = soap_malloc(soap, LARGE_INFO_LENGTH);
	onvif_get_scopes(ProbeMatches.ProbeMatch->Scopes->__item, LARGE_INFO_LENGTH);
	ProbeMatches.ProbeMatch->Scopes->MatchBy = NULL;
	ALLOC_TOKEN(ProbeMatches.ProbeMatch->XAddrs,
	            onvif_get_device_xaddr(stNetCfg.struEtherCfg[0].u32IpAddr));
	ProbeMatches.ProbeMatch->MetadataVersion = 1;
	soap->header->wsa__To =
	    "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
	soap->header->wsa__Action =
	    "http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches";
	soap->header->wsa__RelatesTo = (struct wsa__Relationship*)soap_malloc(soap,
	                               sizeof(struct wsa__Relationship));
	soap->header->wsa__RelatesTo->__item = soap->header->wsa__MessageID;
	soap->header->wsa__RelatesTo->RelationshipType = NULL;
	soap->header->wsa__RelatesTo->__anyAttribute = NULL;
	soap->header->wsa__MessageID = (char*)soap_malloc(soap,
	                               sizeof(char) * INFO_LENGTH);
	strcpy(soap->header->wsa__MessageID, _HwId + 4);
	/* send over current socket as HTTP OK response: */
	soap_send___wsdd__ProbeMatches(soap, "http://", NULL, &ProbeMatches);
#else
	char macaddr[6];
	char _HwId[1024];
	HI_DEV_NET_CFG_S stNetCfg;
	wsdd__ProbeMatchesType ProbeMatches;
	__FUN_BEGIN("\n");
	CLEAR_WSSE_HEAD;
	//soap->buf[soap->buflen] = 0;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	soap->master = stOnvifSocket.eth0Socket;
	ProbeMatches.ProbeMatch = (struct wsdd__ProbeMatchType*)soap_malloc(soap,
	                          sizeof(struct wsdd__ProbeMatchType));
	ProbeMatches.ProbeMatch->XAddrs = (char*)soap_malloc(soap,
	                                  sizeof(char) * INFO_LENGTH);
	ProbeMatches.ProbeMatch->Types = (char*)soap_malloc(soap,
	                                 sizeof(char) * INFO_LENGTH);
	ProbeMatches.ProbeMatch->Scopes = (struct wsdd__ScopesType*)soap_malloc(soap,
	                                  sizeof(struct wsdd__ScopesType));
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties =
	    (struct wsa__ReferencePropertiesType*)soap_malloc(soap,
	            sizeof(struct wsa__ReferencePropertiesType));
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters =
	    (struct wsa__ReferenceParametersType*)soap_malloc(soap,
	            sizeof(struct wsa__ReferenceParametersType));
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName =
	    (struct wsa__ServiceNameType*)soap_malloc(soap,
	            sizeof(struct wsa__ServiceNameType));
	ProbeMatches.ProbeMatch->wsa__EndpointReference.PortType = (char**)soap_malloc(
	            soap, sizeof(char*) * SMALL_INFO_LENGTH);
	ProbeMatches.ProbeMatch->wsa__EndpointReference.__any = (char**)soap_malloc(
	            soap, sizeof(char*) * SMALL_INFO_LENGTH);
	ProbeMatches.ProbeMatch->wsa__EndpointReference.__anyAttribute =
	    (char*)soap_malloc(soap, sizeof(char) * SMALL_INFO_LENGTH);
	ProbeMatches.ProbeMatch->wsa__EndpointReference.Address = (char*)soap_malloc(
	            soap, sizeof(char) * INFO_LENGTH);
	memcpy(macaddr, stNetCfg.struEtherCfg[0].u8MacAddr, sizeof(macaddr));
	sprintf(_HwId, "urn:uuid:2419d68a-2dd2-21b2-a205-%02X%02X%02X%02X%02X%02X",
	        macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	ProbeMatches.__sizeProbeMatch = 1;
	//ProbeMatches.ProbeMatch->Scopes->__item =(char *)soap_malloc(soap, 1024);
	ALLOC_STRUCT_NUM(ProbeMatches.ProbeMatch->Scopes->__item, char,
	                 LARGE_INFO_LENGTH);
	onvif_get_scopes(ProbeMatches.ProbeMatch->Scopes->__item, LARGE_INFO_LENGTH);
	ProbeMatches.ProbeMatch->Scopes->MatchBy = NULL;
	strcpy(ProbeMatches.ProbeMatch->XAddrs,
	       onvif_get_device_xaddr(stNetCfg.struEtherCfg[0].u32IpAddr));
	strcpy(ProbeMatches.ProbeMatch->Types, wsdd__Probe->Types);
	ProbeMatches.ProbeMatch->MetadataVersion = 1;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties->__size = 0;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties->__any =
	    NULL;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters->__size = 0;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters->__any =
	    NULL;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.PortType[0] =
	    (char*)soap_malloc(soap, sizeof(char) * SMALL_INFO_LENGTH);
	strcpy(ProbeMatches.ProbeMatch->wsa__EndpointReference.PortType[0], "ttl");
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName->__item = NULL;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName->PortName = NULL;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName->__anyAttribute =
	    NULL;
	ProbeMatches.ProbeMatch->wsa__EndpointReference.__any[0] = (char*)soap_malloc(
	            soap, sizeof(char) * SMALL_INFO_LENGTH);
	strcpy(ProbeMatches.ProbeMatch->wsa__EndpointReference.__any[0], "Any");
	strcpy(ProbeMatches.ProbeMatch->wsa__EndpointReference.__anyAttribute,
	       "Attribute");
	ProbeMatches.ProbeMatch->wsa__EndpointReference.__size = 0;
	strcpy(ProbeMatches.ProbeMatch->wsa__EndpointReference.Address, _HwId);

	if (soap->header)
	{
		soap->header->wsa__To =
		    "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
		soap->header->wsa__Action =
		    "http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches";
		soap->header->wsa__RelatesTo = (struct wsa__Relationship*)soap_malloc(soap,
		                               sizeof(struct wsa__Relationship));
		soap->header->wsa__RelatesTo->__item = soap->header->wsa__MessageID;
		soap->header->wsa__RelatesTo->RelationshipType = NULL;
		soap->header->wsa__RelatesTo->__anyAttribute = NULL;
		soap->header->wsa__MessageID = (char*)soap_malloc(soap,
		                               sizeof(char) * INFO_LENGTH);
		strcpy(soap->header->wsa__MessageID, _HwId + 4);
	}

#if 1		//增加路由来进行跨网段搜索
	netAddDelRoute(soap, NULL, "255.255.255.0", "eth0", 1);
	soap_send___wsdd__ProbeMatches(soap, "http://", NULL, &ProbeMatches);
	netAddDelRoute(soap, NULL, "255.255.255.0", "eth0", 0);
#else
	/*////////////20140903 ZJ//////////////
	struct sockaddr_in *ProbeMatches_S;
	ProbeMatches_S = (struct sockaddr_in *)&soap->peer;
	ProbeMatches_S->sin_addr.s_addr = htonl(0xEFFFFFFA);
	soap_send___wsdd__ProbeMatches(soap, "http://", NULL, &ProbeMatches);
	//////////////////end////////////////*/
	//	if(strcmp(strchr(wsdd__Probe->Types, ':')+1, "NetworkVideoTransmitter")==0)
	hi_onvif_hello(HI_CARD_ETH0);
#endif
	__FUN_END("\n");
#endif
	return SOAP_OK;
}

int  __wsdd__ProbeMatches(struct soap* soap,
                          struct wsdd__ProbeMatchesType* wsdd__ProbeMatches)
{
	__FUN_BEGIN("\n");
	CLEAR_WSSE_HEAD;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __wsdd__Resolve(struct soap* soap,
                     struct wsdd__ResolveType* wsdd__Resolve)
{
	__FUN_BEGIN("\n");
	CLEAR_WSSE_HEAD;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __wsdd__ResolveMatches(struct soap* soap,
                            struct wsdd__ResolveMatchesType* wsdd__ResolveMatches)
{
	__FUN_BEGIN("\n");
	CLEAR_WSSE_HEAD;
	__FUN_END("\n");
	return SOAP_FAULT;
}

#ifndef SUPPORT_ONVIF_2_6

int  __ns1__CreatePrivacyMask(struct soap* soap,
                              struct _ns1__CreatePrivacyMask* ns1__CreatePrivacyMask,
                              struct _ns1__CreatePrivacyMaskResponse* ns1__CreatePrivacyMaskResponse)
{
	__FUN_BEGIN("\n");
	int pt = 0;
	int nSizePt = 0;
	float left = 1.0, top = -1.0, right = -1.0, button = 1.0;
	int w = 704, h = 576;
	float ptx[4] = {0}, pty[4] = {0};
	HI_DEV_VMASK_CFG_S  stMaskCfg;
	struct ns2__PrivacyMask* PrivacyMask = NULL;

	if (ns1__CreatePrivacyMask == NULL ||
	        ns1__CreatePrivacyMask->PrivacyMask == NULL)
	{
		return SOAP_FAULT;
	}

	PrivacyMask =  ns1__CreatePrivacyMask->PrivacyMask;

	if (strcmp(PrivacyMask->VideoSourceToken,
	           ONVIF_MEDIA_VIDEO_SOURCE_TOKEN) != 0)
	{
		return SOAP_FAULT;
	}

	if (PrivacyMask->MaskArea == NULL)
	{
		return SOAP_OK;
	}

	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMASK_PARAM, 0,
	                     (char*)&stMaskCfg, sizeof(HI_DEV_VMASK_CFG_S));
	nSizePt = PrivacyMask->MaskArea->__sizePoint;

	for (pt = 0; pt < nSizePt; pt++)
	{
		if (pt >= 4)
		{
			continue;
		}

		ptx[pt] = *PrivacyMask->MaskArea->Point[pt].x;
		pty[pt] = *PrivacyMask->MaskArea->Point[pt].y;

		if (ptx[pt] < left )
		{
			left = ptx[pt];
		}

		if (pty[pt] > top )
		{
			top = pty[pt];
		}

		if (ptx[pt] > right)
		{
			right = ptx[pt];
		}

		if (pty[pt] < button)
		{
			button = pty[pt];
		}
	}

	printf("left = %f, top = %f, right = %f, button = %f\r\n", left, top, right,
	       button);
	memset(&stMaskCfg.struArea[0], 0, HI_VMASK_REG_NUM * sizeof(HI_RECT_S));
	//四个点真使人迷惑!!!!
	stMaskCfg.struArea[0].s32X = (float)(w / 2 * (1 + left));
	stMaskCfg.struArea[0].s32Y = (float)(h / 2 * (1 - top));
	stMaskCfg.struArea[0].u32Width = (float)(w / 2 * (right - left));
	stMaskCfg.struArea[0].u32Height = (float)(h / 2) * (top - button);
	printf("x = %d, y = %d, w = %d, h = %d\r\n", stMaskCfg.struArea[0].s32X,
	       stMaskCfg.struArea[0].s32Y,
	       stMaskCfg.struArea[0].u32Width, stMaskCfg.struArea[0].u32Height);

	if (stMaskCfg.u8Enable == 0)
	{
		stMaskCfg.u8Enable = 1;
	}

	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_VMASK_PARAM, 0,
	                     (char*)&stMaskCfg, sizeof(HI_DEV_VMASK_CFG_S));
	__FUN_END("\n");
	return SOAP_OK;
}

int  __ns1__DeletePrivacyMask(struct soap* soap,
                              struct _ns1__DeletePrivacyMask* ns1__DeletePrivacyMask,
                              struct _ns1__DeletePrivacyMaskResponse* ns1__DeletePrivacyMaskResponse)
{
	__FUN_BEGIN("\n");
	char* p = NULL;
	int nIndex = 0;
	HI_DEV_VMASK_CFG_S  stMaskCfg;
	printf("Delete token %s\r\n", ns1__DeletePrivacyMask->PrivacyMaskToken_x0020);

	if (ns1__DeletePrivacyMask == NULL ||
	        ns1__DeletePrivacyMask->PrivacyMaskToken_x0020 == NULL)
	{
		return SOAP_OK;    //SOAP_FAULT;
	}

	p = strstr(ns1__DeletePrivacyMask->PrivacyMaskToken_x0020, "MaskToken_");

	if (p == NULL)
	{
		return SOAP_FAULT;
	}

	p += strlen("MaskToken_");
	nIndex = atoi(p);

	if (nIndex >= HI_VMASK_REG_NUM)
	{
		return SOAP_FAULT;
	}

	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMASK_PARAM, 0,
	                     (char*)&stMaskCfg, sizeof(HI_DEV_VMASK_CFG_S));
	memset(&stMaskCfg.struArea[nIndex], 0, sizeof(HI_RECT_S));
	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_VMASK_PARAM, 0,
	                     (char*)&stMaskCfg, sizeof(HI_DEV_VMASK_CFG_S));
	__FUN_END("\n");
	return SOAP_OK;
}

int  __ns1__GetPrivacyMasks(struct soap* soap,
                            struct _ns1__GetPrivacyMasks* ns1__GetPrivacyMasks,
                            struct _ns1__GetPrivacyMasksResponse* ns1__GetPrivacyMasksResponse)
{
	__FUN_BEGIN("\n");
	int i, j, nMasksize = 0;
	int w = 704, h = 576;
	HI_DEV_VMASK_CFG_S  stMaskCfg;
	char status[HI_VMASK_REG_NUM] = {0};
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMASK_PARAM, 0,
	                     (char*)&stMaskCfg, sizeof(HI_DEV_VMASK_CFG_S));

	if (!stMaskCfg.u8Enable)
	{
		goto NoMask;
	}

	for(i = 0; i < HI_VMASK_REG_NUM; i++)
	{
		if(stMaskCfg.struArea[i].s32X == 0
		        && stMaskCfg.struArea[i].s32Y == 0
		        && stMaskCfg.struArea[i].u32Width == 0
		        && stMaskCfg.struArea[i].u32Height == 0)
		{
			continue;
		}

		nMasksize++;
		status[i] = 1;
	}

	if (nMasksize == 0)
	{
		goto NoMask;
	}

	struct ns2__PrivacyMask* PrivacyMask = NULL;

	ns1__GetPrivacyMasksResponse->__sizePrivacyMask = nMasksize;

	PrivacyMask =  soap_malloc(soap, nMasksize * sizeof(struct ns2__PrivacyMask));

	ns1__GetPrivacyMasksResponse->PrivacyMask = PrivacyMask;

	printf("nMasksize = %d\r\n", nMasksize);

	i = 0;

	for (i = 0, j = 0; j < HI_VMASK_REG_NUM; j++)
	{
		int x, y, pt;

		if (status[j] == 0)
		{
			continue;
		}

		PrivacyMask[i].tt__token = soap_malloc(soap, 32);
		sprintf(PrivacyMask[i].tt__token, "MaskToken_%d", j);
		ALLOC_TOKEN(PrivacyMask[i].VideoSourceToken, ONVIF_MEDIA_VIDEO_SOURCE_TOKEN);
		ALLOC_STRUCT(PrivacyMask[i].MaskArea, struct tt__Polygon);
		PrivacyMask[i].MaskArea->__sizePoint = 4;
		PrivacyMask[i].MaskArea->Point = soap_malloc(soap,
		                                 4 * sizeof(struct tt__Vector));

		for (pt = 0; pt < 4; pt++)
		{
			if (pt == 0)
			{
				x = stMaskCfg.struArea[j].s32X;
				y = stMaskCfg.struArea[j].s32Y;
			}
			else if (pt == 1)
			{
				x = stMaskCfg.struArea[j].s32X + stMaskCfg.struArea[j].u32Width;
				y = stMaskCfg.struArea[j].s32Y;
			}
			else if (pt == 2)
			{
				x = stMaskCfg.struArea[j].s32X;
				y = stMaskCfg.struArea[j].s32Y + stMaskCfg.struArea[j].u32Height;
			}
			else if (pt == 3)
			{
				x = stMaskCfg.struArea[j].s32X + stMaskCfg.struArea[j].u32Width;
				y = stMaskCfg.struArea[j].s32Y + stMaskCfg.struArea[j].u32Height;
			}

			ALLOC_STRUCT(PrivacyMask[i].MaskArea->Point[pt].x, float);
			ALLOC_STRUCT(PrivacyMask[i].MaskArea->Point[pt].y, float);
			*PrivacyMask[i].MaskArea->Point[pt].x = (float)(x - w / 2) / (w / 2);
			*PrivacyMask[i].MaskArea->Point[pt].y  = (float)(h / 2 - y) / (h / 2);
			printf("(%d, %d) ", x, y);
			printf("(%f, %f) ", *PrivacyMask[i].MaskArea->Point[pt].x,
			       *PrivacyMask[i].MaskArea->Point[pt].y);
		}

		PrivacyMask[i].__size = 0;
		PrivacyMask[i].__any = NULL;
		printf("\r\n");
		i++;
	}

	__FUN_END("\n");
	return SOAP_OK;
NoMask:
	ns1__GetPrivacyMasksResponse->__sizePrivacyMask = 0;
	ns1__GetPrivacyMasksResponse->PrivacyMask = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __ns1__GetPrivacyMask(struct soap* soap,
                           struct _ns1__GetPrivacyMask* ns1__GetPrivacyMask,
                           struct _ns1__GetPrivacyMaskResponse* ns1__GetPrivacyMaskResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __ns1__SetPrivacyMask(struct soap* soap,
                           struct _ns1__SetPrivacyMask* ns1__SetPrivacyMask,
                           struct _ns1__SetPrivacyMaskResponse* ns1__SetPrivacyMaskResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

/*
<hikwsd:GetPrivacyMaskOptionsResponse>
<hikwsd:PrivacyMaskOptions>
<hikxsd:MaximumNumberOfAreas>4</hikxsd:MaximumNumberOfAreas>
<hikxsd:Position><hikxsd:XRange>
<tt:Min>-1</tt:Min>
<tt:Max>1</tt:Max>
</hikxsd:XRange>
<hikxsd:YRange>
<tt:Min>-1</tt:Min>
<tt:Max>1</tt:Max>
</hikxsd:YRange>
</hikxsd:Position>
</hikwsd:PrivacyMaskOptions>
</hikwsd:GetPrivacyMaskOptionsResponse>
*/
int  __ns1__GetPrivacyMaskOptions(struct soap* soap,
                                  struct _ns1__GetPrivacyMaskOptions* ns1__GetPrivacyMaskOptions,
                                  struct _ns1__GetPrivacyMaskOptionsResponse*
                                  ns1__GetPrivacyMaskOptionsResponse)
{
	__FUN_BEGIN("\n");
	HI_DEV_VMASK_CFG_S stMaskCfg;
	struct tt__FloatRange* XRange = NULL;
	struct tt__FloatRange* YRange = NULL;
	struct ns2__PrivacyMaskOptions* PrivacyMaskOptions = NULL;

	if ( ns1__GetPrivacyMaskOptions == NULL)
	{
		return SOAP_FAULT;
	}

	printf("%s\r\n", ns1__GetPrivacyMaskOptions->VideoSourceToken_x0020);
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMASK_PARAM, 0,
	                     (char*)&stMaskCfg, sizeof(HI_DEV_VMASK_CFG_S));
	ALLOC_STRUCT(PrivacyMaskOptions, struct ns2__PrivacyMaskOptions);
	ns1__GetPrivacyMaskOptionsResponse->PrivacyMaskOptions = PrivacyMaskOptions;
	PrivacyMaskOptions->MaximumNumberOfAreas = HI_VMASK_REG_NUM;
	ALLOC_STRUCT(PrivacyMaskOptions->Position, struct ns2__PositionOptions);
	ALLOC_STRUCT(XRange, struct tt__FloatRange);
	ALLOC_STRUCT(YRange, struct tt__FloatRange);
	XRange->Max = 1, XRange->Min = -1;
	YRange->Max = 1, YRange->Min = -1;
	PrivacyMaskOptions->Position->XRange = XRange;
	PrivacyMaskOptions->Position->YRange = YRange;
	PrivacyMaskOptions->__size = 0;
	PrivacyMaskOptions->__any = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __ns1__Set3DZoom(struct soap* soap, struct _ns1__Set3DZoom* ns1__Set3DZoom,
                      struct _ns1__Set3DZoomResponse* ns1__Set3DZoomResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __ns1__GetIOInputs(struct soap* soap,
                        struct _ns1__GetIOInputs* ns1__GetIOInputs,
                        struct _ns1__GetIOInputsResponse* ns1__GetIOInputsResponse)
{
	__FUN_BEGIN("\n");
	struct ns2__IOInput* IOInput = NULL;
	HI_DEV_DI_CFG_S stDiCfg;
	ns1__GetIOInputsResponse->__sizeIOInput = 1;
	ns1__GetIOInputsResponse->IOInput = IOInput;
	IOInput = soap_malloc(soap,
	                      ns1__GetIOInputsResponse->__sizeIOInput * sizeof(struct ns2__IOInput));

	if (IOInput == NULL)
	{
		return SOAP_FAULT;
	}

	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_DI_PARAM, 0, (char*)&stDiCfg,
	                     sizeof(HI_DEV_DI_CFG_S));

	if (stDiCfg.u8DiType)  	//0常开探头
	{
		IOInput[0].IdleState = tt__RelayIdleState__closed;
	}
	else
	{
		IOInput[0].IdleState = tt__RelayIdleState__open;
	}

	ALLOC_TOKEN(IOInput[0].tt__token, ONVIF_IOINPUT_TOKEN0);
	IOInput[0].__size = 0;
	IOInput[0].__any = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __ns1__GetIOInputOptions(struct soap* soap,
                              struct _ns1__GetIOInputOptions* ns1__GetIOInputOptions,
                              struct _ns1__GetIOInputOptionsResponse* ns1__GetIOInputOptionsResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __ns1__SetIOInput(struct soap* soap,
                       struct _ns1__SetIOInput* ns1__SetIOInput,
                       struct _ns1__SetIOInputResponse* ns1__SetIOInputResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __ns1__GetPatterns(struct soap* soap,
                        struct _ns1__GetPatterns* ns1__GetPatterns,
                        struct _ns1__GetPatternsResponse* ns1__GetPatternsResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __ns1__StartRecordPattern(struct soap* soap,
                               struct _ns1__StartRecordPattern* ns1__StartRecordPattern,
                               struct _ns1__StartRecordPatternResponse* ns1__StartRecordPatternResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __ns1__StopRecordPattern(struct soap* soap,
                              struct _ns1__StopRecordPattern* ns1__StopRecordPattern,
                              struct _ns1__StopRecordPatternResponse* ns1__StopRecordPatternResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __ns1__RunPattern(struct soap* soap,
                       struct _ns1__RunPattern* ns1__RunPattern,
                       struct _ns1__RunPatternResponse* ns1__RunPatternResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __ns1__StopPattern(struct soap* soap,
                        struct _ns1__StopPattern* ns1__StopPattern,
                        struct _ns1__StopPatternResponse* ns1__StopPatternResponse)
{
	__FUN_BEGIN("\n");
	__FUN_END("\n");
	return SOAP_FAULT;
}
#endif


int  __tan__GetSupportedRules(struct soap* soap,
                              struct _tan__GetSupportedRules* tan__GetSupportedRules,
                              struct _tan__GetSupportedRulesResponse* tan__GetSupportedRulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;//20140605 zj
}

int  __tan__CreateRules(struct soap* soap,
                        struct _tan__CreateRules* tan__CreateRules,
                        struct _tan__CreateRulesResponse* tan__CreateRulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tan__DeleteRules(struct soap* soap,
                        struct _tan__DeleteRules* tan__DeleteRules,
                        struct _tan__DeleteRulesResponse* tan__DeleteRulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tan__GetRules(struct soap* soap, struct _tan__GetRules* tan__GetRules,
                     struct _tan__GetRulesResponse* tan__GetRulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	struct tt__Config* Rule = NULL;
	tan__GetRulesResponse->__sizeRule = 2;
	Rule = soap_malloc(soap,
	                   tan__GetRulesResponse->__sizeRule * sizeof(struct tt__Config));
	tan__GetRulesResponse->Rule = Rule;
	ALLOC_TOKEN(Rule[0].Name, "MyMotionDetectorRule");
	ALLOC_TOKEN(Rule[0].Type, "tt:CellMotionDetector");
	ALLOC_STRUCT(Rule[0].Parameters, struct tt__ItemList);
	Rule[0].Parameters->__sizeElementItem = 0;
	Rule[0].Parameters->Extension = NULL;
	Rule[0].Parameters->ElementItem = NULL;
	Rule[0].Parameters->__sizeSimpleItem = 4;
	Rule[0].Parameters->SimpleItem = soap_malloc(soap,
	                                 Rule[0].Parameters->__sizeSimpleItem * sizeof(struct
	                                         _tt__ItemList_SimpleItem ));
	ALLOC_TOKEN(Rule[0].Parameters->SimpleItem[0].Name, "MinCount");
	ALLOC_TOKEN(Rule[0].Parameters->SimpleItem[0].Value, "1");
	ALLOC_TOKEN(Rule[0].Parameters->SimpleItem[1].Name, "AlarmOnDelay");
	ALLOC_TOKEN(Rule[0].Parameters->SimpleItem[1].Value, "100");
	ALLOC_TOKEN(Rule[0].Parameters->SimpleItem[2].Name, "AlarmOffDelay");
	ALLOC_TOKEN(Rule[0].Parameters->SimpleItem[2].Value, "1000");
	ALLOC_TOKEN(Rule[0].Parameters->SimpleItem[3].Name, "ActiveCells");
	ALLOC_TOKEN(Rule[0].Parameters->SimpleItem[3].Value, "0P8A8A==");
	ALLOC_TOKEN(Rule[1].Name, "MyTamperDetectorRule");
	ALLOC_TOKEN(Rule[1].Type,
	            "ns2:TamperDetector");//"hikxsd:TamperDetector");	//20140530zj
	ALLOC_STRUCT(Rule[1].Parameters, struct tt__ItemList);
	Rule[1].Parameters->__sizeElementItem = 0;
	Rule[1].Parameters->Extension = NULL;
	Rule[1].Parameters->ElementItem = NULL;
	Rule[1].Parameters->__sizeSimpleItem = 0;
	Rule[1].Parameters->SimpleItem = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tan__ModifyRules(struct soap* soap,
                        struct _tan__ModifyRules* tan__ModifyRules,
                        struct _tan__ModifyRulesResponse* tan__ModifyRulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	int i = 0;
	struct tt__Config* pRule = NULL;
	HI_DEV_MD_CFG_S stMdCfg;

	if (strcmp(tan__ModifyRules->ConfigurationToken,
	           ONVIF_VIDEO_ANALY_CFG_TOKEN) != 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	for (i = 0; i < tan__ModifyRules->__sizeRule && i < HI_VMD_REG_NUM; i++)
	{
		pRule = &tan__ModifyRules->Rule[i];

		if (strcmp(pRule->Name, "MyMotionDetectorRule") == 0)
		{
			int j = 0;
			gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMD_PARAM, 0,
			                     (char*)&stMdCfg, sizeof(HI_DEV_MD_CFG_S));

			for (j = 0; j < pRule->Parameters->__sizeSimpleItem; j++)
			{
				if (strcmp(pRule->Parameters->SimpleItem[j].Name, "AlarmOffDelay") == 0)
				{
					stMdCfg.u8AlarmKeepTime = atoi(pRule->Parameters->SimpleItem[j].Value) / 1000;
				}
				else if (strcmp(pRule->Parameters->SimpleItem[j].Name, "ActiveCells") == 0)
				{
					//if (transform_rect_activecells2hi(soap, &stMdCfg, pRule->Parameters->SimpleItem[j].Value) != 0)
					//	return SOAP_FAULT;
					{
						char szPackBits[1024] = {0};
						unsigned char szUnPackBits[1024] = {0};
						int len = 0;//from64tobits(szPackBits, pRule->Parameters->SimpleItem[j].Value);

						if (len < 0)
						{
							return SOAP_FAULT;
						}

						len = PackBitsDecode((unsigned char*)szPackBits, len, szUnPackBits,
						                     sizeof(szUnPackBits), 0);

						if (len < 0)
						{
							return SOAP_FAULT;
						}

						unsigned int stBitmap[DEF_ROW] = {0};
						//if (transform_8bits2bitmap32(stBitmap, szUnPackBits, len) != 0)
						//    return SOAP_FAULT;
						{
							int i = 0;
							int row = 0, col = 0, bitsRow = 0, bitsCol = 0;

							for (i = 0; i < ALL_BITS_NUM; i++)
							{
								row = i / DEF_COL;
								col = i % DEF_COL;
								bitsRow = i / 8;
								bitsCol = i % 8;
								stBitmap[row] |= ((szUnPackBits[bitsRow] >> (7 - bitsCol)) & 0x01) <<
								                 (DEF_COL - 1 - col);
							}
						}
						stMdCfg.u8Enable = 0;
						// if (transform_bitmap_2_rect(stMdCfg.struRect, stBitmap) > 0)
						{
							int i, index, count;
							int row, col, width, height;
							unsigned int rowMask;

							for(i = 0; i < DEF_ROW; i++)
							{
								stBitmap[i] = ~stBitmap[i];
							}

							i = 0;
							memset(stMdCfg.struRect, 0, sizeof(HI_RECT_S)*HI_VMD_REG_NUM);
							count = 0;

							for (index = 0; index < ALL_BITS_NUM;)
							{
								row = index / DEF_COL;
								col = index % DEF_COL;
								width = 1;
								height = 1;

								if (TRANSFORM_CHECK_BIT(stBitmap[row], col))
								{
									TRANSFORM_CLR_BIT(stBitmap[row], col);

									//寻找本行连续最长的区块
									for (i = col + 1; i < DEF_COL; i++)
									{
										//printf("gdb line %d => row %d, col %d, bit %x\n", __LINE__, row, i, (pBitmap[row] & (0x01<<(DEF_COL - 1 - i))));
										if (TRANSFORM_CHECK_BIT(stBitmap[row], i))
										{
											width++;
											TRANSFORM_CLR_BIT(stBitmap[row], i);
										}
										else
										{
											break;
										}
									}

									//根据width生成行掩码
									rowMask = 0;

									for (i = col; i < col + width; i++)
									{
										rowMask |= (0x1 << (DEF_COL - 1 - i));
									}

									//寻找满足行连续长为width，的最长连续列
									for (i = row + 1; i < DEF_ROW; i++)
									{
										if (TRANSFORM_CHECK_ROW(stBitmap[i], rowMask))
										{
											height++;
											TRANSFORM_CLR_ROW(stBitmap[i], rowMask);
										}
										else
										{
											break;
										}
									}

									//设置矩形区域
									if(count < HI_VMD_REG_NUM)
									{
										stMdCfg.struRect[count].s32X = (int) (col * 32);
										stMdCfg.struRect[count].s32Y = (int) (row * 32);
										stMdCfg.struRect[count].u32Width = (int) (width * 32);
										stMdCfg.struRect[count].u32Height = (int ) (height * 32);
										count++;
									}
									else
									{
										break;
									}
								}

								index += width;
							}

							if (count == 0)
							{
								memset(stMdCfg.struRect, 0, sizeof(HI_RECT_S) * HI_VMD_REG_NUM);
							}

							if(count > 0)
							{
								stMdCfg.u8Enable = 1;

								for(i = 0; i < HI_VMD_REG_NUM; i++)
								{
									stMdCfg.struSeg[i][0].u8Open = 1;
									stMdCfg.struSeg[i][0].beginSec = 0;
									stMdCfg.struSeg[i][0].endSec = 24 * 60 * 60 - 1;
								}
							}
						}
					}
				}
			}

			gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_VMD_PARAM, 0,
			                     (char*)&stMdCfg, sizeof(HI_DEV_MD_CFG_S));
		}
		else if (strcmp(pRule->Name, "MyTamperDetectModule") == 0)
		{
			;
		}
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tan__GetServiceCapabilities(struct soap* soap,
                                   struct _tan__GetServiceCapabilities* tan__GetServiceCapabilities,
                                   struct _tan__GetServiceCapabilitiesResponse*
                                   tan__GetServiceCapabilitiesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tan__GetSupportedAnalyticsModules(struct soap* soap,
        struct _tan__GetSupportedAnalyticsModules* tan__GetSupportedAnalyticsModules,
        struct _tan__GetSupportedAnalyticsModulesResponse*
        tan__GetSupportedAnalyticsModulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;//20140605 zj
}

int  __tan__CreateAnalyticsModules(struct soap* soap,
                                   struct _tan__CreateAnalyticsModules* tan__CreateAnalyticsModules,
                                   struct _tan__CreateAnalyticsModulesResponse*
                                   tan__CreateAnalyticsModulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tan__DeleteAnalyticsModules(struct soap* soap,
                                   struct _tan__DeleteAnalyticsModules* tan__DeleteAnalyticsModules,
                                   struct _tan__DeleteAnalyticsModulesResponse*
                                   tan__DeleteAnalyticsModulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tan__GetAnalyticsModules(struct soap* soap,
                                struct _tan__GetAnalyticsModules* tan__GetAnalyticsModules,
                                struct _tan__GetAnalyticsModulesResponse* tan__GetAnalyticsModulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	char acText[64];
	HI_DEV_MD_CFG_S stMdCfg;
	struct tt__ItemList* Parameters = NULL;
	struct tt__Config* AnalyticsModule = NULL;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMD_PARAM, 0,
	                     (char*)&stMdCfg, sizeof(HI_DEV_MD_CFG_S));
	tan__GetAnalyticsModulesResponse->__sizeAnalyticsModule = 2;
	AnalyticsModule = soap_malloc(soap,
	                              tan__GetAnalyticsModulesResponse->__sizeAnalyticsModule * sizeof(
	                                  struct tt__Config));
	tan__GetAnalyticsModulesResponse->AnalyticsModule = AnalyticsModule;
	ALLOC_TOKEN(AnalyticsModule[0].Name, "MyCellMotionModule");
	ALLOC_TOKEN(AnalyticsModule[0].Type, "tt:CellMotionEngine");
	ALLOC_STRUCT(Parameters, struct tt__ItemList);
	AnalyticsModule[0].Parameters = Parameters;
	Parameters->__sizeSimpleItem = 1;
	Parameters->SimpleItem  = soap_malloc(soap,
	                                      Parameters->__sizeSimpleItem * sizeof(struct _tt__ItemList_SimpleItem));
	ALLOC_TOKEN(Parameters->SimpleItem[0].Name, "Sensitivity");

	if (stMdCfg.u8Level > 4)
	{
		stMdCfg.u8Level = 2;
	}

	sprintf(acText, "%d", (4 - stMdCfg.u8Level) * 100 / 4);
	ALLOC_TOKEN(Parameters->SimpleItem[0].Value, acText);
	Parameters->__sizeElementItem = 1;
	Parameters->ElementItem = soap_malloc(soap,
	                                      Parameters->__sizeElementItem * sizeof(struct _tt__ItemList_ElementItem));
	ALLOC_TOKEN(Parameters->ElementItem[0].Name, "Layout");
	Parameters->ElementItem[0].__any = soap_malloc(soap, 1024);
	sprintf(Parameters->ElementItem[0].__any,
	        "<tt:CellLayout Columns=\"22\" Rows=\"18\">"
	        "<tt:Transformation><tt:Translate x=\"0.000000\" y=\"0.000000\"/>"
	        "<tt:Scale x=\"0.000000\" y=\"0.000000\"/></tt:Transformation>"
	        "</tt:CellLayout>");
	Parameters->Extension = NULL;
	Parameters->__anyAttribute = NULL;
	/////////////////////////////////
	Parameters = NULL;
	ALLOC_TOKEN(AnalyticsModule[1].Name, "MyTamperDetectModule");
	ALLOC_TOKEN(AnalyticsModule[1].Type,
	            "ns2:TamperEngine");//"hikxsd:TamperEngine");	//20140530zj
	ALLOC_STRUCT(Parameters, struct tt__ItemList);
	AnalyticsModule[1].Parameters = Parameters;
	Parameters->__sizeSimpleItem = 1;
	Parameters->SimpleItem	= soap_malloc(soap,
	                                      Parameters->__sizeSimpleItem * sizeof(struct _tt__ItemList_SimpleItem));
	ALLOC_TOKEN(Parameters->SimpleItem[0].Name, "Sensitivity");
	ALLOC_TOKEN(Parameters->SimpleItem[0].Value, "100");
	Parameters->__sizeElementItem = 2;
	Parameters->ElementItem = soap_malloc(soap,
	                                      Parameters->__sizeElementItem * sizeof(struct _tt__ItemList_ElementItem));
	ALLOC_TOKEN(Parameters->ElementItem[0].Name, "Transformation");
	Parameters->ElementItem[0].__any = soap_malloc(soap, 1024);
	sprintf(Parameters->ElementItem[0].__any,
	        "<tt:Transformation>"		//20140605 zj
	        "<tt:Translate x=\"-1.000000\" y=\"-1.000000\"/>"
	        "<tt:Scale x=\"0.002841\" y=\"0.003472\"/>"
	        "</tt:Transformation>");
	ALLOC_TOKEN(Parameters->ElementItem[1].Name, "Field");
	Parameters->ElementItem[1].__any = soap_malloc(soap, 1024);
	int x, y, w, h;
	x = stMdCfg.struRect[0].s32X, 		y = stMdCfg.struRect[0].s32Y;
	w = stMdCfg.struRect[0].u32Width,   h = stMdCfg.struRect[0].u32Height;
	sprintf(Parameters->ElementItem[1].__any,
	        "<tt:PolygonConfiguration>"
	        "<tt:Polygon>"
	        "<tt:Point x=\"0\" y=\"0\"/>"
	        "<tt:Point x=\"0\" y=\"576\"/>"
	        "<tt:Point x=\"704\" y=\"576\"/>"
	        "<tt:Point x=\"704\" y=\"0\"/>"
	        "</tt:Polygon>"
	        "</tt:PolygonConfiguration>", x, y, x + w, y,
	        x, y + h, x + w, y + h);
	Parameters->Extension = NULL;
	Parameters->__anyAttribute = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tan__ModifyAnalyticsModules(struct soap* soap,
                                   struct _tan__ModifyAnalyticsModules* tan__ModifyAnalyticsModules,
                                   struct _tan__ModifyAnalyticsModulesResponse*
                                   tan__ModifyAnalyticsModulesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	int i = 0, Value = 0;
	HI_DEV_MD_CFG_S stMdCfg;
	struct tt__Config* pAnalyticsModule = NULL;

	if (strcmp(tan__ModifyAnalyticsModules->ConfigurationToken,
	           ONVIF_VIDEO_ANALY_CFG_TOKEN) != 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	for (i = 0; i < tan__ModifyAnalyticsModules->__sizeAnalyticsModule; i++)
	{
		pAnalyticsModule = &tan__ModifyAnalyticsModules->AnalyticsModule[i];

		if (strcmp(pAnalyticsModule->Name, "MyCellMotionModule") == 0)
		{
			gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMD_PARAM, 0,
			                     (char*)&stMdCfg, sizeof(HI_DEV_MD_CFG_S));

			if(pAnalyticsModule->Parameters)
			{
				if(pAnalyticsModule->Parameters->SimpleItem)
				{
					if(pAnalyticsModule->Parameters->SimpleItem->Value)
					{
						Value = atoi(pAnalyticsModule->Parameters->SimpleItem->Value);
						stMdCfg.u8Level = ((100 - Value) / 25) % 5;
						gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_VMD_PARAM, 0,
						                     (char*)&stMdCfg, sizeof(HI_DEV_MD_CFG_S));
						return SOAP_OK;
					}
				}
			}
		}
		else if (strcmp(pAnalyticsModule->Name, "MyTamperDetectModule") == 0)
		{
			;
		}
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tdn__Hello(struct soap* soap, struct wsdd__HelloType tdn__Hello,
                  struct wsdd__ResolveType* tdn__HelloResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tdn__Bye(struct soap* soap, struct wsdd__ByeType tdn__Bye,
                struct wsdd__ResolveType* tdn__ByeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tdn__Probe(struct soap* soap, struct wsdd__ProbeType tdn__Probe,
                  struct wsdd__ProbeMatchesType* tdn__ProbeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetServices(struct soap* soap,
                        struct _tds__GetServices* tds__GetServices,
                        struct _tds__GetServicesResponse* tds__GetServicesResponse)
{
	HI_U32 ip = htonl(hi_get_sock_ip(soap->socket));
	__FUN_BEGIN("\n");
	CLEAR_WSSE_HEAD;
	tds__GetServicesResponse->__sizeService = 6;
	ALLOC_STRUCT_NUM(tds__GetServicesResponse->Service, struct tds__Service, 6);
	ALLOC_TOKEN(tds__GetServicesResponse->Service[0].Namespace,
	            "http://www.onvif.org/ver10/analytics/wsdl");
	ALLOC_TOKEN(tds__GetServicesResponse->Service[0].XAddr,
	            onvif_get_services_xaddr(ip));
	ALLOC_STRUCT(tds__GetServicesResponse->Service[0].Version,
	             struct tt__OnvifVersion);
	tds__GetServicesResponse->Service[0].Version->Major = 3;
	tds__GetServicesResponse->Service[0].Version->Minor = 1;
	ALLOC_TOKEN(tds__GetServicesResponse->Service[1].Namespace,
	            "http://www.onvif.org/ver10/device/wsdl");
	ALLOC_TOKEN(tds__GetServicesResponse->Service[1].XAddr,
	            onvif_get_services_xaddr(ip));
	ALLOC_STRUCT(tds__GetServicesResponse->Service[1].Version,
	             struct tt__OnvifVersion);
	tds__GetServicesResponse->Service[1].Version->Major = 3;
	tds__GetServicesResponse->Service[1].Version->Minor = 1;
	ALLOC_TOKEN(tds__GetServicesResponse->Service[2].Namespace,
	            "http://www.onvif.org/ver10/events/wsdl");
	ALLOC_TOKEN(tds__GetServicesResponse->Service[2].XAddr,
	            onvif_get_services_xaddr(ip));
	ALLOC_STRUCT(tds__GetServicesResponse->Service[2].Version,
	             struct tt__OnvifVersion);
	tds__GetServicesResponse->Service[2].Version->Major = 3;
	tds__GetServicesResponse->Service[2].Version->Minor = 1;
	ALLOC_TOKEN(tds__GetServicesResponse->Service[3].Namespace,
	            "http://www.onvif.org/ver10/imaging/wsdl");
	ALLOC_TOKEN(tds__GetServicesResponse->Service[3].XAddr,
	            onvif_get_services_xaddr(ip));
	ALLOC_STRUCT(tds__GetServicesResponse->Service[3].Version,
	             struct tt__OnvifVersion);
	tds__GetServicesResponse->Service[3].Version->Major = 3;
	tds__GetServicesResponse->Service[3].Version->Minor = 1;
	ALLOC_TOKEN(tds__GetServicesResponse->Service[4].Namespace,
	            "http://www.onvif.org/ver10/media/wsdl");
	ALLOC_TOKEN(tds__GetServicesResponse->Service[4].XAddr,
	            onvif_get_services_xaddr(ip));
	ALLOC_STRUCT(tds__GetServicesResponse->Service[4].Version,
	             struct tt__OnvifVersion);
	tds__GetServicesResponse->Service[4].Version->Major = 3;
	tds__GetServicesResponse->Service[4].Version->Minor = 1;
	ALLOC_TOKEN(tds__GetServicesResponse->Service[5].Namespace,
	            "http://www.onvif.org/ver10/ptz/wsdl");
	ALLOC_TOKEN(tds__GetServicesResponse->Service[5].XAddr,
	            onvif_get_services_xaddr(ip));
	ALLOC_STRUCT(tds__GetServicesResponse->Service[5].Version,
	             struct tt__OnvifVersion);
	tds__GetServicesResponse->Service[5].Version->Major = 3;
	tds__GetServicesResponse->Service[5].Version->Minor = 1;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__GetServiceCapabilities(struct soap* soap,
                                   struct _tds__GetServiceCapabilities* tds__GetServiceCapabilities,
                                   struct _tds__GetServiceCapabilitiesResponse*
                                   tds__GetServiceCapabilitiesResponse)
{
	__FUN_BEGIN("\n");
	CLEAR_WSSE_HEAD;
	ALLOC_STRUCT(tds__GetServiceCapabilitiesResponse->Capabilities,
	             struct tds__DeviceServiceCapabilities);
	struct tds__NetworkCapabilities*
		Network;	/* required element of type tds:NetworkCapabilities */
	struct tds__SecurityCapabilities*
		Security;	/* required element of type tds:SecurityCapabilities */
	struct tds__SystemCapabilities*
		System;	/* required element of type tds:SystemCapabilities */
	//	struct tds__MiscCapabilities *Misc;	/* optional element of type tds:MiscCapabilities */
	ALLOC_STRUCT(Network, struct tds__NetworkCapabilities);
	ALLOC_STRUCT(Network->NTP, int);
	*Network->NTP = 3;
	ALLOC_STRUCT(Network->HostnameFromDHCP, enum xsd__boolean);
	*Network->HostnameFromDHCP = xsd__boolean__false_;
	ALLOC_STRUCT(Network->Dot11Configuration, enum xsd__boolean);
	*Network->Dot11Configuration = xsd__boolean__false_;
	ALLOC_STRUCT(Network->DynDNS, enum xsd__boolean);
	*Network->DynDNS = xsd__boolean__false_;
	ALLOC_STRUCT(Network->IPVersion6, enum xsd__boolean);
	*Network->IPVersion6 = xsd__boolean__false_;
	ALLOC_STRUCT(Network->ZeroConfiguration, enum xsd__boolean);
	*Network->ZeroConfiguration = xsd__boolean__false_;
	ALLOC_STRUCT(Network->IPFilter, enum xsd__boolean);
	*Network->IPFilter = xsd__boolean__false_;
	ALLOC_STRUCT(Security, struct tds__SecurityCapabilities);
	ALLOC_STRUCT(Security->RELToken, enum xsd__boolean);
	*Security->RELToken = xsd__boolean__false_;
	ALLOC_STRUCT(Security->HttpDigest, enum xsd__boolean);
	*Security->HttpDigest = xsd__boolean__false_;
	ALLOC_STRUCT(Security->UsernameToken, enum xsd__boolean);
	*Security->UsernameToken = xsd__boolean__false_;
	ALLOC_STRUCT(Security->KerberosToken, enum xsd__boolean);
	*Security->KerberosToken = xsd__boolean__false_;
	ALLOC_STRUCT(Security->SAMLToken, enum xsd__boolean);
	*Security->SAMLToken = xsd__boolean__false_;
	ALLOC_STRUCT(Security->X_x002e509Token, enum xsd__boolean);
	*Security->X_x002e509Token = xsd__boolean__false_;
	ALLOC_STRUCT(Security->RemoteUserHandling, enum xsd__boolean);
	*Security->RemoteUserHandling = xsd__boolean__false_;
	ALLOC_STRUCT(Security->Dot1X, enum xsd__boolean);
	*Security->Dot1X = xsd__boolean__false_;
	ALLOC_STRUCT(Security->AccessPolicyConfig, enum xsd__boolean);
	*Security->AccessPolicyConfig = xsd__boolean__false_;
	ALLOC_STRUCT(Security->OnboardKeyGeneration, enum xsd__boolean);
	*Security->OnboardKeyGeneration = xsd__boolean__false_;
	ALLOC_STRUCT(Security->TLS1_x002e2, enum xsd__boolean);
	*Security->TLS1_x002e2 = xsd__boolean__false_;
	ALLOC_STRUCT(Security->TLS1_x002e1, enum xsd__boolean);
	*Security->TLS1_x002e1 = xsd__boolean__false_;
	ALLOC_STRUCT(Security->TLS1_x002e0, enum xsd__boolean);
	*Security->TLS1_x002e0 = xsd__boolean__false_;
	ALLOC_STRUCT(System, struct tds__SystemCapabilities);
	ALLOC_STRUCT(System->HttpSupportInformation, enum xsd__boolean);
	*System->HttpSupportInformation = xsd__boolean__true_;
	ALLOC_STRUCT(System->HttpSystemLogging, enum xsd__boolean);
	*System->HttpSystemLogging = xsd__boolean__true_;
	ALLOC_STRUCT(System->HttpSystemBackup, enum xsd__boolean);
	*System->HttpSystemBackup = xsd__boolean__false_;
	ALLOC_STRUCT(System->HttpFirmwareUpgrade, enum xsd__boolean);
	*System->HttpFirmwareUpgrade = xsd__boolean__true_;
	ALLOC_STRUCT(System->FirmwareUpgrade, enum xsd__boolean);
	*System->FirmwareUpgrade = xsd__boolean__false_;
	ALLOC_STRUCT(System->SystemLogging, enum xsd__boolean);
	*System->SystemLogging = xsd__boolean__true_;
	ALLOC_STRUCT(System->SystemBackup, enum xsd__boolean);
	*System->SystemBackup = xsd__boolean__false_;
	ALLOC_STRUCT(System->RemoteDiscovery, enum xsd__boolean);
	*System->RemoteDiscovery = xsd__boolean__false_;
	ALLOC_STRUCT(System->DiscoveryBye, enum xsd__boolean);
	*System->DiscoveryBye = xsd__boolean__false_;
	ALLOC_STRUCT(System->DiscoveryResolve, enum xsd__boolean);
	*System->DiscoveryResolve = xsd__boolean__false_;
	tds__GetServiceCapabilitiesResponse->Capabilities->Network = Network;
	tds__GetServiceCapabilitiesResponse->Capabilities->Security = Security;
	tds__GetServiceCapabilitiesResponse->Capabilities->System = System;
	tds__GetServiceCapabilitiesResponse->Capabilities->Misc = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__GetDeviceInformation(struct soap* soap,
                                 struct _tds__GetDeviceInformation* tds__GetDeviceInformation,
                                 struct _tds__GetDeviceInformationResponse* tds__GetDeviceInformationResponse)
{
	HI_DEV_CFG_S stDevCfg = {0};
	HI_DEV_NET_CFG_S stNetCfg;
	//HI_U32 version;
	HI_U8* mac;
	char FirmwareVersion[LARGE_INFO_LENGTH];
	char SerialNumber[LARGE_INFO_LENGTH];
	char HardwareId[LARGE_INFO_LENGTH];
	__FUN_BEGIN("\n");
	CHECK_USER;
	unsigned long ip;
	ip = onvif_get_ipaddr(soap);
	//hi_platform_get_dev_info(0, &stDevInfo);
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_DEV_PARAM, 0,
	                     (char*)&stDevCfg, sizeof(HI_DEV_CFG_S));

	//version = stDevCfg.u32SoftVer;
	if(ip == stNetCfg.struEtherCfg[0].u32IpAddr)
	{
		mac = stNetCfg.struEtherCfg[0].u8MacAddr;
	}
	else
	{
		mac = stNetCfg.struEtherCfg[1].u8MacAddr;
	}

	//	sprintf(FirmwareVersion, "%d.%d.%d.%d", (version>>24)&0xff, (version>>16)&0xff, (version>>8)&0xff, (version>>0)&0xff);
	strncpy(FirmwareVersion, stDevCfg.softVersion, strlen(stDevCfg.softVersion));
	FirmwareVersion[strlen(stDevCfg.softVersion)] = 0;
	sprintf(SerialNumber, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2],
	        mac[3], mac[4], mac[5]);
	sprintf(HardwareId, "1419d68a-1dd2-11b2-a105-%02X%02X%02X%02X%02X%02X", mac[0],
	        mac[1], mac[2], mac[3], mac[4], mac[5]);
	tds__GetDeviceInformationResponse->Manufacturer = MANUFACTURER_NAME;
	tds__GetDeviceInformationResponse->Model = "IPC";
	//tds__GetDeviceInformationResponse->Model = MODEL_NAME;
	tds__GetDeviceInformationResponse->FirmwareVersion = (char*)soap_malloc(soap,
	        sizeof(char) * LARGE_INFO_LENGTH);
	tds__GetDeviceInformationResponse->SerialNumber = (char*)soap_malloc(soap,
	        sizeof(char) * LARGE_INFO_LENGTH);
	tds__GetDeviceInformationResponse->HardwareId = (char*)soap_malloc(soap,
	        sizeof(char) * LARGE_INFO_LENGTH);

	if(tds__GetDeviceInformationResponse->FirmwareVersion == NULL ||
	        tds__GetDeviceInformationResponse->SerialNumber == NULL ||
	        tds__GetDeviceInformationResponse->HardwareId == NULL)
	{
		__E("Failed to malloc for information.\n");
		return SOAP_FAULT;
	}

	strcpy(tds__GetDeviceInformationResponse->FirmwareVersion, FirmwareVersion);
	strcpy(tds__GetDeviceInformationResponse->SerialNumber, SerialNumber);
	strcpy(tds__GetDeviceInformationResponse->HardwareId, HardwareId);
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetSystemDateAndTime(struct soap* soap,
                                 struct _tds__SetSystemDateAndTime* tds__SetSystemDateAndTime,
                                 struct _tds__SetSystemDateAndTimeResponse* tds__SetSystemDateAndTimeResponse)
{
	HI_NTP_CFG_S stNtpCfg;
	HI_TIME_S stTime;
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NTP_PARAM, 0,
	                     (char*)&stNtpCfg,  sizeof(HI_NTP_CFG_S));
	stNtpCfg.u8NtpOpen =
	    tds__SetSystemDateAndTime->DateTimeType; //Manual = 0, NTP = 1
	stNtpCfg.u8EuroTime = tds__SetSystemDateAndTime->DaylightSavings;

	/* Time Zone */
	if(tds__SetSystemDateAndTime->TimeZone)
	{
		printf("===========zone============:%s\r\n",
		       tds__SetSystemDateAndTime->TimeZone->TZ);
		stNtpCfg.nTimeZone = hi_get_GMT_TimeZone(
		                         tds__SetSystemDateAndTime->TimeZone->TZ);
		printf("zone:%d\r\n", stNtpCfg.nTimeZone);
	}

	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_NTP_PARAM, 0,
	                     (char*)&stNtpCfg,  sizeof(HI_NTP_CFG_S));

	if(tds__SetSystemDateAndTime->UTCDateTime)
	{
		HI_TIME_S stLTTime;
		gOnvifDevCb.pMsgProc(NULL, HI_GET_SYS_INFO, HI_TIME_INFO, 0,
		                     (char*)&stTime, sizeof(HI_TIME_S));

		if(tds__SetSystemDateAndTime->UTCDateTime->Date)
		{
			stTime.u8Year = (HI_U8) (tds__SetSystemDateAndTime->UTCDateTime->Date->Year -
			                         1900);
			stTime.u8Month = (HI_U8)tds__SetSystemDateAndTime->UTCDateTime->Date->Month;
			stTime.u8Day = (HI_U8)tds__SetSystemDateAndTime->UTCDateTime->Date->Day;
			printf("Y:%d, m:%d, d:%d\r\n", stTime.u8Year, stTime.u8Month, stTime.u8Day);

			if((stTime.u8Month > 12) || (stTime.u8Day > 31))
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidDateTime");
				return SOAP_FAULT;
			}
		}

		if(tds__SetSystemDateAndTime->UTCDateTime->Time)
		{
			stTime.u8Hour = (HI_U8)tds__SetSystemDateAndTime->UTCDateTime->Time->Hour;
			stTime.u8Minute = (HI_U8)tds__SetSystemDateAndTime->UTCDateTime->Time->Minute;
			stTime.u8Second = (HI_U8)tds__SetSystemDateAndTime->UTCDateTime->Time->Second;

			if((stTime.u8Hour > 24) || (stTime.u8Minute > 60) || (stTime.u8Second > 60))
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidDateTime");
				return SOAP_FAULT;
			}
		}

		hi_timer_UTC_2_LocalTime(&stTime, &stLTTime, stNtpCfg.nTimeZone);
		{
			printf("Y:%d, M:%d, D:%d\r\n", stLTTime.u8Year + 1900, stLTTime.u8Month,
			       stLTTime.u8Day);
			printf("H:%d, M:%d, S:%d\r\n", stLTTime.u8Hour, stLTTime.u8Minute,
			       stLTTime.u8Second);
		}
		gOnvifDevCb.pMsgProc(NULL, HI_SET_SYS_INFO, HI_TIME_INFO, 0,
		                     (char*)&stLTTime, sizeof(HI_TIME_S));
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__GetSystemDateAndTime(struct soap* soap,
                                 struct _tds__GetSystemDateAndTime* tds__GetSystemDateAndTime,
                                 struct _tds__GetSystemDateAndTimeResponse* tds__GetSystemDateAndTimeResponse)
{
	HI_NTP_CFG_S stNtpCfg;
	HI_TIME_S stTime;
	HI_TIME_S stUTCTime;
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NTP_PARAM, 0,
	                     (char*)&stNtpCfg,  sizeof(HI_NTP_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_SYS_INFO, HI_TIME_INFO, 0,
	                     (char*)&stTime, sizeof(HI_TIME_S));
	hi_timer_LocalTime_2_UTC(&stTime, &stUTCTime, stNtpCfg.nTimeZone);
	ALLOC_STRUCT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime,
	             struct tt__SystemDateTime);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DateTimeType =
	    (stNtpCfg.u8NtpOpen) ? tt__SetDateTimeType__NTP : tt__SetDateTimeType__Manual;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DaylightSavings =
	    !!stNtpCfg.u8EuroTime;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->Extension = NULL;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->__anyAttribute = NULL;
	ALLOC_STRUCT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone,
	             struct tt__TimeZone);
	ALLOC_STRUCT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime,
	             struct tt__DateTime);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ =
	    hi_get_GMT_TZ(
	        stNtpCfg.nTimeZone);//"GMT+05";//"NZST-12NZDT,M10.1.0/2,M3.3.0/3";//timezone;POSIX 1003.1
	ALLOC_STRUCT(
	    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time,
	    struct tt__Time);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Hour =
	    stUTCTime.u8Hour;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Minute
	    = stUTCTime.u8Minute;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Second
	    = stUTCTime.u8Second;
	ALLOC_STRUCT(
	    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date,
	    struct tt__Date);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Year =
	    stUTCTime.u8Year + 1900;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Month =
	    stUTCTime.u8Month;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Day =
	    stUTCTime.u8Day;
	ALLOC_STRUCT(
	    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime,
	    struct tt__DateTime);
	ALLOC_STRUCT(
	    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time,
	    struct tt__Time);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Hour
	    = stTime.u8Hour;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Minute
	    = stTime.u8Minute;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Second
	    = stTime.u8Second;
	ALLOC_STRUCT(
	    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date,
	    struct tt__Date);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Year
	    = stTime.u8Year + 1900;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Month
	    = stTime.u8Month;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Day =
	    stTime.u8Day;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetSystemFactoryDefault(struct soap* soap,
                                    struct _tds__SetSystemFactoryDefault* tds__SetSystemFactoryDefault,
                                    struct _tds__SetSystemFactoryDefaultResponse*
                                    tds__SetSystemFactoryDefaultResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	int nCmd = HI_RESTART_DEVICE;
	gOnvifDevCb.pMsgProc(NULL, HI_SYS_CTRL_MSG, HI_DEV_CTRL, 0, (void*)&nCmd,
	                     sizeof(int));
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__UpgradeSystemFirmware(struct soap* soap,
                                  struct _tds__UpgradeSystemFirmware* tds__UpgradeSystemFirmware,
                                  struct _tds__UpgradeSystemFirmwareResponse*
                                  tds__UpgradeSystemFirmwareResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

static void hi_onvif_reboot_message(void* data)
{
	int nCmd = HI_RESTART_DEVICE;
	pthread_detach(pthread_self()); /*释放线程资源 */
	sleep(1);
	hi_onvif_bye(HI_CARD_ETH0);
	//hi_onvif_bye(HI_CARD_RA0);
	gOnvifDevCb.pMsgProc(NULL, HI_SYS_CTRL_MSG, HI_DEV_CTRL, 0, (void*)&nCmd,
	                     sizeof(int));
}


int  __tds__SystemReboot(struct soap* soap,
                         struct _tds__SystemReboot* tds__SystemReboot,
                         struct _tds__SystemRebootResponse* tds__SystemRebootResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	pthread_t thdid;

	if(pthread_create(&thdid, "ONVIFReboot", NULL, (void*)&hi_onvif_reboot_message,
	                  (void*)NULL) < 0)
	{
		return SOAP_FAULT;
	}

	tds__SystemRebootResponse->Message = "Rebooting";
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__RestoreSystem(struct soap* soap,
                          struct _tds__RestoreSystem* tds__RestoreSystem,
                          struct _tds__RestoreSystemResponse* tds__RestoreSystemResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tds__GetSystemBackup(struct soap* soap,
                            struct _tds__GetSystemBackup* tds__GetSystemBackup,
                            struct _tds__GetSystemBackupResponse* tds__GetSystemBackupResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tds__GetSystemLog(struct soap* soap,
                         struct _tds__GetSystemLog* tds__GetSystemLog,
                         struct _tds__GetSystemLogResponse* tds__GetSystemLogResponse)
{
	if(tds__GetSystemLog->LogType == tt__SystemLogType__System)
	{
		ALLOC_STRUCT(tds__GetSystemLogResponse->SystemLog, struct tt__SystemLog);
		ALLOC_STRUCT_NUM(tds__GetSystemLogResponse->SystemLog->String, char, 128);
		sprintf(tds__GetSystemLogResponse->SystemLog->String,
		        "system log: no fixed\r\n");
	}
	else if(tds__GetSystemLog->LogType == tt__SystemLogType__Access)
	{
		ALLOC_STRUCT(tds__GetSystemLogResponse->SystemLog, struct tt__SystemLog);
		ALLOC_STRUCT_NUM(tds__GetSystemLogResponse->SystemLog->String, char, 128);
		sprintf(tds__GetSystemLogResponse->SystemLog->String,
		        "access log: no fixed\r\n");
	}
	else
	{
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

int  __tds__GetSystemSupportInformation(struct soap* soap,
                                        struct _tds__GetSystemSupportInformation* tds__GetSystemSupportInformation,
                                        struct _tds__GetSystemSupportInformationResponse*
                                        tds__GetSystemSupportInformationResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tds__GetScopes(struct soap* soap, struct _tds__GetScopes* tds__GetScopes,
                      struct _tds__GetScopesResponse* tds__GetScopesResponse)
{
	int i, numScopes = 0;
	int n = 0;
	__FUN_BEGIN("\n");
	CHECK_USER;

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(strlen(gOnvifInfo.type_scope[i].scope) != 0)
		{
			numScopes++;
		}

		if(strlen(gOnvifInfo.name_scope[i].scope) != 0)
		{
			numScopes++;
		}

		if(strlen(gOnvifInfo.location_scope[i].scope) != 0)
		{
			numScopes++;
		}

		if(strlen(gOnvifInfo.hardware_scope[i].scope) != 0)
		{
			numScopes++;
		}
	}

	tds__GetScopesResponse->Scopes = (struct tt__Scope*)soap_malloc(soap,
	                                 sizeof(struct tt__Scope) * numScopes);

	if(tds__GetScopesResponse->Scopes == NULL)
	{
		__E("Failed to malloc for scopes.\n");
		return SOAP_FAULT;
	}

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(strlen(gOnvifInfo.type_scope[i].scope) != 0)
		{
			tds__GetScopesResponse->Scopes[n].ScopeDef = tt__ScopeDefinition__Fixed;
			tds__GetScopesResponse->Scopes[n].ScopeItem = gOnvifInfo.type_scope[i].scope;
			n++;
		}
	}

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(strlen(gOnvifInfo.name_scope[i].scope) != 0)
		{
			tds__GetScopesResponse->Scopes[n].ScopeDef = tt__ScopeDefinition__Fixed;
			tds__GetScopesResponse->Scopes[n].ScopeItem = gOnvifInfo.name_scope[i].scope;
			n++;
		}
	}

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(strlen(gOnvifInfo.location_scope[i].scope) != 0)
		{
			tds__GetScopesResponse->Scopes[n].ScopeDef = tt__ScopeDefinition__Fixed;
			tds__GetScopesResponse->Scopes[n].ScopeItem =
			    gOnvifInfo.location_scope[i].scope;
			n++;
		}
	}

	for(i = 0; i < ONVIF_SCOPES_NUM; i++)
	{
		if(strlen(gOnvifInfo.hardware_scope[i].scope) != 0)
		{
			tds__GetScopesResponse->Scopes[n].ScopeDef = tt__ScopeDefinition__Fixed;
			tds__GetScopesResponse->Scopes[n].ScopeItem =
			    gOnvifInfo.hardware_scope[i].scope;
			n++;
		}
	}

	tds__GetScopesResponse->__sizeScopes = numScopes;
	__FUN_END("\n");
	return SOAP_OK;
}

#if 0
static int odm_parser_category_and_value(const char* odm, char* category,
        int size_category, char* value, int size_value)
{
	char* p = NULL, *q = NULL;;

	if (odm == NULL)
	{
		return -1;
	}

	if (strncmp(odm, "odm:", 4) != 0)
	{
		return -1;
	}

	//first :
	p = strstr(odm, ":");

	if (p == NULL)
	{
		return -1;
	}

	p++;
	//second :
	q = strstr(p, ":");

	if (q == NULL)
	{
		return -1;
	}

	if (q - p >= size_category - 1)
	{
		return -1;
	}

	strncpy(category, p, q - p);
	category[q - p] = 0;
	q++;

	if (strlen(q) >= size_value - 1)
	{
		return -1;
	}

	strcpy(value, q);
	return 0;
}
#endif


int  __tds__SetScopes(struct soap* soap, struct _tds__SetScopes* tds__SetScopes,
                      struct _tds__SetScopesResponse* tds__SetScopesResponse)
{
	int i, j;
	//char category[128], value[128];
	int size = tds__SetScopes->__sizeScopes;
	__FUN_BEGIN("\n");
	CHECK_USER;

	for(i = 0; i < size; i++)
	{
		for(j = 0; j < ONVIF_SCOPES_NUM; j++)
		{
			if(strcmp(tds__SetScopes->Scopes[i], gOnvifInfo.type_scope[j].scope) == 0)
			{
				onvif_fault(soap, "ter:OperationProhibited", "ter:ScopeOverwrite");
				return SOAP_FAULT;
			}

			if(strcmp(tds__SetScopes->Scopes[i], gOnvifInfo.name_scope[j].scope) == 0)
			{
				onvif_fault(soap, "ter:OperationProhibited", "ter:ScopeOverwrite");
				return SOAP_FAULT;
			}

			if(strcmp(tds__SetScopes->Scopes[i], gOnvifInfo.location_scope[j].scope) == 0)
			{
				onvif_fault(soap, "ter:OperationProhibited", "ter:ScopeOverwrite");
				return SOAP_FAULT;
			}

			if(strcmp(tds__SetScopes->Scopes[i], gOnvifInfo.hardware_scope[j].scope) == 0)
			{
				onvif_fault(soap, "ter:OperationProhibited", "ter:ScopeOverwrite");
				return SOAP_FAULT;
			}
		}
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__AddScopes(struct soap* soap, struct _tds__AddScopes* tds__AddScopes,
                      struct _tds__AddScopesResponse* tds__AddScopesResponse)
{
	onvif_fault(soap, "ter:Action", "ter:TooManyScopes");
	__FUN_BEGIN("Onvif fault process!\r\n");
	__FUN_END("Onvif fault process!\r\n");
	return SOAP_FAULT;
}

int  __tds__RemoveScopes(struct soap* soap,
                         struct _tds__RemoveScopes* tds__RemoveScopes,
                         struct _tds__RemoveScopesResponse* tds__RemoveScopesResponse)
{
	onvif_fault(soap, "ter:OperationProhibited", "ter:FixedScope");
	__FUN_BEGIN("Onvif fault process!\r\n");
	__FUN_END("Onvif fault process!\r\n");
	return SOAP_FAULT;
}

int  __tds__GetDiscoveryMode(struct soap* soap,
                             struct _tds__GetDiscoveryMode* tds__GetDiscoveryMode,
                             struct _tds__GetDiscoveryModeResponse* tds__GetDiscoveryModeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	tds__GetDiscoveryModeResponse->DiscoveryMode = tt__DiscoveryMode__Discoverable;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetDiscoveryMode(struct soap* soap,
                             struct _tds__SetDiscoveryMode* tds__SetDiscoveryMode,
                             struct _tds__SetDiscoveryModeResponse* tds__SetDiscoveryModeResponse)
{
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__GetRemoteDiscoveryMode(struct soap* soap,
                                   struct _tds__GetRemoteDiscoveryMode* tds__GetRemoteDiscoveryMode,
                                   struct _tds__GetRemoteDiscoveryModeResponse*
                                   tds__GetRemoteDiscoveryModeResponse)
{
	onvif_fault(soap, "ter:InvalidArgVal",
	            "ter:GetRemoteDiscoveryModeNotSupported");
	__FUN_BEGIN("Onvif fault process!\r\n");
	__FUN_END("Onvif fault process!\r\n");
	return SOAP_FAULT;
}

int  __tds__SetRemoteDiscoveryMode(struct soap* soap,
                                   struct _tds__SetRemoteDiscoveryMode* tds__SetRemoteDiscoveryMode,
                                   struct _tds__SetRemoteDiscoveryModeResponse*
                                   tds__SetRemoteDiscoveryModeResponse)
{
	onvif_fault(soap, "ter:InvalidArgVal",
	            "ter:SetRemoteDiscoveryModeNotSupported");
	__FUN_BEGIN("Onvif fault process!\r\n");
	__FUN_END("Onvif fault process!\r\n");
	return SOAP_FAULT;
}

int  __tds__GetDPAddresses(struct soap* soap,
                           struct _tds__GetDPAddresses* tds__GetDPAddresses,
                           struct _tds__GetDPAddressesResponse* tds__GetDPAddressesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetEndpointReference(struct soap* soap,
                                 struct _tds__GetEndpointReference* tds__GetEndpointReference,
                                 struct _tds__GetEndpointReferenceResponse* tds__GetEndpointReferenceResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetRemoteUser(struct soap* soap,
                          struct _tds__GetRemoteUser* tds__GetRemoteUser,
                          struct _tds__GetRemoteUserResponse* tds__GetRemoteUserResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__SetRemoteUser(struct soap* soap,
                          struct _tds__SetRemoteUser* tds__SetRemoteUser,
                          struct _tds__SetRemoteUserResponse* tds__SetRemoteUserResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetUsers(struct soap* soap, struct _tds__GetUsers* tds__GetUsers,
                     struct _tds__GetUsersResponse* tds__GetUsersResponse)
{
	int userIndex = 0, j = 0;
	int nCount = 0;
	HI_DEV_USER_CFG_S stUserCfg[HI_MAX_USR_NUM];
	char* pUserName;
	__FUN_BEGIN("\n");
	CHECK_USER;

	for(userIndex = 0; userIndex < HI_MAX_USR_NUM; userIndex++)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, userIndex,
		                     (char*)&stUserCfg[userIndex], sizeof(HI_DEV_USER_CFG_S));

		if(strlen(stUserCfg[userIndex].szUsrName) == 0)
		{
			continue;
		}

		nCount++;
	}

	tds__GetUsersResponse->User = (struct tt__User*)soap_malloc(soap,
	                              (nCount * sizeof(struct tt__User)));

	for (userIndex = 0; userIndex < HI_MAX_USR_NUM; userIndex++)
	{
		if (strlen(stUserCfg[userIndex].szUsrName) == 0)
		{
			continue;
		}

		if (j >= nCount)
		{
			break;
		}

		pUserName = soap_malloc(soap, HI_USER_NAME_LEN);

		if(pUserName == NULL)
		{
			__E("Failed to malloc for user name.\n");
			return SOAP_FAULT;
		}

		memset(pUserName, 0, HI_USER_NAME_LEN);
		strcpy(pUserName, stUserCfg[userIndex].szUsrName);
		tds__GetUsersResponse->User[j].Username = pUserName;

		if(userIndex == 0)  	// admin
		{
			tds__GetUsersResponse->User[j].UserLevel = tt__UserLevel__Administrator;
		}
		else  	// user
		{
			tds__GetUsersResponse->User[j].UserLevel = tt__UserLevel__User;
		}

		tds__GetUsersResponse->User[j].Password = NULL;
		tds__GetUsersResponse->User[j].Extension = NULL;
		tds__GetUsersResponse->User[j].__anyAttribute = NULL;
		j++;
	}

	tds__GetUsersResponse->__sizeUser = nCount;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__CreateUsers(struct soap* soap,
                        struct _tds__CreateUsers* tds__CreateUsers,
                        struct _tds__CreateUsersResponse* tds__CreateUsersResponse)
{
	int size = tds__CreateUsers->__sizeUser;
	int i, j, nCount = 0;
	HI_DEV_USER_CFG_S stUserCfg[HI_MAX_USR_NUM];
	__FUN_BEGIN("\n");
	CHECK_USER;

	for(i = 0; i < HI_MAX_USR_NUM; i++)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, i,
		                     (char*)&stUserCfg[i], sizeof(HI_DEV_USER_CFG_S));

		if (strlen(stUserCfg[i].szUsrName) == 0)
		{
			continue;
		}

		nCount++;
	}

	if((size > HI_MAX_USR_NUM - nCount) || (size < 0))
	{
		onvif_fault(soap, "ter:OperationProhibited", "ter:TooManyUsers");
		return SOAP_FAULT;
	}

	for(i = 0; i < size; i++)
	{
		/////////////////20140610 zj////////////////
		if(tds__CreateUsers->User[i].Username == NULL)
		{
			printf("Username is NULL\n");
			return SOAP_FAULT;
		}
		else if(strlen(tds__CreateUsers->User[i].Username) > HI_USER_NAME_LEN - 1)
		{
			onvif_fault(soap, "ter:OperationProhibited", "ter:UsernameTooLong");
			return SOAP_FAULT;
		}

		if(tds__CreateUsers->User[i].Password == NULL)
		{
			printf("Password is NULL\n");
			return SOAP_FAULT;
		}
		else if(strlen(tds__CreateUsers->User[i].Password) > HI_PASS_WORD_LEN - 1)
		{
			onvif_fault(soap, "ter:OperationProhibited", "ter:PasswordTooLong");
			return SOAP_FAULT;
		}

		////////////////////END///////////////////

		/* check username already exist */
		for(j = 0; j < HI_MAX_USR_NUM; j++)
		{
			if (strcmp(tds__CreateUsers->User[i].Username, stUserCfg[j].szUsrName) == 0)
			{
				onvif_fault(soap, "ter:OperationProhibited", "ter:UsernameClash");
				return SOAP_FAULT;
			}
		}

		/* Save user */
		for(j = 0; j < HI_MAX_USR_NUM; j++)
		{
			if(strlen(stUserCfg[j].szUsrName) != 0)
			{
				continue;
			}

			if ((tds__CreateUsers->User[i].UserLevel == tt__UserLevel__Administrator) &&
			        (j != 0))
			{
				onvif_fault(soap, "ter:OperationProhibited", "ter:UsernameClash");
				return SOAP_FAULT;
			}

			strcpy(stUserCfg[j].szUsrName, tds__CreateUsers->User[i].Username);
			strcpy(stUserCfg[j].szPsw, tds__CreateUsers->User[i].Password);
			gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_USR_PARAM, j,
			                     (char*)&stUserCfg[j], sizeof(HI_DEV_USER_CFG_S));
			break;
		}
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__DeleteUsers(struct soap* soap,
                        struct _tds__DeleteUsers* tds__DeleteUsers,
                        struct _tds__DeleteUsersResponse* tds__DeleteUsersResponse)
{
	int size = tds__DeleteUsers->__sizeUsername;
	int i, j;
	HI_DEV_USER_CFG_S stUserCfg[HI_MAX_USR_NUM];
	__FUN_BEGIN("\n");
	CHECK_USER;

	for(i = 0; i < HI_MAX_USR_NUM; i++)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, i,
		                     (char*)&stUserCfg[i], sizeof(HI_DEV_USER_CFG_S));
	}

	for(i = 0; i < size; i++)
	{
		for(j = 0; j < HI_MAX_USR_NUM; j++)
		{
			if (strcmp(tds__DeleteUsers->Username[i], stUserCfg[j].szUsrName) == 0)
			{
				if(j == 0)  	// 因为第0个为管理员，不能删除
				{
					onvif_fault(soap, "ter:InvalidArgVal", "ter:FixedUser");
					return SOAP_FAULT;
				}

				// delete the user
				memset(stUserCfg[j].szUsrName, 0, sizeof(stUserCfg[j].szUsrName));
				memset(stUserCfg[j].szPsw, 0, sizeof(stUserCfg[j].szPsw));
				break;
			}
		}

		if(j == HI_MAX_USR_NUM)  	// 没有找到 需要删除的用户名
		{
			onvif_fault(soap, "ter:InvalidArgVal", "ter:UsernameMissing");
			return SOAP_FAULT;
		}
	}

	for(i = 0; i < HI_MAX_USR_NUM; i++)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_USR_PARAM, i,
		                     (char*)&stUserCfg[i], sizeof(HI_DEV_USER_CFG_S));
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetUser(struct soap* soap, struct _tds__SetUser* tds__SetUser,
                    struct _tds__SetUserResponse* tds__SetUserResponse)
{
	int size = tds__SetUser->__sizeUser;
	int i, j;
	HI_DEV_USER_CFG_S stUserCfg[HI_MAX_USR_NUM];
	__FUN_BEGIN("\n");
	CHECK_USER;

	for(i = 0; i < HI_MAX_USR_NUM; i++)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, i,
		                     (char*)&stUserCfg[i], sizeof(HI_DEV_USER_CFG_S));
	}

	for(i = 0; i < size; i++)
	{
		/////////////////20140610 zj////////////////
		if(tds__SetUser->User[i].Username == NULL)
		{
			printf("Username is NULL\n");
			return SOAP_FAULT;
		}
		else if(strlen(tds__SetUser->User[i].Username) > HI_USER_NAME_LEN - 1)
		{
			onvif_fault(soap, "ter:OperationProhibited", "ter:UsernameTooLong");
			return SOAP_FAULT;
		}

		if(tds__SetUser->User[i].Password == NULL)
		{
			printf("Password is NULL\n");
			return SOAP_FAULT;
		}
		else if(strlen(tds__SetUser->User[i].Password) > HI_PASS_WORD_LEN - 1)
		{
			onvif_fault(soap, "ter:OperationProhibited", "ter:PasswordTooLong");
			return SOAP_FAULT;
		}

		////////////////////END///////////////////

		for(j = 0; j < HI_MAX_USR_NUM; j++)
		{
			if(strcmp(tds__SetUser->User[i].Username, stUserCfg[j].szUsrName) == 0)
			{
				if ((tds__SetUser->User[i].UserLevel == tt__UserLevel__Administrator) &&
				        (j != 0))
				{
					onvif_fault(soap, "ter:OperationProhibited", "ter:PropertyClash");
					return SOAP_FAULT;
				}

				strcpy(stUserCfg[j].szUsrName, tds__SetUser->User[i].Username);
				strcpy(stUserCfg[j].szPsw, tds__SetUser->User[i].Password);
				gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_USR_PARAM, j,
				                     (char*)&stUserCfg[j], sizeof(HI_DEV_USER_CFG_S));
				break;
			}
		}

		// 找不到需要设置的用户名
		if (j == HI_MAX_USR_NUM)
		{
			onvif_fault(soap, "ter:InvalidArgVal", "ter:UsernameMissing");
			return SOAP_FAULT;
		}
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__GetWsdlUrl(struct soap* soap,
                       struct _tds__GetWsdlUrl* tds__GetWsdlUrl,
                       struct _tds__GetWsdlUrlResponse* tds__GetWsdlUrlResponse)
{
	tds__GetWsdlUrlResponse->WsdlUrl =
	    "http://www.onvif.org/Documents/Specifications.aspx";
	return SOAP_OK;
}

int  __tds__GetCapabilities(struct soap* soap,
                            struct _tds__GetCapabilities* tds__GetCapabilities,
                            struct _tds__GetCapabilitiesResponse* tds__GetCapabilitiesResponse)
{
	//	HI_DEV_INFO_S stDevInfo;
	int size = tds__GetCapabilities->__sizeCategory;
	HI_U32 ip = htonl(hi_get_sock_ip(soap->socket));
	int i;
	struct tt__AnalyticsCapabilities* Analytics = NULL;
	struct tt__DeviceCapabilities* Device = NULL;
	struct tt__EventCapabilities* Events = NULL;
	struct tt__ImagingCapabilities* Imaging = NULL;
	struct tt__MediaCapabilities* Media = NULL;
	struct tt__PTZCapabilities* PTZ = NULL;
	struct tt__CapabilitiesExtension* Extension = NULL;
	struct tt__Capabilities* Capabilities = NULL;
	CLEAR_WSSE_HEAD;
	ALLOC_STRUCT(Capabilities, struct tt__Capabilities);
	soap_default_tt__Capabilities(soap, Capabilities);

	if(size == 0)
	{
		ALLOC_STRUCT(Analytics, struct tt__AnalyticsCapabilities);
		ALLOC_STRUCT(Device, struct tt__DeviceCapabilities);
		ALLOC_STRUCT(Events, struct tt__EventCapabilities);
		ALLOC_STRUCT(Imaging, struct tt__ImagingCapabilities);
		ALLOC_STRUCT(Media, struct tt__MediaCapabilities);
		ALLOC_STRUCT(PTZ, struct tt__PTZCapabilities);
		ALLOC_STRUCT(Extension, struct tt__CapabilitiesExtension);
	}

	for(i = 0; i < size; i++)
	{
		if(tds__GetCapabilities->Category[i] == tt__CapabilityCategory__All)
		{
			ALLOC_STRUCT(Analytics, struct tt__AnalyticsCapabilities);
			ALLOC_STRUCT(Device, struct tt__DeviceCapabilities);
			ALLOC_STRUCT(Events, struct tt__EventCapabilities);
			ALLOC_STRUCT(Imaging, struct tt__ImagingCapabilities);
			ALLOC_STRUCT(Media, struct tt__MediaCapabilities);
			ALLOC_STRUCT(PTZ, struct tt__PTZCapabilities);
			ALLOC_STRUCT(Extension, struct tt__CapabilitiesExtension);
		}
		else if(tds__GetCapabilities->Category[i] ==
		        tt__CapabilityCategory__Analytics)
		{
			ALLOC_STRUCT(Analytics, struct tt__AnalyticsCapabilities);
		}
		else if(tds__GetCapabilities->Category[i] == tt__CapabilityCategory__Device)
		{
			ALLOC_STRUCT(Device, struct tt__DeviceCapabilities);
		}
		else if(tds__GetCapabilities->Category[i] == tt__CapabilityCategory__Events)
		{
			ALLOC_STRUCT(Events, struct tt__EventCapabilities);
		}
		else if(tds__GetCapabilities->Category[i] == tt__CapabilityCategory__Imaging)
		{
			ALLOC_STRUCT(Imaging, struct tt__ImagingCapabilities);
		}
		else if(tds__GetCapabilities->Category[i] == tt__CapabilityCategory__Media)
		{
			ALLOC_STRUCT(Media, struct tt__MediaCapabilities);
		}
		else if(tds__GetCapabilities->Category[i] == tt__CapabilityCategory__PTZ)
		{
			ALLOC_STRUCT(PTZ, struct tt__PTZCapabilities);
		}
	}

	if(Analytics)
	{
		ALLOC_TOKEN(Analytics->XAddr, onvif_get_services_xaddr(ip));
		Analytics->RuleSupport = xsd__boolean__true_;
		Analytics->AnalyticsModuleSupport = xsd__boolean__true_;
		Analytics->__size = 0;
		Analytics->__any = NULL;
		Analytics->__anyAttribute = NULL;
	}

	if(Device)
	{
		struct tt__NetworkCapabilities* Network;
		struct tt__SystemCapabilities* System;
		struct tt__IOCapabilities* IO;
		struct tt__SecurityCapabilities* Security;
		ALLOC_TOKEN(Device->XAddr, onvif_get_services_xaddr(ip));
		ALLOC_STRUCT(Network, struct tt__NetworkCapabilities);
		ALLOC_STRUCT(Network->DynDNS, enum xsd__boolean);
		*Network->DynDNS = xsd__boolean__false_;
		ALLOC_STRUCT(Network->IPVersion6, enum xsd__boolean);
		*Network->IPVersion6 = xsd__boolean__false_;
		ALLOC_STRUCT(Network->ZeroConfiguration, enum xsd__boolean);
		*Network->ZeroConfiguration = xsd__boolean__false_;
		ALLOC_STRUCT(Network->IPFilter, enum xsd__boolean);
		*Network->IPFilter = xsd__boolean__false_;
		ALLOC_STRUCT(Network->Extension, struct tt__NetworkCapabilitiesExtension);
		Network->Extension->__size = 0;
		Network->Extension->__any = NULL;
		ALLOC_STRUCT(Network->Extension->Dot11Configuration, enum xsd__boolean);
		*Network->Extension->Dot11Configuration = xsd__boolean__false_;
		Network->Extension->Extension = NULL;
		Network->Extension->__size = 0;
		Network->Extension->__any = NULL;
		Network->__anyAttribute = NULL;
		Device->Network = Network;
		ALLOC_STRUCT(System, struct tt__SystemCapabilities);
		System->FirmwareUpgrade = xsd__boolean__false_;
		System->SystemLogging = xsd__boolean__true_;
		System->SystemBackup = xsd__boolean__false_;
		System->RemoteDiscovery = xsd__boolean__false_;
		System->DiscoveryBye = xsd__boolean__true_;
		System->DiscoveryResolve = xsd__boolean__false_;
		ALLOC_STRUCT(System->Extension, struct tt__SystemCapabilitiesExtension);
		ALLOC_STRUCT(System->Extension->HttpSystemLogging, enum xsd__boolean);
		*System->Extension->HttpSystemLogging = xsd__boolean__true_;
		ALLOC_STRUCT(System->Extension->HttpFirmwareUpgrade, enum xsd__boolean);
		*System->Extension->HttpFirmwareUpgrade = xsd__boolean__true_;
		ALLOC_STRUCT(System->Extension->HttpSystemBackup, enum xsd__boolean);
		*System->Extension->HttpSystemBackup = xsd__boolean__false_;
		ALLOC_STRUCT(System->Extension->HttpSupportInformation, enum xsd__boolean);
		*System->Extension->HttpSupportInformation = xsd__boolean__true_;
		System->Extension->__size = 0;
		System->Extension->__any = NULL;
		System->Extension->Extension = NULL;
		System->__sizeSupportedVersions = 1;
		ALLOC_STRUCT_NUM(System->SupportedVersions, struct tt__OnvifVersion, 1);
		System->SupportedVersions[0].Major = 3;
		System->SupportedVersions[0].Minor = 1;
		System->__anyAttribute = NULL;
		Device->System = System;
		ALLOC_STRUCT(IO, struct tt__IOCapabilities);
		ALLOC_STRUCT(IO->InputConnectors, int);
		*IO->InputConnectors = 0;
		ALLOC_STRUCT(IO->RelayOutputs, int);
		*IO->RelayOutputs = 1;
		IO->Extension = NULL;
		IO->__anyAttribute = NULL;
		Device->IO = IO;
		ALLOC_STRUCT(Security, struct tt__SecurityCapabilities);
		Security->RELToken = xsd__boolean__false_;
		Security->KerberosToken = xsd__boolean__false_;
		Security->SAMLToken = xsd__boolean__false_;
		Security->X_x002e509Token = xsd__boolean__false_;
		Security->AccessPolicyConfig = xsd__boolean__false_;
		Security->OnboardKeyGeneration = xsd__boolean__false_;
		Security->TLS1_x002e2 = xsd__boolean__false_;
		Security->TLS1_x002e1 = xsd__boolean__false_;
		ALLOC_STRUCT(Security->Extension, struct tt__SecurityCapabilitiesExtension);
		Security->Extension->TLS1_x002e0 = xsd__boolean__false_;
		Security->Extension->Extension = NULL;
		Security->__anyAttribute = NULL;
		Security->__size = 0;
		Security->__any = NULL;
		Security->__anyAttribute = NULL;
		Device->Security = Security;
		Device->__anyAttribute = NULL;
	}

	if(Events)
	{
		ALLOC_TOKEN(Events->XAddr, onvif_get_services_xaddr(ip));
		printf("Events:%s\r\n", Events->XAddr);
		Events->WSSubscriptionPolicySupport = 	xsd__boolean__true_;
		Events->WSPullPointSupport = xsd__boolean__false_;
		Events->WSPausableSubscriptionManagerInterfaceSupport = xsd__boolean__false_;
		Events->__size = 0;
		Events->__any = NULL;
		Events->__anyAttribute = NULL;
	}

	if(Imaging)
	{
		ALLOC_TOKEN(Imaging->XAddr, onvif_get_services_xaddr(ip));
		printf("Imaging.xAddr:%s\r\n", Imaging->XAddr);
		Imaging->__anyAttribute = NULL;
	}

	if(Media)
	{
		ALLOC_TOKEN(Media->XAddr, onvif_get_services_xaddr(ip));
		printf("xAddr:%s\r\n", Media->XAddr);
		ALLOC_STRUCT(Media->StreamingCapabilities,
		             struct tt__RealTimeStreamingCapabilities);
		ALLOC_STRUCT(Media->StreamingCapabilities->RTPMulticast,  enum xsd__boolean);
		*Media->StreamingCapabilities->RTPMulticast = xsd__boolean__false_;
		printf("----1---\r\n");
		ALLOC_STRUCT(Media->StreamingCapabilities->RTP_USCORETCP,  enum xsd__boolean);
		*Media->StreamingCapabilities->RTP_USCORETCP = xsd__boolean__false_;
		printf("----2---\r\n");
		ALLOC_STRUCT(Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP,
		             enum xsd__boolean);
		*Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP = xsd__boolean__true_;
		printf("----3---\r\n");
		Media->StreamingCapabilities->__anyAttribute = NULL;
		Media->StreamingCapabilities->Extension = NULL;
		ALLOC_STRUCT(Media->Extension, struct tt__MediaCapabilitiesExtension);
		printf("----4---\r\n");
		ALLOC_STRUCT(Media->Extension->ProfileCapabilities,
		             struct tt__ProfileCapabilities);
		Media->Extension->ProfileCapabilities->MaximumNumberOfProfiles = 7;
		Media->Extension->ProfileCapabilities->__size = 0;
		Media->Extension->ProfileCapabilities->__any = NULL;
		Media->Extension->ProfileCapabilities->__anyAttribute = NULL;
		Media->Extension->__size = 0;
		Media->Extension->__any = NULL;
		Media->Extension->__anyAttribute = NULL;
		Media->__size = 0;
		Media->__any = NULL;
		Media->__anyAttribute = NULL;
	}

	if(PTZ)
	{
		ALLOC_TOKEN(PTZ->XAddr, onvif_get_services_xaddr(ip));
		PTZ->__size = 0;
		PTZ->__any = NULL;
		PTZ->__anyAttribute = NULL;
		printf("PTZ:%s\r\n", PTZ->XAddr);
	}

	if(Extension)
	{
#ifndef SUPPORT_ONVIF_2_6
		ALLOC_STRUCT(Extension->hikCapabilities, struct tt__hikCapabilities);
		ALLOC_TOKEN(Extension->hikCapabilities->XAddr, onvif_get_services_xaddr(ip));
		Extension->hikCapabilities->IOInputSupport = xsd__boolean__true_;
		Extension->hikCapabilities->PrivacyMaskSupport = xsd__boolean__true_;
		Extension->hikCapabilities->PTZ3DZoomSupport = xsd__boolean__false_;
		Extension->hikCapabilities->PTZPatternSupport = xsd__boolean__false_;
		Extension->hikCapabilities->__size = 0;
		Extension->hikCapabilities->__any = NULL;
		Extension->hikCapabilities->__anyAttribute = NULL;
#endif
		ALLOC_STRUCT(Extension->DeviceIO, struct tt__DeviceIOCapabilities);
		ALLOC_TOKEN(Extension->DeviceIO->XAddr, onvif_get_services_xaddr(ip));
		Extension->DeviceIO->VideoSources = 1;
		Extension->DeviceIO->VideoOutputs = 1;
		Extension->DeviceIO->AudioSources = 1;
		Extension->DeviceIO->AudioOutputs = 1;
		Extension->DeviceIO->RelayOutputs = 1;
		Extension->DeviceIO->__size = 0;
		Extension->DeviceIO->__any = NULL;
		Extension->DeviceIO->__anyAttribute = NULL;
		Extension->Display = NULL;
		ALLOC_STRUCT(Extension->Recording, struct tt__RecordingCapabilities);
		ALLOC_TOKEN(Extension->Recording->XAddr, onvif_get_services_xaddr(ip));
		Extension->Recording->ReceiverSource = xsd__boolean__false_;
		Extension->Recording->MediaProfileSource = xsd__boolean__true_;
		Extension->Recording->DynamicTracks = xsd__boolean__false_;
		Extension->Recording->DynamicRecordings = xsd__boolean__false_;
		Extension->Recording->MaxStringLength = 128;
		Extension->Recording->__size = 0;
		Extension->Recording->__any = NULL;
		Extension->Recording->__anyAttribute = NULL;
		ALLOC_STRUCT(Extension->Search, struct tt__SearchCapabilities);
		ALLOC_STRUCT_NUM(Extension->Search->XAddr, char,
		                 ONVIF_DEVICE_SERVICE_ADDR_SIZE);
		strcpy(Extension->Search->XAddr, onvif_get_services_xaddr(ip));
		Extension->Search->MetadataSearch = xsd__boolean__false_;
		Extension->Search->__size = 0;
		Extension->Search->__any = NULL;
		Extension->Search->__anyAttribute = NULL;
		ALLOC_STRUCT(Extension->Replay, struct tt__ReplayCapabilities);
		ALLOC_TOKEN(Extension->Replay->XAddr, onvif_get_services_xaddr(ip));
		Extension->Replay->__size = 0;
		Extension->Replay->__any = NULL;
		Extension->Replay->__anyAttribute = NULL;
		Extension->__size = 0;
		Extension->__any = NULL;
	}

	Capabilities->Analytics = Analytics;
	Capabilities->Device = Device;
	Capabilities->Events = Events;
	Capabilities->Imaging = Imaging;
	Capabilities->Media = Media;
	Capabilities->PTZ = PTZ;
	Capabilities->Extension = Extension;
	Capabilities->__anyAttribute = NULL;
	tds__GetCapabilitiesResponse->Capabilities = Capabilities;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetDPAddresses(struct soap* soap,
                           struct _tds__SetDPAddresses* tds__SetDPAddresses,
                           struct _tds__SetDPAddressesResponse* tds__SetDPAddressesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetHostname(struct soap* soap,
                        struct _tds__GetHostname* tds__GetHostname,
                        struct _tds__GetHostnameResponse* tds__GetHostnameResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	HI_DEV_NET_CFG_S stNet;
	struct tt__HostnameInformation* HostnameInformation;
	memset(&stNet, 0, sizeof(HI_DEV_NET_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNet, sizeof(HI_DEV_NET_CFG_S));

	if((HostnameInformation = (struct tt__HostnameInformation*)soap_malloc(soap,
	                          sizeof(struct tt__HostnameInformation))) == NULL)
	{
		__E("Failed to to malloc for HostnameInformation.\n");
		return SOAP_FAULT;
	}

	HostnameInformation->Extension = NULL;
	HostnameInformation->__anyAttribute = NULL;
	HostnameInformation->Name = "localhost";
	HI_U32 ipaddr = htonl(hi_get_sock_ip(soap->socket));

	if(ipaddr == stNet.struEtherCfg[0].u32IpAddr)
	{
		HostnameInformation->FromDHCP = !!stNet.struEtherCfg[0].u8DhcpOn;
	}
	else if(ipaddr == stNet.struEtherCfg[1].u32IpAddr)
	{
		HostnameInformation->FromDHCP = !!stNet.struEtherCfg[1].u8DhcpOn;
	}

	tds__GetHostnameResponse->HostnameInformation = HostnameInformation;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetHostname(struct soap* soap,
                        struct _tds__SetHostname* tds__SetHostname,
                        struct _tds__SetHostnameResponse* tds__SetHostnameResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__SetHostnameFromDHCP(struct soap* soap,
                                struct _tds__SetHostnameFromDHCP* tds__SetHostnameFromDHCP,
                                struct _tds__SetHostnameFromDHCPResponse* tds__SetHostnameFromDHCPResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetDNS(struct soap* soap, struct _tds__GetDNS* tds__GetDNS,
                   struct _tds__GetDNSResponse* tds__GetDNSResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	HI_DEV_NET_CFG_S stNetCfg;
	//HI_DEV_ONVIF_CFG_S stOnvifCfg;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	//hi_platform_get_onvif_cfg(0, &stOnvifCfg);
	tds__GetDNSResponse->DNSInformation = (struct tt__DNSInformation*)soap_malloc(
	        soap, sizeof(struct tt__DNSInformation));

	if(tds__GetDNSResponse->DNSInformation == NULL)
	{
		__E("Failed to malloc for dns information.\n");
		return SOAP_FAULT;
	}

	memset(tds__GetDNSResponse->DNSInformation, 0,
	       sizeof(struct tt__DNSInformation));
	HI_U32 ip = htonl(hi_get_sock_ip(soap->socket));

	if(ip == stNetCfg.struEtherCfg[0].u32IpAddr)
	{
		tds__GetDNSResponse->DNSInformation->FromDHCP =
		    !!stNetCfg.struEtherCfg[0].u8DhcpOn;
	}
	else if(ip == stNetCfg.struEtherCfg[1].u32IpAddr)
	{
		tds__GetDNSResponse->DNSInformation->FromDHCP =
		    !!stNetCfg.struEtherCfg[1].u8DhcpOn;
	}

#if 1
	tds__GetDNSResponse->DNSInformation->__sizeSearchDomain = 0;
	tds__GetDNSResponse->DNSInformation->SearchDomain = NULL;
#else
	tds__GetDNSResponse->DNSInformation->__sizeSearchDomain = 1;
	tds__GetDNSResponse->DNSInformation->SearchDomain = (char**)soap_malloc(soap,
	        sizeof(char*));

	if(tds__GetDNSResponse->DNSInformation->SearchDomain == NULL)
	{
		__E("Failed to malloc for search domain.\n");
		return SOAP_FAULT;
	}

	tds__GetDNSResponse->DNSInformation->SearchDomain[0] = (char*)soap_malloc(soap,
	        HI_IP_ADDR_LEN);

	if(tds__GetDNSResponse->DNSInformation->SearchDomain[0] == NULL)
	{
		__E("Failed to malloc for SearchDomain[0].\n");
		return SOAP_FAULT;
	}

	strcpy(tds__GetDNSResponse->DNSInformation->SearchDomain[0], "www.szdowse.com");
#endif

	if(stNetCfg.u8DnsDhcp)
	{
		tds__GetDNSResponse->DNSInformation->__sizeDNSFromDHCP = 2;
		tds__GetDNSResponse->DNSInformation->DNSFromDHCP = soap_malloc(soap,
		        sizeof(struct tt__IPAddress));

		if(tds__GetDNSResponse->DNSInformation->DNSFromDHCP == NULL)
		{
			__E("Failed to malloc for dns from dhcp.\n");
			return SOAP_FAULT;
		}

		tds__GetDNSResponse->DNSInformation->DNSFromDHCP[0].Type = tt__IPType__IPv4;
		tds__GetDNSResponse->DNSInformation->DNSFromDHCP[0].IPv4Address =
		    (char*)soap_malloc(soap, sizeof(char) * HI_IP_ADDR_LEN);

		if(tds__GetDNSResponse->DNSInformation->DNSFromDHCP[0].IPv4Address == NULL)
		{
			__E("Failed to malloc for ipv4 address.\n");
			return SOAP_FAULT;
		}

		__D("\n");
		snprintf(tds__GetDNSResponse->DNSInformation->DNSFromDHCP[0].IPv4Address,
		         HI_IP_ADDR_LEN,
		         "%d.%d.%d.%d",
		         (stNetCfg.u32DnsIp1 >> 24) & 0xff,
		         (stNetCfg.u32DnsIp1 >> 16) & 0xff,
		         (stNetCfg.u32DnsIp1 >> 8) & 0xff,
		         (stNetCfg.u32DnsIp1 >> 0) & 0xff);
		tds__GetDNSResponse->DNSInformation->DNSFromDHCP[0].IPv6Address = NULL;
		tds__GetDNSResponse->DNSInformation->DNSFromDHCP[1].Type = tt__IPType__IPv4;
		tds__GetDNSResponse->DNSInformation->DNSFromDHCP[1].IPv4Address =
		    (char*)soap_malloc(soap, sizeof(char) * HI_IP_ADDR_LEN);

		if(tds__GetDNSResponse->DNSInformation->DNSFromDHCP[1].IPv4Address == NULL)
		{
			__E("Failed to malloc for ipv4 address[0].\n");
			return SOAP_FAULT;
		}

		__D("\n");
		snprintf(tds__GetDNSResponse->DNSInformation->DNSFromDHCP[1].IPv4Address,
		         HI_IP_ADDR_LEN,
		         "%d.%d.%d.%d",
		         (stNetCfg.u32DnsIp2 >> 24) & 0xff,
		         (stNetCfg.u32DnsIp2 >> 16) & 0xff,
		         (stNetCfg.u32DnsIp2 >> 8) & 0xff,
		         (stNetCfg.u32DnsIp2 >> 0) & 0xff);
		tds__GetDNSResponse->DNSInformation->DNSFromDHCP[1].IPv6Address = NULL;
	}
	else
	{
		__D("\n");
		tds__GetDNSResponse->DNSInformation->__sizeDNSManual = 2;
		tds__GetDNSResponse->DNSInformation->DNSManual = soap_malloc(soap,
		        sizeof(struct tt__IPAddress) * 2);

		if(tds__GetDNSResponse->DNSInformation->DNSManual == NULL)
		{
			__E("Failed to malloc for dns manual.\n");
			return SOAP_FAULT;
		}

		tds__GetDNSResponse->DNSInformation->DNSManual[0].Type = tt__IPType__IPv4;
		__D("\n");
		tds__GetDNSResponse->DNSInformation->DNSManual[0].IPv4Address =
		    (char*)soap_malloc(soap, sizeof(char) * HI_IP_ADDR_LEN);

		if(tds__GetDNSResponse->DNSInformation->DNSManual[0].IPv4Address == NULL)
		{
			__E("Failed to malloc for ipv4 address.\n");
			return SOAP_FAULT;
		}

		snprintf(tds__GetDNSResponse->DNSInformation->DNSManual[0].IPv4Address,
		         HI_IP_ADDR_LEN,
		         "%d.%d.%d.%d",
		         (stNetCfg.u32DnsIp1 >> 24) & 0xff,
		         (stNetCfg.u32DnsIp1 >> 16) & 0xff,
		         (stNetCfg.u32DnsIp1 >> 8) & 0xff,
		         (stNetCfg.u32DnsIp1 >> 0) & 0xff);
		tds__GetDNSResponse->DNSInformation->DNSManual[0].IPv6Address = NULL;
		tds__GetDNSResponse->DNSInformation->DNSManual[1].Type = tt__IPType__IPv4;
		__D("\n");
		tds__GetDNSResponse->DNSInformation->DNSManual[1].IPv4Address =
		    (char*)soap_malloc(soap, sizeof(char) * HI_IP_ADDR_LEN);

		if(tds__GetDNSResponse->DNSInformation->DNSManual[1].IPv4Address == NULL)
		{
			__E("Failed to malloc for ipv4 address.\n");
			return SOAP_FAULT;
		}

		snprintf(tds__GetDNSResponse->DNSInformation->DNSManual[1].IPv4Address,
		         HI_IP_ADDR_LEN,
		         "%d.%d.%d.%d",
		         (stNetCfg.u32DnsIp2 >> 24) & 0xff,
		         (stNetCfg.u32DnsIp2 >> 16) & 0xff,
		         (stNetCfg.u32DnsIp2 >> 8) & 0xff,
		         (stNetCfg.u32DnsIp2 >> 0) & 0xff);
		tds__GetDNSResponse->DNSInformation->DNSManual[1].IPv6Address = NULL;
	}

	tds__GetDNSResponse->DNSInformation->Extension = NULL;
	tds__GetDNSResponse->DNSInformation->__anyAttribute = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetDNS(struct soap* soap, struct _tds__SetDNS* tds__SetDNS,
                   struct _tds__SetDNSResponse* tds__SetDNSResponse)
{
	int i;
	HI_DEV_NET_CFG_S stNetCfg;
	//HI_DEV_ONVIF_CFG_S stOnvifCfg;
	HI_U32 u32IpAddr;
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));

	//hi_platform_get_onvif_cfg(0, &stOnvifCfg);
	if(tds__SetDNS->FromDHCP)
	{
		stNetCfg.u8DnsDhcp = 1;
	}

#if 0

	if(tds__SetDNS->__sizeSearchDomain > 0)
	{
		if(tds__SetDNS->SearchDomain != NULL)
		{
			if(tds__SetDNS->SearchDomain[0] != NULL)
			{
				strncpy(stOnvifCfg.szDomain, tds__SetDNS->SearchDomain[0],
				        sizeof(stOnvifCfg.szDomain) - 1);
				stOnvifCfg.szDomain[sizeof(stOnvifCfg.szDomain) - 1] = '\0';
				hi_platform_set_onvif_cfg(0, &stOnvifCfg);
			}
		}
	}

#endif

	if(tds__SetDNS->DNSManual != NULL)
	{
		int dnsIndex = 0;

		for(i = 0; i < tds__SetDNS->__sizeDNSManual && dnsIndex < 2; i++)
		{
			if(tds__SetDNS->DNSManual[i].Type == tt__IPType__IPv6)
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidIPv6Address");
				return SOAP_FAULT;
			}

			if(tds__SetDNS->DNSManual[i].IPv4Address == NULL ||
			        !isValidIp4(tds__SetDNS->DNSManual[i].IPv4Address))
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidIPv4Address");
				return SOAP_FAULT;
			}

			u32IpAddr = hi_ip_a2n(tds__SetDNS->DNSManual[i].IPv4Address);

			if(dnsIndex == 0)
			{
				stNetCfg.u32DnsIp1 = u32IpAddr;
			}
			else if(dnsIndex == 1)
			{
				stNetCfg.u32DnsIp2 = u32IpAddr;
			}

			dnsIndex++;
		}
	}

	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	//hi_platform_set_onvif_cfg(0, &stOnvifCfg);
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__GetNTP(struct soap* soap, struct _tds__GetNTP* tds__GetNTP,
                   struct _tds__GetNTPResponse* tds__GetNTPResponse)
{
	HI_NTP_CFG_S stNtpCfg;
	struct tt__NTPInformation* NTPInformation;
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NTP_PARAM, 0,
	                     (char*)&stNtpCfg,  sizeof(HI_NTP_CFG_S));
	NTPInformation = (struct tt__NTPInformation*)soap_malloc(soap,
	                 sizeof(struct tt__NTPInformation));

	if(NTPInformation == NULL)
	{
		__E("Failed to malloc for ntp information.\n");
		return SOAP_FAULT;
	}

	NTPInformation->FromDHCP = xsd__boolean__false_;
	NTPInformation->__sizeNTPFromDHCP = 0;
	NTPInformation->NTPFromDHCP = NULL;
	NTPInformation->__sizeNTPManual = 1;

	if(stNtpCfg.u8NtpOpen)
	{
		NTPInformation->NTPManual = (struct tt__NetworkHost*)soap_malloc(soap,
		                            sizeof(struct tt__NetworkHost));

		if(NTPInformation->NTPManual == NULL)
		{
			__E("Failed to malloc for ntp manual.\n");
			return SOAP_FAULT;
		}

		NTPInformation->NTPManual->Type = tt__NetworkHostType__IPv4;
		NTPInformation->NTPManual->IPv4Address = (char*)soap_malloc(soap, sizeof(char));

		if(NTPInformation->NTPManual->IPv4Address == NULL)
		{
			__E("Failed to malloc for dns name.\n");
			return SOAP_FAULT;
		}

		strncpy(NTPInformation->NTPManual->IPv4Address,
		        stNtpCfg.szNtpServer, sizeof(stNtpCfg.szNtpServer - 1));
		//NTPInformation->NTPManual->IPv4Address[0][HI_IP_ADDR_LEN - 1] = '\0';
		NTPInformation->NTPManual->IPv6Address = NULL;
		NTPInformation->NTPManual->DNSname = NULL;
	}
	else
	{
		NTPInformation->NTPManual = NULL;
	}

	NTPInformation->Extension = NULL;
	NTPInformation->__anyAttribute = NULL;
	tds__GetNTPResponse->NTPInformation = NTPInformation;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetNTP(struct soap* soap, struct _tds__SetNTP* tds__SetNTP,
                   struct _tds__SetNTPResponse* tds__SetNTPResponse)
{
	HI_NTP_CFG_S stNtpCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if (tds__SetNTP->FromDHCP == xsd__boolean__true_)
	{
		onvif_fault(soap, "ter:NotSupported", "ter:SetDHCPNotAllowed");
		return SOAP_FAULT;
	}

	if(tds__SetNTP->__sizeNTPManual > 0 && tds__SetNTP->NTPManual != NULL)
	{
		if (tds__SetNTP->NTPManual->IPv6Address != NULL ||
		        tds__SetNTP->NTPManual->Type == tt__NetworkHostType__IPv6)
		{
			onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidIPv6Address");
			return SOAP_FAULT;
		}

		if(tds__SetNTP->NTPManual->DNSname != NULL ||
		        tds__SetNTP->NTPManual->Type == tt__NetworkHostType__DNS)
		{
			onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidDnsName");
			return SOAP_FAULT;
		}

		if(tds__SetNTP->NTPManual->IPv4Address != NULL  &&
		        tds__SetNTP->NTPManual->Type == tt__NetworkHostType__IPv4)
		{
			gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NTP_PARAM, 0,
			                     (char*)&stNtpCfg,	sizeof(HI_NTP_CFG_S));

			if(!isValidIp4(tds__SetNTP->NTPManual->IPv4Address))
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidIPv4Address");
				return SOAP_FAULT;
			}

			strncpy(stNtpCfg.szNtpServer, tds__SetNTP->NTPManual->IPv4Address,
			        sizeof(stNtpCfg.szNtpServer) - 1);
			stNtpCfg.szNtpServer[sizeof(stNtpCfg.szNtpServer) - 1] = '\0';
			printf("=========onvif SetNTP=======:%d\r\n", stNtpCfg.nTimeZone);
			gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_NTP_PARAM, 0,
			                     (char*)&stNtpCfg,	sizeof(HI_NTP_CFG_S));
		}
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__GetDynamicDNS(struct soap* soap,
                          struct _tds__GetDynamicDNS* tds__GetDynamicDNS,
                          struct _tds__GetDynamicDNSResponse* tds__GetDynamicDNSResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__SetDynamicDNS(struct soap* soap,
                          struct _tds__SetDynamicDNS* tds__SetDynamicDNS,
                          struct _tds__SetDynamicDNSResponse* tds__SetDynamicDNSResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetNetworkInterfaces(struct soap* soap,
                                 struct _tds__GetNetworkInterfaces* tds__GetNetworkInterfaces,
                                 struct _tds__GetNetworkInterfacesResponse* tds__GetNetworkInterfacesResponse)
{
	HI_DEV_NET_CFG_S stNetCfg;
	HI_U8* pMac;
	HI_U32 u32IpAddr;
	HI_U8 u8DhcpOn = 0;
	char _mac[64];
	char _IPAddr[64];
	char* pEthName = NULL;
	//int nEth = 0;
	struct tt__NetworkInterface* NetworkInterfaces;
	__FUN_BEGIN("\n");
	CHECK_USER;
	unsigned long ip;
	ALLOC_STRUCT(NetworkInterfaces, struct tt__NetworkInterface);
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	ip = htonl(hi_get_sock_ip(soap->socket));

	if(ip == stNetCfg.struEtherCfg[0].u32IpAddr)
	{
		printf("getinterface:eth0\r\n");
		pMac = stNetCfg.struEtherCfg[0].u8MacAddr;
		u32IpAddr = stNetCfg.struEtherCfg[0].u32IpAddr;
		u8DhcpOn = !!stNetCfg.struEtherCfg[0].u8DhcpOn;
		pEthName = ETH_NAME;
		//nEth = 0;
	}
	else
	{
		printf("getinterface:ra0\r\n");
		pMac = stNetCfg.struEtherCfg[1].u8MacAddr;
		u32IpAddr = stNetCfg.struEtherCfg[1].u32IpAddr;
		u8DhcpOn = !!stNetCfg.struEtherCfg[1].u8DhcpOn;
		pEthName = ETH_NAME1;
		//nEth = 1;
	}

	sprintf(_mac, "%02X:%02X:%02X:%02X:%02X:%02X", pMac[0], pMac[1], pMac[2],
	        pMac[3], pMac[4], pMac[5]);
	sprintf(_IPAddr, "%d.%d.%d.%d", (u32IpAddr >> 24) & 0xff,
	        (u32IpAddr >> 16) & 0xff, (u32IpAddr >> 8) & 0xff,
	        (u32IpAddr >> 0) & 0xff);
	NetworkInterfaces->Enabled = xsd__boolean__true_;
	ALLOC_TOKEN(NetworkInterfaces->token, pEthName);
	ALLOC_STRUCT(NetworkInterfaces->Info, struct tt__NetworkInterfaceInfo);
	ALLOC_TOKEN(NetworkInterfaces->Info->Name, pEthName);
	ALLOC_TOKEN(NetworkInterfaces->Info->HwAddress, _mac);
	ALLOC_STRUCT_INT(NetworkInterfaces->Info->MTU, int, 1500);
#if 0
	ALLOC_STRUCT(NetworkInterfaces->Link, struct tt__NetworkInterfaceLink);
	ALLOC_STRUCT(NetworkInterfaces->Link->AdminSettings,
	             struct tt__NetworkInterfaceConnectionSetting);

	switch(stNetCfg.struEtherCfg[nEth].u8NetcardSpeed == 0)
	{
		case 0:
			NetworkInterfaces->Link->AdminSettings->AutoNegotiation = xsd__boolean__true_;
			NetworkInterfaces->Link->AdminSettings->Speed = 100;
			NetworkInterfaces->Link->AdminSettings->Duplex = tt__Duplex__Full;
			break;

		case 1:
			NetworkInterfaces->Link->AdminSettings->AutoNegotiation = xsd__boolean__false_;
			NetworkInterfaces->Link->AdminSettings->Speed = 100;
			NetworkInterfaces->Link->AdminSettings->Duplex = tt__Duplex__Full;
			break;

		case 2:
			NetworkInterfaces->Link->AdminSettings->AutoNegotiation = xsd__boolean__false_;
			NetworkInterfaces->Link->AdminSettings->Speed = 10;
			NetworkInterfaces->Link->AdminSettings->Duplex = tt__Duplex__Full;
			break;
	}

	ALLOC_STRUCT(NetworkInterfaces->Link->OperSettings,
	             struct tt__NetworkInterfaceConnectionSetting);
	NetworkInterfaces->Link->OperSettings->AutoNegotiation = xsd__boolean__true_;
	NetworkInterfaces->Link->OperSettings->Speed = 0;
	NetworkInterfaces->Link->OperSettings->Duplex = tt__Duplex__Full;
#else
	NetworkInterfaces->Link = NULL;
#endif
	ALLOC_STRUCT(NetworkInterfaces->IPv4, struct tt__IPv4NetworkInterface);
	NetworkInterfaces->IPv4->Enabled = xsd__boolean__true_;
	ALLOC_STRUCT(NetworkInterfaces->IPv4->Config, struct tt__IPv4Configuration);
	NetworkInterfaces->IPv4->Config->__sizeManual = 1;
	ALLOC_STRUCT(NetworkInterfaces->IPv4->Config->Manual,
	             struct tt__PrefixedIPv4Address);
	ALLOC_TOKEN(NetworkInterfaces->IPv4->Config->Manual->Address, _IPAddr);
	NetworkInterfaces->IPv4->Config->Manual->PrefixLength = 24;
	NetworkInterfaces->IPv4->Config->LinkLocal = NULL;
	NetworkInterfaces->IPv4->Config->FromDHCP = NULL;
	NetworkInterfaces->IPv4->Config->DHCP = u8DhcpOn;
	NetworkInterfaces->IPv4->Config->__size = 0;
	NetworkInterfaces->IPv4->Config->__any = NULL;
	NetworkInterfaces->IPv4->Config->__anyAttribute = NULL;
	NetworkInterfaces->IPv6 = NULL;
	NetworkInterfaces->Extension = NULL;
	NetworkInterfaces->__anyAttribute = NULL;
	tds__GetNetworkInterfacesResponse->__sizeNetworkInterfaces = 1;
	tds__GetNetworkInterfacesResponse->NetworkInterfaces = NetworkInterfaces;
	__FUN_END("\n");
	return SOAP_OK;
}

HI_DEV_NET_CFG_S stgNetCfg;
void* pthread_SetNetworkInterfaces(void* args)
{
	pthread_detach(pthread_self());
	sleep(1);
	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stgNetCfg, sizeof(HI_DEV_NET_CFG_S));
	return NULL;
}

int  __tds__SetNetworkInterfaces(struct soap* soap,
                                 struct _tds__SetNetworkInterfaces* tds__SetNetworkInterfaces,
                                 struct _tds__SetNetworkInterfacesResponse* tds__SetNetworkInterfacesResponse)
{
	HI_DEV_NET_CFG_S stNetCfg;
	HI_DEV_ETHERNET_CFG_S* pEthernerCfg;
	char* pEthName = NULL;
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	unsigned long ip = htonl(hi_get_sock_ip(soap->socket));

	if(ip == stNetCfg.struEtherCfg[0].u32IpAddr)
	{
		pEthernerCfg = &stNetCfg.struEtherCfg[0];
		pEthName = ETH_NAME;
	}
	else
	{
		pEthernerCfg = &stNetCfg.struEtherCfg[1];
		pEthName = ETH_NAME1;
	}

	if(tds__SetNetworkInterfaces->InterfaceToken != NULL)
	{
		printf("ethname:%s\r\n", tds__SetNetworkInterfaces->InterfaceToken);

		if(strcmp(tds__SetNetworkInterfaces->InterfaceToken, pEthName))
		{
			onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidNetworkInterface");
			return SOAP_FAULT;
		}
	}

	if(tds__SetNetworkInterfaces->NetworkInterface == NULL)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidNetworkInterface");
		return SOAP_FAULT;
	}

	/*
		if(tds__SetNetworkInterfaces->NetworkInterface->MTU != NULL)
		{
			onvif_fault(soap,"ter:InvalidArgVal", "ter:SettingMTUNotSupported");
			return SOAP_FAULT;
		}
		if(tds__SetNetworkInterfaces->NetworkInterface->Link != NULL)
		{
			onvif_fault(soap,"ter:InvalidArgVal", "ter:SettingLinkValuesNotSupported");
			return SOAP_FAULT;
		}
	*/
	/*
		if(tds__SetNetworkInterfaces->NetworkInterface->IPv6 != NULL)
		{
			if(*tds__SetNetworkInterfaces->NetworkInterface->IPv6->Enabled)
			{
				onvif_fault(soap,"ter:InvalidArgVal", "ter:IPv6NotSupported");
				return SOAP_FAULT;
			}
		}
	*/
	if(tds__SetNetworkInterfaces->NetworkInterface->IPv4 == NULL)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:IPv4ValuesMissing");
		return SOAP_FAULT;
	}

	if(tds__SetNetworkInterfaces->NetworkInterface->IPv4->Enabled != NULL)
	{
		if(tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual != NULL)
		{
			if(tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->Address != NULL)
			{
				if(isValidIp4(
				            tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->Address) ==
				        0)   // Check IP address
				{
					onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidIPv4Address");
					return SOAP_FAULT;
				}

				pEthernerCfg->u32IpAddr = hi_ip_a2n(
				                              tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->Address);
			}
		}

		if(tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP != NULL)
		{
			pEthernerCfg->u8DhcpOn =
			    *tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP;
		}

#if 0
		hi_platform_set_net_cfg(0, &stNetCfg);
#else
		pthread_t id;
		stgNetCfg = stNetCfg;
		pthread_create(&id, "onvifsetNet", NULL, pthread_SetNetworkInterfaces, NULL);
#endif
	}

	tds__SetNetworkInterfacesResponse->RebootNeeded = xsd__boolean__true_;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__GetNetworkProtocols(struct soap* soap,
                                struct _tds__GetNetworkProtocols* tds__GetNetworkProtocols,
                                struct _tds__GetNetworkProtocolsResponse* tds__GetNetworkProtocolsResponse)
{
	HI_DEV_NET_CFG_S	stNetCfg;
	struct tt__NetworkProtocol* NetworkProtocols;
	__FUN_BEGIN("\n");
	CHECK_USER;
	tds__GetNetworkProtocolsResponse->__sizeNetworkProtocols = 2;
	NetworkProtocols = (struct tt__NetworkProtocol*)soap_malloc(soap,
	                   tds__GetNetworkProtocolsResponse->__sizeNetworkProtocols * sizeof(
	                       struct tt__NetworkProtocol));

	if(NetworkProtocols == NULL)
	{
		__E("Failed to malloc for network protocols.\n");
		return SOAP_FAULT;
	}

	NetworkProtocols[0].Name = tt__NetworkProtocolType__HTTP;
	NetworkProtocols[0].Enabled = xsd__boolean__true_;
	NetworkProtocols[0].__sizePort = 1;
	NetworkProtocols[0].Port = (int*)soap_malloc(soap, sizeof(int));

	if(NetworkProtocols[0].Port == NULL)
	{
		__E("Failed to malloc for network protocols.\n");
		return SOAP_FAULT;
	}

	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	NetworkProtocols[0].Port[0] =
	    stNetCfg.u16HttpPort;//gOnvifInfo.HttpPort;//DEFAULT_ONVIF_SERVER_PORT;
	NetworkProtocols[0].Extension = NULL;
	NetworkProtocols[0].__anyAttribute = NULL;
	NetworkProtocols[1].Name = tt__NetworkProtocolType__RTSP;
	NetworkProtocols[1].Enabled = xsd__boolean__true_;
	NetworkProtocols[1].__sizePort = 1;
	NetworkProtocols[1].Port = (int*)soap_malloc(soap,
	                           NetworkProtocols[1].__sizePort * sizeof(int));

	if(NetworkProtocols[1].Port == NULL)
	{
		__E("Failed to malloc for port.\n");
		return SOAP_FAULT;
	}

	NetworkProtocols[1].Port[0] = 554;
	NetworkProtocols[1].Extension = NULL;
	NetworkProtocols[1].__anyAttribute = NULL;
	tds__GetNetworkProtocolsResponse->NetworkProtocols = NetworkProtocols;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetNetworkProtocols(struct soap* soap,
                                struct _tds__SetNetworkProtocols* tds__SetNetworkProtocols,
                                struct _tds__SetNetworkProtocolsResponse* tds__SetNetworkProtocolsResponse)
{
	int i;
	HI_DEV_NET_CFG_S stNetCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(tds__SetNetworkProtocols->NetworkProtocols == NULL)
	{
		return SOAP_FAULT;
	}

	for(i = 0; i < tds__SetNetworkProtocols->__sizeNetworkProtocols; i++)
	{
		switch(tds__SetNetworkProtocols->NetworkProtocols[0].Name)
		{
			case tt__NetworkProtocolType__HTTP:
				gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
				                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));

				if(tds__SetNetworkProtocols->NetworkProtocols[0].Enabled ==
				        xsd__boolean__false_)
				{
					return SOAP_FAULT;
				}

				if(tds__SetNetworkProtocols->NetworkProtocols[0].__sizePort > 0
				        && tds__SetNetworkProtocols->NetworkProtocols[0].Port != NULL)
				{
					stNetCfg.u16HttpPort = tds__SetNetworkProtocols->NetworkProtocols[0].Port[0];
				}

				gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_NET_PARAM, 0,
				                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
				return SOAP_OK;

			case tt__NetworkProtocolType__HTTPS:
				onvif_fault(soap, "ter:InvalidArgVal", "ter:ServiceNotSupported");
				return SOAP_FAULT;

			case tt__NetworkProtocolType__RTSP:
				return SOAP_FAULT;

			default:
				return SOAP_FAULT;
		}
	}

	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetNetworkDefaultGateway(struct soap* soap,
                                     struct _tds__GetNetworkDefaultGateway* tds__GetNetworkDefaultGateway,
                                     struct _tds__GetNetworkDefaultGatewayResponse*
                                     tds__GetNetworkDefaultGatewayResponse)
{
	HI_DEV_NET_CFG_S stNetCfg;
	HI_U32 u32Gateway = 0;
	char _GatewayAddress[HI_IP_ADDR_LEN];
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	HI_U32 ip = htonl(hi_get_sock_ip(soap->socket));

	if(ip == stNetCfg.struEtherCfg[0].u32IpAddr)
	{
		u32Gateway = stNetCfg.struEtherCfg[0].u32GateWay;
	}
	else
	{
		u32Gateway = stNetCfg.struEtherCfg[1].u32GateWay;
	}

	snprintf(_GatewayAddress, sizeof(_GatewayAddress) - 1,
	         "%d.%d.%d.%d",
	         (u32Gateway >> 24) & 0xff,
	         (u32Gateway >> 16) & 0xff,
	         (u32Gateway >> 8) & 0xff,
	         (u32Gateway >> 0) & 0xff);
	printf("GateWayAddress:%s\r\n", _GatewayAddress);
	ALLOC_STRUCT(tds__GetNetworkDefaultGatewayResponse->NetworkGateway,
	             struct tt__NetworkGateway);
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway->__sizeIPv4Address = 1;
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway->__sizeIPv6Address = 0;
	ALLOC_STRUCT(tds__GetNetworkDefaultGatewayResponse->NetworkGateway->IPv4Address,
	             char*);
	ALLOC_TOKEN(
	    tds__GetNetworkDefaultGatewayResponse->NetworkGateway->IPv4Address[0],
	    _GatewayAddress);
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetNetworkDefaultGateway(struct soap* soap,
                                     struct _tds__SetNetworkDefaultGateway* tds__SetNetworkDefaultGateway,
                                     struct _tds__SetNetworkDefaultGatewayResponse*
                                     tds__SetNetworkDefaultGatewayResponse)
{
#if 0
	HI_DEV_NET_CFG_S stNetCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(tds__SetNetworkDefaultGateway->__sizeIPv6Address ||
	        !tds__SetNetworkDefaultGateway->__sizeIPv4Address ||
	        isValidIp4(tds__SetNetworkDefaultGateway->IPv4Address) == 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidGatewayAddress");
		return SOAP_FAULT;
	}

	hi_platform_get_net_cfg(0, &stNetCfg);
	unsigned long ip = htonl(hi_get_sock_ip(soap->socket));

	if(ip == stNetCfg.struEtherCfg[0].u32IpAddr)
	{
		stNetCfg.struEtherCfg[0].u32GateWay = hi_ip_a2n(
		        tds__SetNetworkDefaultGateway->IPv4Address);
	}
	else
	{
		stNetCfg.struEtherCfg[1].u32GateWay = hi_ip_a2n(
		        tds__SetNetworkDefaultGateway->IPv4Address);
	}

	hi_platform_set_net_cfg(0, &stNetCfg);
	__FUN_END("\n");
	return SOAP_OK;
#endif
	return SOAP_OK;
}

int  __tds__GetZeroConfiguration(struct soap* soap,
                                 struct _tds__GetZeroConfiguration* tds__GetZeroConfiguration,
                                 struct _tds__GetZeroConfigurationResponse* tds__GetZeroConfigurationResponse)
{
	char _IPAddr[HI_IP_ADDR_LEN];
	HI_U32 u32IpAddr;
	HI_DEV_NET_CFG_S stNetCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	u32IpAddr = onvif_get_ipaddr(soap);
	snprintf(_IPAddr, sizeof(_IPAddr),
	         "%d.%d.%d.%d",
	         (u32IpAddr >> 24) & 0xff,
	         (u32IpAddr >> 16) & 0xff,
	         (u32IpAddr >> 8) & 0xff,
	         (u32IpAddr >> 0) & 0xff);
	ALLOC_STRUCT(tds__GetZeroConfigurationResponse->ZeroConfiguration,
	             struct tt__NetworkZeroConfiguration);
	tds__GetZeroConfigurationResponse->ZeroConfiguration->InterfaceToken = ETH_NAME;
	ALLOC_TOKEN(
	    tds__GetZeroConfigurationResponse->ZeroConfiguration->InterfaceToken, ETH_NAME);
	tds__GetZeroConfigurationResponse->ZeroConfiguration->Enabled = 1;
	tds__GetZeroConfigurationResponse->ZeroConfiguration->__sizeAddresses = 1;
	ALLOC_STRUCT(tds__GetZeroConfigurationResponse->ZeroConfiguration->Addresses,
	             char*);
	ALLOC_TOKEN(tds__GetZeroConfigurationResponse->ZeroConfiguration->Addresses[0],
	            _IPAddr);
	ALLOC_TOKEN(tds__GetZeroConfigurationResponse->ZeroConfiguration->Addresses[1],
	            LINK_LOCAL_ADDR)
	tds__GetZeroConfigurationResponse->ZeroConfiguration->Extension = NULL;
	tds__GetZeroConfigurationResponse->ZeroConfiguration->__anyAttribute = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetZeroConfiguration(struct soap* soap,
                                 struct _tds__SetZeroConfiguration* tds__SetZeroConfiguration,
                                 struct _tds__SetZeroConfigurationResponse* tds__SetZeroConfigurationResponse)
{
#if 1
	__FUN_BEGIN("\n");
	CHECK_USER;
	printf("the interface Token is %s\r\n",
	       tds__SetZeroConfiguration->InterfaceToken);
	printf("the Enabled Token is %d\r\n", tds__SetZeroConfiguration->Enabled);
	__FUN_END("\n");
	return SOAP_OK;
#else
	onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidNetworkInterface");
	__FUN_BEGIN("Onvif fault process!\r\n");
	__FUN_END("Onvif fault process!\r\n");
	return SOAP_FAULT;
#endif
}

int  __tds__GetIPAddressFilter(struct soap* soap,
                               struct _tds__GetIPAddressFilter* tds__GetIPAddressFilter,
                               struct _tds__GetIPAddressFilterResponse* tds__GetIPAddressFilterResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__SetIPAddressFilter(struct soap* soap,
                               struct _tds__SetIPAddressFilter* tds__SetIPAddressFilter,
                               struct _tds__SetIPAddressFilterResponse* tds__SetIPAddressFilterResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__AddIPAddressFilter(struct soap* soap,
                               struct _tds__AddIPAddressFilter* tds__AddIPAddressFilter,
                               struct _tds__AddIPAddressFilterResponse* tds__AddIPAddressFilterResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__RemoveIPAddressFilter(struct soap* soap,
                                  struct _tds__RemoveIPAddressFilter* tds__RemoveIPAddressFilter,
                                  struct _tds__RemoveIPAddressFilterResponse*
                                  tds__RemoveIPAddressFilterResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetAccessPolicy(struct soap* soap,
                            struct _tds__GetAccessPolicy* tds__GetAccessPolicy,
                            struct _tds__GetAccessPolicyResponse* tds__GetAccessPolicyResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__SetAccessPolicy(struct soap* soap,
                            struct _tds__SetAccessPolicy* tds__SetAccessPolicy,
                            struct _tds__SetAccessPolicyResponse* tds__SetAccessPolicyResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__CreateCertificate(struct soap* soap,
                              struct _tds__CreateCertificate* tds__CreateCertificate,
                              struct _tds__CreateCertificateResponse* tds__CreateCertificateResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetCertificates(struct soap* soap,
                            struct _tds__GetCertificates* tds__GetCertificates,
                            struct _tds__GetCertificatesResponse* tds__GetCertificatesResponse)
{
	tds__GetCertificatesResponse->__sizeNvtCertificate = 0;
	tds__GetCertificatesResponse->NvtCertificate = NULL;
	//ALLOC_STRUCT(tds__GetCertificatesResponse->NvtCertificate, struct tt__Certificate);
	return SOAP_OK;
}

int  __tds__GetCertificatesStatus(struct soap* soap,
                                  struct _tds__GetCertificatesStatus* tds__GetCertificatesStatus,
                                  struct _tds__GetCertificatesStatusResponse*
                                  tds__GetCertificatesStatusResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__SetCertificatesStatus(struct soap* soap,
                                  struct _tds__SetCertificatesStatus* tds__SetCertificatesStatus,
                                  struct _tds__SetCertificatesStatusResponse*
                                  tds__SetCertificatesStatusResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__DeleteCertificates(struct soap* soap,
                               struct _tds__DeleteCertificates* tds__DeleteCertificates,
                               struct _tds__DeleteCertificatesResponse* tds__DeleteCertificatesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetPkcs10Request(struct soap* soap,
                             struct _tds__GetPkcs10Request* tds__GetPkcs10Request,
                             struct _tds__GetPkcs10RequestResponse* tds__GetPkcs10RequestResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__LoadCertificates(struct soap* soap,
                             struct _tds__LoadCertificates* tds__LoadCertificates,
                             struct _tds__LoadCertificatesResponse* tds__LoadCertificatesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetClientCertificateMode(struct soap* soap,
                                     struct _tds__GetClientCertificateMode* tds__GetClientCertificateMode,
                                     struct _tds__GetClientCertificateModeResponse*
                                     tds__GetClientCertificateModeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__SetClientCertificateMode(struct soap* soap,
                                     struct _tds__SetClientCertificateMode* tds__SetClientCertificateMode,
                                     struct _tds__SetClientCertificateModeResponse*
                                     tds__SetClientCertificateModeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetRelayOutputs(struct soap* soap,
                            struct _tds__GetRelayOutputs* tds__GetRelayOutputs,
                            struct _tds__GetRelayOutputsResponse* tds__GetRelayOutputsResponse)
{
	struct tt__RelayOutput* RelayOutputs;
	HI_DEV_DI_CFG_S stDiCfg;
	HI_DEV_CFG_S stDevCfg = {0};
	char token[ONVIF_TOKEN_LEN];
	int i;
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_DEV_PARAM, 0,
	                     (char*)&stDevCfg, sizeof(HI_DEV_CFG_S));
	ALLOC_STRUCT_NUM(RelayOutputs, struct tt__RelayOutput, HI_MAX_SUPPORT_DI);

	for(i = 0; i < HI_MAX_SUPPORT_DI && i < stDevCfg.u8DiNum; i++)
	{
		memset(token, 0, sizeof(token));
		sprintf(token, "%s_%d", ONVIF_RELAY_OUTPUT_TOKEN, i);
		ALLOC_TOKEN(RelayOutputs[i].token, token);
		ALLOC_STRUCT(RelayOutputs[i].Properties, struct tt__RelayOutputSettings);
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_DI_PARAM, i,
		                     (char*)&stDiCfg, sizeof(HI_DEV_DI_CFG_S));
		RelayOutputs[i].Properties->Mode = tt__RelayMode__Monostable;
		RelayOutputs[i].Properties->DelayTime = 10 * 1000;
		//ALLOC_TOKEN(RelayOutputs[i].Properties->DelayTime, "PT10S");
		RelayOutputs[i].Properties->IdleState = (stDiCfg.u8DiType == 0) ?
		                                        tt__RelayIdleState__open : tt__RelayIdleState__closed;
		RelayOutputs[i].__size = 0;
		RelayOutputs[i].__any = NULL;
		RelayOutputs[i].__anyAttribute = NULL;
	}

	tds__GetRelayOutputsResponse->__sizeRelayOutputs = stDevCfg.u8DiNum;
	tds__GetRelayOutputsResponse->RelayOutputs = RelayOutputs;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetRelayOutputSettings(struct soap* soap,
                                   struct _tds__SetRelayOutputSettings* tds__SetRelayOutputSettings,
                                   struct _tds__SetRelayOutputSettingsResponse*
                                   tds__SetRelayOutputSettingsResponse)
{
	int i;
	char token[64];
	HI_DEV_DI_CFG_S stDiCfg;
	HI_DEV_CFG_S stDevCfg = {0};
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_DEV_PARAM, 0,
	                     (char*)&stDevCfg, sizeof(HI_DEV_CFG_S));

	for(i = 0; i < HI_MAX_SUPPORT_DI  && i < stDevCfg.u8DiNum; i++)
	{
		sprintf(token, "%s_%d", ONVIF_RELAY_OUTPUT_TOKEN, i);

		if(strcmp(token, tds__SetRelayOutputSettings->RelayOutputToken) == 0)
		{
			break;
		}
	}

	if(i == stDevCfg.u8DiNum)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:RelayToken");
		return SOAP_FAULT;
	}

	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_DI_PARAM, i,
	                     (char*)&stDiCfg, sizeof(HI_DEV_DI_CFG_S));
	stDiCfg.u8DiType = (tds__SetRelayOutputSettings->Properties->IdleState ==
	                    tt__RelayIdleState__open) ? 0 : 1;
	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_DI_PARAM, i,
	                     (char*)&stDiCfg, sizeof(HI_DEV_DI_CFG_S));
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tds__SetRelayOutputState(struct soap* soap,
                                struct _tds__SetRelayOutputState* tds__SetRelayOutputState,
                                struct _tds__SetRelayOutputStateResponse* tds__SetRelayOutputStateResponse)
{
	int i;
	char token[64];
	HI_DEV_CFG_S stDevCfg = {0};
	HI_ALRAMOUT_CTRL_S alarmCtrl = {0};
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_DEV_PARAM, 0,
	                     (char*)&stDevCfg, sizeof(HI_DEV_CFG_S));

	for(i = 0; i < HI_MAX_SUPPORT_DI && i < stDevCfg.u8DiNum; i++)
	{
		sprintf(token, "%s_%d", ONVIF_RELAY_OUTPUT_TOKEN, i);

		if(strcmp(token, tds__SetRelayOutputState->RelayOutputToken) == 0)
		{
			break;
		}
	}

	if(i == stDevCfg.u8DiNum)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:RelayToken");
		return SOAP_FAULT;
	}

	if(tds__SetRelayOutputState->LogicalState == tt__RelayLogicalState__active)
	{
		alarmCtrl.u32ManlStatus = 1;
	}
	else
	{
		alarmCtrl.u32ManlStatus = 0;
	}

	alarmCtrl.u32SetManl = 1;
	gOnvifDevCb.pMsgProc(NULL, HI_SYS_CTRL_MSG, HI_ALARM_OUT_CMD, 0,
	                     (char*)&alarmCtrl, sizeof(HI_ALRAMOUT_CTRL_S));
	return SOAP_OK;
}

int  __tds__SendAuxiliaryCommand(struct soap* soap,
                                 struct _tds__SendAuxiliaryCommand* tds__SendAuxiliaryCommand,
                                 struct _tds__SendAuxiliaryCommandResponse* tds__SendAuxiliaryCommandResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetCACertificates(struct soap* soap,
                              struct _tds__GetCACertificates* tds__GetCACertificates,
                              struct _tds__GetCACertificatesResponse* tds__GetCACertificatesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__LoadCertificateWithPrivateKey(struct soap* soap,
        struct _tds__LoadCertificateWithPrivateKey* tds__LoadCertificateWithPrivateKey,
        struct _tds__LoadCertificateWithPrivateKeyResponse*
        tds__LoadCertificateWithPrivateKeyResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetCertificateInformation(struct soap* soap,
                                      struct _tds__GetCertificateInformation* tds__GetCertificateInformation,
                                      struct _tds__GetCertificateInformationResponse*
                                      tds__GetCertificateInformationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__LoadCACertificates(struct soap* soap,
                               struct _tds__LoadCACertificates* tds__LoadCACertificates,
                               struct _tds__LoadCACertificatesResponse* tds__LoadCACertificatesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__CreateDot1XConfiguration(struct soap* soap,
                                     struct _tds__CreateDot1XConfiguration* tds__CreateDot1XConfiguration,
                                     struct _tds__CreateDot1XConfigurationResponse*
                                     tds__CreateDot1XConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__SetDot1XConfiguration(struct soap* soap,
                                  struct _tds__SetDot1XConfiguration* tds__SetDot1XConfiguration,
                                  struct _tds__SetDot1XConfigurationResponse*
                                  tds__SetDot1XConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetDot1XConfiguration(struct soap* soap,
                                  struct _tds__GetDot1XConfiguration* tds__GetDot1XConfiguration,
                                  struct _tds__GetDot1XConfigurationResponse*
                                  tds__GetDot1XConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetDot1XConfigurations(struct soap* soap,
                                   struct _tds__GetDot1XConfigurations* tds__GetDot1XConfigurations,
                                   struct _tds__GetDot1XConfigurationsResponse*
                                   tds__GetDot1XConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__DeleteDot1XConfiguration(struct soap* soap,
                                     struct _tds__DeleteDot1XConfiguration* tds__DeleteDot1XConfiguration,
                                     struct _tds__DeleteDot1XConfigurationResponse*
                                     tds__DeleteDot1XConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetDot11Capabilities(struct soap* soap,
                                 struct _tds__GetDot11Capabilities* tds__GetDot11Capabilities,
                                 struct _tds__GetDot11CapabilitiesResponse* tds__GetDot11CapabilitiesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetDot11Status(struct soap* soap,
                           struct _tds__GetDot11Status* tds__GetDot11Status,
                           struct _tds__GetDot11StatusResponse* tds__GetDot11StatusResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__ScanAvailableDot11Networks(struct soap* soap,
                                       struct _tds__ScanAvailableDot11Networks* tds__ScanAvailableDot11Networks,
                                       struct _tds__ScanAvailableDot11NetworksResponse*
                                       tds__ScanAvailableDot11NetworksResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__GetSystemUris(struct soap* soap,
                          struct _tds__GetSystemUris* tds__GetSystemUris,
                          struct _tds__GetSystemUrisResponse* tds__GetSystemUrisResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__StartFirmwareUpgrade(struct soap* soap,
                                 struct _tds__StartFirmwareUpgrade* tds__StartFirmwareUpgrade,
                                 struct _tds__StartFirmwareUpgradeResponse* tds__StartFirmwareUpgradeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tds__StartSystemRestore(struct soap* soap,
                               struct _tds__StartSystemRestore* tds__StartSystemRestore,
                               struct _tds__StartSystemRestoreResponse* tds__StartSystemRestoreResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

static int __waitforEvent(int nTimeout, HI_ALARM_INFO_S* pstAlarm)
{
	int ret = 0;
	time_t t1;
	t1 = time(NULL);

	while(1)
	{
		if (time(NULL) - t1 >= nTimeout)
		{
			ret = -1;
			break;
		}

		ret = _onvif_getAlarm(pstAlarm);

		if (ret != 0)
		{
			sleep(1);
			continue;
		}

		return 0;
	}

	return ret;
}

int  __tev__PullMessages(struct soap* soap,
                         struct _tev__PullMessages* tev__PullMessages,
                         struct _tev__PullMessagesResponse* tev__PullMessagesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	int nRet = 0;
	HI_ALARM_INFO_S stAlarm;
	tev_topic_t* pstTopic = NULL;
	tev_Subscription_t* pSubscription = NULL;

	if (tev__PullMessages == NULL)
	{
		return SOAP_FAULT;
	}

	int nTimeout = tev__PullMessages->Timeout;
	///////////20140904 ZJ///////////////
	char* tmp = NULL;

	if(soap->header != NULL)
		if(soap->header->wsa5__To != NULL)
		{
			tmp = strstr(soap->header->wsa5__To, "?subscribe=");
		}

	/////////////END////////////////////
	if (tmp == NULL)
	{
		return SOAP_FAULT;
	}

	nTimeout = 20;
	tmp += strlen("?subscribe=");
	int id = atoi(tmp);
	printf("nTimeout = %d\r\n", nTimeout);
	nRet = __waitforEvent(nTimeout, &stAlarm);
	printf("__waitforEvent ret= %d, type = %d\r\n", nRet, stAlarm.u32AlarmType);
	pSubscription = tev_GetSubscriptionFromManager(&g_tev_SubscriptionManager, id);

	if (pSubscription == NULL)
	{
		return SOAP_FAULT;
	}

	tev__PullMessagesResponse->CurrentTime = time(NULL);
	tev__PullMessagesResponse->TerminationTime = pSubscription->tick +
	        tev__PullMessagesResponse->CurrentTime;

	if (nRet == 0)
	{
		time_t   b_time;
		struct   tm*   tim;
		char* topicAny = NULL;

		if (stAlarm.u32AlarmType == HI_MD_ALARM_HAPPEN)
		{
			topicAny = "MotionAlarm";
			pstTopic = &g_tev_TopicSet[0];
		}
		else if (stAlarm.u32AlarmType == HI_DI_ALARM_HAPPEN)
		{
			topicAny = "IoAlarm";
			pstTopic = &g_tev_TopicSet[1];
		}
		else
		{
			return SOAP_FAULT;
		}

		b_time = time(NULL);
		tim = localtime(&b_time);
		tev__PullMessagesResponse->__sizeNotificationMessage = 1;
		tev__PullMessagesResponse->wsnt__NotificationMessage = soap_malloc(soap,
		        1 * sizeof(struct wsnt__NotificationMessageHolderType));
		tev__PullMessagesResponse->wsnt__NotificationMessage[0].SubscriptionReference =
		    NULL;
		tev__PullMessagesResponse->wsnt__NotificationMessage[0].ProducerReference =
		    NULL;
		tev__PullMessagesResponse->wsnt__NotificationMessage[0].Topic = soap_malloc(
		            soap, sizeof(struct wsnt__TopicExpressionType));
		tev__PullMessagesResponse->wsnt__NotificationMessage[0].Topic->Dialect =
		    soap_malloc(soap, 128);
		tev__PullMessagesResponse->wsnt__NotificationMessage[0].Topic->__any =
		    soap_malloc(soap, 128);
		tev__PullMessagesResponse->wsnt__NotificationMessage[0].Topic->__anyAttribute =
		    NULL;
		tev__PullMessagesResponse->wsnt__NotificationMessage[0].Topic->__mixed = NULL;
		strcpy(tev__PullMessagesResponse->wsnt__NotificationMessage[0].Topic->Dialect,
		       "http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet");
		strcpy(tev__PullMessagesResponse->wsnt__NotificationMessage[0].Topic->__any,
		       topicAny);
		tev__PullMessagesResponse->wsnt__NotificationMessage[0].Message.__any =
		    soap_malloc(soap, 1024);
		sprintf(tev__PullMessagesResponse->wsnt__NotificationMessage[0].Message.__any,
		        "<tt:Message UtcTime=\"%d-%02d-%02dT%02d:%02d:%02d.%d\" PropertyOperation=\"Initialized\">"
		        "<tt:Source>"
		        "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>"
		        "</tt:Source>"
		        "<tt:Data>"
		        "<tt:SimpleItem Name=\"%s\" Value=\"%s\" />"
		        "</tt:Data>"
		        "</tt:Message>",
		        (tim->tm_year + 1900), (tim->tm_mon + 1), tim->tm_mday, tim->tm_hour,
		        tim->tm_min, tim->tm_sec, 100,
		        pstTopic->source.name, pstTopic->source.type, pstTopic->data.name,
		        pstTopic->data.type);
	}

	if (soap->header)
	{
		soap_default_SOAP_ENV__Header(soap, soap->header);
		soap->header->wsa5__Action = (char*)
		                             "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesResponse";
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tev__Seek(struct soap* soap, struct _tev__Seek* tev__Seek,
                 struct _tev__SeekResponse* tev__SeekResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tev__SetSynchronizationPoint(struct soap* soap,
                                    struct _tev__SetSynchronizationPoint* tev__SetSynchronizationPoint,
                                    struct _tev__SetSynchronizationPointResponse*
                                    tev__SetSynchronizationPointResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	int nStream = 0;
	gOnvifDevCb.pStreamReqIframe(0, nStream);
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tev__GetServiceCapabilities(struct soap* soap,
                                   struct _tev__GetServiceCapabilities* tev__GetServiceCapabilities,
                                   struct _tev__GetServiceCapabilitiesResponse*
                                   tev__GetServiceCapabilitiesResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int	__tev__CreatePullPointSubscription(struct soap* soap,
                                       struct _tev__CreatePullPointSubscription* tev__CreatePullPointSubscription,
                                       struct _tev__CreatePullPointSubscriptionResponse*
                                       tev__CreatePullPointSubscriptionResponse)
{
	int nSubscribe;
	time_t nTimeOut = -1;
	struct _tev__CreatePullPointSubscription* ns1__CreatePullPointSubscription;
	struct _tev__CreatePullPointSubscriptionResponse*
		ns1__CreatePullPointSubscriptionResponse;
	__FUN_BEGIN("\r\n");
	ns1__CreatePullPointSubscription = tev__CreatePullPointSubscription;
	ns1__CreatePullPointSubscriptionResponse =
	    tev__CreatePullPointSubscriptionResponse;

	if(tev__CreatePullPointSubscription->InitialTerminationTime != NULL)
	{
		PullPoint_TimeOut = __ns11__Parse_Duration_Type(
		                        tev__CreatePullPointSubscription->InitialTerminationTime);
	}

	if(ns1__CreatePullPointSubscription->InitialTerminationTime)
	{
		int InitialTerminationTime;

		if((InitialTerminationTime = __ns11__Parse_Duration_Type(
		                                 ns1__CreatePullPointSubscription->InitialTerminationTime)))
		{
			time_t nowtime = time(NULL);
			ns1__CreatePullPointSubscriptionResponse->wsnt__CurrentTime = nowtime;
			ns1__CreatePullPointSubscriptionResponse->wsnt__TerminationTime = nowtime +
			        InitialTerminationTime;
			nTimeOut = InitialTerminationTime;
		}
		else if((InitialTerminationTime = __ns11__Parse_DateTime_Type(
		                                      ns1__CreatePullPointSubscription->InitialTerminationTime)))
		{
			time_t nowtime = time(NULL);

			if(InitialTerminationTime > nowtime)
			{
				ns1__CreatePullPointSubscriptionResponse->wsnt__CurrentTime = nowtime;
				ns1__CreatePullPointSubscriptionResponse->wsnt__TerminationTime =
				    InitialTerminationTime;
				nTimeOut = InitialTerminationTime - nowtime;
			}
			else
			{
				return SOAP_FAULT;
			}
		}
	}

	//动态创建SubscriptionManager
	nSubscribe = tev_InsertSubscriptionToManager(&g_tev_SubscriptionManager, NULL,
	             nTimeOut);

	if(nSubscribe)
	{
		char localIP[128];
		unsigned long ip;
		ip = onvif_get_ipaddr(soap);
		hi_ip_n2a(ip, localIP, 128);
		ns1__CreatePullPointSubscriptionResponse->SubscriptionReference.Address =
		    soap_malloc(soap, sizeof(char) * 64);
		sprintf(ns1__CreatePullPointSubscriptionResponse->SubscriptionReference.Address,
		        "http://%s:%d/onvif/event_service?subscribe=%d",
		        localIP, 80, nSubscribe);

		if (soap->header)
		{
			soap_default_SOAP_ENV__Header(soap, soap->header);
			soap->header->wsa5__Action = (char*)
			                             "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionResponse";
		}
	}
	else
	{
		return SOAP_FAULT;
	}

	__FUN_END("\r\n");
	return SOAP_OK;
}

int	__tev__GetEventProperties(struct soap* soap,
                              struct _tev__GetEventProperties* tev__GetEventProperties,
                              struct _tev__GetEventPropertiesResponse* tev__GetEventPropertiesResponse)
{
	__FUN_BEGIN("\r\n");
	tev__GetEventPropertiesResponse->__sizeTopicNamespaceLocation =
	    g_tev_TopicNamespaceLocationNum;
	tev__GetEventPropertiesResponse->TopicNamespaceLocation =
	    g_tev_TopicNamespaceLocation;
	tev__GetEventPropertiesResponse->wsnt__FixedTopicSet = xsd__boolean__true_;
	tev__GetEventPropertiesResponse->wstop__TopicSet = (struct wstop__TopicSetType*)
	        soap_malloc(soap, sizeof(struct wstop__TopicSetType));
	tev__GetEventPropertiesResponse->wstop__TopicSet->documentation = NULL;
	tev__GetEventPropertiesResponse->wstop__TopicSet->__size = g_tev_TopicNum;
	tev__GetEventPropertiesResponse->wstop__TopicSet->__any = (char**)soap_malloc(
	            soap, sizeof(char*)*g_tev_TopicNum);
	tev__GetEventPropertiesResponse->wstop__TopicSet->__anyAttribute = NULL;
	int i = 0;

	for (i = 0; i < g_tev_TopicNum; i++)
	{
		tev__GetEventPropertiesResponse->wstop__TopicSet->__any[i] = (char*)soap_malloc(
		            soap, 1024);//DEV_LINE_LEN);
		memset(tev__GetEventPropertiesResponse->wstop__TopicSet->__any[i], 0,
		       DEV_LINE_LEN);
#if 0
		snprintf(tev__GetEventPropertiesResponse->wstop__TopicSet->__any[i],
		         DEV_LINE_LEN - 1,
		         "<%s wstop:topic=\"true\">"
		         "<tt:MessageDescription>"
		         "<tt:Source>"
		         "<tt:SimpleItem Name=\"%s\" Type=\"%s\" />"
		         "</tt:Source>"
		         "<tt:Data>"
		         "<tt:SimpleItem Name=\"%s\" Type=\"%s\" />"
		         "</tt:Data>"
		         "<tt:Key>"
		         "<tt:SimpleItem Name=\"%s\" Type=\"%s\" />"
		         "</tt:Key>"
		         "</tt:MessageDescription>"
		         "</%s>",
		         g_tev_TopicSet[i].topic,
		         g_tev_TopicSet[i].source.name, g_tev_TopicSet[i].source.type,
		         g_tev_TopicSet[i].data.name, g_tev_TopicSet[i].data.type,
		         g_tev_TopicSet[i].key.name, g_tev_TopicSet[i].key.type,
		         g_tev_TopicSet[i].topic);
#else
		strcpy(tev__GetEventPropertiesResponse->wstop__TopicSet->__any[i],
		       topic_set[i]);
#endif
	}

	tev__GetEventPropertiesResponse->__sizeTopicExpressionDialect =
	    g_tev_TopicExpressionDialectNum;
	tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect =
	    g_tev_TopicExpressionDialect;
	tev__GetEventPropertiesResponse->__sizeMessageContentFilterDialect =
	    g_tev_MessageContentFilterDialectNum;
	tev__GetEventPropertiesResponse->MessageContentFilterDialect =
	    g_tev_MessageContentFilterDialect;
	tev__GetEventPropertiesResponse->__sizeProducerPropertiesFilterDialect = 0;
	tev__GetEventPropertiesResponse->ProducerPropertiesFilterDialect = NULL;
	tev__GetEventPropertiesResponse->__sizeMessageContentSchemaLocation =
	    g_tev_MessageContentSchemaLocationNum;
	tev__GetEventPropertiesResponse->MessageContentSchemaLocation =
	    g_tev_MessageContentSchemaLocation;
	tev__GetEventPropertiesResponse->__size = 0;
	tev__GetEventPropertiesResponse->__any = NULL;

	if (soap->header)
	{
		soap_default_SOAP_ENV__Header(soap, soap->header);
		soap->header->wsa5__Action = (char*)
		                             "http://www.onvif.org/ver10/events/wsdl/EventPortType/GetEventPropertiesResponse";
	}

	__FUN_END("\r\n");
	return SOAP_OK;
}

int  __tev__Renew(struct soap* soap, struct _wsnt__Renew* wsnt__Renew,
                  struct _wsnt__RenewResponse* wsnt__RenewResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	tev_Subscription_t* pstSubscription = NULL;
	int nPostpone = 0;

	if (soap->header == NULL || soap->header->wsa5__To == NULL)
	{
		return SOAP_FAULT;
	}

	printf("%s\r\n", soap->header->wsa5__To);
	char* tmp = strstr(soap->header->wsa5__To, "device_service/");

	if (tmp == NULL)
	{
		return SOAP_FAULT;
	}

	if(wsnt__Renew->TerminationTime)
	{
		//nPostpone = __ns11__Parse_Duration_Type(wsnt__Renew->TerminationTime);
	}

	tmp += strlen("device_service/");
	int id = atoi(tmp);
	nPostpone = 60;

	if (tev_RenewSubscriptionFromManager(&g_tev_SubscriptionManager, id,
	                                     nPostpone) < 0)
	{
		return SOAP_FAULT;
	}

	pstSubscription = tev_GetSubscriptionFromManager(&g_tev_SubscriptionManager,
	                  id);

	if (pstSubscription == NULL)
	{
		return SOAP_FAULT;
	}

	printf("renew postpone %d, tick = %d\r\n", nPostpone, pstSubscription->tick);
	wsnt__RenewResponse->CurrentTime = (time_t*)soap_malloc(soap, sizeof(time_t));
	*wsnt__RenewResponse->CurrentTime = time(NULL);
	wsnt__RenewResponse->TerminationTime = *wsnt__RenewResponse->CurrentTime +
	                                       pstSubscription->tick;

	if (soap->header)
	{
		soap_default_SOAP_ENV__Header(soap, soap->header);
		soap->header->wsa5__Action = (char*)
		                             "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewResponse";
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tev__Unsubscribe(struct soap* soap,
                        struct _wsnt__Unsubscribe* wsnt__Unsubscribe,
                        struct _wsnt__UnsubscribeResponse* wsnt__UnsubscribeResponse)
{
	if (soap->header == NULL || soap->header->wsa5__To == NULL)
	{
		return SOAP_FAULT;
	}

	char* tmp = strstr(soap->header->wsa5__To, "device_service/");

	if (tmp == NULL)
	{
		return SOAP_FAULT;
	}

	tmp += strlen("device_service/");
	int id = atoi(tmp);

	if (tev_DeleteSubscriptionFromManager(&g_tev_SubscriptionManager, id) < 0)
	{
		return SOAP_FAULT;
	}

	wsnt__UnsubscribeResponse->__size = 0;
	wsnt__UnsubscribeResponse->__any = NULL;

	if (soap->header)
	{
		soap_default_SOAP_ENV__Header(soap, soap->header);
		soap->header->wsa5__Action = (char*)
		                             "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/UnsubscribeResponse";
	}

	return SOAP_OK;
}

int  __tev__Subscribe(struct soap* soap,
                      struct _wsnt__Subscribe* wsnt__Subscribe,
                      struct _wsnt__SubscribeResponse* wsnt__SubscribeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	int nSubscribe;
	time_t nTimeOut = -1;

	if(wsnt__Subscribe->InitialTerminationTime)
	{
		int InitialTerminationTime;

		if((InitialTerminationTime = __ns11__Parse_Duration_Type(
		                                 wsnt__Subscribe->InitialTerminationTime)))
		{
			time_t nowtime = time(NULL);
			wsnt__SubscribeResponse->CurrentTime = (time_t*)soap_malloc(soap,
			                                       sizeof(time_t));
			wsnt__SubscribeResponse->TerminationTime = (time_t*)soap_malloc(soap,
			        sizeof(time_t));
			*wsnt__SubscribeResponse->CurrentTime = nowtime;
			*wsnt__SubscribeResponse->TerminationTime = nowtime + InitialTerminationTime;
			nTimeOut = InitialTerminationTime;
		}
		else if((InitialTerminationTime = __ns11__Parse_DateTime_Type(
		                                      wsnt__Subscribe->InitialTerminationTime)))
		{
			time_t nowtime = time(NULL);

			if(InitialTerminationTime > nowtime)
			{
				wsnt__SubscribeResponse->CurrentTime = (time_t*)soap_malloc(soap,
				                                       sizeof(time_t));
				wsnt__SubscribeResponse->TerminationTime = (time_t*)soap_malloc(soap,
				        sizeof(time_t));
				*wsnt__SubscribeResponse->CurrentTime = nowtime;
				*wsnt__SubscribeResponse->TerminationTime = InitialTerminationTime;
				nTimeOut = InitialTerminationTime - nowtime;
			}
			else
			{
				return SOAP_FAULT;
			}
		}
	}

	nSubscribe = tev_InsertSubscriptionToManager(&g_tev_SubscriptionManager,
	             wsnt__Subscribe->ConsumerReference.Address, nTimeOut);

	if(nSubscribe)
	{
		char localIP[128];
		unsigned long ip;
		ip = onvif_get_ipaddr(soap);
		hi_ip_n2a(ip, localIP, 128);
		wsnt__SubscribeResponse->SubscriptionReference.Address = soap_malloc(soap,
		        sizeof(char) * 64);
		sprintf(wsnt__SubscribeResponse->SubscriptionReference.Address,
		        "http://%s:%d/onvif/device_service/%d",
		        localIP, 80, nSubscribe);

		if (soap->header)
		{
			soap_default_SOAP_ENV__Header(soap, soap->header);
			soap->header->wsa5__Action = (char*)
			                             "http://docs.oasis-open.org/wsn/bw-2/NotificationProducer/SubscribeResponse";
		}
	}
	else
	{
		return SOAP_FAULT;
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tev__GetCurrentMessage(struct soap* soap,
                              struct _wsnt__GetCurrentMessage* wsnt__GetCurrentMessage,
                              struct _wsnt__GetCurrentMessageResponse* wsnt__GetCurrentMessageResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__Notify(struct soap* soap, struct _wsnt__Notify* wsnt__Notify)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__GetMessages(struct soap* soap,
                        struct _wsnt__GetMessages* wsnt__GetMessages,
                        struct _wsnt__GetMessagesResponse* wsnt__GetMessagesResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__DestroyPullPoint(struct soap* soap,
                             struct _wsnt__DestroyPullPoint* wsnt__DestroyPullPoint,
                             struct _wsnt__DestroyPullPointResponse* wsnt__DestroyPullPointResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__Notify_(struct soap* soap, struct _wsnt__Notify* wsnt__Notify)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__CreatePullPoint(struct soap* soap,
                            struct _wsnt__CreatePullPoint* wsnt__CreatePullPoint,
                            struct _wsnt__CreatePullPointResponse* wsnt__CreatePullPointResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__Renew_(struct soap* soap, struct _wsnt__Renew* wsnt__Renew,
                   struct _wsnt__RenewResponse* wsnt__RenewResponse)
{
	//postpone TerminationTime
	__FUN_BEGIN("Temperarily no process!\r\n");
	printf("%s\r\n", wsnt__Renew->TerminationTime);
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__Unsubscribe_(struct soap* soap,
                         struct _wsnt__Unsubscribe* wsnt__Unsubscribe,
                         struct _wsnt__UnsubscribeResponse* wsnt__UnsubscribeResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__PauseSubscription(struct soap* soap,
                              struct _wsnt__PauseSubscription* wsnt__PauseSubscription,
                              struct _wsnt__PauseSubscriptionResponse* wsnt__PauseSubscriptionResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __tev__ResumeSubscription(struct soap* soap,
                               struct _wsnt__ResumeSubscription* wsnt__ResumeSubscription,
                               struct _wsnt__ResumeSubscriptionResponse* wsnt__ResumeSubscriptionResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_FAULT;
}

int  __timg__GetServiceCapabilities(struct soap* soap,
                                    struct _timg__GetServiceCapabilities* timg__GetServiceCapabilities,
                                    struct _timg__GetServiceCapabilitiesResponse*
                                    timg__GetServiceCapabilitiesResponse)
{
	return SOAP_FAULT;
}

int  __timg__GetImagingSettings(struct soap* soap,
                                struct _timg__GetImagingSettings* timg__GetImagingSettings,
                                struct _timg__GetImagingSettingsResponse* timg__GetImagingSettingsResponse)
{
	__FUN_BEGIN("\r\n");
	struct tt__ImagingSettings20* timg__ImagingSettings;
	HI_DEV_IMA_CFG_S stIma;
	HI_DEV_3A_CFG_S st3a;
	float Brightness = 0;
	float Contrast = 0;
	float Saturation = 0;
	float Sharpness = 0;
	//enum tt__IrCutFilterMode IrCutFilterMode;

	if(timg__GetImagingSettings->VideoSourceToken == NULL)
	{
		return SOAP_FAULT;
	}

	if(timg__GetImagingSettings)
	{
		if(strcmp(timg__GetImagingSettings->VideoSourceToken,
		          ONVIF_MEDIA_VIDEO_SOURCE_TOKEN) != 0)
		{
			return SOAP_FAULT;
		}
	}

	memset(&stIma, 0, sizeof(HI_DEV_IMA_CFG_S));
	memset(&st3a, 0, sizeof(HI_DEV_3A_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_IMA_PARAM, 0,
	                     (char*)&stIma, sizeof(HI_DEV_IMA_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_3A_PARAM, 0,
	                     (char*)&st3a, sizeof(HI_DEV_3A_CFG_S));

	if((timg__ImagingSettings = (struct tt__ImagingSettings20*)soap_malloc(soap,
	                            sizeof(struct tt__ImagingSettings20))) == NULL)
	{
		__E("Failed to malloc for ImagingSettings.\n");
		return SOAP_FAULT;
	}

	timg__ImagingSettings->BacklightCompensation = NULL;
	timg__ImagingSettings->Brightness = (float*)soap_malloc(soap, sizeof(float));

	if(timg__ImagingSettings->Brightness == NULL)
	{
		__E("Failed to malloc for Brightness.\n");
		return SOAP_FAULT;
	}

	*timg__ImagingSettings->Brightness = 0.0f;	//20140909 zj

	if((timg__ImagingSettings->Contrast = (float*)soap_malloc(soap,
	                                      sizeof(float))) == NULL)
	{
		__E("Failed to malloc for Contrast.\n");
		return SOAP_FAULT;
	}

	*timg__ImagingSettings->Contrast = 0.0f;	//20140909 zj

	if((timg__ImagingSettings->ColorSaturation = (float*)soap_malloc(soap,
	        sizeof(float))) == NULL)
	{
		__E("Failed to malloc for ColorSaturation.\n");
		return SOAP_FAULT;
	}

	*timg__ImagingSettings->ColorSaturation = 0.0f;	//20140909 zj

	if((timg__ImagingSettings->Sharpness = (float*)soap_malloc(soap,
	                                       sizeof(float))) == NULL)
	{
		__E("Failed to malloc for Sharpness.\n");
		return SOAP_FAULT;
	}

	*timg__ImagingSettings->Sharpness = 0.0f;	//20140909 zj

	if(stIma.eSuppMask & VCT_IMA_BRIGHTNESS)
	{
		Brightness = (float)stIma.struParam[IMA_BRIGHTNESS_TYPE].u32Value;
		*timg__ImagingSettings->Brightness = Brightness;
	}

	if(stIma.eSuppMask & VCT_IMA_CONTRAST)
	{
		Contrast = (float)stIma.struParam[IMA_CONTRAST_TYPE].u32Value;
		*timg__ImagingSettings->Contrast = Contrast;
	}

	if(stIma.eSuppMask & VCT_IMA_SATURATION)
	{
		Saturation = (float)stIma.struParam[IMA_SATURATION_TYPE].u32Value;
		*timg__ImagingSettings->ColorSaturation  = 	Saturation;
	}

	if(stIma.eSuppMask & VCT_IMA_SHARPNESS)
	{
		Sharpness = (float)stIma.struParam[IMA_SHARPNESS_TYPE].u32Value;
		*timg__ImagingSettings->Sharpness  = Sharpness;
	}

	timg__ImagingSettings->Exposure = NULL;
	timg__ImagingSettings->Focus = NULL;
#if 0

	if(st3a.eSuppMask & VCT_3A_IRCFMODE)
	{
		if(st3a.struIrcfMode.u8Value == 0)
		{
			IrCutFilterMode = tt__IrCutFilterMode__AUTO;
		}

		if(st3a.struIrcfMode.u8Value == 1)
		{
			IrCutFilterMode = tt__IrCutFilterMode__OFF;
		}

		if(st3a.struIrcfMode.u8Value == 2)
		{
			IrCutFilterMode = tt__IrCutFilterMode__ON;
		}

		timg__ImagingSettings->IrCutFilter = (enum tt__IrCutFilterMode*)soap_malloc(
		        soap, sizeof(enum tt__IrCutFilterMode));
		*timg__ImagingSettings->IrCutFilter = IrCutFilterMode;
	}

#else
	timg__ImagingSettings->IrCutFilter = NULL;
#endif
	timg__ImagingSettings->WideDynamicRange = NULL;
	timg__ImagingSettings->WhiteBalance = NULL;
	timg__ImagingSettings->Extension = NULL;
	timg__ImagingSettings->__anyAttribute = NULL;
	timg__GetImagingSettingsResponse->ImagingSettings  = timg__ImagingSettings;
	__FUN_END("\r\n");
	return SOAP_OK;
}

int  __timg__SetImagingSettings(struct soap* soap,
                                struct _timg__SetImagingSettings* timg__SetImagingSettings,
                                struct _timg__SetImagingSettingsResponse* timg__SetImagingSettingsResponse)
{
	__FUN_BEGIN("\r\n");
	HI_DEV_IMA_CFG_S stIma;
	HI_DEV_3A_CFG_S st3a;
	float Brightness = 0;
	float Contrast = 0;
	float Saturation = 0;
	float Sharpness = 0;
	//	enum tt__IrCutFilterMode IrCutFilterMode;

	if(timg__SetImagingSettings->VideoSourceToken == NULL)
	{
		return SOAP_FAULT;
	}

	if(timg__SetImagingSettings)
	{
		if(strcmp(timg__SetImagingSettings->VideoSourceToken,
		          ONVIF_MEDIA_VIDEO_SOURCE_TOKEN) != 0)
		{
			return SOAP_FAULT;
		}
	}

	memset(&stIma, 0, sizeof(HI_DEV_IMA_CFG_S));
	memset(&st3a, 0, sizeof(HI_DEV_3A_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_IMA_PARAM, 0,
	                     (char*)&stIma, sizeof(HI_DEV_IMA_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_3A_PARAM, 0,
	                     (char*)&st3a, sizeof(HI_DEV_3A_CFG_S));

	if(timg__SetImagingSettings->ImagingSettings)
	{
		if(timg__SetImagingSettings->ImagingSettings->Brightness)
		{
			if(ONVIF_IMAGING_MAX < *timg__SetImagingSettings->ImagingSettings->Brightness ||
			        *timg__SetImagingSettings->ImagingSettings->Brightness < ONVIF_IMAGING_MIN)
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:SettingsInvalid");
				return SOAP_FAULT;
			}

			Brightness = *timg__SetImagingSettings->ImagingSettings->Brightness;
			stIma.struParam[IMA_BRIGHTNESS_TYPE].u32Value = (int)Brightness;
		}

		if(timg__SetImagingSettings->ImagingSettings->Contrast)
		{
			if(ONVIF_IMAGING_MAX < *timg__SetImagingSettings->ImagingSettings->Contrast ||
			        *timg__SetImagingSettings->ImagingSettings->Contrast < ONVIF_IMAGING_MIN)
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:SettingsInvalid");
				return SOAP_FAULT;
			}

			Contrast = *timg__SetImagingSettings->ImagingSettings->Contrast;
			stIma.struParam[IMA_CONTRAST_TYPE].u32Value = (int)Contrast;
		}

		if(timg__SetImagingSettings->ImagingSettings->ColorSaturation)
		{
			if(ONVIF_IMAGING_MAX <
			        *timg__SetImagingSettings->ImagingSettings->ColorSaturation ||
			        *timg__SetImagingSettings->ImagingSettings->ColorSaturation <
			        ONVIF_IMAGING_MIN)
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:SettingsInvalid");
				return SOAP_FAULT;
			}

			Saturation = *timg__SetImagingSettings->ImagingSettings->ColorSaturation;
			stIma.struParam[IMA_SATURATION_TYPE].u32Value = (int)Saturation;
		}

		if(timg__SetImagingSettings->ImagingSettings->Sharpness)
		{
			if(ONVIF_IMAGING_MAX < *timg__SetImagingSettings->ImagingSettings->Sharpness ||
			        *timg__SetImagingSettings->ImagingSettings->Sharpness < ONVIF_IMAGING_MIN)
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:SettingsInvalid");
				return SOAP_FAULT;
			}

			Sharpness = *timg__SetImagingSettings->ImagingSettings->Sharpness;
			stIma.struParam[IMA_SHARPNESS_TYPE].u32Value = (int)Sharpness;
		}

		if(timg__SetImagingSettings->ImagingSettings->IrCutFilter)
		{
#if 0
			IrCutFilterMode = *timg__SetImagingSettings->ImagingSettings->IrCutFilter;

			if(IrCutFilterMode == tt__IrCutFilterMode__AUTO)
			{
				st3a.struIrcfMode.u8Value = 0;
			}
			else if(IrCutFilterMode == tt__IrCutFilterMode__OFF)
			{
				st3a.struIrcfMode.u8Value = 1;
			}
			else if(IrCutFilterMode == tt__IrCutFilterMode__ON)
			{
				st3a.struIrcfMode.u8Value = 2;
			}
			else
			{
				onvif_fault(soap, "ter:InvalidArgVal", "ter:SettingsInvalid");
				return SOAP_FAULT;
			}

#endif
		}

		gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_IMA_PARAM, 0,
		                     (char*)&stIma, sizeof(HI_DEV_IMA_CFG_S));
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_3A_PARAM, 0,
		                     (char*)&st3a, sizeof(HI_DEV_3A_CFG_S));
	}

	return SOAP_OK;
	onvif_fault(soap, "ter:ActionNotSupported", "ter:NoImagingForSource");
	return SOAP_FAULT;
	__FUN_END("\r\n");
}

int  __timg__GetOptions(struct soap* soap,
                        struct _timg__GetOptions* timg__GetOptions,
                        struct _timg__GetOptionsResponse* timg__GetOptionsResponse)
{
	struct tt__ImagingOptions20* ImagingOptions;
	HI_DEV_3A_CFG_S st3a;
	float Max = ONVIF_IMAGING_MAX;
	float Min = ONVIF_IMAGING_MIN;
	//enum tt__IrCutFilterMode IrCutFilterMode;
	__FUN_BEGIN("\r\n");

	if(timg__GetOptions->VideoSourceToken == NULL)
	{
		return SOAP_FAULT;
	}

	if(timg__GetOptions)
	{
		if(strcmp(timg__GetOptions->VideoSourceToken,
		          ONVIF_MEDIA_VIDEO_SOURCE_TOKEN) != 0)
		{
			//onvif_fault(soap,"ter:InvalidArgVal", "ter:NoSource");
			return SOAP_FAULT;
		}
	}

	memset(&st3a, 0, sizeof(HI_DEV_3A_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_3A_PARAM, 0,
	                     (char*)&st3a, sizeof(HI_DEV_3A_CFG_S));

	if((ImagingOptions = (struct tt__ImagingOptions20*)soap_malloc(soap,
	                     sizeof(struct tt__ImagingOptions20))) == NULL)
	{
		__E("Failed to malloc for tt__ImagingOptions20.\n");
		return SOAP_FAULT;
	}

	ImagingOptions->BacklightCompensation = NULL;
	ImagingOptions->Brightness = (struct tt__FloatRange*)soap_malloc(soap,
	                             sizeof(struct tt__FloatRange));

	if(ImagingOptions->Brightness == NULL)
	{
		__E("Failed to malloc for Brightness.\n");
		return SOAP_FAULT;
	}

	if((ImagingOptions->Contrast = (struct tt__FloatRange*)soap_malloc(soap,
	                               sizeof(struct tt__FloatRange))) == NULL)
	{
		__E("Failed to malloc for Contrast.\n");
		return SOAP_FAULT;
	}

	if((ImagingOptions->ColorSaturation = (struct tt__FloatRange*)soap_malloc(soap,
	                                      sizeof(struct tt__FloatRange))) == NULL)
	{
		__E("Failed to malloc for ColorSaturation.\n");
		return SOAP_FAULT;
	}

	if((ImagingOptions->Sharpness = (struct tt__FloatRange*)soap_malloc(soap,
	                                sizeof(struct tt__FloatRange))) == NULL)
	{
		__E("Failed to malloc for Sharpness.\n");
		return SOAP_FAULT;
	}

	ImagingOptions->Brightness->Max = Max;
	ImagingOptions->Brightness->Min = Min;
	ImagingOptions->Contrast->Max = Max;
	ImagingOptions->Contrast->Min = Min;
	ImagingOptions->ColorSaturation->Max = Max;
	ImagingOptions->ColorSaturation->Min = Min;
	ImagingOptions->Sharpness->Max = Max;
	ImagingOptions->Sharpness->Min = Min;
	ImagingOptions->Exposure = NULL;
	ImagingOptions->Focus = NULL;
#if 0

	if(st3a.eSuppMask & VCT_3A_IRCFMODE)
	{
		if(st3a.struIrcfMode.u8Value == 0)
		{
			IrCutFilterMode = tt__IrCutFilterMode__AUTO;
		}

		if(st3a.struIrcfMode.u8Value == 1)
		{
			IrCutFilterMode = tt__IrCutFilterMode__OFF;
		}

		if(st3a.struIrcfMode.u8Value == 2)
		{
			IrCutFilterMode = tt__IrCutFilterMode__ON;
		}

		ImagingOptions->IrCutFilterModes = (enum tt__IrCutFilterMode*)soap_malloc(soap,
		                                   sizeof(enum tt__IrCutFilterMode));
		ImagingOptions->__sizeIrCutFilterModes =  1;
		*ImagingOptions->IrCutFilterModes = IrCutFilterMode;
	}

#else
	ImagingOptions->IrCutFilterModes = NULL;
#endif
	ImagingOptions->WideDynamicRange = NULL;
	ImagingOptions->WhiteBalance = NULL;
	ImagingOptions->Extension = NULL;
	ImagingOptions->__anyAttribute = NULL;
	timg__GetOptionsResponse->ImagingOptions  = ImagingOptions;
	__FUN_END("\r\n");
	return SOAP_OK;
}

int  __timg__Move(struct soap* soap, struct _timg__Move* timg__Move,
                  struct _timg__MoveResponse* timg__MoveResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\r\n");
	return SOAP_OK;
}

int  __timg__Stop(struct soap* soap, struct _timg__Stop* timg__Stop,
                  struct _timg__StopResponse* timg__StopResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	return SOAP_OK;
	__FUN_END("\n");
}

int  __timg__GetStatus(struct soap* soap,
                       struct _timg__GetStatus* timg__GetStatus,
                       struct _timg__GetStatusResponse* timg__GetStatusResponse)
{
	return SOAP_FAULT;
}

int  __timg__GetMoveOptions(struct soap* soap,
                            struct _timg__GetMoveOptions* timg__GetMoveOptions,
                            struct _timg__GetMoveOptionsResponse* timg__GetMoveOptionsResponse)
{
	struct tt__MoveOptions20* MoveOptions;
	float a = ONVIF_IMAGING_MIN;
	float b = ONVIF_IMAGING_MAX;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(timg__GetMoveOptions->VideoSourceToken == NULL)
	{
		return SOAP_FAULT;
	}

	if(timg__GetMoveOptions)
	{
		if(strcmp(timg__GetMoveOptions->VideoSourceToken,
		          ONVIF_MEDIA_VIDEO_SOURCE_TOKEN) != 0)
		{
			onvif_fault(soap, "ter:InvalidArgVal", "ter:NoSource");
			return SOAP_FAULT;
		}
	}

	if((MoveOptions = (struct tt__MoveOptions20*)soap_malloc(soap,
	                  sizeof(struct tt__MoveOptions20))) == NULL)
	{
		__E("Failed to malloc for MoveOptions.\n");
		return SOAP_FAULT;
	}

	if((MoveOptions->Relative = (struct tt__RelativeFocusOptions20*)soap_malloc(
	                                soap, sizeof(struct tt__RelativeFocusOptions20))) == NULL)
	{
		__E("Failed to malloc for Relative.\n");
		return SOAP_FAULT;
	}

	if((MoveOptions->Relative->Distance = (struct tt__FloatRange*)soap_malloc(soap,
	                                      sizeof(struct tt__FloatRange))) == NULL)
	{
		__E("Failed to malloc for Distance.\n");
		return SOAP_FAULT;
	}

	MoveOptions->Relative->Distance->Max = b;
	MoveOptions->Relative->Distance->Min = a;
	MoveOptions->Absolute = NULL;
	MoveOptions->Continuous = NULL;
	MoveOptions->Relative->Speed = NULL;
	timg__GetMoveOptionsResponse->MoveOptions = MoveOptions;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tmd__GetServiceCapabilities(struct soap* soap,
                                   struct _tmd__GetServiceCapabilities* tmd__GetServiceCapabilities,
                                   struct _tmd__GetServiceCapabilitiesResponse*
                                   tmd__GetServiceCapabilitiesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}



#ifndef SUPPORT_ONVIF_2_6

int  __tmd__GetRelayOutputOptions(struct soap* soap,
                                  struct _tmd__GetRelayOutputOptions* tmd__GetRelayOutputOptions,
                                  struct _tmd__GetRelayOutputOptionsResponse*
                                  tmd__GetRelayOutputOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}



int  __tmd__GetAudioSources(struct soap* soap,
                            struct _trt__GetAudioSources* trt__GetAudioSources,
                            struct _trt__GetAudioSourcesResponse* trt__GetAudioSourcesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetAudioOutputs(struct soap* soap,
                            struct _trt__GetAudioOutputs* trt__GetAudioOutputs,
                            struct _trt__GetAudioOutputsResponse* trt__GetAudioOutputsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetVideoSources(struct soap* soap,
                            struct _trt__GetVideoSources* trt__GetVideoSources,
                            struct _trt__GetVideoSourcesResponse* trt__GetVideoSourcesResponse)
{
	int idx;
	HI_ONVIF_VIDEO_SOURCE_S OnvifVideoSource;
	struct tt__VideoSource* VideoSources;
	struct tt__VideoSource* pVideoSources;
	__FUN_BEGIN("\n");
	CHECK_USER;
	trt__GetVideoSourcesResponse->__sizeVideoSources = ONVIF_VIDEO_SOURCE_NUM;
	ALLOC_STRUCT_NUM(pVideoSources, struct tt__VideoSource, ONVIF_VIDEO_SOURCE_NUM);

	for(idx = 0; idx < ONVIF_VIDEO_SOURCE_NUM; idx++)
	{
		HI_DEV_IMA_CFG_S stIma;
		HI_DEV_3A_CFG_S st3a;
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_IMA_PARAM, idx,
		                     (char*)&stIma, sizeof(HI_DEV_IMA_CFG_S));
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_3A_PARAM, idx,
		                     (char*)&st3a, sizeof(HI_DEV_3A_CFG_S));
		VideoSources = &pVideoSources[idx];
		onvif_get_video_source(idx, &OnvifVideoSource);
		ALLOC_TOKEN(VideoSources->token, OnvifVideoSource.token);
		VideoSources->Framerate = OnvifVideoSource.Framerate;
		ALLOC_STRUCT(VideoSources->Resolution, struct tt__VideoResolution);
		VideoSources->Resolution->Width = OnvifVideoSource.Resolution.Width;
		VideoSources->Resolution->Height = OnvifVideoSource.Resolution.Height;
		ALLOC_STRUCT(VideoSources->Imaging, struct tt__ImagingSettings);
		ALLOC_STRUCT(VideoSources->Imaging->BacklightCompensation,
		             struct tt__BacklightCompensation);
		VideoSources->Imaging->BacklightCompensation->Mode =
		    tt__BacklightCompensationMode__OFF;
		ALLOC_STRUCT(VideoSources->Imaging->Brightness, float);
		ALLOC_STRUCT(VideoSources->Imaging->ColorSaturation, float);
		ALLOC_STRUCT(VideoSources->Imaging->Contrast, float);
		ALLOC_STRUCT(VideoSources->Imaging->Sharpness, float);
		ALLOC_STRUCT(VideoSources->Imaging->WhiteBalance, struct tt__WhiteBalance);
		ALLOC_STRUCT(VideoSources->Extension, struct tt__VideoSourceExtension);
		ALLOC_STRUCT(VideoSources->Extension->Imaging, struct tt__ImagingSettings20);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->Brightness, float);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->ColorSaturation, float);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->Contrast, float);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->Sharpness, float);
		//ALLOC_STRUCT(VideoSources->Extension->Imaging->WideDynamicRange, struct tt__WideDynamicRange20);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->WhiteBalance,
		             struct tt__WhiteBalance20);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->WhiteBalance->CrGain, float);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->WhiteBalance->CbGain, float);

		if(stIma.eSuppMask & VCT_IMA_BRIGHTNESS)
		{
			*VideoSources->Imaging->Brightness = (float)
			                                     stIma.struParam[IMA_BRIGHTNESS_TYPE].u32Value;
			*VideoSources->Extension->Imaging->Brightness = (float)
			        stIma.struParam[IMA_BRIGHTNESS_TYPE].u32Value;
		}

		if(stIma.eSuppMask & VCT_IMA_CONTRAST)
		{
			*VideoSources->Imaging->Contrast = (float)
			                                   stIma.struParam[IMA_CONTRAST_TYPE].u32Value;
			*VideoSources->Extension->Imaging->Contrast = (float)
			        stIma.struParam[IMA_CONTRAST_TYPE].u32Value;
		}

		if(stIma.eSuppMask & VCT_IMA_SATURATION)
		{
			*VideoSources->Imaging->ColorSaturation  = (float)
			        stIma.struParam[IMA_SATURATION_TYPE].u32Value;
			*VideoSources->Extension->Imaging->ColorSaturation = (float)
			        stIma.struParam[IMA_SATURATION_TYPE].u32Value;
		}

		if(stIma.eSuppMask & VCT_IMA_SHARPNESS)
		{
			*VideoSources->Imaging->Sharpness  = (float)
			                                     stIma.struParam[IMA_SHARPNESS_TYPE].u32Value;
			*VideoSources->Extension->Imaging->Sharpness = (float)
			        stIma.struParam[IMA_SHARPNESS_TYPE].u32Value;
		}

		VideoSources->Imaging->WhiteBalance->Mode = tt__WhiteBalanceMode__AUTO;
		VideoSources->Extension->Imaging->WhiteBalance->Mode =
		    tt__WhiteBalanceMode__AUTO;
#if 0

		if(stIma.eSuppMask & VCT_IMA_RED)
		{
			VideoSources->Imaging->WhiteBalance->CrGain = (float)stIma.struRed.u8Value;
			*VideoSources->Extension->Imaging->WhiteBalance->CrGain =
			    (float)stIma.struRed.u8Value;
		}

		if(stIma.eSuppMask & VCT_IMA_BLUE)
		{
			VideoSources->Imaging->WhiteBalance->CbGain = (float)stIma.struBlue.u8Value;
			*VideoSources->Extension->Imaging->WhiteBalance->CbGain =
			    (float)stIma.struBlue.u8Value;
		}

#endif
	}

	trt__GetVideoSourcesResponse->VideoSources = pVideoSources;
	__FUN_END("\n");
	return SOAP_OK;
	//__FUN_BEGIN("\n");CHECK_USER;
	//__FUN_END("\n");
	//return SOAP_FAULT;
}
#else

SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetAudioSources(struct soap* soap,
        struct tmd__Get* trt__GetAudioSources, struct tmd__GetResponse* trt__GetAudioSourcesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetAudioOutputs(struct soap* soap,
        struct tmd__Get* trt__GetAudioOutputs, struct tmd__GetResponse* trt__GetAudioOutputsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetVideoSources(struct soap* soap,
        struct tmd__Get* trt__GetVideoSources, struct tmd__GetResponse* trt__GetVideoSourcesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}



#endif

int  __tmd__GetVideoOutputs(struct soap* soap,
                            struct _tmd__GetVideoOutputs* tmd__GetVideoOutputs,
                            struct _tmd__GetVideoOutputsResponse* tmd__GetVideoOutputsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetVideoSourceConfiguration(struct soap* soap,
                                        struct _tmd__GetVideoSourceConfiguration* tmd__GetVideoSourceConfiguration,
                                        struct _tmd__GetVideoSourceConfigurationResponse*
                                        tmd__GetVideoSourceConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetVideoOutputConfiguration(struct soap* soap,
                                        struct _tmd__GetVideoOutputConfiguration* tmd__GetVideoOutputConfiguration,
                                        struct _tmd__GetVideoOutputConfigurationResponse*
                                        tmd__GetVideoOutputConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetAudioSourceConfiguration(struct soap* soap,
                                        struct _tmd__GetAudioSourceConfiguration* tmd__GetAudioSourceConfiguration,
                                        struct _tmd__GetAudioSourceConfigurationResponse*
                                        tmd__GetAudioSourceConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetAudioOutputConfiguration(struct soap* soap,
                                        struct _tmd__GetAudioOutputConfiguration* tmd__GetAudioOutputConfiguration,
                                        struct _tmd__GetAudioOutputConfigurationResponse*
                                        tmd__GetAudioOutputConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__SetVideoSourceConfiguration(struct soap* soap,
                                        struct _tmd__SetVideoSourceConfiguration* tmd__SetVideoSourceConfiguration,
                                        struct _tmd__SetVideoSourceConfigurationResponse*
                                        tmd__SetVideoSourceConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__SetVideoOutputConfiguration(struct soap* soap,
                                        struct _tmd__SetVideoOutputConfiguration* tmd__SetVideoOutputConfiguration,
                                        struct _tmd__SetVideoOutputConfigurationResponse*
                                        tmd__SetVideoOutputConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__SetAudioSourceConfiguration(struct soap* soap,
                                        struct _tmd__SetAudioSourceConfiguration* tmd__SetAudioSourceConfiguration,
                                        struct _tmd__SetAudioSourceConfigurationResponse*
                                        tmd__SetAudioSourceConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__SetAudioOutputConfiguration(struct soap* soap,
                                        struct _tmd__SetAudioOutputConfiguration* tmd__SetAudioOutputConfiguration,
                                        struct _tmd__SetAudioOutputConfigurationResponse*
                                        tmd__SetAudioOutputConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetVideoSourceConfigurationOptions(struct soap* soap,
        struct _tmd__GetVideoSourceConfigurationOptions*
        tmd__GetVideoSourceConfigurationOptions,
        struct _tmd__GetVideoSourceConfigurationOptionsResponse*
        tmd__GetVideoSourceConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetVideoOutputConfigurationOptions(struct soap* soap,
        struct _tmd__GetVideoOutputConfigurationOptions*
        tmd__GetVideoOutputConfigurationOptions,
        struct _tmd__GetVideoOutputConfigurationOptionsResponse*
        tmd__GetVideoOutputConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetAudioSourceConfigurationOptions(struct soap* soap,
        struct _tmd__GetAudioSourceConfigurationOptions*
        tmd__GetAudioSourceConfigurationOptions,
        struct _tmd__GetAudioSourceConfigurationOptionsResponse*
        tmd__GetAudioSourceConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetAudioOutputConfigurationOptions(struct soap* soap,
        struct _tmd__GetAudioOutputConfigurationOptions*
        tmd__GetAudioOutputConfigurationOptions,
        struct _tmd__GetAudioOutputConfigurationOptionsResponse*
        tmd__GetAudioOutputConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetRelayOutputs(struct soap* soap,
                            struct _tds__GetRelayOutputs* tds__GetRelayOutputs,
                            struct _tds__GetRelayOutputsResponse* tds__GetRelayOutputsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__SetRelayOutputSettings(struct soap* soap,
                                   struct _tmd__SetRelayOutputSettings* tmd__SetRelayOutputSettings,
                                   struct _tmd__SetRelayOutputSettingsResponse*
                                   tmd__SetRelayOutputSettingsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__SetRelayOutputState(struct soap* soap,
                                struct _tds__SetRelayOutputState* tds__SetRelayOutputState,
                                struct _tds__SetRelayOutputStateResponse* tds__SetRelayOutputStateResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetDigitalInputs(struct soap* soap,
                             struct _tmd__GetDigitalInputs* tmd__GetDigitalInputs,
                             struct _tmd__GetDigitalInputsResponse* tmd__GetDigitalInputsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetSerialPorts(struct soap* soap,
                           struct _tmd__GetSerialPorts* tmd__GetSerialPorts,
                           struct _tmd__GetSerialPortsResponse* tmd__GetSerialPortsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetSerialPortConfiguration(struct soap* soap,
                                       struct _tmd__GetSerialPortConfiguration* tmd__GetSerialPortConfiguration,
                                       struct _tmd__GetSerialPortConfigurationResponse*
                                       tmd__GetSerialPortConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__SetSerialPortConfiguration(struct soap* soap,
                                       struct _tmd__SetSerialPortConfiguration* tmd__SetSerialPortConfiguration,
                                       struct _tmd__SetSerialPortConfigurationResponse*
                                       tmd__SetSerialPortConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__GetSerialPortConfigurationOptions(struct soap* soap,
        struct _tmd__GetSerialPortConfigurationOptions*
        tmd__GetSerialPortConfigurationOptions,
        struct _tmd__GetSerialPortConfigurationOptionsResponse*
        tmd__GetSerialPortConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tmd__SendReceiveSerialCommand(struct soap* soap,
                                     struct _tmd__SendReceiveSerialCommand* tmd__SendReceiveSerialCommand,
                                     struct _tmd__SendReceiveSerialCommandResponse*
                                     tmd__SendReceiveSerialCommandResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tptz__GetServiceCapabilities(struct soap* soap,
                                    struct _tptz__GetServiceCapabilities* tptz__GetServiceCapabilities,
                                    struct _tptz__GetServiceCapabilitiesResponse*
                                    tptz__GetServiceCapabilitiesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tptz__GetConfigurations(struct soap* soap,
                               struct _tptz__GetConfigurations* tptz__GetConfigurations,
                               struct _tptz__GetConfigurationsResponse* tptz__GetConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	tptz__GetConfigurationsResponse->__sizePTZConfiguration = 1;
	ALLOC_STRUCT(tptz__GetConfigurationsResponse->PTZConfiguration,
	             struct tt__PTZConfiguration);
	tptz__GetConfigurationsResponse->PTZConfiguration->Name = ONVIF_PTZ_NAME;
	tptz__GetConfigurationsResponse->PTZConfiguration->UseCount = 1;
	tptz__GetConfigurationsResponse->PTZConfiguration->token = ONVIF_PTZ_TOKEN;
	tptz__GetConfigurationsResponse->PTZConfiguration->NodeToken = ONVIF_PTZ_TOKEN;
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultAbsolutePantTiltPositionSpace
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultAbsoluteZoomPositionSpace
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace";
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultRelativePanTiltTranslationSpace
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultRelativeZoomTranslationSpace
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace ";
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultContinuousPanTiltVelocitySpace
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace ";
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultContinuousZoomVelocitySpace
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace ";
	ALLOC_STRUCT(tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZSpeed,
	             struct tt__PTZSpeed);
	ALLOC_STRUCT(
	    tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZSpeed->PanTilt,
	    struct tt__Vector2D);
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZSpeed->PanTilt->space
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace ";
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZSpeed->PanTilt->x =
	    1.0;
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZSpeed->PanTilt->y =
	    0.5;
	ALLOC_STRUCT(
	    tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZSpeed->Zoom,
	    struct tt__Vector1D);
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZSpeed->Zoom->space
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace ";
	tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZSpeed->Zoom->x =
	    0.0;
	//ALLOC_TOKEN(tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZTimeout, "PT5S");
	ALLOC_STRUCT(
	    tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZTimeout, LONG64);
	*tptz__GetConfigurationsResponse->PTZConfiguration->DefaultPTZTimeout = 5 *
	        1000;
	//#####################################################
	ALLOC_STRUCT(tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits,
	             struct tt__PanTiltLimits);
	ALLOC_STRUCT(
	    tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits->Range,
	    struct tt__Space2DDescription);
	tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits->Range->URI =
	    "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
	ALLOC_STRUCT(
	    tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits->Range->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits->Range->XRange->Max
	    = 1.0;
	tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits->Range->XRange->Min
	    = - 1.0;
	ALLOC_STRUCT(
	    tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits->Range->YRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits->Range->YRange->Max
	    = 1.0;
	tptz__GetConfigurationsResponse->PTZConfiguration->PanTiltLimits->Range->YRange->Min
	    = - 1.0;
	//############################################################
	ALLOC_STRUCT(tptz__GetConfigurationsResponse->PTZConfiguration->ZoomLimits,
	             struct tt__ZoomLimits);
	ALLOC_STRUCT(
	    tptz__GetConfigurationsResponse->PTZConfiguration->ZoomLimits->Range,
	    struct tt__Space1DDescription);
	tptz__GetConfigurationsResponse->PTZConfiguration->ZoomLimits->Range->URI =
	    "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace";
	ALLOC_STRUCT(
	    tptz__GetConfigurationsResponse->PTZConfiguration->ZoomLimits->Range->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationsResponse->PTZConfiguration->ZoomLimits->Range->XRange->Max
	    = 1.0;
	tptz__GetConfigurationsResponse->PTZConfiguration->ZoomLimits->Range->XRange->Min
	    = 0;
	tptz__GetConfigurationsResponse->PTZConfiguration->Extension = NULL;
	tptz__GetConfigurationsResponse->PTZConfiguration->__anyAttribute = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetPresets(struct soap* soap,
                        struct _tptz__GetPresets* tptz__GetPresets,
                        struct _tptz__GetPresetsResponse* tptz__GetPresetsResponse)
{
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__SetPreset(struct soap* soap,
                       struct _tptz__SetPreset* tptz__SetPreset,
                       struct _tptz__SetPresetResponse* tptz__SetPresetResponse)
{
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__RemovePreset(struct soap* soap,
                          struct _tptz__RemovePreset* tptz__RemovePreset,
                          struct _tptz__RemovePresetResponse* tptz__RemovePresetResponse)
{
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GotoPreset(struct soap* soap,
                        struct _tptz__GotoPreset* tptz__GotoPreset,
                        struct _tptz__GotoPresetResponse* tptz__GotoPresetResponse)
{
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetStatus(struct soap* soap,
                       struct _tptz__GetStatus* tptz__GetStatus,
                       struct _tptz__GetStatusResponse* tptz__GetStatusResponse)
{
	return SOAP_OK;
}

int  __tptz__GetConfiguration(struct soap* soap,
                              struct _tptz__GetConfiguration* tptz__GetConfiguration,
                              struct _tptz__GetConfigurationResponse* tptz__GetConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT(tptz__GetConfigurationResponse->PTZConfiguration,
	             struct tt__PTZConfiguration);
	tptz__GetConfigurationResponse->PTZConfiguration->Name = ONVIF_PTZ_NAME;
	tptz__GetConfigurationResponse->PTZConfiguration->UseCount = 1;
	tptz__GetConfigurationResponse->PTZConfiguration->token = ONVIF_PTZ_TOKEN;
	tptz__GetConfigurationResponse->PTZConfiguration->NodeToken = ONVIF_PTZ_TOKEN;
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultAbsolutePantTiltPositionSpace
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultAbsoluteZoomPositionSpace
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace";
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultRelativePanTiltTranslationSpace
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultRelativeZoomTranslationSpace
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace ";
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultContinuousPanTiltVelocitySpace
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace ";
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultContinuousZoomVelocitySpace
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace ";
	ALLOC_STRUCT(tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZSpeed,
	             struct tt__PTZSpeed);
	ALLOC_STRUCT(
	    tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZSpeed->PanTilt,
	    struct tt__Vector2D);
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZSpeed->PanTilt->space
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace ";
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZSpeed->PanTilt->x =
	    1.0;
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZSpeed->PanTilt->y =
	    0.5;
	ALLOC_STRUCT(
	    tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZSpeed->Zoom,
	    struct tt__Vector1D);
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZSpeed->Zoom->space =
	    "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace ";
	tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZSpeed->Zoom->x =
	    0.0;
	//ALLOC_TOKEN(tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZTimeout, "PT30S");
	ALLOC_STRUCT(
	    tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZTimeout, LONG64);
	*tptz__GetConfigurationResponse->PTZConfiguration->DefaultPTZTimeout = 5 * 1000;
	//#####################################################
	ALLOC_STRUCT(tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits,
	             struct tt__PanTiltLimits);
	ALLOC_STRUCT(
	    tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits->Range,
	    struct tt__Space2DDescription);
	ALLOC_TOKEN(
	    tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits->Range->URI,
	    "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
	ALLOC_STRUCT(
	    tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits->Range->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits->Range->XRange->Max
	    = 1.0;
	tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits->Range->XRange->Min
	    = -1.0;
	ALLOC_STRUCT(
	    tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits->Range->YRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits->Range->YRange->Max
	    = 1.0;
	tptz__GetConfigurationResponse->PTZConfiguration->PanTiltLimits->Range->YRange->Min
	    = -1.0;
	//############################################################
	ALLOC_STRUCT(tptz__GetConfigurationResponse->PTZConfiguration->ZoomLimits,
	             struct tt__ZoomLimits);
	ALLOC_STRUCT(
	    tptz__GetConfigurationResponse->PTZConfiguration->ZoomLimits->Range,
	    struct tt__Space1DDescription);
	ALLOC_TOKEN(
	    tptz__GetConfigurationResponse->PTZConfiguration->ZoomLimits->Range->URI,
	    "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace");
	ALLOC_STRUCT(
	    tptz__GetConfigurationResponse->PTZConfiguration->ZoomLimits->Range->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationResponse->PTZConfiguration->ZoomLimits->Range->XRange->Max
	    = 1.0;
	tptz__GetConfigurationResponse->PTZConfiguration->ZoomLimits->Range->XRange->Min
	    = 0;
	tptz__GetConfigurationResponse->PTZConfiguration->Extension = NULL;
	tptz__GetConfigurationResponse->PTZConfiguration->__anyAttribute = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetNodes(struct soap* soap, struct _tptz__GetNodes* tptz__GetNodes,
                      struct _tptz__GetNodesResponse* tptz__GetNodesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	tptz__GetNodesResponse->__sizePTZNode = 1;
	tptz__GetNodesResponse->PTZNode = (struct tt__PTZNode*)soap_malloc( soap,
	                                  sizeof(struct tt__PTZNode));

	if(tptz__GetNodesResponse->PTZNode == NULL)
	{
		__E("Failed to malloc for PTZNode.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->token = ONVIF_PTZ_TOKEN;
	tptz__GetNodesResponse->PTZNode->Name = ONVIF_PTZ_NAME;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces = NULL;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces = (struct tt__PTZSpaces*)
	        soap_malloc(soap, sizeof(struct tt__PTZSpaces));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces == NULL)
	{
		__E("Failed to malloc for SupportedPTZSpaces.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeAbsolutePanTiltPositionSpace
	    = 1;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeRelativePanTiltTranslationSpace
	    = 1;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeContinuousPanTiltVelocitySpace
	    = 1;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeAbsoluteZoomPositionSpace
	    = 1;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeContinuousZoomVelocitySpace
	    = 1;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizePanTiltSpeedSpace =
	    1;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeRelativeZoomTranslationSpace
	    = 1;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeZoomSpeedSpace = 1;
	//#############################################################################################
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace
	    = (struct tt__Space2DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space2DDescription));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace
	        == NULL)
	{
		__E("Failed to malloc for AbsolutePanTiltPositionSpace.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->XRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->XRange->Min
	    = -1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->YRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->YRange
	        == NULL)
	{
		__E("Failed to malloc for YRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->YRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->YRange->Min
	    = -1.0;
	/////////////////////////////////////////////////////////////////////////////
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace
	    = (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace
	        == NULL)
	{
		__E("Failed to malloc for AbsoluteZoomPositionSpace.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace";
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->XRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->XRange->Min
	    = 0.0;
	//#############################################################################################
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace
	    = (struct tt__Space2DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space2DDescription));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace
	        == NULL)
	{
		__E("Failed to malloc for RelativePanTiltTranslationSpace.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->XRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->XRange->Min
	    = -1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->YRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->YRange
	        == NULL)
	{
		__E("Failed to malloc for YRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->YRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->YRange->Min
	    = -1.0;
	///////////////////////////////////////////////////////////////////////////////////////////
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace
	    = (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace
	        == NULL)
	{
		__E("Failed to malloc for RelativeZoomTranslationSpace.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace ";
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->XRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->XRange->Min
	    = 0.0;
	//#############################################################################################
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace
	    = (struct tt__Space2DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space2DDescription));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace
	        == NULL)
	{
		__E("Failed to malloc for ContinuousPanTiltVelocitySpace.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace ";
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange->Min
	    = -1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange
	        == NULL)
	{
		__E("Failed to malloc for YRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange->Min
	    = -1.0;
	///////////////////////////////////////////////////////////////////////////////////////////
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace
	    = (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace
	        == NULL)
	{
		__E("Failed to malloc for ContinuousZoomVelocitySpace.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace ";
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange->Min
	    = -1.0;	 //20140625 zj
	//######################################################################################
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace =
	    (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace ==
	        NULL)
	{
		__E("Failed to malloc for PanTiltSpeedSpace.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->URI =
	    "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace ";
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->XRange->Max
	    = 1.0;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->XRange->Min
	    = 0.0;
	//######################################################################################
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace =
	    (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace ==
	        NULL)
	{
		__E("Failed to malloc for ZoomSpeedSpace.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace->URI =
	    "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace";
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace->XRange =
	    (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace->XRange->Min
	    = -1.0;//20140625 zj
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->Extension = NULL;
	tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__anyAttribute = NULL;
	tptz__GetNodesResponse->PTZNode->MaximumNumberOfPresets = ONVIF_PTZ_PRESET_NUM;
	tptz__GetNodesResponse->PTZNode->HomeSupported = xsd__boolean__true_;
	tptz__GetNodesResponse->PTZNode->__sizeAuxiliaryCommands = 0;
	tptz__GetNodesResponse->PTZNode->AuxiliaryCommands = NULL;
	//////////////////////20140829////////////////////////
	ALLOC_STRUCT(tptz__GetNodesResponse->PTZNode->Extension,
	             struct tt__PTZNodeExtension);
	ALLOC_STRUCT(tptz__GetNodesResponse->PTZNode->Extension->SupportedPresetTour,
	             struct tt__PTZPresetTourSupported);
	tptz__GetNodesResponse->PTZNode->Extension->SupportedPresetTour->MaximumNumberOfPresetTours
	    = 1;
	//////////////////////////END/////////////////////////
	tptz__GetNodesResponse->PTZNode->__anyAttribute = NULL;
	tptz__GetNodesResponse->PTZNode->FixedHomePosition = xsd__boolean__false_;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetNode(struct soap* soap, struct _tptz__GetNode* tptz__GetNode,
                     struct _tptz__GetNodeResponse* tptz__GetNodeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(strcmp(tptz__GetNode->NodeToken, ONVIF_PTZ_TOKEN) != 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoEntity");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode = (struct tt__PTZNode*)soap_malloc( soap,
	                                 sizeof(struct tt__PTZNode));

	if(tptz__GetNodeResponse->PTZNode == NULL)
	{
		__E("Failed to malloc for PTZNode.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode, 0x00, sizeof(struct tt__PTZNode));
	tptz__GetNodeResponse->PTZNode->token = ONVIF_PTZ_TOKEN;
	tptz__GetNodeResponse->PTZNode->Name = ONVIF_PTZ_NAME;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces = NULL;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces = (struct tt__PTZSpaces*)
	        soap_malloc(soap, sizeof(struct tt__PTZSpaces));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces == NULL)
	{
		__E("Failed to malloc for SupportedPTZSpaces.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces, 0x00, sizeof(struct tt__PTZSpaces));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeAbsolutePanTiltPositionSpace
	    = 1;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeRelativePanTiltTranslationSpace
	    = 1;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeContinuousPanTiltVelocitySpace
	    = 1;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeAbsoluteZoomPositionSpace
	    = 1;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeContinuousZoomVelocitySpace
	    = 1;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizePanTiltSpeedSpace = 1;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeRelativeZoomTranslationSpace
	    = 1;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeZoomSpeedSpace = 1;
	//#############################################################################################
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace
	    = (struct tt__Space2DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space2DDescription));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace
	        == NULL)
	{
		__E("Failed to malloc for AbsolutePanTiltPositionSpace.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace, 0x00,
	       sizeof(struct tt__Space2DDescription));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->XRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->XRange->Min
	    = -1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->YRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->YRange
	        == NULL)
	{
		__E("Failed to malloc for YRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->YRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace->YRange->Min
	    = -1.0;
	/////////////////////////////////////////////////////////////////////////////
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace =
	    (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace
	        == NULL)
	{
		__E("Failed to malloc for AbsoluteZoomPositionSpace.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace, 0x00,
	       sizeof(struct tt__Space1DDescription));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace";
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->XRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsoluteZoomPositionSpace->XRange->Min
	    = 0.0;
	//#############################################################################################
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace
	    = (struct tt__Space2DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space2DDescription));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace
	        == NULL)
	{
		__E("Failed to malloc for RelativePanTiltTranslationSpace.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace, 0x00,
	       sizeof(struct tt__Space2DDescription));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->XRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->XRange->Min
	    = -1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->YRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->YRange
	        == NULL)
	{
		__E("Failed to malloc for YRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->YRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace->YRange->Min
	    = -1.0;
	///////////////////////////////////////////////////////////////////////////////////////////
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace
	    = (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace
	        == NULL)
	{
		__E("Failed to malloc for RelativeZoomTranslationSpace.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace, 0x00,
	       sizeof(struct tt__Space1DDescription));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace ";
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->XRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativeZoomTranslationSpace->XRange->Min
	    = 0.0;
	//#############################################################################################
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace
	    = (struct tt__Space2DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space2DDescription));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace
	        == NULL)
	{
		__E("Failed to malloc for ContinuousPanTiltVelocitySpace.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace, 0x00,
	       sizeof(struct tt__Space2DDescription));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace ";
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange->Min
	    = -1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange
	        == NULL)
	{
		__E("Failed to malloc for YRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange->Min
	    = -1.0;
	///////////////////////////////////////////////////////////////////////////////////////////
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace
	    = (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace
	        == NULL)
	{
		__E("Failed to malloc for ContinuousZoomVelocitySpace.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace, 0x00,
	       sizeof(struct tt__Space1DDescription));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace ";
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange->Min
	    = -1.0;	//20140625 zj
	//######################################################################################
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace =
	    (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace ==
	        NULL)
	{
		__E("Failed to malloc for PanTiltSpeedSpace.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace, 0x00,
	       sizeof(struct tt__Space1DDescription));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->URI =
	    "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace ";
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->XRange
	    = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->XRange
	        == NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->XRange->Max
	    = 1.0;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace->XRange->Min
	    = 0.0;
	//######################################################################################
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace =
	    (struct tt__Space1DDescription*)soap_malloc(soap,
	            sizeof(struct tt__Space1DDescription));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace == NULL)
	{
		__E("Failed to malloc for ZoomSpeedSpace.\n");
		return SOAP_FAULT;
	}

	memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace, 0x00,
	       sizeof(struct tt__Space1DDescription));
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace->URI =
	    "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace";
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace->XRange =
	    (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));

	if(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace->XRange ==
	        NULL)
	{
		__E("Failed to malloc for XRange.\n");
		return SOAP_FAULT;
	}

	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ZoomSpeedSpace->XRange->Min
	    = -1.0;//20140625 zj
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->Extension = NULL;
	tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__anyAttribute = NULL;
	tptz__GetNodeResponse->PTZNode->MaximumNumberOfPresets = ONVIF_PTZ_PRESET_NUM;
	tptz__GetNodeResponse->PTZNode->HomeSupported = xsd__boolean__true_;
	tptz__GetNodeResponse->PTZNode->FixedHomePosition = xsd__boolean__false_;
	tptz__GetNodeResponse->PTZNode->__sizeAuxiliaryCommands = 1;
	tptz__GetNodeResponse->PTZNode->AuxiliaryCommands = (char**)soap_malloc(soap,
	        sizeof(char) * 3);
	tptz__GetNodeResponse->PTZNode->AuxiliaryCommands[0] = (char*)soap_malloc(soap,
	        sizeof(char) * 64);
	strcpy(tptz__GetNodeResponse->PTZNode->AuxiliaryCommands[0], "command");
	//////////////////////20140829////////////////////////
	ALLOC_STRUCT(tptz__GetNodeResponse->PTZNode->Extension,
	             struct tt__PTZNodeExtension);
	ALLOC_STRUCT(tptz__GetNodeResponse->PTZNode->Extension->SupportedPresetTour,
	             struct tt__PTZPresetTourSupported);
	tptz__GetNodeResponse->PTZNode->Extension->SupportedPresetTour->MaximumNumberOfPresetTours
	    = 1;
	//////////////////////////END/////////////////////////
	tptz__GetNodeResponse->PTZNode->__anyAttribute = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__SetConfiguration(struct soap* soap,
                              struct _tptz__SetConfiguration* tptz__SetConfiguration,
                              struct _tptz__SetConfigurationResponse* tptz__SetConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(strcmp(tptz__SetConfiguration->PTZConfiguration->Name, "TestConfig") == 0)
	{
		onvif_fault(soap, "ter:ActionNotSupported", "ter:PTZNotSupported");
		return SOAP_FAULT;
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetConfigurationOptions(struct soap* soap,
                                     struct _tptz__GetConfigurationOptions* tptz__GetConfigurationOptions,
                                     struct _tptz__GetConfigurationOptionsResponse*
                                     tptz__GetConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT(tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions,
	             struct tt__PTZConfigurationOptions);
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->PTZTimeout,
	    struct tt__DurationRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->PTZTimeout->Max
	    = 30 * 1000; //"PT30S";
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->PTZTimeout->Min
	    = 1000;//"PT1S";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces,
	    struct tt__PTZSpaces);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__sizeAbsolutePanTiltPositionSpace
	    = 1;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__sizeRelativePanTiltTranslationSpace
	    = 1;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__sizeContinuousPanTiltVelocitySpace
	    = 1;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__sizeAbsoluteZoomPositionSpace
	    = 1;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__sizeContinuousZoomVelocitySpace
	    = 1;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__sizePanTiltSpeedSpace
	    = 1;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__sizeRelativeZoomTranslationSpace
	    = 1;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__sizeZoomSpeedSpace
	    = 1;
	//#############################################################################################
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace,
	    struct tt__Space2DDescription);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace->XRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace->XRange->Min
	    = -1.0;
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace->YRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace->YRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace->YRange->Min
	    = -1.0;
	/////////////////////////////////////////////////////////////////////////////
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsoluteZoomPositionSpace,
	    struct tt__Space1DDescription);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsoluteZoomPositionSpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsoluteZoomPositionSpace->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsoluteZoomPositionSpace->XRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->AbsoluteZoomPositionSpace->XRange->Min
	    = 0.0;
	//#############################################################################################
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace,
	    struct tt__Space2DDescription);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace->XRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace->XRange->Min
	    = -1.0;
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace->YRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace->YRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace->YRange->Min
	    = -1.0;
	///////////////////////////////////////////////////////////////////////////////////////////
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativeZoomTranslationSpace,
	    struct tt__Space1DDescription);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativeZoomTranslationSpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace ";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativeZoomTranslationSpace->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativeZoomTranslationSpace->XRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->RelativeZoomTranslationSpace->XRange->Min
	    = 0.0;
	//#############################################################################################
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace,
	    struct tt__Space2DDescription);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace ";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace->XRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace->XRange->Min
	    = -1.0;
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace->YRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace->YRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace->YRange->Min
	    = -1.0;
	///////////////////////////////////////////////////////////////////////////////////////////
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace,
	    struct tt__Space1DDescription);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace ";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace->XRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace->XRange->Min
	    = -1.0;
	//######################################################################################
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->PanTiltSpeedSpace,
	    struct tt__Space1DDescription);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->PanTiltSpeedSpace->URI
	    = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace ";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->PanTiltSpeedSpace->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->PanTiltSpeedSpace->XRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->PanTiltSpeedSpace->XRange->Min
	    = 0.0;
	//######################################################################################
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ZoomSpeedSpace,
	    struct tt__Space1DDescription);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ZoomSpeedSpace->URI
	    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace";
	ALLOC_STRUCT(
	    tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ZoomSpeedSpace->XRange,
	    struct tt__FloatRange);
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ZoomSpeedSpace->XRange->Max
	    = 1.0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ZoomSpeedSpace->XRange->Min
	    = -1.0;		//20140625 zj
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->Extension
	    = NULL;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->__anyAttribute
	    = NULL;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->__size = 0;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->__any = NULL;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->__anyAttribute =
	    NULL;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->PTControlDirection
	    = NULL;
	tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Extension =
	    NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GotoHomePosition(struct soap* soap,
                              struct _tptz__GotoHomePosition* tptz__GotoHomePosition,
                              struct _tptz__GotoHomePositionResponse* tptz__GotoHomePositionResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__SetHomePosition(struct soap* soap,
                             struct _tptz__SetHomePosition* tptz__SetHomePosition,
                             struct _tptz__SetHomePositionResponse* tptz__SetHomePositionResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	return SOAP_FAULT;
}

int  __tptz__ContinuousMove(struct soap* soap,
                            struct _tptz__ContinuousMove* tptz__ContinuousMove,
                            struct _tptz__ContinuousMoveResponse* tptz__ContinuousMoveResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__RelativeMove(struct soap* soap,
                          struct _tptz__RelativeMove* tptz__RelativeMove,
                          struct _tptz__RelativeMoveResponse* tptz__RelativeMoveResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tptz__SendAuxiliaryCommand(struct soap* soap,
                                  struct _tptz__SendAuxiliaryCommand* tptz__SendAuxiliaryCommand,
                                  struct _tptz__SendAuxiliaryCommandResponse*
                                  tptz__SendAuxiliaryCommandResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __tptz__AbsoluteMove(struct soap* soap,
                          struct _tptz__AbsoluteMove* tptz__AbsoluteMove,
                          struct _tptz__AbsoluteMoveResponse* tptz__AbsoluteMoveResponse)
{
	__FUN_BEGIN("Temperarily no process!\r\n");
	__FUN_END("Temperarily no process!\r\n");
	return SOAP_OK;
}

int  __tptz__Stop(struct soap* soap, struct _tptz__Stop* tptz__Stop,
                  struct _tptz__StopResponse* tptz__StopResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetPresetTours(struct soap* soap,
                            struct _tptz__GetPresetTours* tptz__GetPresetTours,
                            struct _tptz__GetPresetToursResponse* tptz__GetPresetToursResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	/////////////////////20140619 zj///////////////////
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetPresetTour(struct soap* soap,
                           struct _tptz__GetPresetTour* tptz__GetPresetTour,
                           struct _tptz__GetPresetTourResponse* tptz__GetPresetTourResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	/////////////////////////20140619 zj//////////////////////////
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetPresetTourOptions(struct soap* soap,
                                  struct _tptz__GetPresetTourOptions* tptz__GetPresetTourOptions,
                                  struct _tptz__GetPresetTourOptionsResponse*
                                  tptz__GetPresetTourOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__CreatePresetTour(struct soap* soap,
                              struct _tptz__CreatePresetTour* tptz__CreatePresetTour,
                              struct _tptz__CreatePresetTourResponse* tptz__CreatePresetTourResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	///////////////20140619 zj//////////////////
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__ModifyPresetTour(struct soap* soap,
                              struct _tptz__ModifyPresetTour* tptz__ModifyPresetTour,
                              struct _tptz__ModifyPresetTourResponse* tptz__ModifyPresetTourResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__OperatePresetTour(struct soap* soap,
                               struct _tptz__OperatePresetTour* tptz__OperatePresetTour,
                               struct _tptz__OperatePresetTourResponse* tptz__OperatePresetTourResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	///////////////20140619 zj//////////////////
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__RemovePresetTour(struct soap* soap,
                              struct _tptz__RemovePresetTour* tptz__RemovePresetTour,
                              struct _tptz__RemovePresetTourResponse* tptz__RemovePresetTourResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	///////////////20140619 zj//////////////////
	__FUN_END("\n");
	return SOAP_OK;
}

int  __tptz__GetCompatibleConfigurations(struct soap* soap,
        struct _tptz__GetCompatibleConfigurations* tptz__GetCompatibleConfigurations,
        struct _tptz__GetCompatibleConfigurationsResponse*
        tptz__GetCompatibleConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

#ifndef SUPPORT_ONVIF_2_6

int __tptz__Pix3DPosMove(struct soap* soap,
                         struct _tptz__tptz_Pix3DPosMove* tptz__tptz_Pix3DPosMove,
                         struct _tptz__tptz_Pix3DPosMoveResponse* tptz__tptz_Pix3DPosMoveResponse)
{
	__FUN_END("\n");
	return SOAP_OK;
}


int __tptz__SetBallParam(struct soap* soap,
                         struct _tptz__tptz_SetBallParam* tptz__tptz_SetBallParam,
                         struct _tptz__SetBallParamResponse* tptz__SetBallParamResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;
}



int __tptz__GetBallParam(struct soap* soap,
                         struct _tptz__tptz_GetBallParam* tptz__tptz_GetBallParam,
                         struct _tptz__GetBallParamResponse* tptz__GetBallParamResponse)
{
	return SOAP_OK;
}

int __tptz__SetTrackingMovePosition(struct soap* soap,
                                    struct _tptz__SetTrackingMovePosition* tptz__SetTrackingMovePosition,
                                    struct _tptz__SetTrackingMovePositionResponse*
                                    tptz__SetTrackingMovePositionResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_OK;
}


int  __trt__GetVideoSources(struct soap* soap,
                            struct _trt__GetVideoSources* trt__GetVideoSources,
                            struct _trt__GetVideoSourcesResponse* trt__GetVideoSourcesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

#endif

int  __trt__GetServiceCapabilities(struct soap* soap,
                                   struct _trt__GetServiceCapabilities* trt__GetServiceCapabilities,
                                   struct _trt__GetServiceCapabilitiesResponse*
                                   trt__GetServiceCapabilitiesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}



int  __trt__GetAudioSources(struct soap* soap,
                            struct _trt__GetAudioSources* trt__GetAudioSources,
                            struct _trt__GetAudioSourcesResponse* trt__GetAudioSourcesResponse)
{
	int idx;
	HI_ONVIF_AUDIO_SOURCE_S OnvifAudioSource;
	struct tt__AudioSource* AudioSources;
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT_NUM(AudioSources, struct tt__AudioSource, ONVIF_AUDIO_SOURCE_NUM);

	for(idx = 0; idx < ONVIF_AUDIO_SOURCE_NUM; idx++)
	{
		onvif_get_audio_source(idx, &OnvifAudioSource);
		ALLOC_TOKEN(AudioSources[idx ].token, OnvifAudioSource.token);
		AudioSources[idx ].Channels = OnvifAudioSource.Channels;
	}

	trt__GetAudioSourcesResponse->__sizeAudioSources = ONVIF_AUDIO_SOURCE_NUM;
	trt__GetAudioSourcesResponse->AudioSources = AudioSources;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetAudioOutputs(struct soap* soap,
                            struct _trt__GetAudioOutputs* trt__GetAudioOutputs,
                            struct _trt__GetAudioOutputsResponse* trt__GetAudioOutputsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__CreateProfile(struct soap* soap,
                          struct _trt__CreateProfile* trt__CreateProfile,
                          struct _trt__CreateProfileResponse* trt__CreateProfileResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	struct tt__Profile* Profile;
	struct tt__VideoSourceConfiguration* VideoSourceConfiguration;
	struct tt__VideoEncoderConfiguration* VideoEncoderConfiguration;
	struct tt__AudioSourceConfiguration* AudioSourceConfiguration;
	struct tt__AudioEncoderConfiguration* AudioEncoderConfiguration;
	struct tt__PTZConfiguration* PTZConfiguration;
	HI_ONVIF_PROFILE_S OnvifProfile;
	HI_ONVIF_VIDEO_SOURCE_CFG_S OnvifVideoSourceCfg;
	HI_ONVIF_VIDEO_ENCODER_CFG_S OnvifVideoEncoderCfg;
	HI_ONVIF_AUDIO_SOURCE_CFG_S OnvifAudioSourceCfg;
	HI_ONVIF_AUDIO_ENCODER_CFG_S OnvifAudioEncoderCfg;
	HI_DEV_VIDEO_CFG_S VideoCfg;

	if(trt__CreateProfile->Name == NULL)
	{
		return SOAP_FAULT;
	}

	unsigned long ip;
	ip = onvif_get_ipaddr(soap);

	if(trt__CreateProfile->Token == NULL)
	{
		printf(">>>>>>>the name is %s\r\n", trt__CreateProfile->Name);
		onvif_get_profile(0, ip, &OnvifProfile);
	}
	else if(onvif_get_profile_from_token(ip, trt__CreateProfile->Token,
	                                     &OnvifProfile) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(Profile, struct tt__Profile);
	ALLOC_TOKEN(Profile->Name, OnvifProfile.Name);
	ALLOC_TOKEN(Profile->token, OnvifProfile.token);
	ALLOC_STRUCT(Profile->fixed,  enum xsd__boolean);
	*Profile->fixed = OnvifProfile.fixed;	// 这个profile不能删除
	onvif_get_video_source_cfg(OnvifProfile.videoSourceCfgIdx,
	                           &OnvifVideoSourceCfg);
	ALLOC_STRUCT(VideoSourceConfiguration,  struct tt__VideoSourceConfiguration);
	ALLOC_TOKEN(VideoSourceConfiguration->Name, OnvifVideoSourceCfg.Name);
	ALLOC_TOKEN(VideoSourceConfiguration->SourceToken,
	            OnvifVideoSourceCfg.SourceToken);
	VideoSourceConfiguration->UseCount = OnvifVideoSourceCfg.UseCount;
	ALLOC_STRUCT(VideoSourceConfiguration->Bounds, struct tt__IntRectangle);
	VideoSourceConfiguration->Bounds->y = OnvifVideoSourceCfg.Bounds.y;
	VideoSourceConfiguration->Bounds->x = OnvifVideoSourceCfg.Bounds.x;
	VideoSourceConfiguration->Bounds->width  = OnvifVideoSourceCfg.Bounds.width;
	VideoSourceConfiguration->Bounds->height = OnvifVideoSourceCfg.Bounds.height;
	Profile->VideoSourceConfiguration = VideoSourceConfiguration;
	/* AudioSourceConfiguration */
	onvif_get_audio_source_cfg(OnvifProfile.audioSourceCfgIdx,
	                           &OnvifAudioSourceCfg);
	ALLOC_STRUCT(AudioSourceConfiguration, struct tt__AudioSourceConfiguration);
	ALLOC_TOKEN(AudioSourceConfiguration->Name, OnvifAudioSourceCfg.Name);
	ALLOC_TOKEN(AudioSourceConfiguration->token, OnvifAudioSourceCfg.token);
	ALLOC_TOKEN(AudioSourceConfiguration->SourceToken,
	            OnvifAudioSourceCfg.SourceToken);
	AudioSourceConfiguration->UseCount = OnvifAudioSourceCfg.UseCount;
	Profile->AudioSourceConfiguration = AudioSourceConfiguration;
	/*VideoEncoderConfiguration */
	onvif_get_video_encoder_cfg(OnvifProfile.videoEncoderCfgIdx,
	                            &OnvifVideoEncoderCfg);
	ALLOC_STRUCT(VideoEncoderConfiguration, struct tt__VideoEncoderConfiguration);
	ALLOC_TOKEN(VideoEncoderConfiguration->Name, OnvifVideoEncoderCfg.Name);
	ALLOC_TOKEN(VideoEncoderConfiguration->token, OnvifVideoEncoderCfg.token);
	VideoEncoderConfiguration->UseCount = OnvifVideoEncoderCfg.UseCount;
	VideoEncoderConfiguration->Encoding = OnvifVideoEncoderCfg.Encoding;
	ALLOC_STRUCT(VideoEncoderConfiguration->Resolution, struct tt__VideoResolution);
	VideoEncoderConfiguration->Resolution->Width =
	    OnvifVideoEncoderCfg.Resolution.Width;
	VideoEncoderConfiguration->Resolution->Height =
	    OnvifVideoEncoderCfg.Resolution.Height;
	VideoEncoderConfiguration->Quality = OnvifVideoEncoderCfg.Quality;
	ALLOC_STRUCT(VideoEncoderConfiguration->RateControl,
	             struct tt__VideoRateControl);
	VideoEncoderConfiguration->RateControl->FrameRateLimit =
	    OnvifVideoEncoderCfg.FrameRate;
	VideoEncoderConfiguration->RateControl->EncodingInterval =
	    OnvifVideoEncoderCfg.Gop;
	VideoEncoderConfiguration->RateControl->BitrateLimit =
	    OnvifVideoEncoderCfg.Bitrate;
	VideoEncoderConfiguration->MPEG4 = NULL;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VIDEO_PARAM,  0,
	                     (char*)&VideoCfg, sizeof(HI_DEV_VIDEO_CFG_S));
	ALLOC_STRUCT(VideoEncoderConfiguration->H264, struct tt__H264Configuration);
	VideoEncoderConfiguration->H264->GovLength = VideoCfg.struVenc[0].u8Gop;

	switch(VideoCfg.struVenc[0].u8Profile)
	{
		case HI_VENC_PROFILE_HIGH:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__High;
			break;

		case HI_VENC_PROFILE_MAIN:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Main;
			break;

		case HI_VENC_PROFILE_BASELINE:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Baseline;
			break;
	}

	ALLOC_STRUCT(VideoEncoderConfiguration->Multicast,
	             struct tt__MulticastConfiguration);
	ALLOC_STRUCT(VideoEncoderConfiguration->Multicast->Address,
	             struct tt__IPAddress);
	VideoEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
	ALLOC_TOKEN(VideoEncoderConfiguration->Multicast->Address->IPv4Address,
	            ONVIF_WSD_MC_ADDR);
	VideoEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
	VideoEncoderConfiguration->Multicast->Port = 9090;
	VideoEncoderConfiguration->Multicast->TTL = 1500;
	VideoEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
	//ALLOC_TOKEN(VideoEncoderConfiguration->SessionTimeout, "PT5S");
	//yyyyyyyyyyyyyyyyyy
	VideoEncoderConfiguration->SessionTimeout = 5000;
	Profile->VideoEncoderConfiguration = VideoEncoderConfiguration;
	/* AudioEncoderConfiguration */
	onvif_get_audio_encoder_cfg(OnvifProfile.audioEncoderCfgIdx,
	                            &OnvifAudioEncoderCfg);
	ALLOC_STRUCT(AudioEncoderConfiguration, struct tt__AudioEncoderConfiguration);
	ALLOC_TOKEN(AudioEncoderConfiguration->Name, OnvifAudioEncoderCfg.Name);
	AudioEncoderConfiguration->UseCount = OnvifAudioEncoderCfg.UseCount;
	ALLOC_TOKEN(AudioEncoderConfiguration->token, OnvifAudioEncoderCfg.token);
	AudioEncoderConfiguration->Encoding = OnvifAudioEncoderCfg.Encoding;
	AudioEncoderConfiguration->Bitrate = OnvifAudioEncoderCfg.Bitrate;
	AudioEncoderConfiguration->SampleRate = OnvifAudioEncoderCfg.SampleRate;
	ALLOC_STRUCT(AudioEncoderConfiguration->Multicast,
	             struct tt__MulticastConfiguration);
	ALLOC_STRUCT(AudioEncoderConfiguration->Multicast->Address,
	             struct tt__IPAddress);
	AudioEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
	ALLOC_TOKEN(AudioEncoderConfiguration->Multicast->Address->IPv4Address,
	            ONVIF_WSD_MC_ADDR);
	AudioEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
	AudioEncoderConfiguration->Multicast->Port = 9090;
	AudioEncoderConfiguration->Multicast->TTL = 1500;
	AudioEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
	//ALLOC_TOKEN(AudioEncoderConfiguration->SessionTimeout, "PT15S");
	//yyyyyyyyyyyyyyyyyy
	AudioEncoderConfiguration->SessionTimeout = 15000;
	Profile->AudioEncoderConfiguration = AudioEncoderConfiguration;
	/* VideoAnalyticsConfiguration */
	Profile->VideoAnalyticsConfiguration = NULL;
	/* PTZConfiguration */
	ALLOC_STRUCT(PTZConfiguration, struct tt__PTZConfiguration);
	ALLOC_TOKEN(PTZConfiguration->Name, ONVIF_PTZ_NAME);
	PTZConfiguration->UseCount = 1;
	ALLOC_TOKEN(PTZConfiguration->token, ONVIF_PTZ_TOKEN);
	ALLOC_TOKEN(PTZConfiguration->token, ONVIF_PTZ_TOKEN);
	ALLOC_TOKEN(PTZConfiguration->NodeToken, ONVIF_PTZ_TOKEN);
	ALLOC_TOKEN(PTZConfiguration->DefaultAbsolutePantTiltPositionSpace,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
	ALLOC_TOKEN(PTZConfiguration->DefaultAbsoluteZoomPositionSpace,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace");
	ALLOC_TOKEN(PTZConfiguration->DefaultRelativePanTiltTranslationSpace,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace");
	ALLOC_TOKEN(PTZConfiguration->DefaultRelativeZoomTranslationSpace,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace ");
	ALLOC_TOKEN(PTZConfiguration->DefaultContinuousPanTiltVelocitySpace,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace ");
	ALLOC_TOKEN(PTZConfiguration->DefaultContinuousZoomVelocitySpace,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace ");
	ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed, struct tt__PTZSpeed);
	ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed->PanTilt, struct tt__Vector2D);
	ALLOC_TOKEN(PTZConfiguration->DefaultPTZSpeed->PanTilt->space,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace ");
	PTZConfiguration->DefaultPTZSpeed->PanTilt->x = 1.0;
	PTZConfiguration->DefaultPTZSpeed->PanTilt->y = 0.5;
	ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed->Zoom, struct tt__Vector1D);
	ALLOC_TOKEN(PTZConfiguration->DefaultPTZSpeed->Zoom->space,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace ");
	PTZConfiguration->DefaultPTZSpeed->Zoom->x = 0.0;
	//yyyyyyyyyyyyyyyyyyyyy
	ALLOC_STRUCT(PTZConfiguration->DefaultPTZTimeout, LONG64);
	*PTZConfiguration->DefaultPTZTimeout = 5000;
	//ALLOC_TOKEN(PTZConfiguration->DefaultPTZTimeout, "PT5S");
	//#####################################################
	ALLOC_STRUCT(PTZConfiguration->PanTiltLimits, struct tt__PanTiltLimits);
	ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range,
	             struct tt__Space2DDescription);
	ALLOC_TOKEN(PTZConfiguration->PanTiltLimits->Range->URI,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
	ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range->XRange,
	             struct tt__FloatRange);
	PTZConfiguration->PanTiltLimits->Range->XRange->Max = 1.0;
	PTZConfiguration->PanTiltLimits->Range->XRange->Min = - 1.0;
	ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range->YRange,
	             struct tt__FloatRange);
	PTZConfiguration->PanTiltLimits->Range->YRange->Max = 1.0;
	PTZConfiguration->PanTiltLimits->Range->YRange->Min = - 1.0;
	//############################################################
	ALLOC_STRUCT(PTZConfiguration->ZoomLimits, struct tt__ZoomLimits);
	ALLOC_STRUCT(PTZConfiguration->ZoomLimits->Range,
	             struct tt__Space1DDescription);
	ALLOC_TOKEN(PTZConfiguration->ZoomLimits->Range->URI,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace");
	ALLOC_STRUCT(PTZConfiguration->ZoomLimits->Range->XRange,
	             struct tt__FloatRange);
	PTZConfiguration->ZoomLimits->Range->XRange->Max = 1.0;
	PTZConfiguration->ZoomLimits->Range->XRange->Min = 0.0;
	Profile->PTZConfiguration = PTZConfiguration;
	/* MetadataConfiguration */
	Profile->MetadataConfiguration = NULL;
	//ALLOC_STRUCT(Profile->Extension, struct tt__ProfileExtension);
	//ALLOC_STRUCT(Profile->Extension->AudioOutputConfiguration, struct tt__ProfileExtension);
	trt__CreateProfileResponse->Profile = Profile;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetProfile(struct soap* soap,
                       struct _trt__GetProfile* trt__GetProfile,
                       struct _trt__GetProfileResponse* trt__GetProfileResponse)
{
	struct tt__Profile* Profile;
	struct tt__VideoSourceConfiguration* VideoSourceConfiguration;
	struct tt__VideoEncoderConfiguration* VideoEncoderConfiguration;
	struct tt__AudioSourceConfiguration* AudioSourceConfiguration;
	struct tt__AudioEncoderConfiguration* AudioEncoderConfiguration;
	struct tt__PTZConfiguration* PTZConfiguration;
	HI_DEV_VIDEO_CFG_S VideoCfg;
	HI_ONVIF_PROFILE_S OnvifProfile;
	HI_ONVIF_VIDEO_SOURCE_CFG_S OnvifVideoSourceCfg;
	HI_ONVIF_VIDEO_ENCODER_CFG_S OnvifVideoEncoderCfg;
	HI_ONVIF_AUDIO_SOURCE_CFG_S OnvifAudioSourceCfg;
	HI_ONVIF_AUDIO_ENCODER_CFG_S OnvifAudioEncoderCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;
	unsigned long ip;
	ip = onvif_get_ipaddr(soap);

	if(trt__GetProfile->ProfileToken == NULL)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidInputToken");
		return SOAP_FAULT;
	}

	if(onvif_get_profile_from_token(ip, trt__GetProfile->ProfileToken,
	                                &OnvifProfile) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(Profile, struct tt__Profile);
	ALLOC_TOKEN(Profile->Name, OnvifProfile.Name);
	ALLOC_TOKEN(Profile->token, OnvifProfile.token);
	ALLOC_STRUCT(Profile->fixed,  enum xsd__boolean);
	*Profile->fixed = OnvifProfile.fixed;	// 这个profile不能删除
	/* VideoSourceConfiguration */
	onvif_get_video_source_cfg(OnvifProfile.videoSourceCfgIdx,
	                           &OnvifVideoSourceCfg);
	ALLOC_STRUCT(VideoSourceConfiguration,  struct tt__VideoSourceConfiguration);
	ALLOC_TOKEN(VideoSourceConfiguration->Name, OnvifVideoSourceCfg.Name);
	ALLOC_TOKEN(VideoSourceConfiguration->SourceToken,
	            OnvifVideoSourceCfg.SourceToken);
	VideoSourceConfiguration->UseCount = OnvifVideoSourceCfg.UseCount;
	ALLOC_STRUCT(VideoSourceConfiguration->Bounds, struct tt__IntRectangle);
	VideoSourceConfiguration->Bounds->y = OnvifVideoSourceCfg.Bounds.y;
	VideoSourceConfiguration->Bounds->x = OnvifVideoSourceCfg.Bounds.x;
	VideoSourceConfiguration->Bounds->width  = OnvifVideoSourceCfg.Bounds.width;
	VideoSourceConfiguration->Bounds->height = OnvifVideoSourceCfg.Bounds.height;
	Profile->VideoSourceConfiguration = VideoSourceConfiguration;
	/* AudioSourceConfiguration */
	onvif_get_audio_source_cfg(OnvifProfile.audioSourceCfgIdx,
	                           &OnvifAudioSourceCfg);
	ALLOC_STRUCT(AudioSourceConfiguration, struct tt__AudioSourceConfiguration);
	ALLOC_TOKEN(AudioSourceConfiguration->Name, OnvifAudioSourceCfg.Name);
	ALLOC_TOKEN(AudioSourceConfiguration->token, OnvifAudioSourceCfg.token);
	ALLOC_TOKEN(AudioSourceConfiguration->SourceToken,
	            OnvifAudioSourceCfg.SourceToken);
	AudioSourceConfiguration->UseCount = OnvifAudioSourceCfg.UseCount;
	Profile->AudioSourceConfiguration = AudioSourceConfiguration;
	/*VideoEncoderConfiguration */
	onvif_get_video_encoder_cfg(OnvifProfile.videoEncoderCfgIdx,
	                            &OnvifVideoEncoderCfg);
	ALLOC_STRUCT(VideoEncoderConfiguration, struct tt__VideoEncoderConfiguration);
	ALLOC_TOKEN(VideoEncoderConfiguration->Name, OnvifVideoEncoderCfg.Name);
	ALLOC_TOKEN(VideoEncoderConfiguration->token, OnvifVideoEncoderCfg.token);
	VideoEncoderConfiguration->UseCount = OnvifVideoEncoderCfg.UseCount;
	VideoEncoderConfiguration->Encoding = OnvifVideoEncoderCfg.Encoding;
	ALLOC_STRUCT(VideoEncoderConfiguration->Resolution, struct tt__VideoResolution);
	VideoEncoderConfiguration->Resolution->Width =
	    OnvifVideoEncoderCfg.Resolution.Width;
	VideoEncoderConfiguration->Resolution->Height =
	    OnvifVideoEncoderCfg.Resolution.Height;
	VideoEncoderConfiguration->Quality = OnvifVideoEncoderCfg.Quality;
	ALLOC_STRUCT(VideoEncoderConfiguration->RateControl,
	             struct tt__VideoRateControl);
	VideoEncoderConfiguration->RateControl->FrameRateLimit =
	    OnvifVideoEncoderCfg.FrameRate;
	VideoEncoderConfiguration->RateControl->EncodingInterval =
	    OnvifVideoEncoderCfg.Gop;
	VideoEncoderConfiguration->RateControl->BitrateLimit =
	    OnvifVideoEncoderCfg.Bitrate;
	VideoEncoderConfiguration->MPEG4 = NULL;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VIDEO_PARAM,  0,
	                     (char*)&VideoCfg, sizeof(HI_DEV_VIDEO_CFG_S));
	ALLOC_STRUCT(VideoEncoderConfiguration->H264, struct tt__H264Configuration);
	VideoEncoderConfiguration->H264->GovLength = VideoCfg.struVenc[0].u8Gop;

	switch(VideoCfg.struVenc[0].u8Profile)
	{
		case HI_VENC_PROFILE_HIGH:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__High;
			break;

		case HI_VENC_PROFILE_MAIN:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Main;
			break;

		case HI_VENC_PROFILE_BASELINE:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Baseline;
			break;
	}

	ALLOC_STRUCT(VideoEncoderConfiguration->Multicast,
	             struct tt__MulticastConfiguration);
	ALLOC_STRUCT(VideoEncoderConfiguration->Multicast->Address,
	             struct tt__IPAddress);
	VideoEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
	ALLOC_TOKEN(VideoEncoderConfiguration->Multicast->Address->IPv4Address,
	            ONVIF_WSD_MC_ADDR);
	VideoEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
	VideoEncoderConfiguration->Multicast->Port = 9090;
	VideoEncoderConfiguration->Multicast->TTL = 1500;
	VideoEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
	//yyyyyyyyy
	//ALLOC_TOKEN(VideoEncoderConfiguration->SessionTimeout, "PT5S");
	VideoEncoderConfiguration->SessionTimeout = 5000;
	Profile->VideoEncoderConfiguration = VideoEncoderConfiguration;
	/* AudioEncoderConfiguration */
	onvif_get_audio_encoder_cfg(OnvifProfile.audioEncoderCfgIdx,
	                            &OnvifAudioEncoderCfg);
	ALLOC_STRUCT(AudioEncoderConfiguration, struct tt__AudioEncoderConfiguration);
	ALLOC_TOKEN(AudioEncoderConfiguration->Name, OnvifAudioEncoderCfg.Name);
	AudioEncoderConfiguration->UseCount = OnvifAudioEncoderCfg.UseCount;
	ALLOC_TOKEN(AudioEncoderConfiguration->token, OnvifAudioEncoderCfg.token);
	AudioEncoderConfiguration->Encoding = OnvifAudioEncoderCfg.Encoding;
	AudioEncoderConfiguration->Bitrate = OnvifAudioEncoderCfg.Bitrate;
	AudioEncoderConfiguration->SampleRate = OnvifAudioEncoderCfg.SampleRate;
	ALLOC_STRUCT(AudioEncoderConfiguration->Multicast,
	             struct tt__MulticastConfiguration);
	ALLOC_STRUCT(AudioEncoderConfiguration->Multicast->Address,
	             struct tt__IPAddress);
	AudioEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
	ALLOC_TOKEN(AudioEncoderConfiguration->Multicast->Address->IPv4Address,
	            ONVIF_WSD_MC_ADDR);
	AudioEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
	AudioEncoderConfiguration->Multicast->Port = 9090;
	AudioEncoderConfiguration->Multicast->TTL = 1500;
	AudioEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
	//yyyyyyyyyyy
	//ALLOC_TOKEN(AudioEncoderConfiguration->SessionTimeout, "PT15S");
	AudioEncoderConfiguration->SessionTimeout = 15000;
	Profile->AudioEncoderConfiguration = AudioEncoderConfiguration;
	/* VideoAnalyticsConfiguration */
	Profile->VideoAnalyticsConfiguration = NULL;
	/* PTZConfiguration */
	ALLOC_STRUCT(PTZConfiguration, struct tt__PTZConfiguration);
	ALLOC_TOKEN(PTZConfiguration->Name, ONVIF_PTZ_NAME);
	PTZConfiguration->UseCount = 1;
	ALLOC_TOKEN(PTZConfiguration->token, ONVIF_PTZ_TOKEN);
	ALLOC_TOKEN(PTZConfiguration->token, ONVIF_PTZ_TOKEN);
	ALLOC_TOKEN(PTZConfiguration->NodeToken, ONVIF_PTZ_TOKEN);
	ALLOC_TOKEN(PTZConfiguration->DefaultAbsolutePantTiltPositionSpace,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
	ALLOC_TOKEN(PTZConfiguration->DefaultAbsoluteZoomPositionSpace,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace");
	ALLOC_TOKEN(PTZConfiguration->DefaultRelativePanTiltTranslationSpace,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace");
	ALLOC_TOKEN(PTZConfiguration->DefaultRelativeZoomTranslationSpace,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace ");
	ALLOC_TOKEN(PTZConfiguration->DefaultContinuousPanTiltVelocitySpace,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace ");
	ALLOC_TOKEN(PTZConfiguration->DefaultContinuousZoomVelocitySpace,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace ");
	ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed, struct tt__PTZSpeed);
	ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed->PanTilt, struct tt__Vector2D);
	ALLOC_TOKEN(PTZConfiguration->DefaultPTZSpeed->PanTilt->space,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace ");
	PTZConfiguration->DefaultPTZSpeed->PanTilt->x = 1.0;
	PTZConfiguration->DefaultPTZSpeed->PanTilt->y = 0.5;
	ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed->Zoom, struct tt__Vector1D);
	ALLOC_TOKEN(PTZConfiguration->DefaultPTZSpeed->Zoom->space,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace ");
	PTZConfiguration->DefaultPTZSpeed->Zoom->x = 0.0;
	//yyyyyyyyy
	//ALLOC_TOKEN(PTZConfiguration->DefaultPTZTimeout, "PT5S");
	ALLOC_STRUCT(PTZConfiguration->DefaultPTZTimeout, LONG64);
	*PTZConfiguration->DefaultPTZTimeout = 5;
	//#####################################################
	ALLOC_STRUCT(PTZConfiguration->PanTiltLimits, struct tt__PanTiltLimits);
	ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range,
	             struct tt__Space2DDescription);
	ALLOC_TOKEN(PTZConfiguration->PanTiltLimits->Range->URI,
	            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
	ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range->XRange,
	             struct tt__FloatRange);
	PTZConfiguration->PanTiltLimits->Range->XRange->Max = 1.0;
	PTZConfiguration->PanTiltLimits->Range->XRange->Min = - 1.0;
	ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range->YRange,
	             struct tt__FloatRange);
	PTZConfiguration->PanTiltLimits->Range->YRange->Max = 1.0;
	PTZConfiguration->PanTiltLimits->Range->YRange->Min = - 1.0;
	//############################################################
	ALLOC_STRUCT(PTZConfiguration->ZoomLimits, struct tt__ZoomLimits);
	ALLOC_STRUCT(PTZConfiguration->ZoomLimits->Range,
	             struct tt__Space1DDescription);
	ALLOC_TOKEN(PTZConfiguration->ZoomLimits->Range->URI,
	            "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace");
	ALLOC_STRUCT(PTZConfiguration->ZoomLimits->Range->XRange,
	             struct tt__FloatRange);
	PTZConfiguration->ZoomLimits->Range->XRange->Max = 1.0;
	PTZConfiguration->ZoomLimits->Range->XRange->Min = 0.0;
	Profile->PTZConfiguration = PTZConfiguration;
	/* MetadataConfiguration */
	Profile->MetadataConfiguration = NULL;
	//ALLOC_STRUCT(Profile->Extension, struct tt__ProfileExtension);
	//ALLOC_STRUCT(Profile->Extension->AudioOutputConfiguration, struct tt__ProfileExtension);
	trt__GetProfileResponse->Profile = Profile;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetProfiles(struct soap* soap,
                        struct _trt__GetProfiles* trt__GetProfiles,
                        struct _trt__GetProfilesResponse* trt__GetProfilesResponse)
{
	int idx = 0;
	struct tt__Profile* Profile, *ProfileArray;
	struct tt__VideoSourceConfiguration* VideoSourceConfiguration;
	struct tt__VideoEncoderConfiguration* VideoEncoderConfiguration;
	struct tt__AudioSourceConfiguration* AudioSourceConfiguration;
	struct tt__AudioEncoderConfiguration* AudioEncoderConfiguration;
	struct tt__PTZConfiguration* PTZConfiguration;
	HI_ONVIF_PROFILE_S OnvifProfile;
	HI_ONVIF_VIDEO_SOURCE_CFG_S OnvifVideoSourceCfg;
	HI_ONVIF_VIDEO_ENCODER_CFG_S OnvifVideoEncoderCfg;
	HI_ONVIF_AUDIO_SOURCE_CFG_S OnvifAudioSourceCfg;
	HI_ONVIF_AUDIO_ENCODER_CFG_S OnvifAudioEncoderCfg;
	HI_DEV_VIDEO_CFG_S VideoCfg;
	unsigned long ip;
	ip = onvif_get_ipaddr(soap);
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT_NUM(ProfileArray, struct tt__Profile, ONVIF_PROFILE_NUM);

	for(idx = 0; idx < ONVIF_PROFILE_NUM; idx++)
	{
		onvif_get_profile(idx, ip, &OnvifProfile);
		Profile = &ProfileArray[idx];
		ALLOC_TOKEN(Profile->Name, OnvifProfile.Name);
		ALLOC_TOKEN(Profile->token, OnvifProfile.token);
		ALLOC_STRUCT(Profile->fixed, enum xsd__boolean);
		*Profile->fixed = OnvifProfile.fixed;	// 这个profile不能删除
		/* VideoSourceConfiguration */
		onvif_get_video_source_cfg(OnvifProfile.videoSourceCfgIdx,
		                           &OnvifVideoSourceCfg);
		ALLOC_STRUCT(VideoSourceConfiguration,  struct tt__VideoSourceConfiguration);
		ALLOC_TOKEN(VideoSourceConfiguration->token, OnvifVideoSourceCfg.token);
		ALLOC_TOKEN(VideoSourceConfiguration->Name, OnvifVideoSourceCfg.Name);
		ALLOC_TOKEN(VideoSourceConfiguration->SourceToken,
		            OnvifVideoSourceCfg.SourceToken);
		VideoSourceConfiguration->UseCount = OnvifVideoSourceCfg.UseCount;
		ALLOC_STRUCT(VideoSourceConfiguration->Bounds, struct tt__IntRectangle);
		VideoSourceConfiguration->Bounds->y = OnvifVideoSourceCfg.Bounds.y;
		VideoSourceConfiguration->Bounds->x = OnvifVideoSourceCfg.Bounds.x;
		VideoSourceConfiguration->Bounds->width  = OnvifVideoSourceCfg.Bounds.width;
		VideoSourceConfiguration->Bounds->height = OnvifVideoSourceCfg.Bounds.height;
		Profile->VideoSourceConfiguration = VideoSourceConfiguration;
		/* AudioSourceConfiguration */
		onvif_get_audio_source_cfg(OnvifProfile.audioSourceCfgIdx,
		                           &OnvifAudioSourceCfg);
		ALLOC_STRUCT(AudioSourceConfiguration, struct tt__AudioSourceConfiguration);
		ALLOC_TOKEN(AudioSourceConfiguration->Name, OnvifAudioSourceCfg.Name);
		ALLOC_TOKEN(AudioSourceConfiguration->token, OnvifAudioSourceCfg.token);
		ALLOC_TOKEN(AudioSourceConfiguration->SourceToken,
		            OnvifAudioSourceCfg.SourceToken);
		AudioSourceConfiguration->UseCount = OnvifAudioSourceCfg.UseCount;
		Profile->AudioSourceConfiguration = AudioSourceConfiguration;
		/*VideoEncoderConfiguration */
		onvif_get_video_encoder_cfg(OnvifProfile.videoEncoderCfgIdx,
		                            &OnvifVideoEncoderCfg);

		if(0 &&  OnvifVideoEncoderCfg.AdvanceEncType == 4)
		{
			//如果是 h265视频时，则不填写 VideoEncoderConfiguration 结构体。
			__D("Is h265 encode,not write VideoEncoderConfiguration!\n");
			Profile->VideoEncoderConfiguration = NULL;
		}
		else
		{
			ALLOC_STRUCT(VideoEncoderConfiguration, struct tt__VideoEncoderConfiguration);
			ALLOC_TOKEN(VideoEncoderConfiguration->Name, OnvifVideoEncoderCfg.Name);
			ALLOC_TOKEN(VideoEncoderConfiguration->token, OnvifVideoEncoderCfg.token);
			VideoEncoderConfiguration->UseCount = OnvifVideoEncoderCfg.UseCount;
			VideoEncoderConfiguration->Encoding = OnvifVideoEncoderCfg.Encoding;
			ALLOC_STRUCT(VideoEncoderConfiguration->Resolution, struct tt__VideoResolution);
			VideoEncoderConfiguration->Resolution->Width =
			    OnvifVideoEncoderCfg.Resolution.Width;
			VideoEncoderConfiguration->Resolution->Height =
			    OnvifVideoEncoderCfg.Resolution.Height;
			VideoEncoderConfiguration->Quality = OnvifVideoEncoderCfg.Quality;
			ALLOC_STRUCT(VideoEncoderConfiguration->RateControl,
			             struct tt__VideoRateControl);
			VideoEncoderConfiguration->RateControl->FrameRateLimit =
			    OnvifVideoEncoderCfg.FrameRate;
			VideoEncoderConfiguration->RateControl->EncodingInterval =
			    OnvifVideoEncoderCfg.Gop;
			VideoEncoderConfiguration->RateControl->BitrateLimit =
			    OnvifVideoEncoderCfg.Bitrate;
			VideoEncoderConfiguration->MPEG4 = NULL;
			gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VIDEO_PARAM,  0,
			                     (char*)&VideoCfg, sizeof(HI_DEV_VIDEO_CFG_S));
			ALLOC_STRUCT(VideoEncoderConfiguration->H264, struct tt__H264Configuration);
#if 0

			if(idx == 0)
			{
				VideoEncoderConfiguration->H264->GovLength = VideoCfg.struVenc[0].u8Gop;
			}
			else
			{
				VideoEncoderConfiguration->H264->GovLength = VideoCfg.struVenc[1].u8Gop;
			}

#else
			VideoEncoderConfiguration->H264->GovLength = VideoCfg.struVenc[idx].u8Gop;
#endif

			switch(VideoCfg.struVenc[idx].u8Profile)
			{
				case HI_VENC_PROFILE_HIGH:
					VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__High;
					break;

				case HI_VENC_PROFILE_MAIN:
					VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Main;
					break;

				case HI_VENC_PROFILE_BASELINE:
					VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Baseline;
					break;
			}

			ALLOC_STRUCT(VideoEncoderConfiguration->Multicast,
			             struct tt__MulticastConfiguration);
			ALLOC_STRUCT(VideoEncoderConfiguration->Multicast->Address,
			             struct tt__IPAddress);
			VideoEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
			ALLOC_TOKEN(VideoEncoderConfiguration->Multicast->Address->IPv4Address,
			            ONVIF_WSD_MC_ADDR);
			VideoEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
			VideoEncoderConfiguration->Multicast->Port = 9090;
			VideoEncoderConfiguration->Multicast->TTL = 1500;
			VideoEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
			//yyyyyyyyyyyyyy
			VideoEncoderConfiguration->SessionTimeout = 5000;
			//ALLOC_TOKEN(VideoEncoderConfiguration->SessionTimeout, "PT5S");
			Profile->VideoEncoderConfiguration = VideoEncoderConfiguration;
		}

		/* VideoAnalyticsConfiguration */
		///////////////////////////////////////////
#if 1
		struct tt__VideoAnalyticsConfiguration* Configurations = NULL;
		struct tt__AnalyticsEngineConfiguration* AnalyticsEngineConfiguration = NULL;
		struct tt__RuleEngineConfiguration* RuleEngineConfiguration = NULL;
		//trt__GetVideoAnalyticsConfigurationsResponse->__sizeConfigurations = 1;
		ALLOC_STRUCT(Configurations, struct tt__VideoAnalyticsConfiguration);
		//trt__GetVideoAnalyticsConfigurationsResponse->Configurations = Configurations;
		Configurations->Name = soap_malloc(soap, 32);
		Configurations->UseCount = 2;
		Configurations->token = soap_malloc(soap, 32);
		strcpy(Configurations->Name, ONVIF_VIDEO_ANALY_CFG_NAME);
		strcpy(Configurations->token, ONVIF_VIDEO_ANALY_CFG_TOKEN);
		Configurations->__size = 0;
		Configurations->__any = NULL;
		Configurations->__anyAttribute = NULL;
		ALLOC_STRUCT(AnalyticsEngineConfiguration,
		             struct tt__AnalyticsEngineConfiguration);
		ALLOC_STRUCT(RuleEngineConfiguration, struct tt__RuleEngineConfiguration);
		Configurations->AnalyticsEngineConfiguration = AnalyticsEngineConfiguration;
		Configurations->RuleEngineConfiguration = RuleEngineConfiguration;
		struct tt__Config* AnalyticsModule = NULL;
		AnalyticsEngineConfiguration->__sizeAnalyticsModule = 2;
		AnalyticsModule =  soap_malloc(soap,
		                               AnalyticsEngineConfiguration->__sizeAnalyticsModule * sizeof(
		                                   struct tt__Config));
		AnalyticsEngineConfiguration->AnalyticsModule = AnalyticsModule;
		AnalyticsEngineConfiguration->Extension = NULL;
		AnalyticsEngineConfiguration->__anyAttribute = NULL;
		ALLOC_TOKEN(AnalyticsModule[0].Name, "MyCellMotionModule");
		ALLOC_TOKEN(AnalyticsModule[0].Type, "tt:CellMotionEngine");
		ALLOC_STRUCT(AnalyticsModule[0].Parameters, struct tt__ItemList);
		AnalyticsModule[0].Parameters->__sizeSimpleItem = 1;
		AnalyticsModule[0].Parameters->SimpleItem = soap_malloc(soap,
		        AnalyticsModule[0].Parameters->__sizeSimpleItem * sizeof(
		            struct _tt__ItemList_SimpleItem));
		ALLOC_TOKEN(AnalyticsModule[0].Parameters->SimpleItem->Name, "Sensitivity");
		ALLOC_TOKEN(AnalyticsModule[0].Parameters->SimpleItem->Value, "75");
		AnalyticsModule[0].Parameters->__sizeElementItem = 1;
		AnalyticsModule[0].Parameters->ElementItem = soap_malloc(soap,
		        AnalyticsModule[0].Parameters->__sizeElementItem * sizeof(
		            struct _tt__ItemList_ElementItem));
		ALLOC_TOKEN(AnalyticsModule[0].Parameters->ElementItem->Name,  "Layout");
		AnalyticsModule[0].Parameters->ElementItem->__any = soap_malloc(soap, 1024);
		sprintf(AnalyticsModule[0].Parameters->ElementItem->__any,
		        "<tt:CellLayout Columns=\"22\" Rows=\"18\">"
		        "<tt:Transformation>"
		        "<tt:Translate x=\"0.000000\" y=\"0.000000\"/>"
		        "<tt:Scale x=\"0.000000\" y=\"0.000000\"/>"
		        "</tt:Transformation>"
		        "</tt:CellLayout>");
		ALLOC_TOKEN(AnalyticsModule[1].Name, "MyTamperDetectModule");
		ALLOC_TOKEN(AnalyticsModule[1].Type,
		            "ns2:TamperEngine");//"hikxsd:TamperEngine");////20140605 zj
		ALLOC_STRUCT(AnalyticsModule[1].Parameters, struct tt__ItemList);
		AnalyticsModule[1].Parameters->__sizeSimpleItem = 1;
		AnalyticsModule[1].Parameters->SimpleItem = soap_malloc(soap,
		        AnalyticsModule[1].Parameters->__sizeSimpleItem * sizeof(
		            struct _tt__ItemList_SimpleItem));
		ALLOC_TOKEN(AnalyticsModule[1].Parameters->SimpleItem->Name, "Sensitivity");
		ALLOC_TOKEN(AnalyticsModule[1].Parameters->SimpleItem->Value, "100");
		AnalyticsModule[1].Parameters->__sizeElementItem = 2;
		AnalyticsModule[1].Parameters->ElementItem = soap_malloc(soap,
		        AnalyticsModule[1].Parameters->__sizeElementItem * sizeof(
		            struct _tt__ItemList_ElementItem));
		ALLOC_TOKEN(AnalyticsModule[1].Parameters->ElementItem[0].Name,
		            "Transformation");
		AnalyticsModule[1].Parameters->ElementItem[0].__any = soap_malloc(soap, 1024);
		sprintf(AnalyticsModule[1].Parameters->ElementItem[0].__any,
		        "<tt:Transformation>"
		        "<tt:Translate x=\"-1.000000\" y=\"-1.000000\"/>"
		        "<tt:Scale x=\"0.002841\" y=\"0.003472\"/>"
		        "</tt:Transformation>");
		ALLOC_TOKEN(AnalyticsModule[1].Parameters->ElementItem[1].Name,  "Field");
		AnalyticsModule[1].Parameters->ElementItem[1].__any = soap_malloc(soap, 1024);
		sprintf(AnalyticsModule[1].Parameters->ElementItem[1].__any,
		        "<tt:PolygonConfiguration>"
		        "<tt:Polygon>"
		        "<tt:Point x=\"0\" y=\"0\"/>"
		        "<tt:Point x=\"0\" y=\"576\"/>"
		        "<tt:Point x=\"704\" y=\"576\"/>"
		        "<tt:Point x=\"704\" y=\"0\"/>"
		        "</tt:Polygon>"
		        "</tt:PolygonConfiguration>");
		RuleEngineConfiguration->__sizeRule = 2;
		RuleEngineConfiguration->Extension = NULL;
		RuleEngineConfiguration->__anyAttribute = NULL;
		RuleEngineConfiguration->Rule = soap_malloc(soap,
		                                RuleEngineConfiguration->__sizeRule * sizeof(struct tt__Config));
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Name, "MyMotionDetectorRule");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Type, "tt:CellMotionDetector");
		ALLOC_STRUCT(RuleEngineConfiguration->Rule[0].Parameters, struct tt__ItemList);
		RuleEngineConfiguration->Rule[0].Parameters->__sizeSimpleItem = 4;
		RuleEngineConfiguration->Rule[0].Parameters->SimpleItem = soap_malloc(soap,
		        RuleEngineConfiguration->Rule[0].Parameters->__sizeSimpleItem * sizeof(
		            struct _tt__ItemList_SimpleItem));
		RuleEngineConfiguration->Rule[0].Parameters->Extension = NULL;
		RuleEngineConfiguration->Rule[0].Parameters->__anyAttribute = NULL;
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[0].Name,
		            "MinCount");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[0].Value,
		            "1");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[1].Name,
		            "AlarmOnDelay");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[1].Value,
		            "100");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[2].Name,
		            "AlarmOffDelay");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[2].Value,
		            "1000");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[3].Name,
		            "ActiveCells");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[3].Value,
		            "0P8A8A==");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[1].Name, "MyTamperDetectorRule");
		ALLOC_TOKEN(RuleEngineConfiguration->Rule[1].Type,
		            "ns2:TamperDetector");//"hikxsd:TamperDetector");////20140605 zj
		ALLOC_STRUCT(RuleEngineConfiguration->Rule[1].Parameters, struct tt__ItemList);
		RuleEngineConfiguration->Rule[1].Parameters->__sizeElementItem = 0;
		RuleEngineConfiguration->Rule[1].Parameters->ElementItem = NULL;
		RuleEngineConfiguration->Rule[1].Parameters->__sizeSimpleItem = 0;
		RuleEngineConfiguration->Rule[1].Parameters->SimpleItem = NULL;
		RuleEngineConfiguration->Rule[1].Parameters->Extension = NULL;
		RuleEngineConfiguration->Rule[1].Parameters->__anyAttribute = NULL;
		Profile->VideoAnalyticsConfiguration = Configurations;
#else
		Profile->VideoAnalyticsConfiguration = NULL;
#endif
		////////////////////////////////////////
		/* AudioEncoderConfiguration */
		onvif_get_audio_encoder_cfg(OnvifProfile.audioEncoderCfgIdx,
		                            &OnvifAudioEncoderCfg);
		ALLOC_STRUCT(AudioEncoderConfiguration, struct tt__AudioEncoderConfiguration);
		ALLOC_TOKEN(AudioEncoderConfiguration->Name, OnvifAudioEncoderCfg.Name);
		AudioEncoderConfiguration->UseCount = OnvifAudioEncoderCfg.UseCount;
		ALLOC_TOKEN(AudioEncoderConfiguration->token, OnvifAudioEncoderCfg.token);
		AudioEncoderConfiguration->Encoding = OnvifAudioEncoderCfg.Encoding;
		AudioEncoderConfiguration->Bitrate = OnvifAudioEncoderCfg.Bitrate;
		AudioEncoderConfiguration->SampleRate = OnvifAudioEncoderCfg.SampleRate;
		ALLOC_STRUCT(AudioEncoderConfiguration->Multicast,
		             struct tt__MulticastConfiguration);
		ALLOC_STRUCT(AudioEncoderConfiguration->Multicast->Address,
		             struct tt__IPAddress);
		AudioEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
		ALLOC_TOKEN(AudioEncoderConfiguration->Multicast->Address->IPv4Address,
		            ONVIF_WSD_MC_ADDR);
		AudioEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
		AudioEncoderConfiguration->Multicast->Port = 9090;
		AudioEncoderConfiguration->Multicast->TTL = 1500;
		AudioEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
		//yyyyyyyyyyyyyyyy
		AudioEncoderConfiguration->SessionTimeout = 15000;
		//ALLOC_TOKEN(AudioEncoderConfiguration->SessionTimeout, "PT15S");
		Profile->AudioEncoderConfiguration = AudioEncoderConfiguration;
		/* PTZConfiguration */
		ALLOC_STRUCT(PTZConfiguration, struct tt__PTZConfiguration);
		ALLOC_TOKEN(PTZConfiguration->Name, ONVIF_PTZ_NAME);
		PTZConfiguration->UseCount = 1;
		ALLOC_TOKEN(PTZConfiguration->token, ONVIF_PTZ_TOKEN);
		ALLOC_TOKEN(PTZConfiguration->token, ONVIF_PTZ_TOKEN);
		ALLOC_TOKEN(PTZConfiguration->NodeToken, ONVIF_PTZ_TOKEN);
		ALLOC_TOKEN(PTZConfiguration->DefaultAbsolutePantTiltPositionSpace,
		            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
		ALLOC_TOKEN(PTZConfiguration->DefaultAbsoluteZoomPositionSpace,
		            "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace");
		ALLOC_TOKEN(PTZConfiguration->DefaultRelativePanTiltTranslationSpace,
		            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace");
		ALLOC_TOKEN(PTZConfiguration->DefaultRelativeZoomTranslationSpace,
		            "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace ");
		ALLOC_TOKEN(PTZConfiguration->DefaultContinuousPanTiltVelocitySpace,
		            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace ");
		ALLOC_TOKEN(PTZConfiguration->DefaultContinuousZoomVelocitySpace,
		            "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace ");
		ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed, struct tt__PTZSpeed);
		ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed->PanTilt, struct tt__Vector2D);
		ALLOC_TOKEN(PTZConfiguration->DefaultPTZSpeed->PanTilt->space,
		            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace ");
		PTZConfiguration->DefaultPTZSpeed->PanTilt->x = 1.0;
		PTZConfiguration->DefaultPTZSpeed->PanTilt->y = 0.5;
		ALLOC_STRUCT(PTZConfiguration->DefaultPTZSpeed->Zoom, struct tt__Vector1D);
		ALLOC_TOKEN(PTZConfiguration->DefaultPTZSpeed->Zoom->space,
		            "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace ");
		PTZConfiguration->DefaultPTZSpeed->Zoom->x = 0.0;
		//yyyyyy
		//PTZConfiguration->DefaultPTZTimeout
		ALLOC_STRUCT(PTZConfiguration->DefaultPTZTimeout, LONG64);
		*PTZConfiguration->DefaultPTZTimeout = 5000;
		//ALLOC_TOKEN(PTZConfiguration->DefaultPTZTimeout, "PT5S");
		//#####################################################
		ALLOC_STRUCT(PTZConfiguration->PanTiltLimits, struct tt__PanTiltLimits);
		ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range,
		             struct tt__Space2DDescription);
		ALLOC_TOKEN(PTZConfiguration->PanTiltLimits->Range->URI,
		            "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
		ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range->XRange,
		             struct tt__FloatRange);
		PTZConfiguration->PanTiltLimits->Range->XRange->Max = 1.0;
		PTZConfiguration->PanTiltLimits->Range->XRange->Min = - 1.0;
		ALLOC_STRUCT(PTZConfiguration->PanTiltLimits->Range->YRange,
		             struct tt__FloatRange);
		PTZConfiguration->PanTiltLimits->Range->YRange->Max = 1.0;
		PTZConfiguration->PanTiltLimits->Range->YRange->Min = - 1.0;
		//############################################################
		ALLOC_STRUCT(PTZConfiguration->ZoomLimits, struct tt__ZoomLimits);
		ALLOC_STRUCT(PTZConfiguration->ZoomLimits->Range,
		             struct tt__Space1DDescription);
		ALLOC_TOKEN(PTZConfiguration->ZoomLimits->Range->URI,
		            "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace");
		ALLOC_STRUCT(PTZConfiguration->ZoomLimits->Range->XRange,
		             struct tt__FloatRange);
		PTZConfiguration->ZoomLimits->Range->XRange->Max = 1.0;
		PTZConfiguration->ZoomLimits->Range->XRange->Min = 0.0;
		Profile->PTZConfiguration = PTZConfiguration;
		/* MetadataConfiguration */
		struct tt__MetadataConfiguration* MetadataConfiguration = NULL;;
		ALLOC_STRUCT(MetadataConfiguration, struct tt__MetadataConfiguration);
		ALLOC_TOKEN(MetadataConfiguration->Name, "MetaData");
		ALLOC_TOKEN(MetadataConfiguration->token, "MetaDataToken");
		MetadataConfiguration->UseCount = 1;
		MetadataConfiguration->PTZStatus = NULL;
		MetadataConfiguration->Events = NULL;
		ALLOC_STRUCT(MetadataConfiguration->Analytics, enum xsd__boolean);
		*MetadataConfiguration->Analytics = xsd__boolean__false_;
		ALLOC_STRUCT(MetadataConfiguration->Multicast,
		             struct tt__MulticastConfiguration);
		ALLOC_STRUCT(MetadataConfiguration->Multicast->Address, struct tt__IPAddress);
		MetadataConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
		MetadataConfiguration->Multicast->Address->IPv4Address = soap_malloc(soap, 32);
		strcpy(MetadataConfiguration->Multicast->Address->IPv4Address,
		       ONVIF_WSD_MC_ADDR);
		MetadataConfiguration->Multicast->Address->IPv6Address = NULL;
		MetadataConfiguration->Multicast->AutoStart = xsd__boolean__false_;
		MetadataConfiguration->Multicast->Port = 0;
		MetadataConfiguration->Multicast->TTL = 0;
		MetadataConfiguration->Multicast->__any = NULL;
		MetadataConfiguration->Multicast->__size = 0;
		MetadataConfiguration->Multicast->__anyAttribute = NULL;
		MetadataConfiguration->SessionTimeout = 12 * 60 * 1000;
		MetadataConfiguration->__size = 0;
		MetadataConfiguration->__any = NULL;
		MetadataConfiguration->Extension = NULL;
		MetadataConfiguration->AnalyticsEngineConfiguration = NULL;
		MetadataConfiguration->__anyAttribute = NULL;
		Profile->MetadataConfiguration = MetadataConfiguration;
	}

	trt__GetProfilesResponse->__sizeProfiles = ONVIF_PROFILE_NUM;
	trt__GetProfilesResponse->Profiles = ProfileArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__AddVideoEncoderConfiguration(struct soap* soap,
        struct _trt__AddVideoEncoderConfiguration* trt__AddVideoEncoderConfiguration,
        struct _trt__AddVideoEncoderConfigurationResponse*
        trt__AddVideoEncoderConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__AddVideoSourceConfiguration(struct soap* soap,
                                        struct _trt__AddVideoSourceConfiguration* trt__AddVideoSourceConfiguration,
                                        struct _trt__AddVideoSourceConfigurationResponse*
                                        trt__AddVideoSourceConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__AddAudioEncoderConfiguration(struct soap* soap,
        struct _trt__AddAudioEncoderConfiguration* trt__AddAudioEncoderConfiguration,
        struct _trt__AddAudioEncoderConfigurationResponse*
        trt__AddAudioEncoderConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__AddAudioSourceConfiguration(struct soap* soap,
                                        struct _trt__AddAudioSourceConfiguration* trt__AddAudioSourceConfiguration,
                                        struct _trt__AddAudioSourceConfigurationResponse*
                                        trt__AddAudioSourceConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__AddPTZConfiguration(struct soap* soap,
                                struct _trt__AddPTZConfiguration* trt__AddPTZConfiguration,
                                struct _trt__AddPTZConfigurationResponse* trt__AddPTZConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__AddVideoAnalyticsConfiguration(struct soap* soap,
        struct _trt__AddVideoAnalyticsConfiguration*
        trt__AddVideoAnalyticsConfiguration,
        struct _trt__AddVideoAnalyticsConfigurationResponse*
        trt__AddVideoAnalyticsConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	//if(trt__AddVideoAnalyticsConfiguration)
	//	return SOAP_FAULT;
	//printf("===%s==%s===\r\n", trt__AddVideoAnalyticsConfiguration->ProfileToken, trt__AddVideoAnalyticsConfiguration->ConfigurationToken);
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__AddMetadataConfiguration(struct soap* soap,
                                     struct _trt__AddMetadataConfiguration* trt__AddMetadataConfiguration,
                                     struct _trt__AddMetadataConfigurationResponse*
                                     trt__AddMetadataConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__AddAudioOutputConfiguration(struct soap* soap,
                                        struct _trt__AddAudioOutputConfiguration* trt__AddAudioOutputConfiguration,
                                        struct _trt__AddAudioOutputConfigurationResponse*
                                        trt__AddAudioOutputConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__AddAudioDecoderConfiguration(struct soap* soap,
        struct _trt__AddAudioDecoderConfiguration* trt__AddAudioDecoderConfiguration,
        struct _trt__AddAudioDecoderConfigurationResponse*
        trt__AddAudioDecoderConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemoveVideoEncoderConfiguration(struct soap* soap,
        struct _trt__RemoveVideoEncoderConfiguration*
        trt__RemoveVideoEncoderConfiguration,
        struct _trt__RemoveVideoEncoderConfigurationResponse*
        trt__RemoveVideoEncoderConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemoveVideoSourceConfiguration(struct soap* soap,
        struct _trt__RemoveVideoSourceConfiguration*
        trt__RemoveVideoSourceConfiguration,
        struct _trt__RemoveVideoSourceConfigurationResponse*
        trt__RemoveVideoSourceConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemoveAudioEncoderConfiguration(struct soap* soap,
        struct _trt__RemoveAudioEncoderConfiguration*
        trt__RemoveAudioEncoderConfiguration,
        struct _trt__RemoveAudioEncoderConfigurationResponse*
        trt__RemoveAudioEncoderConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemoveAudioSourceConfiguration(struct soap* soap,
        struct _trt__RemoveAudioSourceConfiguration*
        trt__RemoveAudioSourceConfiguration,
        struct _trt__RemoveAudioSourceConfigurationResponse*
        trt__RemoveAudioSourceConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemovePTZConfiguration(struct soap* soap,
                                   struct _trt__RemovePTZConfiguration* trt__RemovePTZConfiguration,
                                   struct _trt__RemovePTZConfigurationResponse*
                                   trt__RemovePTZConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemoveVideoAnalyticsConfiguration(struct soap* soap,
        struct _trt__RemoveVideoAnalyticsConfiguration*
        trt__RemoveVideoAnalyticsConfiguration,
        struct _trt__RemoveVideoAnalyticsConfigurationResponse*
        trt__RemoveVideoAnalyticsConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemoveMetadataConfiguration(struct soap* soap,
                                        struct _trt__RemoveMetadataConfiguration* trt__RemoveMetadataConfiguration,
                                        struct _trt__RemoveMetadataConfigurationResponse*
                                        trt__RemoveMetadataConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemoveAudioOutputConfiguration(struct soap* soap,
        struct _trt__RemoveAudioOutputConfiguration*
        trt__RemoveAudioOutputConfiguration,
        struct _trt__RemoveAudioOutputConfigurationResponse*
        trt__RemoveAudioOutputConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__RemoveAudioDecoderConfiguration(struct soap* soap,
        struct _trt__RemoveAudioDecoderConfiguration*
        trt__RemoveAudioDecoderConfiguration,
        struct _trt__RemoveAudioDecoderConfigurationResponse*
        trt__RemoveAudioDecoderConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__DeleteProfile(struct soap* soap,
                          struct _trt__DeleteProfile* trt__DeleteProfile,
                          struct _trt__DeleteProfileResponse* trt__DeleteProfileResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetVideoSourceConfigurations(struct soap* soap,
        struct _trt__GetVideoSourceConfigurations* trt__GetVideoSourceConfigurations,
        struct _trt__GetVideoSourceConfigurationsResponse*
        trt__GetVideoSourceConfigurationsResponse)
{
	int idx;
	HI_ONVIF_VIDEO_SOURCE_CFG_S OnvifVideoSourceCfg;
	struct tt__VideoSourceConfiguration* VideoSourceConfiguration,
		       *VideoSourceConfigurationArray;
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT_NUM(VideoSourceConfigurationArray,
	                 struct tt__VideoSourceConfiguration, ONVIF_VIDEO_SOURCE_CFG_NUM);

	for(idx = 0; idx < ONVIF_VIDEO_SOURCE_CFG_NUM; idx++)
	{
		VideoSourceConfiguration = &VideoSourceConfigurationArray[idx];
		/* VideoSourceConfiguration */
		onvif_get_video_source_cfg(idx, &OnvifVideoSourceCfg);
		ALLOC_TOKEN(VideoSourceConfiguration->Name, OnvifVideoSourceCfg.Name);
		ALLOC_TOKEN(VideoSourceConfiguration->SourceToken,
		            OnvifVideoSourceCfg.SourceToken);
		VideoSourceConfiguration->UseCount = OnvifVideoSourceCfg.UseCount;
		ALLOC_STRUCT(VideoSourceConfiguration->Bounds, struct tt__IntRectangle);
		VideoSourceConfiguration->Bounds->y = OnvifVideoSourceCfg.Bounds.y;
		VideoSourceConfiguration->Bounds->x = OnvifVideoSourceCfg.Bounds.x;
		VideoSourceConfiguration->Bounds->width  = OnvifVideoSourceCfg.Bounds.width;
		VideoSourceConfiguration->Bounds->height = OnvifVideoSourceCfg.Bounds.height;
	}

	trt__GetVideoSourceConfigurationsResponse->__sizeConfigurations =
	    ONVIF_VIDEO_SOURCE_CFG_NUM;
	trt__GetVideoSourceConfigurationsResponse->Configurations =
	    VideoSourceConfigurationArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetVideoEncoderConfigurations(struct soap* soap,
        struct _trt__GetVideoEncoderConfigurations* trt__GetVideoEncoderConfigurations,
        struct _trt__GetVideoEncoderConfigurationsResponse*
        trt__GetVideoEncoderConfigurationsResponse)
{
	int idx;
	HI_ONVIF_VIDEO_ENCODER_CFG_S OnvifVideoEncoderCfg;;
	HI_DEV_VIDEO_CFG_S VideoCfg;
	struct tt__VideoEncoderConfiguration* VideoEncoderConfiguration,
		       *VideoEncoderConfigurationArray;
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT_NUM(VideoEncoderConfigurationArray,
	                 struct tt__VideoEncoderConfiguration, ONVIF_VIDEO_ENCODER_CFG_NUM);

	for(idx = 0; idx < ONVIF_VIDEO_ENCODER_CFG_NUM; idx++)
	{
		onvif_get_video_encoder_cfg(idx, &OnvifVideoEncoderCfg);
		/*VideoEncoderConfiguration */
		VideoEncoderConfiguration = &VideoEncoderConfigurationArray[idx];
		//ALLOC_STRUCT(VideoEncoderConfiguration, struct tt__VideoEncoderConfiguration);
		ALLOC_TOKEN(VideoEncoderConfiguration->Name, OnvifVideoEncoderCfg.Name);
		ALLOC_TOKEN(VideoEncoderConfiguration->token, OnvifVideoEncoderCfg.token);
		VideoEncoderConfiguration->UseCount = OnvifVideoEncoderCfg.UseCount;
		VideoEncoderConfiguration->Encoding = OnvifVideoEncoderCfg.Encoding;
		ALLOC_STRUCT(VideoEncoderConfiguration->Resolution, struct tt__VideoResolution);
		VideoEncoderConfiguration->Resolution->Width =
		    OnvifVideoEncoderCfg.Resolution.Width;
		VideoEncoderConfiguration->Resolution->Height =
		    OnvifVideoEncoderCfg.Resolution.Height;
		VideoEncoderConfiguration->Quality = OnvifVideoEncoderCfg.Quality;
		ALLOC_STRUCT(VideoEncoderConfiguration->RateControl,
		             struct tt__VideoRateControl);
		VideoEncoderConfiguration->RateControl->FrameRateLimit =
		    OnvifVideoEncoderCfg.FrameRate;
		VideoEncoderConfiguration->RateControl->EncodingInterval =
		    1;//OnvifVideoEncoderCfg.Gop;
		VideoEncoderConfiguration->RateControl->BitrateLimit =
		    OnvifVideoEncoderCfg.Bitrate;
		VideoEncoderConfiguration->MPEG4 = NULL;
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VIDEO_PARAM,  0,
		                     (char*)&VideoCfg, sizeof(HI_DEV_VIDEO_CFG_S));
		ALLOC_STRUCT(VideoEncoderConfiguration->H264, struct tt__H264Configuration);
#if 0

		if(idx == 0)
		{
			VideoEncoderConfiguration->H264->GovLength = VideoCfg.struMainVenc.u8Gop;
		}
		else
		{
			VideoEncoderConfiguration->H264->GovLength = VideoCfg.struSubVenc.u8Gop;
		}

#else
		VideoEncoderConfiguration->H264->GovLength = VideoCfg.struVenc[idx].u8Gop;
#endif

		switch(VideoCfg.struVenc[idx].u8Profile)
		{
			case HI_VENC_PROFILE_HIGH:
				VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__High;
				break;

			case HI_VENC_PROFILE_MAIN:
				VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Main;
				break;

			case HI_VENC_PROFILE_BASELINE:
				VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Baseline;
				break;
		}

		ALLOC_STRUCT(VideoEncoderConfiguration->Multicast,
		             struct tt__MulticastConfiguration);
		ALLOC_STRUCT(VideoEncoderConfiguration->Multicast->Address,
		             struct tt__IPAddress);
		VideoEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
		ALLOC_TOKEN(VideoEncoderConfiguration->Multicast->Address->IPv4Address,
		            ONVIF_WSD_MC_ADDR);
		VideoEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
		VideoEncoderConfiguration->Multicast->Port = 9090;
		VideoEncoderConfiguration->Multicast->TTL = 1500;
		VideoEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
		//yyyyyyyyyy
		VideoEncoderConfiguration->SessionTimeout = 5000;
		//ALLOC_TOKEN(VideoEncoderConfiguration->SessionTimeout, "PT5S");
	}

	trt__GetVideoEncoderConfigurationsResponse->__sizeConfigurations =
	    ONVIF_VIDEO_ENCODER_CFG_NUM;
	trt__GetVideoEncoderConfigurationsResponse->Configurations =
	    VideoEncoderConfigurationArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetAudioSourceConfigurations(struct soap* soap,
        struct _trt__GetAudioSourceConfigurations* trt__GetAudioSourceConfigurations,
        struct _trt__GetAudioSourceConfigurationsResponse*
        trt__GetAudioSourceConfigurationsResponse)
{
	int idx;
	HI_ONVIF_AUDIO_SOURCE_CFG_S OnvifAudioSourceCfg;
	struct tt__AudioSourceConfiguration* AudioSourceConfiguration,
		       *AudioSourceConfigurationArray;
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT_NUM(AudioSourceConfigurationArray,
	                 struct tt__AudioSourceConfiguration, ONVIF_AUDIO_SOURCE_CFG_NUM);

	for(idx = 0; idx < ONVIF_AUDIO_SOURCE_CFG_NUM; idx++)
	{
		/* AudioSourceConfiguration */
		onvif_get_audio_source_cfg(idx, &OnvifAudioSourceCfg);
		AudioSourceConfiguration = &AudioSourceConfigurationArray[idx];
		ALLOC_TOKEN(AudioSourceConfiguration->Name, OnvifAudioSourceCfg.Name);
		ALLOC_TOKEN(AudioSourceConfiguration->token, OnvifAudioSourceCfg.token);
		ALLOC_TOKEN(AudioSourceConfiguration->SourceToken,
		            OnvifAudioSourceCfg.SourceToken);
		AudioSourceConfiguration->UseCount = OnvifAudioSourceCfg.UseCount;
	}

	trt__GetAudioSourceConfigurationsResponse->__sizeConfigurations =
	    ONVIF_AUDIO_SOURCE_CFG_NUM;
	trt__GetAudioSourceConfigurationsResponse->Configurations =
	    AudioSourceConfigurationArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetAudioEncoderConfigurations(struct soap* soap,
        struct _trt__GetAudioEncoderConfigurations* trt__GetAudioEncoderConfigurations,
        struct _trt__GetAudioEncoderConfigurationsResponse*
        trt__GetAudioEncoderConfigurationsResponse)
{
	int idx;
	HI_ONVIF_AUDIO_ENCODER_CFG_S OnvifAudioEncoderCfg;;
	struct tt__AudioEncoderConfiguration* AudioEncoderConfiguration,
		       *AudioEncoderConfigurationArray;
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT_NUM(AudioEncoderConfigurationArray,
	                 struct tt__AudioEncoderConfiguration, ONVIF_AUDIO_SOURCE_CFG_NUM);

	for(idx = 0; idx < ONVIF_AUDIO_SOURCE_CFG_NUM; idx++)
	{
		onvif_get_audio_encoder_cfg(idx, &OnvifAudioEncoderCfg);
		AudioEncoderConfiguration = &AudioEncoderConfigurationArray[idx];
		ALLOC_TOKEN(AudioEncoderConfiguration->Name, OnvifAudioEncoderCfg.Name);
		AudioEncoderConfiguration->UseCount = OnvifAudioEncoderCfg.UseCount;
		ALLOC_TOKEN(AudioEncoderConfiguration->token, OnvifAudioEncoderCfg.token);
		AudioEncoderConfiguration->Encoding = OnvifAudioEncoderCfg.Encoding;
		AudioEncoderConfiguration->Bitrate = OnvifAudioEncoderCfg.Bitrate;
		AudioEncoderConfiguration->SampleRate = OnvifAudioEncoderCfg.SampleRate;
		ALLOC_STRUCT(AudioEncoderConfiguration->Multicast,
		             struct tt__MulticastConfiguration);
		ALLOC_STRUCT(AudioEncoderConfiguration->Multicast->Address,
		             struct tt__IPAddress);
		AudioEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
		ALLOC_TOKEN(AudioEncoderConfiguration->Multicast->Address->IPv4Address,
		            ONVIF_WSD_MC_ADDR);
		AudioEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
		AudioEncoderConfiguration->Multicast->Port = 9090;
		AudioEncoderConfiguration->Multicast->TTL = 1500;
		AudioEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
		//yyyyyyyyyy
		AudioEncoderConfiguration->SessionTimeout = 15000;
		//ALLOC_TOKEN(AudioEncoderConfiguration->SessionTimeout, "PT15S");
	}

	trt__GetAudioEncoderConfigurationsResponse->__sizeConfigurations =
	    ONVIF_AUDIO_ENCODER_CFG_NUM;
	trt__GetAudioEncoderConfigurationsResponse->Configurations =
	    AudioEncoderConfigurationArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetVideoAnalyticsConfigurations(struct soap* soap,
        struct _trt__GetVideoAnalyticsConfigurations*
        trt__GetVideoAnalyticsConfigurations,
        struct _trt__GetVideoAnalyticsConfigurationsResponse*
        trt__GetVideoAnalyticsConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	struct tt__VideoAnalyticsConfiguration* Configurations = NULL;
	struct tt__AnalyticsEngineConfiguration* AnalyticsEngineConfiguration = NULL;
	struct tt__RuleEngineConfiguration* RuleEngineConfiguration = NULL;
	trt__GetVideoAnalyticsConfigurationsResponse->__sizeConfigurations = 1;
	ALLOC_STRUCT(Configurations, struct tt__VideoAnalyticsConfiguration);
	trt__GetVideoAnalyticsConfigurationsResponse->Configurations = Configurations;
	Configurations->Name = soap_malloc(soap, 32);
	Configurations->UseCount = 2;
	Configurations->token = soap_malloc(soap, 32);
	strcpy(Configurations->Name, ONVIF_VIDEO_ANALY_CFG_NAME);
	strcpy(Configurations->token, ONVIF_VIDEO_ANALY_CFG_TOKEN);
	Configurations->__size = 0;
	Configurations->__any = NULL;
	Configurations->__anyAttribute = NULL;
	ALLOC_STRUCT(AnalyticsEngineConfiguration,
	             struct tt__AnalyticsEngineConfiguration);
	ALLOC_STRUCT(RuleEngineConfiguration, struct tt__RuleEngineConfiguration);
	Configurations->AnalyticsEngineConfiguration = AnalyticsEngineConfiguration;
	Configurations->RuleEngineConfiguration = RuleEngineConfiguration;
	struct tt__Config* AnalyticsModule = NULL;
	AnalyticsEngineConfiguration->__sizeAnalyticsModule = 2;
	AnalyticsModule =  soap_malloc(soap,
	                               AnalyticsEngineConfiguration->__sizeAnalyticsModule * sizeof(
	                                   struct tt__Config));
	AnalyticsEngineConfiguration->AnalyticsModule = AnalyticsModule;
	AnalyticsEngineConfiguration->Extension = NULL;
	AnalyticsEngineConfiguration->__anyAttribute = NULL;
	ALLOC_TOKEN(AnalyticsModule[0].Name, "MyCellMotionModule");
	ALLOC_TOKEN(AnalyticsModule[0].Type, "tt:CellMotionEngine");
	ALLOC_STRUCT(AnalyticsModule[0].Parameters, struct tt__ItemList);
	AnalyticsModule[0].Parameters->__sizeSimpleItem = 1;
	AnalyticsModule[0].Parameters->SimpleItem = soap_malloc(soap,
	        AnalyticsModule[0].Parameters->__sizeSimpleItem * sizeof(
	            struct _tt__ItemList_SimpleItem));
	ALLOC_TOKEN(AnalyticsModule[0].Parameters->SimpleItem->Name, "Sensitivity");
	ALLOC_TOKEN(AnalyticsModule[0].Parameters->SimpleItem->Value, "75");
	AnalyticsModule[0].Parameters->__sizeElementItem = 1;
	AnalyticsModule[0].Parameters->ElementItem = soap_malloc(soap,
	        AnalyticsModule[0].Parameters->__sizeElementItem * sizeof(
	            struct _tt__ItemList_ElementItem));
	ALLOC_TOKEN(AnalyticsModule[0].Parameters->ElementItem->Name,  "Layout");
	AnalyticsModule[0].Parameters->ElementItem->__any = soap_malloc(soap, 1024);
	sprintf(AnalyticsModule[0].Parameters->ElementItem->__any,
	        "<tt:CellLayout Columns=\"22\" Rows=\"18\">"
	        "<tt:Transformation>"
	        "<tt:Translate x=\"0.000000\" y=\"0.000000\"/>"
	        "<tt:Scale x=\"0.000000\" y=\"0.000000\"/>"
	        "</tt:Transformation>"
	        "</tt:CellLayout>");
	ALLOC_TOKEN(AnalyticsModule[1].Name, "MyTamperDetectModule");
	ALLOC_TOKEN(AnalyticsModule[1].Type,
	            "ns2:TamperEngine");//"hikxsd:TamperEngine");	//20140530zj
	ALLOC_STRUCT(AnalyticsModule[1].Parameters, struct tt__ItemList);
	AnalyticsModule[1].Parameters->__sizeSimpleItem = 1;
	AnalyticsModule[1].Parameters->SimpleItem = soap_malloc(soap,
	        AnalyticsModule[1].Parameters->__sizeSimpleItem * sizeof(
	            struct _tt__ItemList_SimpleItem));
	ALLOC_TOKEN(AnalyticsModule[1].Parameters->SimpleItem->Name, "Sensitivity");
	ALLOC_TOKEN(AnalyticsModule[1].Parameters->SimpleItem->Value, "100");
	AnalyticsModule[1].Parameters->__sizeElementItem = 2;
	AnalyticsModule[1].Parameters->ElementItem = soap_malloc(soap,
	        AnalyticsModule[1].Parameters->__sizeElementItem * sizeof(
	            struct _tt__ItemList_ElementItem));
	ALLOC_TOKEN(AnalyticsModule[1].Parameters->ElementItem[0].Name,
	            "Transformation");
	AnalyticsModule[1].Parameters->ElementItem[0].__any = soap_malloc(soap, 1024);
	sprintf(AnalyticsModule[1].Parameters->ElementItem[0].__any,
	        "<tt:Transformation>"
	        "<tt:Translate x=\"-1.000000\" y=\"-1.000000\"/>"
	        "<tt:Scale x=\"0.002841\" y=\"0.003472\"/>"
	        "</tt:Transformation>");
	ALLOC_TOKEN(AnalyticsModule[1].Parameters->ElementItem[1].Name,  "Field");
	AnalyticsModule[1].Parameters->ElementItem[1].__any = soap_malloc(soap, 1024);
	sprintf(AnalyticsModule[1].Parameters->ElementItem[1].__any,
	        "<tt:PolygonConfiguration>"
	        "<tt:Polygon>"
	        "<tt:Point x=\"0\" y=\"0\"/>"
	        "<tt:Point x=\"0\" y=\"576\"/>"
	        "<tt:Point x=\"704\" y=\"576\"/>"
	        "<tt:Point x=\"704\" y=\"0\"/>"
	        "</tt:Polygon>"
	        "</tt:PolygonConfiguration>");
	RuleEngineConfiguration->__sizeRule = 2;
	RuleEngineConfiguration->Extension = NULL;
	RuleEngineConfiguration->__anyAttribute = NULL;
	RuleEngineConfiguration->Rule = soap_malloc(soap,
	                                RuleEngineConfiguration->__sizeRule * sizeof(struct tt__Config));
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Name, "MyMotionDetectorRule");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Type, "tt:CellMotionDetector");
	ALLOC_STRUCT(RuleEngineConfiguration->Rule[0].Parameters, struct tt__ItemList);
	RuleEngineConfiguration->Rule[0].Parameters->__sizeSimpleItem = 4;
	RuleEngineConfiguration->Rule[0].Parameters->SimpleItem = soap_malloc(soap,
	        RuleEngineConfiguration->Rule[0].Parameters->__sizeSimpleItem * sizeof(
	            struct _tt__ItemList_SimpleItem));
	RuleEngineConfiguration->Rule[0].Parameters->Extension = NULL;
	RuleEngineConfiguration->Rule[0].Parameters->__anyAttribute = NULL;
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[0].Name,
	            "MinCount");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[0].Value,
	            "1");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[1].Name,
	            "AlarmOnDelay");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[1].Value,
	            "100");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[2].Name,
	            "AlarmOffDelay");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[2].Value,
	            "1000");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[3].Name,
	            "ActiveCells");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[0].Parameters->SimpleItem[3].Value,
	            "0P8A8A==");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[1].Name, "MyTamperDetectorRule");
	ALLOC_TOKEN(RuleEngineConfiguration->Rule[1].Type,
	            "ns2:TamperDetector");//"hikxsd:TamperDetector");//20140530zj
	ALLOC_STRUCT(RuleEngineConfiguration->Rule[1].Parameters, struct tt__ItemList);
	RuleEngineConfiguration->Rule[1].Parameters->__sizeElementItem = 0;
	RuleEngineConfiguration->Rule[1].Parameters->ElementItem = NULL;
	RuleEngineConfiguration->Rule[1].Parameters->__sizeSimpleItem = 0;
	RuleEngineConfiguration->Rule[1].Parameters->SimpleItem = NULL;
	RuleEngineConfiguration->Rule[1].Parameters->Extension = NULL;
	RuleEngineConfiguration->Rule[1].Parameters->__anyAttribute = NULL;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetMetadataConfigurations(struct soap* soap,
                                      struct _trt__GetMetadataConfigurations* trt__GetMetadataConfigurations,
                                      struct _trt__GetMetadataConfigurationsResponse*
                                      trt__GetMetadataConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetAudioOutputConfigurations(struct soap* soap,
        struct _trt__GetAudioOutputConfigurations* trt__GetAudioOutputConfigurations,
        struct _trt__GetAudioOutputConfigurationsResponse*
        trt__GetAudioOutputConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetAudioDecoderConfigurations(struct soap* soap,
        struct _trt__GetAudioDecoderConfigurations* trt__GetAudioDecoderConfigurations,
        struct _trt__GetAudioDecoderConfigurationsResponse*
        trt__GetAudioDecoderConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetVideoSourceConfiguration(struct soap* soap,
                                        struct _trt__GetVideoSourceConfiguration* trt__GetVideoSourceConfiguration,
                                        struct _trt__GetVideoSourceConfigurationResponse*
                                        trt__GetVideoSourceConfigurationResponse)
{
	HI_ONVIF_VIDEO_SOURCE_CFG_S OnvifVideoSourceCfg;
	struct tt__VideoSourceConfiguration* VideoSourceConfiguration;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(onvif_get_video_source_cfg_from_token(
	            trt__GetVideoSourceConfiguration->ConfigurationToken,
	            &OnvifVideoSourceCfg)  < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(VideoSourceConfiguration,  struct tt__VideoSourceConfiguration);
	ALLOC_TOKEN(VideoSourceConfiguration->Name, OnvifVideoSourceCfg.Name);
	ALLOC_TOKEN(VideoSourceConfiguration->SourceToken,
	            OnvifVideoSourceCfg.SourceToken);
	VideoSourceConfiguration->UseCount = OnvifVideoSourceCfg.UseCount;
	ALLOC_STRUCT(VideoSourceConfiguration->Bounds, struct tt__IntRectangle);
	VideoSourceConfiguration->Bounds->y = OnvifVideoSourceCfg.Bounds.y;
	VideoSourceConfiguration->Bounds->x = OnvifVideoSourceCfg.Bounds.x;
	VideoSourceConfiguration->Bounds->width  = OnvifVideoSourceCfg.Bounds.width;
	VideoSourceConfiguration->Bounds->height = OnvifVideoSourceCfg.Bounds.height;
	trt__GetVideoSourceConfigurationResponse->Configuration =
	    VideoSourceConfiguration;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetVideoEncoderConfiguration(struct soap* soap,
        struct _trt__GetVideoEncoderConfiguration* trt__GetVideoEncoderConfiguration,
        struct _trt__GetVideoEncoderConfigurationResponse*
        trt__GetVideoEncoderConfigurationResponse)
{
	HI_ONVIF_VIDEO_ENCODER_CFG_S OnvifVideoEncoderCfg;
	HI_DEV_VIDEO_CFG_S VideoCfg;
	struct tt__VideoEncoderConfiguration* VideoEncoderConfiguration;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(onvif_get_video_encoder_cfg_from_token(
	            trt__GetVideoEncoderConfiguration->ConfigurationToken,
	            &OnvifVideoEncoderCfg) < 0)
	{
		__E("Failed to get video encoder cfg.\n");
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	/*VideoEncoderConfiguration */
	ALLOC_STRUCT(VideoEncoderConfiguration, struct tt__VideoEncoderConfiguration);
	ALLOC_TOKEN(VideoEncoderConfiguration->Name, OnvifVideoEncoderCfg.Name);
	ALLOC_TOKEN(VideoEncoderConfiguration->token, OnvifVideoEncoderCfg.token);
	VideoEncoderConfiguration->UseCount = OnvifVideoEncoderCfg.UseCount;
	VideoEncoderConfiguration->Encoding = OnvifVideoEncoderCfg.Encoding;
	ALLOC_STRUCT(VideoEncoderConfiguration->Resolution, struct tt__VideoResolution);
	VideoEncoderConfiguration->Resolution->Width =
	    OnvifVideoEncoderCfg.Resolution.Width;
	VideoEncoderConfiguration->Resolution->Height =
	    OnvifVideoEncoderCfg.Resolution.Height;
	VideoEncoderConfiguration->Quality = OnvifVideoEncoderCfg.Quality;
	ALLOC_STRUCT(VideoEncoderConfiguration->RateControl,
	             struct tt__VideoRateControl);
	VideoEncoderConfiguration->RateControl->FrameRateLimit =
	    OnvifVideoEncoderCfg.FrameRate;
	VideoEncoderConfiguration->RateControl->EncodingInterval =
	    1;//OnvifVideoEncoderCfg.Gop;
	VideoEncoderConfiguration->RateControl->BitrateLimit =
	    OnvifVideoEncoderCfg.Bitrate;
	VideoEncoderConfiguration->MPEG4 = NULL;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VIDEO_PARAM,  0,
	                     (char*)&VideoCfg, sizeof(HI_DEV_VIDEO_CFG_S));
	ALLOC_STRUCT(VideoEncoderConfiguration->H264, struct tt__H264Configuration);
	VideoEncoderConfiguration->H264->GovLength = VideoCfg.struVenc[0].u8Gop;

	switch(VideoCfg.struVenc[0].u8Profile)
	{
		case HI_VENC_PROFILE_HIGH:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__High;
			break;

		case HI_VENC_PROFILE_MAIN:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Main;
			break;

		case HI_VENC_PROFILE_BASELINE:
			VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Baseline;
			break;
	}

	ALLOC_STRUCT(VideoEncoderConfiguration->Multicast,
	             struct tt__MulticastConfiguration);
	ALLOC_STRUCT(VideoEncoderConfiguration->Multicast->Address,
	             struct tt__IPAddress);
	VideoEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
	ALLOC_TOKEN(VideoEncoderConfiguration->Multicast->Address->IPv4Address,
	            ONVIF_WSD_MC_ADDR);
	VideoEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
	VideoEncoderConfiguration->Multicast->Port = 9090;
	VideoEncoderConfiguration->Multicast->TTL = 1500;
	VideoEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
	//yyyyyyyyyyyyyyy
	VideoEncoderConfiguration->SessionTimeout = 5000;
	//ALLOC_TOKEN(VideoEncoderConfiguration->SessionTimeout, "PT5S");
	trt__GetVideoEncoderConfigurationResponse->Configuration =
	    VideoEncoderConfiguration;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetAudioSourceConfiguration(struct soap* soap,
                                        struct _trt__GetAudioSourceConfiguration* trt__GetAudioSourceConfiguration,
                                        struct _trt__GetAudioSourceConfigurationResponse*
                                        trt__GetAudioSourceConfigurationResponse)
{
	//	int idx;
	HI_ONVIF_AUDIO_SOURCE_CFG_S OnvifAudioSourceCfg;
	struct tt__AudioSourceConfiguration* AudioSourceConfiguration;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(onvif_get_audio_source_cfg_from_token(
	            trt__GetAudioSourceConfiguration->ConfigurationToken,
	            &OnvifAudioSourceCfg)  < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(AudioSourceConfiguration, struct tt__AudioSourceConfiguration);
	ALLOC_TOKEN(AudioSourceConfiguration->Name, OnvifAudioSourceCfg.Name);
	ALLOC_TOKEN(AudioSourceConfiguration->token, OnvifAudioSourceCfg.token);
	ALLOC_TOKEN(AudioSourceConfiguration->SourceToken,
	            OnvifAudioSourceCfg.SourceToken);
	AudioSourceConfiguration->UseCount = OnvifAudioSourceCfg.UseCount;
	AudioSourceConfiguration->__size = 0;
	AudioSourceConfiguration->__any = NULL;
	AudioSourceConfiguration->__anyAttribute = NULL;
	trt__GetAudioSourceConfigurationResponse->Configuration =
	    AudioSourceConfiguration;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetAudioEncoderConfiguration(struct soap* soap,
        struct _trt__GetAudioEncoderConfiguration* trt__GetAudioEncoderConfiguration,
        struct _trt__GetAudioEncoderConfigurationResponse*
        trt__GetAudioEncoderConfigurationResponse)
{
	HI_ONVIF_AUDIO_ENCODER_CFG_S OnvifAudioEncoderCfg;;
	HI_DEV_AUDIO_CFG_S stAutoCfg;
	struct tt__AudioEncoderConfiguration* AudioEncoderConfiguration;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(onvif_get_audio_encoder_cfg_from_token(
	            trt__GetAudioEncoderConfiguration->ConfigurationToken,
	            &OnvifAudioEncoderCfg) < 0)
	{
		__E("Failed to get Audio encoder cfg.\n");
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	memset(&stAutoCfg, 0, sizeof(HI_DEV_AUDIO_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_AUDIO_PARAM, 0,
	                     (char*)&stAutoCfg, sizeof(HI_DEV_AUDIO_CFG_S));
	stAutoCfg.u8Open = 1;
	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_AUDIO_PARAM, 0,
	                     (char*)&stAutoCfg, sizeof(HI_DEV_AUDIO_CFG_S));
	ALLOC_STRUCT(AudioEncoderConfiguration, struct tt__AudioEncoderConfiguration)
	ALLOC_TOKEN(AudioEncoderConfiguration->Name, OnvifAudioEncoderCfg.Name);
	AudioEncoderConfiguration->UseCount = OnvifAudioEncoderCfg.UseCount;
	ALLOC_TOKEN(AudioEncoderConfiguration->token, OnvifAudioEncoderCfg.token);
	AudioEncoderConfiguration->Encoding = OnvifAudioEncoderCfg.Encoding;
	AudioEncoderConfiguration->Bitrate = OnvifAudioEncoderCfg.Bitrate;
	AudioEncoderConfiguration->SampleRate = OnvifAudioEncoderCfg.SampleRate;
	ALLOC_STRUCT(AudioEncoderConfiguration->Multicast,
	             struct tt__MulticastConfiguration);
	ALLOC_STRUCT(AudioEncoderConfiguration->Multicast->Address,
	             struct tt__IPAddress);
	AudioEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
	ALLOC_TOKEN(AudioEncoderConfiguration->Multicast->Address->IPv4Address,
	            ONVIF_WSD_MC_ADDR);
	AudioEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
	AudioEncoderConfiguration->Multicast->Port = 9090;
	AudioEncoderConfiguration->Multicast->TTL = 1500;
	AudioEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
	//yyyyyyyyyyyyyyy
	AudioEncoderConfiguration->SessionTimeout = 5000;
	//ALLOC_TOKEN(AudioEncoderConfiguration->SessionTimeout, OnvifAudioEncoderCfg.SessionTimeout);
	trt__GetAudioEncoderConfigurationResponse->Configuration =
	    AudioEncoderConfiguration;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetVideoAnalyticsConfiguration(struct soap* soap,
        struct _trt__GetVideoAnalyticsConfiguration*
        trt__GetVideoAnalyticsConfiguration,
        struct _trt__GetVideoAnalyticsConfigurationResponse*
        trt__GetVideoAnalyticsConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetMetadataConfiguration(struct soap* soap,
                                     struct _trt__GetMetadataConfiguration* trt__GetMetadataConfiguration,
                                     struct _trt__GetMetadataConfigurationResponse*
                                     trt__GetMetadataConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetAudioOutputConfiguration(struct soap* soap,
                                        struct _trt__GetAudioOutputConfiguration* trt__GetAudioOutputConfiguration,
                                        struct _trt__GetAudioOutputConfigurationResponse*
                                        trt__GetAudioOutputConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetAudioDecoderConfiguration(struct soap* soap,
        struct _trt__GetAudioDecoderConfiguration* trt__GetAudioDecoderConfiguration,
        struct _trt__GetAudioDecoderConfigurationResponse*
        trt__GetAudioDecoderConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetCompatibleVideoEncoderConfigurations(struct soap* soap,
        struct _trt__GetCompatibleVideoEncoderConfigurations*
        trt__GetCompatibleVideoEncoderConfigurations,
        struct _trt__GetCompatibleVideoEncoderConfigurationsResponse*
        trt__GetCompatibleVideoEncoderConfigurationsResponse)
{
	int idx;
	HI_DEV_VIDEO_CFG_S VideoCfg;
	HI_ONVIF_VIDEO_ENCODER_CFG_S OnvifVideoEncoderCfg;
	struct tt__VideoEncoderConfiguration* VideoEncoderConfiguration,
		       *VideoEncoderConfigurationArray;
	__FUN_BEGIN("\n");
	CHECK_USER;
	/////////////////////////20140910 ZJ//////////////////////
	VideoEncoderConfigurationArray = soap_malloc(soap,
	                                 sizeof(struct tt__VideoEncoderConfiguration) * ONVIF_VIDEO_ENCODER_CFG_NUM);

	if(VideoEncoderConfigurationArray == NULL)
	{
		return SOAP_FAULT;
	}

	memset(VideoEncoderConfigurationArray, 0,
	       sizeof(struct tt__VideoEncoderConfiguration)*ONVIF_VIDEO_ENCODER_CFG_NUM);
	/////////////////////////////end//////////////////////////

	for(idx = 0; idx < ONVIF_VIDEO_ENCODER_CFG_NUM; idx++)
	{
		onvif_get_video_encoder_cfg(idx, &OnvifVideoEncoderCfg);
		VideoEncoderConfiguration = &VideoEncoderConfigurationArray[idx];

		if(OnvifVideoEncoderCfg.Name != NULL)   //20140910 ZJ
		{
			ALLOC_TOKEN(VideoEncoderConfiguration->Name, OnvifVideoEncoderCfg.Name);
		}

		if(OnvifVideoEncoderCfg.token != NULL)   //20140910 ZJ
		{
			ALLOC_TOKEN(VideoEncoderConfiguration->token, OnvifVideoEncoderCfg.token);
		}

		VideoEncoderConfiguration->UseCount = OnvifVideoEncoderCfg.UseCount;
		VideoEncoderConfiguration->Encoding = OnvifVideoEncoderCfg.Encoding;
		ALLOC_STRUCT(VideoEncoderConfiguration->Resolution, struct tt__VideoResolution);
		VideoEncoderConfiguration->Resolution->Width =
		    OnvifVideoEncoderCfg.Resolution.Width;
		VideoEncoderConfiguration->Resolution->Height =
		    OnvifVideoEncoderCfg.Resolution.Height;
		VideoEncoderConfiguration->Quality = OnvifVideoEncoderCfg.Quality;
		ALLOC_STRUCT(VideoEncoderConfiguration->RateControl,
		             struct tt__VideoRateControl);
		VideoEncoderConfiguration->RateControl->FrameRateLimit =
		    OnvifVideoEncoderCfg.FrameRate;
		VideoEncoderConfiguration->RateControl->EncodingInterval =
		    OnvifVideoEncoderCfg.Gop;
		VideoEncoderConfiguration->RateControl->BitrateLimit =
		    OnvifVideoEncoderCfg.Bitrate;
		VideoEncoderConfiguration->MPEG4 = NULL;
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VIDEO_PARAM,  0,
		                     (char*)&VideoCfg, sizeof(HI_DEV_VIDEO_CFG_S));
		ALLOC_STRUCT(VideoEncoderConfiguration->H264, struct tt__H264Configuration);
#if 0

		if(idx == 0)
		{
			VideoEncoderConfiguration->H264->GovLength = VideoCfg.struMainVenc.u8Gop;
		}
		else
		{
			VideoEncoderConfiguration->H264->GovLength = VideoCfg.struSubVenc.u8Gop;
		}

#else
		VideoEncoderConfiguration->H264->GovLength = VideoCfg.struVenc[idx].u8Gop;
#endif

		switch(VideoCfg.struVenc[idx].u8Profile)
		{
			case HI_VENC_PROFILE_HIGH:
				VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__High;
				break;

			case HI_VENC_PROFILE_MAIN:
				VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Main;
				break;

			case HI_VENC_PROFILE_BASELINE:
				VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__Baseline;
				break;
		}

		ALLOC_STRUCT(VideoEncoderConfiguration->Multicast,
		             struct tt__MulticastConfiguration);
		ALLOC_STRUCT(VideoEncoderConfiguration->Multicast->Address,
		             struct tt__IPAddress);
		VideoEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
		ALLOC_TOKEN(VideoEncoderConfiguration->Multicast->Address->IPv4Address,
		            ONVIF_WSD_MC_ADDR);
		VideoEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
		VideoEncoderConfiguration->Multicast->Port = 9090;
		VideoEncoderConfiguration->Multicast->TTL = 1500;
		VideoEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
		//yyyyyyyyyyyyyyy
		VideoEncoderConfiguration->SessionTimeout = 5000;
		//ALLOC_TOKEN(VideoEncoderConfiguration->SessionTimeout, "PT5S");
	}

	trt__GetCompatibleVideoEncoderConfigurationsResponse->__sizeConfigurations =
	    ONVIF_VIDEO_ENCODER_CFG_NUM;
	trt__GetCompatibleVideoEncoderConfigurationsResponse->Configurations =
	    VideoEncoderConfigurationArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetCompatibleVideoSourceConfigurations(struct soap* soap,
        struct _trt__GetCompatibleVideoSourceConfigurations*
        trt__GetCompatibleVideoSourceConfigurations,
        struct _trt__GetCompatibleVideoSourceConfigurationsResponse*
        trt__GetCompatibleVideoSourceConfigurationsResponse)
{
	int idx;
	HI_ONVIF_VIDEO_SOURCE_CFG_S OnvifVideoSourceCfg;
	struct tt__VideoSourceConfiguration* VideoSourceConfiguration,
		       *VideoSourceConfigurationArray;
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT_NUM(VideoSourceConfigurationArray,
	                 struct tt__VideoSourceConfiguration, ONVIF_VIDEO_SOURCE_CFG_NUM);

	for(idx = 0; idx < ONVIF_VIDEO_SOURCE_CFG_NUM; idx++)
	{
		VideoSourceConfiguration = &VideoSourceConfigurationArray[idx];
		/* VideoSourceConfiguration */
		onvif_get_video_source_cfg(idx, &OnvifVideoSourceCfg);
		ALLOC_TOKEN(VideoSourceConfiguration->Name, OnvifVideoSourceCfg.Name);
		ALLOC_TOKEN(VideoSourceConfiguration->SourceToken,
		            OnvifVideoSourceCfg.SourceToken);
		VideoSourceConfiguration->UseCount = OnvifVideoSourceCfg.UseCount;
		ALLOC_STRUCT(VideoSourceConfiguration->Bounds, struct tt__IntRectangle);
		VideoSourceConfiguration->Bounds->y = OnvifVideoSourceCfg.Bounds.y;
		VideoSourceConfiguration->Bounds->x = OnvifVideoSourceCfg.Bounds.x;
		VideoSourceConfiguration->Bounds->width  = OnvifVideoSourceCfg.Bounds.width;
		VideoSourceConfiguration->Bounds->height = OnvifVideoSourceCfg.Bounds.height;
	}

	trt__GetCompatibleVideoSourceConfigurationsResponse->__sizeConfigurations =
	    ONVIF_VIDEO_SOURCE_CFG_NUM;
	trt__GetCompatibleVideoSourceConfigurationsResponse->Configurations =
	    VideoSourceConfigurationArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetCompatibleAudioEncoderConfigurations(struct soap* soap,
        struct _trt__GetCompatibleAudioEncoderConfigurations*
        trt__GetCompatibleAudioEncoderConfigurations,
        struct _trt__GetCompatibleAudioEncoderConfigurationsResponse*
        trt__GetCompatibleAudioEncoderConfigurationsResponse)
{
	int idx;
	HI_ONVIF_AUDIO_ENCODER_CFG_S OnvifAudioEncoderCfg;;
	struct tt__AudioEncoderConfiguration* AudioEncoderConfiguration,
		       *AudioEncoderConfigurationArray;
	__FUN_BEGIN("\n");
	CHECK_USER;
	/////////////////////////20140910 ZJ//////////////////////
	AudioEncoderConfigurationArray = soap_malloc(soap,
	                                 sizeof(struct tt__AudioEncoderConfiguration) * ONVIF_AUDIO_ENCODER_CFG_NUM);

	if(AudioEncoderConfigurationArray == NULL)
	{
		return SOAP_FAULT;
	}

	memset(AudioEncoderConfigurationArray, 0,
	       sizeof(struct tt__AudioEncoderConfiguration)*ONVIF_AUDIO_ENCODER_CFG_NUM);
	/////////////////////////////END//////////////////////////

	for(idx = 0; idx < ONVIF_AUDIO_ENCODER_CFG_NUM; idx++)
	{
		onvif_get_audio_encoder_cfg(idx, &OnvifAudioEncoderCfg);
		AudioEncoderConfiguration = &AudioEncoderConfigurationArray[idx];

		if(OnvifAudioEncoderCfg.Name != NULL)   //20140910 ZJ
		{
			ALLOC_TOKEN(AudioEncoderConfiguration->Name, OnvifAudioEncoderCfg.Name);
		}

		AudioEncoderConfiguration->UseCount = OnvifAudioEncoderCfg.UseCount;

		if(OnvifAudioEncoderCfg.token != NULL)   //20140910 ZJ
		{
			ALLOC_TOKEN(AudioEncoderConfiguration->token, OnvifAudioEncoderCfg.token);
		}

		AudioEncoderConfiguration->Encoding = OnvifAudioEncoderCfg.Encoding;
		AudioEncoderConfiguration->Bitrate = OnvifAudioEncoderCfg.Bitrate;
		AudioEncoderConfiguration->SampleRate = OnvifAudioEncoderCfg.SampleRate;
		ALLOC_STRUCT(AudioEncoderConfiguration->Multicast,
		             struct tt__MulticastConfiguration);
		ALLOC_STRUCT(AudioEncoderConfiguration->Multicast->Address,
		             struct tt__IPAddress);
		AudioEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
		ALLOC_TOKEN(AudioEncoderConfiguration->Multicast->Address->IPv4Address,
		            ONVIF_WSD_MC_ADDR);
		AudioEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
		AudioEncoderConfiguration->Multicast->Port = 9090;
		AudioEncoderConfiguration->Multicast->TTL = 1500;
		AudioEncoderConfiguration->Multicast->AutoStart = xsd__boolean__false_;
		//ALLOC_TOKEN(AudioEncoderConfiguration->SessionTimeout, "PT15S");
		//yyyyyyyyyyyyyyy
		AudioEncoderConfiguration->SessionTimeout = 15000;
	}

	trt__GetCompatibleAudioEncoderConfigurationsResponse->__sizeConfigurations =
	    ONVIF_AUDIO_ENCODER_CFG_NUM;
	trt__GetCompatibleAudioEncoderConfigurationsResponse->Configurations =
	    AudioEncoderConfigurationArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetCompatibleAudioSourceConfigurations(struct soap* soap,
        struct _trt__GetCompatibleAudioSourceConfigurations*
        trt__GetCompatibleAudioSourceConfigurations,
        struct _trt__GetCompatibleAudioSourceConfigurationsResponse*
        trt__GetCompatibleAudioSourceConfigurationsResponse)
{
	int idx;
	HI_ONVIF_AUDIO_SOURCE_CFG_S OnvifAudioSourceCfg;
	struct tt__AudioSourceConfiguration* AudioSourceConfiguration,
		       *AudioSourceConfigurationArray;
	__FUN_BEGIN("\n");
	CHECK_USER;
	ALLOC_STRUCT_NUM(AudioSourceConfigurationArray,
	                 struct tt__AudioSourceConfiguration, ONVIF_AUDIO_SOURCE_CFG_NUM);

	for(idx = 0; idx < ONVIF_AUDIO_SOURCE_CFG_NUM; idx++)
	{
		/* AudioSourceConfiguration */
		onvif_get_audio_source_cfg(idx, &OnvifAudioSourceCfg);
		AudioSourceConfiguration = &AudioSourceConfigurationArray[idx];
		ALLOC_TOKEN(AudioSourceConfiguration->Name, OnvifAudioSourceCfg.Name);
		ALLOC_TOKEN(AudioSourceConfiguration->token, OnvifAudioSourceCfg.token);
		ALLOC_TOKEN(AudioSourceConfiguration->SourceToken,
		            OnvifAudioSourceCfg.SourceToken);
		AudioSourceConfiguration->UseCount = OnvifAudioSourceCfg.UseCount;
	}

	trt__GetCompatibleAudioSourceConfigurationsResponse->__sizeConfigurations =
	    ONVIF_AUDIO_SOURCE_CFG_NUM;
	trt__GetCompatibleAudioSourceConfigurationsResponse->Configurations =
	    AudioSourceConfigurationArray;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetCompatibleVideoAnalyticsConfigurations(struct soap* soap,
        struct _trt__GetCompatibleVideoAnalyticsConfigurations*
        trt__GetCompatibleVideoAnalyticsConfigurations,
        struct _trt__GetCompatibleVideoAnalyticsConfigurationsResponse*
        trt__GetCompatibleVideoAnalyticsConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetCompatibleMetadataConfigurations(struct soap* soap,
        struct _trt__GetCompatibleMetadataConfigurations*
        trt__GetCompatibleMetadataConfigurations,
        struct _trt__GetCompatibleMetadataConfigurationsResponse*
        trt__GetCompatibleMetadataConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetCompatibleAudioOutputConfigurations(struct soap* soap,
        struct _trt__GetCompatibleAudioOutputConfigurations*
        trt__GetCompatibleAudioOutputConfigurations,
        struct _trt__GetCompatibleAudioOutputConfigurationsResponse*
        trt__GetCompatibleAudioOutputConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetCompatibleAudioDecoderConfigurations(struct soap* soap,
        struct _trt__GetCompatibleAudioDecoderConfigurations*
        trt__GetCompatibleAudioDecoderConfigurations,
        struct _trt__GetCompatibleAudioDecoderConfigurationsResponse*
        trt__GetCompatibleAudioDecoderConfigurationsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__SetVideoSourceConfiguration(struct soap* soap,
                                        struct _trt__SetVideoSourceConfiguration* trt__SetVideoSourceConfiguration,
                                        struct _trt__SetVideoSourceConfigurationResponse*
                                        trt__SetVideoSourceConfigurationResponse)
{
	HI_ONVIF_VIDEO_SOURCE_CFG_S OnvifVideoSourceCfg;
	HI_ONVIF_VIDEO_SOURCE_S OnvifVideoSource;

	if(trt__SetVideoSourceConfiguration->Configuration != NULL)
	{
		return SOAP_FAULT;
	}

	if(onvif_get_video_source_cfg_from_token(
	            trt__SetVideoSourceConfiguration->Configuration->token,
	            &OnvifVideoSourceCfg) < 0
	        || onvif_get_video_source_from_token(
	            trt__SetVideoSourceConfiguration->Configuration->SourceToken,
	            &OnvifVideoSource) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	onvif_fault(soap, "ter:InvalidArgVal", "ter:ConfigModify");
	return SOAP_FAULT;
}

int  __trt__SetVideoEncoderConfiguration(struct soap* soap,
        struct _trt__SetVideoEncoderConfiguration* trt__SetVideoEncoderConfiguration,
        struct _trt__SetVideoEncoderConfigurationResponse*
        trt__SetVideoEncoderConfigurationResponse)
{
	struct tt__VideoEncoderConfiguration* Configuration;
	HI_DEV_VENC_CFG_S stVencCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;
	Configuration = trt__SetVideoEncoderConfiguration->Configuration;

	if(strcmp(Configuration->token, ONVIF_MEDIA_VIDEO_MAIN_ENCODER_TOKEN) == 0)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VMAIN_PARAM, 0,
		                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	}
	else if(strcmp(Configuration->token,
	               ONVIF_MEDIA_VIDEO_SUB_ENCODER_TOKEN) == 0)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_VSUB_PARAM, 0,
		                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	}
	else
	{
		return SOAP_FAULT;
	}

	if(Configuration->Encoding != tt__VideoEncoding__H264)
	{
		__E("The Encoding is not h264.\n");
		goto ConfigModifyError;
	}

	if(Configuration->Resolution != NULL)
	{
		if((Configuration->Resolution->Width !=
		        gOnvifInfo.MediaConfig.h264Options[0].Width
		        && Configuration->Resolution->Width !=
		        gOnvifInfo.MediaConfig.h264Options[1].Width)
		        || (Configuration->Resolution->Height !=
		            gOnvifInfo.MediaConfig.h264Options[0].Height
		            && Configuration->Resolution->Height !=
		            gOnvifInfo.MediaConfig.h264Options[1].Height))
		{
			return SOAP_FAULT;
		}

		gOnvifInfo.MediaConfig.VideoResolutionconfig.Width =
		    Configuration->Resolution->Width;
		gOnvifInfo.MediaConfig.VideoResolutionconfig.Height =
		    Configuration->Resolution->Height;
		__D("Width = %d\n", Configuration->Resolution->Width);
		__D("Height = %d\n", Configuration->Resolution->Height);
	}

	stVencCfg.u8PicQuilty = Configuration->Quality;

	if(Configuration->RateControl != NULL)
	{
		if(Configuration->RateControl->FrameRateLimit > 0)
		{
			stVencCfg.u8FrameRate = Configuration->RateControl->FrameRateLimit;
		}

		if(Configuration->RateControl->BitrateLimit > 0)
		{
			stVencCfg.u32BitrateRate = Configuration->RateControl->BitrateLimit;
		}

		//stVencCfg.u8Gop = Configuration->RateControl->EncodingInterval;
		gOnvifInfo.MediaConfig.VideoEncoderRateControl.FrameRateLimit =
		    Configuration->RateControl->FrameRateLimit;
		gOnvifInfo.MediaConfig.VideoEncoderRateControl.BitrateLimit =
		    Configuration->RateControl->BitrateLimit;
		gOnvifInfo.MediaConfig.VideoEncoderRateControl.EncodingInterval =
		    Configuration->RateControl->EncodingInterval;
		__D("FrameRate = %d\n", Configuration->RateControl->FrameRateLimit);
		__D("BitrateLimit = %d\n", Configuration->RateControl->BitrateLimit);
		__D("EncodingInterval = %d\n", Configuration->RateControl->EncodingInterval);
	}

	if(Configuration->H264 != NULL)
	{
		if(Configuration->H264->GovLength < 1 || Configuration->H264->GovLength > 200)
		{
			goto ConfigModifyError;
		}

		stVencCfg.u8Gop = Configuration->H264->GovLength;
	}

	if(strcmp(Configuration->token, ONVIF_MEDIA_VIDEO_MAIN_ENCODER_TOKEN) == 0)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_VMAIN_PARAM, 0,
		                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	}
	else if(strcmp(Configuration->token,
	               ONVIF_MEDIA_VIDEO_SUB_ENCODER_TOKEN) == 0)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_VSUB_PARAM, 0,
		                     (char*)&stVencCfg, sizeof(HI_DEV_VENC_CFG_S));
	}

	__FUN_END("\n");
	return SOAP_OK;
ConfigModifyError:
	onvif_fault(soap, "ter:InvalidArgVal", "ter:ConfigModify");
	return SOAP_FAULT;
}

int  __trt__SetAudioSourceConfiguration(struct soap* soap,
                                        struct _trt__SetAudioSourceConfiguration* trt__SetAudioSourceConfiguration,
                                        struct _trt__SetAudioSourceConfigurationResponse*
                                        trt__SetAudioSourceConfigurationResponse)
{
	HI_ONVIF_AUDIO_SOURCE_CFG_S OnvifAudioSourceCfg;
	HI_ONVIF_AUDIO_SOURCE_S OnvifAudioSource;
	int i;
	__FUN_BEGIN("\n");
	CHECK_USER;

	for(i = 0; i < ONVIF_AUDIO_SOURCE_CFG_NUM; i++)
	{
		if(strcmp(trt__SetAudioSourceConfiguration->Configuration->token,
		          gOnvifInfo.OnvifAudioSource[i].token))
		{
			onvif_fault(soap, "ter:InvalidArgVal", "ter:ConfigModify");
			return SOAP_FAULT;
		}
	}

	if(onvif_get_audio_source_cfg_from_token(
	            trt__SetAudioSourceConfiguration->Configuration->token,
	            &OnvifAudioSourceCfg) < 0
	        || onvif_get_audio_source_from_token(
	            trt__SetAudioSourceConfiguration->Configuration->SourceToken,
	            &OnvifAudioSource) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__SetAudioEncoderConfiguration(struct soap* soap,
        struct _trt__SetAudioEncoderConfiguration* trt__SetAudioEncoderConfiguration,
        struct _trt__SetAudioEncoderConfigurationResponse*
        trt__SetAudioEncoderConfigurationResponse)
{
	HI_DEV_AUDIO_CFG_S stAutoCfg;
	struct tt__AudioEncoderConfiguration* Configuration;
	HI_ONVIF_AUDIO_ENCODER_CFG_S OnvifEncoderCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;
	memset(&stAutoCfg, 0, sizeof(HI_DEV_AUDIO_CFG_S));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_AUDIO_PARAM, 0,
	                     (char*)&stAutoCfg, sizeof(HI_DEV_AUDIO_CFG_S));
	stAutoCfg.u8Open = 1;
	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_AUDIO_PARAM, 0,
	                     (char*)&stAutoCfg, sizeof(HI_DEV_AUDIO_CFG_S));
	Configuration = trt__SetAudioEncoderConfiguration->Configuration;

	if(onvif_get_audio_encoder_cfg_from_token(Configuration->token,
	        &OnvifEncoderCfg) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	if(Configuration->Encoding != tt__AudioEncoding__G711)
	{
		__E("The Encoding is not G711.\n");
		goto ConfigModifyError;
	}

	if(strcmp(Configuration->token, "AudioSourceConfigureToken") == 0)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_AUDIO_PARAM, 0,
		                     (char*)&stAutoCfg, sizeof(HI_DEV_AUDIO_CFG_S));
	}

	if(Configuration->SampleRate)
	{
		if(Configuration->SampleRate != 8000)
		{
			__E("The SampleRate is invalid\r\n");
			goto ConfigModifyError;
		}

		stAutoCfg.u32SampleRate = Configuration->SampleRate;
	}

	if(strcmp(Configuration->token, "AudioSourceConfigureToken") == 0)
	{
		gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_AUDIO_PARAM, 0,
		                     (char*)&stAutoCfg, sizeof(HI_DEV_AUDIO_CFG_S));
	}

	__FUN_END("\n");
	return SOAP_OK;
ConfigModifyError:
	onvif_fault(soap, "ter:InvalidArgVal", "ter:ConfigModify");
	return SOAP_FAULT;
	//onvif_fault(soap, "ter:ActionNotSupported", "ter:AudioNotSupported");
	return SOAP_OK;
}

int  __trt__SetVideoAnalyticsConfiguration(struct soap* soap,
        struct _trt__SetVideoAnalyticsConfiguration*
        trt__SetVideoAnalyticsConfiguration,
        struct _trt__SetVideoAnalyticsConfigurationResponse*
        trt__SetVideoAnalyticsConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__SetMetadataConfiguration(struct soap* soap,
                                     struct _trt__SetMetadataConfiguration* trt__SetMetadataConfiguration,
                                     struct _trt__SetMetadataConfigurationResponse*
                                     trt__SetMetadataConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__SetAudioOutputConfiguration(struct soap* soap,
                                        struct _trt__SetAudioOutputConfiguration* trt__SetAudioOutputConfiguration,
                                        struct _trt__SetAudioOutputConfigurationResponse*
                                        trt__SetAudioOutputConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__SetAudioDecoderConfiguration(struct soap* soap,
        struct _trt__SetAudioDecoderConfiguration* trt__SetAudioDecoderConfiguration,
        struct _trt__SetAudioDecoderConfigurationResponse*
        trt__SetAudioDecoderConfigurationResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetVideoSourceConfigurationOptions(struct soap* soap,
        struct _trt__GetVideoSourceConfigurationOptions*
        trt__GetVideoSourceConfigurationOptions,
        struct _trt__GetVideoSourceConfigurationOptionsResponse*
        trt__GetVideoSourceConfigurationOptionsResponse)
{
	struct tt__VideoSourceConfigurationOptions* Options;
	struct tt__IntRectangleRange* BoundsRange;
	HI_ONVIF_VIDEO_SOURCE_CFG_S OnvifVideoSourceCfg;
	//	HI_DEV_VIDEO_CFG_S VideoCfg;

	if(onvif_get_video_source_cfg_from_token(
	            trt__GetVideoSourceConfigurationOptions->ConfigurationToken,
	            &OnvifVideoSourceCfg) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(Options, struct tt__VideoSourceConfigurationOptions);
	ALLOC_STRUCT(BoundsRange, struct tt__IntRectangleRange);
	ALLOC_STRUCT(BoundsRange->XRange, struct tt__IntRange);
	ALLOC_STRUCT(BoundsRange->YRange, struct tt__IntRange);
	ALLOC_STRUCT(BoundsRange->WidthRange, struct tt__IntRange);
	ALLOC_STRUCT(BoundsRange->HeightRange, struct tt__IntRange);
	BoundsRange->XRange->Min = 0;
	BoundsRange->XRange->Max = 0;
	BoundsRange->YRange->Min = 0;
	BoundsRange->YRange->Max = 0;
	BoundsRange->WidthRange->Min = OnvifVideoSourceCfg.Bounds.width;
	BoundsRange->WidthRange->Max = OnvifVideoSourceCfg.Bounds.width;
	BoundsRange->HeightRange->Min = OnvifVideoSourceCfg.Bounds.height;
	BoundsRange->HeightRange->Max = OnvifVideoSourceCfg.Bounds.height;
	Options->BoundsRange = BoundsRange;
	Options->__sizeVideoSourceTokensAvailable = 1;
	ALLOC_STRUCT_NUM(Options->VideoSourceTokensAvailable, char*, 1);
	ALLOC_TOKEN(Options->VideoSourceTokensAvailable[0],
	            OnvifVideoSourceCfg.SourceToken);
	trt__GetVideoSourceConfigurationOptionsResponse->Options = Options;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetVideoEncoderConfigurationOptions(struct soap* soap,
        struct _trt__GetVideoEncoderConfigurationOptions*
        trt__GetVideoEncoderConfigurationOptions,
        struct _trt__GetVideoEncoderConfigurationOptionsResponse*
        trt__GetVideoEncoderConfigurationOptionsResponse)
{
	HI_ONVIF_VIDEO_ENCODER_CFG_S OnvifVideoEncoderCfg;
	struct tt__VideoEncoderConfigurationOptions* Options;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(onvif_get_video_encoder_cfg_from_token(
	            trt__GetVideoEncoderConfigurationOptions->ConfigurationToken,
	            &OnvifVideoEncoderCfg) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(Options, struct tt__VideoEncoderConfigurationOptions);
	ALLOC_STRUCT(Options->QualityRange, struct tt__IntRange);
	Options->QualityRange->Min = 0;
	Options->QualityRange->Max = 5;
	Options->JPEG = NULL;
	Options->MPEG4 = NULL;
	ALLOC_STRUCT(Options->H264, struct tt__H264Options);
	ALLOC_STRUCT(Options->H264->GovLengthRange, struct tt__IntRange);
	Options->H264->GovLengthRange->Min = 1;
	Options->H264->GovLengthRange->Max = 100;
	ALLOC_STRUCT(Options->H264->FrameRateRange, struct tt__IntRange);
	Options->H264->FrameRateRange->Min = 10;
	Options->H264->FrameRateRange->Max = 30;
	ALLOC_STRUCT(Options->H264->EncodingIntervalRange, struct tt__IntRange);
	Options->H264->EncodingIntervalRange->Min = 0;
	Options->H264->EncodingIntervalRange->Max = 100;
	ALLOC_STRUCT_NUM(Options->H264->H264ProfilesSupported, enum tt__H264Profile, 3);
	Options->H264->__sizeH264ProfilesSupported = 3;
	Options->H264->H264ProfilesSupported[0] = tt__H264Profile__High;
	Options->H264->H264ProfilesSupported[1] = tt__H264Profile__Main;
	Options->H264->H264ProfilesSupported[2] = tt__H264Profile__Baseline;
	Options->H264->__sizeResolutionsAvailable = 1;

	if(strcmp(trt__GetVideoEncoderConfigurationOptions->ConfigurationToken,
	          ONVIF_MEDIA_VIDEO_MAIN_ENCODER_TOKEN) == 0)
	{
		//Options->H264->__sizeResolutionsAvailable = 2;
		//Options->H264->ResolutionsAvailable = gOnvifInfo.MediaConfig.h264Options;
		ALLOC_STRUCT(Options->H264->ResolutionsAvailable, struct tt__VideoResolution);
		*Options->H264->ResolutionsAvailable = gOnvifInfo.MediaConfig.h264Options[0];
	}
	else if(strcmp(trt__GetVideoEncoderConfigurationOptions->ConfigurationToken,
	               ONVIF_MEDIA_VIDEO_SUB_ENCODER_TOKEN) == 0)
	{
		ALLOC_STRUCT(Options->H264->ResolutionsAvailable, struct tt__VideoResolution);
		*Options->H264->ResolutionsAvailable = gOnvifInfo.MediaConfig.h264Options[1];
	}

	Options->Extension = NULL;
	Options->__anyAttribute = NULL;
	trt__GetVideoEncoderConfigurationOptionsResponse->Options = Options;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetAudioSourceConfigurationOptions(struct soap* soap,
        struct _trt__GetAudioSourceConfigurationOptions*
        trt__GetAudioSourceConfigurationOptions,
        struct _trt__GetAudioSourceConfigurationOptionsResponse*
        trt__GetAudioSourceConfigurationOptionsResponse)
{
	struct tt__AudioSourceConfigurationOptions* AudioSourceConfigurationOptions;
	HI_ONVIF_AUDIO_SOURCE_CFG_S OnvifAudioSourceCfg;
	//	int i;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(trt__GetAudioSourceConfigurationOptions->ConfigurationToken != NULL)
	{
		if(onvif_get_audio_source_cfg_from_token(
		            trt__GetAudioSourceConfigurationOptions->ConfigurationToken,
		            &OnvifAudioSourceCfg) < 0)
		{
			onvif_fault(soap, "ter:InvalidArgVal", "ter:NoAudioSource");
			return SOAP_FAULT;
		}
	}

	ALLOC_STRUCT(AudioSourceConfigurationOptions,
	             struct tt__AudioSourceConfigurationOptions);
	AudioSourceConfigurationOptions->__sizeInputTokensAvailable = 1;
	ALLOC_STRUCT_NUM(AudioSourceConfigurationOptions->InputTokensAvailable, char*,
	                 3);
	ALLOC_TOKEN(AudioSourceConfigurationOptions->InputTokensAvailable[0],
	            OnvifAudioSourceCfg.SourceToken);
	trt__GetAudioSourceConfigurationOptionsResponse->Options =
	    AudioSourceConfigurationOptions;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetAudioEncoderConfigurationOptions(struct soap* soap,
        struct _trt__GetAudioEncoderConfigurationOptions*
        trt__GetAudioEncoderConfigurationOptions,
        struct _trt__GetAudioEncoderConfigurationOptionsResponse*
        trt__GetAudioEncoderConfigurationOptionsResponse)
{
	HI_ONVIF_AUDIO_ENCODER_CFG_S OnvifAudioEncoderCfg;
	struct tt__AudioEncoderConfigurationOptions* Options;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(onvif_get_audio_encoder_cfg_from_token(
	            trt__GetAudioEncoderConfigurationOptions->ConfigurationToken,
	            &OnvifAudioEncoderCfg) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	Options = (struct tt__AudioEncoderConfigurationOptions*)soap_malloc(soap,
	          sizeof(struct tt__AudioEncoderConfigurationOptions));

	if(Options == NULL)
	{
		__E("Failed to malloc for options.\n");
		return SOAP_FAULT;
	}

	Options->__sizeOptions = 1;
	Options->Options = (struct tt__AudioEncoderConfigurationOption*)soap_malloc(
	                       soap, sizeof(struct tt__AudioEncoderConfigurationOption));

	if(Options == NULL)
	{
		__E("Failed to malloc for options.\n");
		return SOAP_FAULT;
	}

	Options->Options->Encoding = OnvifAudioEncoderCfg.Encoding;
	Options->Options->BitrateList = (struct tt__IntList*)soap_malloc(soap,
	                                sizeof(struct tt__IntList));

	if(Options->Options->BitrateList == NULL)
	{
		__E("Failed to malloc for BitrateList.\n");
		return SOAP_FAULT;
	}

	Options->Options->SampleRateList = (struct tt__IntList*)soap_malloc(soap,
	                                   sizeof(struct tt__IntList));

	if(Options->Options->SampleRateList == NULL)
	{
		__E("Failed to malloc for SampleRateList.\n");
		return SOAP_FAULT;
	}

	Options->Options->BitrateList->Items = (int*)soap_malloc(soap, sizeof(int) * 4);
	Options->Options->SampleRateList->Items = (int*)soap_malloc(soap,
	        sizeof(int) * 4);
	Options->Options->BitrateList->__sizeItems = 1;
	*Options->Options->BitrateList->Items = OnvifAudioEncoderCfg.Bitrate;
	Options->Options->SampleRateList->__sizeItems = 1;
	*Options->Options->SampleRateList->Items = OnvifAudioEncoderCfg.SampleRate;
	Options->Options->__size = 0;
	Options->Options->__any = NULL;
	Options->Options->__anyAttribute = NULL;
	trt__GetAudioEncoderConfigurationOptionsResponse->Options = Options;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetMetadataConfigurationOptions(struct soap* soap,
        struct _trt__GetMetadataConfigurationOptions*
        trt__GetMetadataConfigurationOptions,
        struct _trt__GetMetadataConfigurationOptionsResponse*
        trt__GetMetadataConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetAudioOutputConfigurationOptions(struct soap* soap,
        struct _trt__GetAudioOutputConfigurationOptions*
        trt__GetAudioOutputConfigurationOptions,
        struct _trt__GetAudioOutputConfigurationOptionsResponse*
        trt__GetAudioOutputConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetAudioDecoderConfigurationOptions(struct soap* soap,
        struct _trt__GetAudioDecoderConfigurationOptions*
        trt__GetAudioDecoderConfigurationOptions,
        struct _trt__GetAudioDecoderConfigurationOptionsResponse*
        trt__GetAudioDecoderConfigurationOptionsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetGuaranteedNumberOfVideoEncoderInstances(struct soap* soap,
        struct _trt__GetGuaranteedNumberOfVideoEncoderInstances*
        trt__GetGuaranteedNumberOfVideoEncoderInstances,
        struct _trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse*
        trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;

	if(strcmp(trt__GetGuaranteedNumberOfVideoEncoderInstances->ConfigurationToken,
	          ONVIF_MEDIA_VIDEO_MAIN_CFG_TOKEN) != 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->TotalNumber = 1;
	return SOAP_OK;
}

int  __trt__GetStreamUri(struct soap* soap,
                         struct _trt__GetStreamUri* trt__GetStreamUri,
                         struct _trt__GetStreamUriResponse* trt__GetStreamUriResponse)
{
	struct tt__StreamSetup* StreamSetup;
	struct tt__MediaUri* MediaUri;
	HI_ONVIF_PROFILE_S OnvifProfile;
	__FUN_BEGIN("\n");
	CHECK_USER;
	unsigned long ip;
	ip = onvif_get_ipaddr(soap);

	if(onvif_get_profile_from_token(ip, trt__GetStreamUri->ProfileToken,
	                                &OnvifProfile) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	if(trt__GetStreamUri->StreamSetup == NULL)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidStreamSetup");
		return SOAP_FAULT;
	}

	StreamSetup = trt__GetStreamUri->StreamSetup;

	if(StreamSetup->Stream != tt__StreamType__RTP_Unicast)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:InvalidStreamSetup");
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(MediaUri, struct tt__MediaUri);
	ALLOC_TOKEN(MediaUri->Uri, OnvifProfile.streamUri);
	/*
		if(strcmp("MainStreamProfileToken",trt__GetStreamUri->ProfileToken) == 0 )
		strcpy(MediaUri->Uri,"rtsp://192.168.8.8/user=admin_password=tlJwpbo6_channel=1_stream=0.sdp?real_stream");
		else
		strcpy(MediaUri->Uri,"rtsp://192.168.8.8/user=admin_password=tlJwpbo6_channel=1_stream=1.sdp?real_stream");


		if(strcmp("MainStreamProfileToken",trt__GetStreamUri->ProfileToken) == 0 )
			strcpy(MediaUri->Uri,"rtsp://192.168.8.89:8554/h264ESVideoTest");
			else
			strcpy(MediaUri->Uri,"rtsp://192.168.8.89:8554/h264ESVideoTest");



		//strcpy(MediaUri->Uri,"rtsp://192.168.8.89:8554/testStream");


		if(strcmp("MainStreamProfileToken",trt__GetStreamUri->ProfileToken) == 0 )
			strcpy(MediaUri->Uri,"rtsp://192.168.8.89:7554/dev=IPC-2362299625233248/media=0/channel=0&level=0");
		else
			strcpy(MediaUri->Uri,"rtsp://192.168.8.89:7554/dev=IPC-2362299625233248/media=0/channel=0&level=1");
		*/
	/*
	if(strcmp(ONVIF_MEDIA_MAIN_PROFILE_TOKEN, trt__GetStreamUri->ProfileToken) == 0 )
	{
		onvif_net_get_media_url(0, MediaUri->Uri);
	}
	else
	{
		onvif_net_get_media_url(1, MediaUri->Uri);
	}

	printf("use %s %s\n", trt__GetStreamUri->ProfileToken, MediaUri->Uri);
	*/
	/*
	if(gOnvifInfo.MediaConfig.VideoResolutionconfig.Height == gOnvifInfo.MediaConfig.h264Options[1].Height &&
		gOnvifInfo.MediaConfig.VideoResolutionconfig.Width == gOnvifInfo.MediaConfig.h264Options[1].Width)
			onvif_get_profile(1, &OnvifProfile);
	*/
	MediaUri->InvalidAfterConnect = xsd__boolean__false_;
	MediaUri->InvalidAfterReboot = xsd__boolean__false_;
	//yyyyyyyyyyyyyyy
	MediaUri->Timeout = 3000;
	//ALLOC_TOKEN(MediaUri->Timeout, "PT3S");
	trt__GetStreamUriResponse->MediaUri = MediaUri;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__StartMulticastStreaming(struct soap* soap,
                                    struct _trt__StartMulticastStreaming* trt__StartMulticastStreaming,
                                    struct _trt__StartMulticastStreamingResponse*
                                    trt__StartMulticastStreamingResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__StopMulticastStreaming(struct soap* soap,
                                   struct _trt__StopMulticastStreaming* trt__StopMulticastStreaming,
                                   struct _trt__StopMulticastStreamingResponse*
                                   trt__StopMulticastStreamingResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__SetSynchronizationPoint(struct soap* soap,
                                    struct _trt__SetSynchronizationPoint* trt__SetSynchronizationPoint,
                                    struct _trt__SetSynchronizationPointResponse*
                                    trt__SetSynchronizationPointResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	//int nCh = 0;
	int nStream = 0;

	if(strcmp(trt__SetSynchronizationPoint->ProfileToken,
	          ONVIF_MEDIA_SUB_PROFILE_TOKEN) == 0)
	{
		nStream = 1;
	}
	else if(strcmp(trt__SetSynchronizationPoint->ProfileToken,
	               ONVIF_MEDIA_MAIN_PROFILE_TOKEN) == 0)
	{
		nStream = 0;
	}
	else
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	gOnvifDevCb.pStreamReqIframe(0, nStream);
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetSnapshotUri(struct soap* soap,
                           struct _trt__GetSnapshotUri* trt__GetSnapshotUri,
                           struct _trt__GetSnapshotUriResponse* trt__GetSnapshotUriResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	//	int tryNum = 0;
	char ipaddr[32];
	//	char Uri[512];
	HI_ONVIF_PROFILE_S OnvifProfile;
	struct tt__MediaUri* MediaUri;
	unsigned long ip;
	ip = htonl(hi_get_sock_ip(soap->socket));

	if(onvif_get_profile_from_token(ip, trt__GetSnapshotUri->ProfileToken,
	                                &OnvifProfile) < 0)
	{
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(MediaUri, struct tt__MediaUri);
	ALLOC_STRUCT_NUM(MediaUri->Uri, char, ONVIF_ADDRESS_LEN)
	hi_ip_n2a(ip, ipaddr, 32);
	sprintf(MediaUri->Uri, "http://%s/snapshot.jpg", ipaddr);
	MediaUri->InvalidAfterConnect = xsd__boolean__false_;
	MediaUri->InvalidAfterReboot = xsd__boolean__false_;
	//yyyyyyyyyyyyyyy
	MediaUri->Timeout = 3000;
	//ALLOC_TOKEN(MediaUri->Timeout, "PT3S");
	MediaUri->__size = 0;
	MediaUri->__any = NULL;
	MediaUri->__anyAttribute = NULL;
	trt__GetSnapshotUriResponse->MediaUri = MediaUri;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetVideoSourceModes(struct soap* soap,
                                struct _trt__GetVideoSourceModes* trt__GetVideoSourceModes,
                                struct _trt__GetVideoSourceModesResponse* trt__GetVideoSourceModesResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__SetVideoSourceMode(struct soap* soap,
                               struct _trt__SetVideoSourceMode* trt__SetVideoSourceMode,
                               struct _trt__SetVideoSourceModeResponse* trt__SetVideoSourceModeResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetOSDs(struct soap* soap, struct _trt__GetOSDs* trt__GetOSDs,
                    struct _trt__GetOSDsResponse* trt__GetOSDsResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	int i = 0;
	HI_DEV_OSD_CFG_S stOsdCfg;
	struct tt__OSDConfiguration* OSDConfiguration;
	//--0 24 hour --1 12 hour
	const char* timeFormat[] = {"HH:mm:ss", "hh:mm:ss tt"};
	const char* dateFormat[] = {"yyyy/MM/dd", "MM/dd/yyyy", "dd/MM/yyyy"};

	if (!trt__GetOSDs)
	{
		return SOAP_FAULT;
	}

	trt__GetOSDsResponse->__sizeOSDs = 2;
	OSDConfiguration = soap_malloc(soap,
	                               trt__GetOSDsResponse->__sizeOSDs * sizeof(struct tt__OSDConfiguration));
	memset(OSDConfiguration, 0,
	       trt__GetOSDsResponse->__sizeOSDs * sizeof(struct tt__OSDConfiguration));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_OSD_PARAM, 0,
	                     (char*)&stOsdCfg, sizeof(HI_DEV_OSD_CFG_S));

	for (i = 0; i < trt__GetOSDsResponse->__sizeOSDs; i++)
	{
		unsigned int Video_ShowWight  = 704;
		unsigned int Video_ShowHeight = 576;
		OSDConfiguration[i].token = soap_malloc(soap, 32 * sizeof(char));

		if (i == 0)
		{
			sprintf(OSDConfiguration[i].token, ONVIF_OSD_DATETIME_TOKEN);
		}
		else if (i == 1)
		{
			sprintf(OSDConfiguration[i].token, ONVIF_OSD_CHN_TOKEN);
		}

		ALLOC_STRUCT(OSDConfiguration[i].VideoSourceConfigurationToken,
		             struct tt__OSDReference);
		OSDConfiguration[i].VideoSourceConfigurationToken->__item = soap_malloc(soap,
		        32 * sizeof(char));
		sprintf(OSDConfiguration[i].VideoSourceConfigurationToken->__item,
		        trt__GetOSDs->ConfigurationToken);
		OSDConfiguration[i].VideoSourceConfigurationToken->__anyAttribute = NULL;
		OSDConfiguration[i].Type = tt__OSDType__Text;
		ALLOC_STRUCT(OSDConfiguration[i].Position, struct tt__OSDPosConfiguration);
		OSDConfiguration[i].Position->Type = soap_malloc(soap, 32);
		sprintf(OSDConfiguration[i].Position->Type, "Custom");
		ALLOC_STRUCT(OSDConfiguration[i].Position->Pos, struct tt__Vector);
		ALLOC_STRUCT(OSDConfiguration[i].Position->Pos->x, float);
		ALLOC_STRUCT(OSDConfiguration[i].Position->Pos->y, float);

		////////////////////////20140605 zj///////////////////////////////
		if (i == 0)
		{
			//set time data
			*OSDConfiguration[i].Position->Pos->x = (float)(stOsdCfg.struTimeOrg.s32X) /
			                                        (Video_ShowWight / 2) - 1;
			stOsdCfg.struTimeOrg.s32Y = Video_ShowHeight - stOsdCfg.struTimeOrg.s32Y;
			*OSDConfiguration[i].Position->Pos->y = (float)(stOsdCfg.struTimeOrg.s32Y) /
			                                        (Video_ShowHeight / 2) - 1;
		}
		else
		{
			//set title
			*OSDConfiguration[i].Position->Pos->x = (float)(stOsdCfg.struUsrOrg[0].s32X) /
			                                        (Video_ShowWight / 2) - 1;
			stOsdCfg.struUsrOrg[0].s32Y = Video_ShowHeight - stOsdCfg.struUsrOrg[0].s32Y;
			*OSDConfiguration[i].Position->Pos->y = (float)(stOsdCfg.struUsrOrg[0].s32Y) /
			                                        (Video_ShowHeight / 2) - 1;
		}

		////////////////////////////////END ///////////////////////////////
		OSDConfiguration[i].Position->Extension = NULL;
		OSDConfiguration[i].Position->__anyAttribute = NULL;
		ALLOC_STRUCT(OSDConfiguration[i].TextString, struct tt__OSDTextConfiguration);
		OSDConfiguration[i].TextString->Type = soap_malloc(soap, 32);

		if (i == 0)
		{
			sprintf(OSDConfiguration[i].TextString->Type, "DateAndTime");
		}
		else if (i == 1)
		{
			sprintf(OSDConfiguration[i].TextString->Type, "Plain");
		}

		if (i == 0)
		{
			if (stOsdCfg.u8TimeOsdType > 5)
			{
				stOsdCfg.u8TimeOsdType = 0;
			}

			if (stOsdCfg.u8HourOsdType > 1)
			{
				stOsdCfg.u8HourOsdType = 0;
			}

			OSDConfiguration[i].TextString->DateFormat = soap_malloc(soap, 32);
			sprintf(OSDConfiguration[i].TextString->DateFormat,
			        dateFormat[stOsdCfg.u8TimeOsdType]);
			OSDConfiguration[i].TextString->TimeFormat = soap_malloc(soap, 32);
			sprintf(OSDConfiguration[i].TextString->TimeFormat,
			        timeFormat[stOsdCfg.u8HourOsdType]);
		}
		else
		{
			OSDConfiguration[i].TextString->DateFormat = NULL;
			OSDConfiguration[i].TextString->TimeFormat = NULL;
		}

		ALLOC_STRUCT(OSDConfiguration[i].TextString->FontSize, int);
		*OSDConfiguration[i].TextString->FontSize = 32;
#if 0
		ALLOC_STRUCT(OSDConfiguration[i].TextString->FontColor, struct tt__OSDColor);
		ALLOC_STRUCT(OSDConfiguration[i].TextString->FontColor->Color,
		             struct tt__Color);
		OSDConfiguration[i].TextString->FontColor->Color->X = 1;
		OSDConfiguration[i].TextString->FontColor->Color->Y = 1;
		OSDConfiguration[i].TextString->FontColor->Color->Z = 1;
		OSDConfiguration[i].TextString->FontColor->Color->Colorspace = soap_malloc(soap,
		        32);
		sprintf(OSDConfiguration[i].TextString->FontColor->Color->Colorspace,
		        "Colorspace1");
		ALLOC_STRUCT(OSDConfiguration[i].TextString->FontColor->Transparent, int);
		*OSDConfiguration[i].TextString->FontColor->Transparent = 50;
		OSDConfiguration[i].TextString->FontColor->__anyAttribute = NULL;
		ALLOC_STRUCT(OSDConfiguration[i].TextString->BackgroundColor,
		             struct tt__OSDColor);
		ALLOC_STRUCT(OSDConfiguration[i].TextString->BackgroundColor->Color,
		             struct tt__Color);
		OSDConfiguration[i].TextString->BackgroundColor->Color->X = 1;
		OSDConfiguration[i].TextString->BackgroundColor->Color->Y = 1;
		OSDConfiguration[i].TextString->BackgroundColor->Color->Z = 1;
		OSDConfiguration[i].TextString->BackgroundColor->Color->Colorspace =
		    soap_malloc(soap, 32);
		sprintf(OSDConfiguration[i].TextString->BackgroundColor->Color->Colorspace,
		        "Colorspace1");
		ALLOC_STRUCT(OSDConfiguration[i].TextString->BackgroundColor->Transparent, int);
		*OSDConfiguration[i].TextString->BackgroundColor->Transparent = 50;
		OSDConfiguration[i].TextString->BackgroundColor->__anyAttribute = NULL;
#else
		OSDConfiguration[i].TextString->FontColor = NULL;
		OSDConfiguration[i].TextString->BackgroundColor = NULL;
#endif

		if (i == 1)
		{
			OSDConfiguration[i].TextString->PlainText = soap_malloc(soap, HI_OSD_NAME_LEN);
			memset(OSDConfiguration[i].TextString->PlainText, 0, HI_OSD_NAME_LEN);
			hi_gb2312_to_utf8(stOsdCfg.szUsrOsdInfo[0], strlen(stOsdCfg.szUsrOsdInfo[0]),
			                  OSDConfiguration[i].TextString->PlainText);
			printf("osd: %s  size=%d\n", stOsdCfg.szUsrOsdInfo[0], strlen(stOsdCfg.szUsrOsdInfo[0]));
		}
		else
		{
			OSDConfiguration[i].TextString->PlainText = NULL;
		}

		OSDConfiguration[i].Image = NULL;
#if 1
		OSDConfiguration[i].TextString->Extension = NULL;
		OSDConfiguration[i].TextString->__anyAttribute = NULL;
#else
		OSDConfiguration[i].__anyAttribute = NULL;
		ALLOC_STRUCT(OSDConfiguration[i].Extension,
		             struct tt__OSDConfigurationExtension);
		OSDConfiguration[i].Extension->__size =  1;
		OSDConfiguration[i].Extension->__any = soap_malloc(soap, 32);
		sprintf(OSDConfiguration[i].Extension->__any, "ChannelName");
		OSDConfiguration[i].Extension->__anyAttribute = soap_malloc(soap, 32);
		sprintf(OSDConfiguration[i].Extension->__anyAttribute, "true");
#endif
	}

	trt__GetOSDsResponse->OSDs = OSDConfiguration;
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__GetOSD(struct soap* soap, struct _trt__GetOSD* trt__GetOSD,
                   struct _trt__GetOSDResponse* trt__GetOSDResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	__FUN_END("\n");
	return SOAP_FAULT;
}

int  __trt__GetOSDOptions(struct soap* soap,
                          struct _trt__GetOSDOptions* trt__GetOSDOptions,
                          struct _trt__GetOSDOptionsResponse* trt__GetOSDOptionsResponse)
{
	//--0 YYYY-MM-DD HH:MM:SS
	//--1 MM-DD-YYYY HH:MM:SS
	//--2 DD-MM-YYYY HH:MM:SS
	//--3 YYYY/MM/DD HH:MM:SS
	//--4 MM/DD/YYYY HH:MM:SS
	//--5 DD/MM/YYYY HH:MM:SS
	const char* timeFormat[] = {"HH:mm:ss", "hh:mm:ss tt"};
	const char* dateFormat[] = {"yyyy/MM/dd", "MM/dd/yyyy", "dd/MM/yyyy"};
	int i = 0;
	HI_DEV_OSD_CFG_S  stOsdCfg;
	struct tt__MaximumNumberOfOSDs*  MaximumNumberOfOSDs = NULL;
	struct tt__OSDTextOptions*	OSDTextOptions = NULL;
	//struct tt__ColorOptions  *ColorOptions = NULL;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if (!trt__GetOSDOptions)
	{
		return SOAP_FAULT;
	}

	ALLOC_STRUCT(trt__GetOSDOptionsResponse->OSDOptions,
	             struct tt__OSDConfigurationOptions);
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_OSD_PARAM, 0,
	                     (char*)&stOsdCfg, sizeof(HI_DEV_OSD_CFG_S));
	//trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs;
	ALLOC_STRUCT(MaximumNumberOfOSDs, struct tt__MaximumNumberOfOSDs);
	//for (i = 0; i < MaximumNumberOfOSDs->Total; i++)
	//{
	ALLOC_STRUCT(MaximumNumberOfOSDs->Image, int);
	MaximumNumberOfOSDs->Image = 0;
	ALLOC_STRUCT(MaximumNumberOfOSDs->DateAndTime, int);
	*MaximumNumberOfOSDs->DateAndTime = 1;
	ALLOC_STRUCT(MaximumNumberOfOSDs->Date, int);
	*MaximumNumberOfOSDs->Date = 1;
	ALLOC_STRUCT(MaximumNumberOfOSDs->PlainText, int);
	*MaximumNumberOfOSDs->PlainText = 1;
	ALLOC_STRUCT(MaximumNumberOfOSDs->Time, int);
	*MaximumNumberOfOSDs->Time = 1;
	MaximumNumberOfOSDs->Total = 4;
	trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs =
	    MaximumNumberOfOSDs;
	//}
	//MaximumNumberOfOSDs->
	//osd type
	trt__GetOSDOptionsResponse->OSDOptions->__sizeType = 1;
	ALLOC_STRUCT(trt__GetOSDOptionsResponse->OSDOptions->Type, enum tt__OSDType);
	trt__GetOSDOptionsResponse->OSDOptions->Type = tt__OSDType__Text;
	trt__GetOSDOptionsResponse->OSDOptions->__sizePositionOption = 1;
	trt__GetOSDOptionsResponse->OSDOptions->PositionOption = (char**)soap_malloc(
	            soap, trt__GetOSDOptionsResponse->OSDOptions->__sizePositionOption * sizeof(
	                char*));

	for (i = 0; i < trt__GetOSDOptionsResponse->OSDOptions->__sizePositionOption;
	        i++)
	{
		trt__GetOSDOptionsResponse->OSDOptions->PositionOption[i] = soap_malloc(soap,
		        32 * sizeof(char));
		sprintf(trt__GetOSDOptionsResponse->OSDOptions->PositionOption[i], "Custom");
	}

	/////////////////////////////////////////////////////
	ALLOC_STRUCT(OSDTextOptions, struct tt__OSDTextOptions);
	OSDTextOptions->__sizeType = 4;
	OSDTextOptions->Type = (char**)soap_malloc(soap,
	                       OSDTextOptions->__sizeType * sizeof(char*));

	for (i = 0; i < OSDTextOptions->__sizeType; i++)
	{
		OSDTextOptions->Type[i] = (char*)soap_malloc(soap, sizeof(char) * 64);

		if (i == 0)
		{
			strcpy(OSDTextOptions->Type[i], "PlainText");
		}
		else if (i == 1)
		{
			strcpy(OSDTextOptions->Type[i], "Date");
		}
		else if (i == 2)
		{
			strcpy(OSDTextOptions->Type[i], "Time");
		}
		else if (i == 3)
		{
			strcpy(OSDTextOptions->Type[i], "DateAndTime");
		}
	}

	ALLOC_STRUCT(OSDTextOptions->FontSizeRange, struct tt__IntRange);
	OSDTextOptions->FontSizeRange->Max = 32;
	OSDTextOptions->FontSizeRange->Min = 8;
	OSDTextOptions->__sizeDateFormat = sizeof(dateFormat) / sizeof(dateFormat[0]);
	OSDTextOptions->DateFormat = (char**)soap_malloc(soap,
	                             OSDTextOptions->__sizeDateFormat * sizeof(char*));

	for (i = 0; i < OSDTextOptions->__sizeDateFormat; i++)
	{
		OSDTextOptions->DateFormat[i] = (char*)soap_malloc(soap, sizeof(char) * 64);
		sprintf(OSDTextOptions->DateFormat[i], dateFormat[i]);
	}

	OSDTextOptions->__sizeTimeFormat = 2;
	OSDTextOptions->TimeFormat = (char**)soap_malloc(soap,
	                             OSDTextOptions->__sizeTimeFormat * sizeof(char*));

	for (i = 0; i < OSDTextOptions->__sizeTimeFormat; i++)
	{
		OSDTextOptions->TimeFormat[i] = (char*)soap_malloc(soap, sizeof(char) * 64);
		sprintf(OSDTextOptions->TimeFormat[i], timeFormat[i]);
	}

#if 0
	ALLOC_STRUCT(OSDTextOptions->FontColor, struct tt__OSDColorOptions);
	memset(OSDTextOptions->FontColor, 0, sizeof(struct tt__OSDColorOptions));
	ALLOC_STRUCT(ColorOptions, struct tt__ColorOptions);
	ColorOptions->__sizeColorList = 4;
	ColorOptions->ColorList = (struct tt__Color*)soap_malloc(soap,
	                          ColorOptions->__sizeColorList * sizeof(struct tt__Color));

	for (i = 0; i < ColorOptions->__sizeColorList; i++)
	{
		ColorOptions->ColorList[i].X = i;
		ColorOptions->ColorList[i].Y = i;
		ColorOptions->ColorList[i].Z = i;
		ColorOptions->ColorList[i].Colorspace = (char*)soap_malloc(soap, 25);
		sprintf(ColorOptions->ColorList[i].Colorspace, "Colorspace%d", i);
	}

	ColorOptions->__sizeColorspaceRange = 1;
	ColorOptions->ColorspaceRange =  (struct tt__ColorspaceRange*)soap_malloc(soap,
	                                 sizeof(struct tt__ColorspaceRange));
	ALLOC_STRUCT(ColorOptions->ColorspaceRange->X, struct tt__FloatRange);
	ColorOptions->ColorspaceRange->X->Max = 50;
	ColorOptions->ColorspaceRange->X->Min = 50;
	ALLOC_STRUCT(ColorOptions->ColorspaceRange->Y, struct tt__FloatRange);
	ColorOptions->ColorspaceRange->Y->Max = 50;
	ColorOptions->ColorspaceRange->Y->Min = 50;
	ALLOC_STRUCT(ColorOptions->ColorspaceRange->Z, struct tt__FloatRange);
	ColorOptions->ColorspaceRange->Z->Max = 50;
	ColorOptions->ColorspaceRange->Z->Min = 50;
	ColorOptions->ColorspaceRange->Colorspace = (char*)soap_malloc(soap, 32);
	sprintf(ColorOptions->ColorspaceRange->Colorspace, "Colorspace");
	ColorOptions->ColorspaceRange->__anyAttribute = NULL;
	OSDTextOptions->FontColor->Color = ColorOptions;
	OSDTextOptions->FontColor->Extension = NULL;
	OSDTextOptions->FontColor->__anyAttribute = NULL;
	ALLOC_STRUCT(OSDTextOptions->FontColor->Transparent, struct tt__IntRange);
	OSDTextOptions->FontColor->Transparent->Max = 50;
	OSDTextOptions->FontColor->Transparent->Min = 50;
#else
	OSDTextOptions->FontColor = NULL;
	OSDTextOptions->BackgroundColor = NULL;
#endif
	trt__GetOSDOptionsResponse->OSDOptions->TextOption = OSDTextOptions;
	////////////////////////////////////////////////
	trt__GetOSDOptionsResponse->OSDOptions->ImageOption = NULL;
	trt__GetOSDOptionsResponse->OSDOptions->Extension = NULL;
	trt__GetOSDOptionsResponse->OSDOptions->__anyAttribute = NULL;
	trt__GetOSDOptionsResponse->__size = 0;
	trt__GetOSDOptionsResponse->__any = NULL;
	//trt__GetOSDOptions->ConfigurationToken
	__FUN_END("\n");
	return SOAP_OK;
}






int  __trt__SetOSD(struct soap* soap, struct _trt__SetOSD* trt__SetOSD,
                   struct _trt__SetOSDResponse* trt__SetOSDResponse)
{
	//--0 24 hour --1 12 hour
	const char* timeFormat[] = {"HH:mm:ss", "hh:mm:ss tt"};
	const char* dateFormat[] = {"yyyy/MM/dd", "MM/dd/yyyy", "dd/MM/yyyy"};
	int nToken = 0;
	int i = 0;
	float x = 0, y = 0;
	HI_DEV_OSD_CFG_S stOsdCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;

	if (trt__SetOSD == NULL || trt__SetOSD->OSD == NULL)
	{
		return SOAP_FAULT;
	}

	if (trt__SetOSD->OSD->VideoSourceConfigurationToken == NULL ||
	        trt__SetOSD->OSD->VideoSourceConfigurationToken->__item == NULL)
	{
		return SOAP_FAULT;
	}

	if (trt__SetOSD->OSD->token == NULL)
	{
		return SOAP_FAULT;
	}

	if (strcmp(trt__SetOSD->OSD->token, ONVIF_OSD_CHN_TOKEN) == 0)
	{
		nToken = 1;
	}
	else if (strcmp(trt__SetOSD->OSD->token, ONVIF_OSD_DATETIME_TOKEN) == 0)
	{
		nToken = 0;
	}
	else
	{
		return SOAP_FAULT;
	}

	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_OSD_PARAM, 0,
	                     (char*)&stOsdCfg, sizeof(HI_DEV_OSD_CFG_S));

	if (trt__SetOSD->OSD->Type != tt__OSDType__Text)
	{
		return SOAP_FAULT;
	}

	////////////////////////20140605 zj///////////////////////////////
	if (trt__SetOSD->OSD->Position)
	{
		unsigned int Video_ShowWight  = 704;
		unsigned int Video_ShowHeight = 576;
		x = *trt__SetOSD->OSD->Position->Pos->x;
		y = *trt__SetOSD->OSD->Position->Pos->y;

		if (nToken == 1)
		{
			printf("set title============\r\n");
			stOsdCfg.struUsrOrg[0].s32X = (HI_S32)((x + 1) * (Video_ShowWight / 2));
			stOsdCfg.struUsrOrg[0].s32Y = (HI_S32)((y + 1) * (Video_ShowHeight / 2));
			stOsdCfg.struUsrOrg[0].s32Y = Video_ShowHeight - stOsdCfg.struUsrOrg[0].s32Y;
		}
		else if (nToken == 0)
		{
			printf("set time============\r\n");
			stOsdCfg.struTimeOrg.s32X = (HI_S32)((x + 1) * (Video_ShowWight / 2));
			stOsdCfg.struTimeOrg.s32Y = (HI_S32)((y + 1) * (Video_ShowHeight / 2));
			stOsdCfg.struTimeOrg.s32Y = Video_ShowHeight - stOsdCfg.struTimeOrg.s32Y;
		}
	}

	////////////////////////////////END ///////////////////////////////

	if (trt__SetOSD->OSD->TextString)
	{
		if (nToken == 1)
		{
			stOsdCfg.u8EnableUsrOsd[0] = 1;

			if (trt__SetOSD->OSD->TextString->PlainText)
			{
				memset(stOsdCfg.szUsrOsdInfo[0], 0, sizeof(stOsdCfg.szUsrOsdInfo[0]));
				{
					printf("set osd: %s  strlen=%d\n", trt__SetOSD->OSD->TextString->PlainText,
					       strlen(trt__SetOSD->OSD->TextString->PlainText));

					if( strlen(trt__SetOSD->OSD->TextString->PlainText) > HI_OSD_NAME_LEN / 4)
					{
						printf("text_len = %d > HI_OSD_NAME_LEN / 4,not set string.\n",
						       strlen(trt__SetOSD->OSD->TextString->PlainText));
					}
					else
					{
						hi_utf8_to_gb2312(trt__SetOSD->OSD->TextString->PlainText,
						                  strlen(trt__SetOSD->OSD->TextString->PlainText), stOsdCfg.szUsrOsdInfo[0]);
					}
				}
				//hi_utf8_to_gb2312(const char * utf8,int len,char * temp)
				//strncpy(stOsdCfg.szUsrOsdInfo[0], trt__SetOSD->OSD->TextString->PlainText, sizeof(stOsdCfg.szUsrOsdInfo[0])-1);
			}
		}
		else
		{
			stOsdCfg.u8EnableTimeOsd = 1;

			if (trt__SetOSD->OSD->TextString->DateFormat)
			{
				for (i = 0; i < sizeof(dateFormat) / sizeof(dateFormat[0]); i++)
				{
					if(strcmp(trt__SetOSD->OSD->TextString->DateFormat, dateFormat[i]) == 0)
					{
						stOsdCfg.u8TimeOsdType = i;
						break;
					}
				}
			}

			if (trt__SetOSD->OSD->TextString->TimeFormat)
			{
				for (i = 0; i < sizeof(timeFormat) / sizeof(timeFormat[0]); i++)
				{
					if (strcmp(trt__SetOSD->OSD->TextString->TimeFormat, timeFormat[i]) == 0)
					{
						stOsdCfg.u8HourOsdType = i;
						break;
					}
				}
			}
		}
	}

	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_OSD_PARAM, 0,
	                     (char*)&stOsdCfg, sizeof(HI_DEV_OSD_CFG_S));
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__CreateOSD(struct soap* soap, struct _trt__CreateOSD* trt__CreateOSD,
                      struct _trt__CreateOSDResponse* trt__CreateOSDResponse)
{
	__FUN_BEGIN("\n");
	CHECK_USER;
	//////////////////////20140605 zj///////////////////////
	HI_DEV_OSD_CFG_S stOsdCfg;
	static HI_CHAR szLabelNameBak[HI_OSD_REG_NUM][HI_OSD_NAME_LEN];
	static HI_FLOAT struLabelOrgBak[HI_OSD_REG_NUM][2];
	int i;
	float x, y;
	float Video_ShowWight  = 704.0;
	float Video_ShowHeight = 576.0;
	unsigned char FontW = 10, NumLine = 0;

	if (trt__CreateOSD == NULL ||
	        trt__CreateOSD->OSD->TextString->PlainText == NULL)
	{
		return SOAP_FAULT;
	}

	printf("\n__trt__CreateOSD\n");
	printf("trt__CreateOSD->OSD->token:%s\n", trt__CreateOSD->OSD->token);
#if 1
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_OSD_PARAM, 0,
	                     (char*)&stOsdCfg, sizeof(HI_DEV_OSD_CFG_S));

	if(strcmp(trt__CreateOSD->OSD->token, "osdToken_0") == 0)
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_1";
		NumLine = 1;
		memcpy(szLabelNameBak[1], trt__CreateOSD->OSD->TextString->PlainText,
		       strlen(trt__CreateOSD->OSD->TextString->PlainText) + 1);
		struLabelOrgBak[1][0] = *(trt__CreateOSD->OSD->Position->Pos->x);
		struLabelOrgBak[1][1] = *(trt__CreateOSD->OSD->Position->Pos->y);
	}
	else if(strcmp(trt__CreateOSD->OSD->token, "osdToken_1") == 0)
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_2";
		NumLine = 2;
		memcpy(szLabelNameBak[2], trt__CreateOSD->OSD->TextString->PlainText,
		       strlen(trt__CreateOSD->OSD->TextString->PlainText) + 1);
		struLabelOrgBak[2][0] = *(trt__CreateOSD->OSD->Position->Pos->x);
		struLabelOrgBak[2][1] = *(trt__CreateOSD->OSD->Position->Pos->y);
	}
	else if(strcmp(trt__CreateOSD->OSD->token, "osdToken_2") == 0)
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_3";
		NumLine = 3;
		memcpy(szLabelNameBak[3], trt__CreateOSD->OSD->TextString->PlainText,
		       strlen(trt__CreateOSD->OSD->TextString->PlainText) + 1);
		struLabelOrgBak[3][0] = *(trt__CreateOSD->OSD->Position->Pos->x);
		struLabelOrgBak[3][1] = *(trt__CreateOSD->OSD->Position->Pos->y);
	}
	else if(strcmp(trt__CreateOSD->OSD->token, "osdToken_3") == 0)
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_4";
		NumLine = 4;
		memcpy(szLabelNameBak[4], trt__CreateOSD->OSD->TextString->PlainText,
		       strlen(trt__CreateOSD->OSD->TextString->PlainText) + 1);
		struLabelOrgBak[4][0] = *(trt__CreateOSD->OSD->Position->Pos->x);
		struLabelOrgBak[4][1] = *(trt__CreateOSD->OSD->Position->Pos->y);
	}
	else if(strcmp(trt__CreateOSD->OSD->token, "osdToken_4") == 0)
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_5";
	}
	else if(strcmp(trt__CreateOSD->OSD->token, "osdToken_5") == 0)
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_6";
	}
	else if(strcmp(trt__CreateOSD->OSD->token, "osdToken_6") == 0)
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_7";
	}
	else if(strcmp(trt__CreateOSD->OSD->token, "osdToken_7") == 0)
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_8";
	}
	else
	{
		trt__CreateOSDResponse->OSDToken = "osdToken_0";
		NumLine = 0;
		memcpy(szLabelNameBak[0], trt__CreateOSD->OSD->TextString->PlainText,
		       strlen(trt__CreateOSD->OSD->TextString->PlainText) + 1);
		struLabelOrgBak[0][0] = *(trt__CreateOSD->OSD->Position->Pos->x);
		struLabelOrgBak[0][1] = *(trt__CreateOSD->OSD->Position->Pos->y);
	}

	for(i = 0; i <= NumLine; i++)
	{
		memcpy(stOsdCfg.szUsrOsdInfo[i], szLabelNameBak[i],
		       strlen(szLabelNameBak[i]) + 1);
	}

	for(i = HI_OSD_REG_NUM - 1; i > NumLine; i--)
	{
		memset(stOsdCfg.szUsrOsdInfo[i], '\0', 1);//HI_NAME_LEN);
	}

	for(i = 0; i <= NumLine; i++)
	{
		x = struLabelOrgBak[i][0];
		y = struLabelOrgBak[i][1];
		stOsdCfg.struUsrOrg[i].s32X = (HI_S32)((x + 1) * (Video_ShowWight / 2));
		stOsdCfg.struUsrOrg[i].s32X += (HI_S32)(FontW * strlen(szLabelNameBak[i]));
		stOsdCfg.struUsrOrg[i].s32Y = (HI_S32)((y + 1) * (Video_ShowHeight / 2));
		stOsdCfg.struUsrOrg[i].s32Y = (HI_S32)(Video_ShowHeight -
		                                       stOsdCfg.struUsrOrg[i].s32Y);

		if (!stOsdCfg.u8EnableUsrOsd[i])
		{
			stOsdCfg.u8EnableUsrOsd[i] = 1;
		}
	}

	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_OSD_PARAM, 0,
	                     (char*)&stOsdCfg, sizeof(HI_DEV_OSD_CFG_S));
#else
#endif
	//////////////////////////END///////////////////////////
	__FUN_END("\n");
	return SOAP_OK;
}

int  __trt__DeleteOSD(struct soap* soap, struct _trt__DeleteOSD* trt__DeleteOSD,
                      struct _trt__DeleteOSDResponse* trt__DeleteOSDResponse)
{
	HI_DEV_OSD_CFG_S stOsdCfg;
	__FUN_BEGIN("\n");
	CHECK_USER;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_OSD_PARAM, 0,
	                     (char*)&stOsdCfg, sizeof(HI_DEV_OSD_CFG_S));

	if (trt__DeleteOSD == NULL || trt__DeleteOSD->OSDToken == NULL)
	{
		return SOAP_FAULT;
	}

	if (strcmp(trt__DeleteOSD->OSDToken, ONVIF_OSD_DATETIME_TOKEN) == 0)
	{
		stOsdCfg.u8EnableTimeOsd = 0;
	}
	else if (strcmp(trt__DeleteOSD->OSDToken, ONVIF_OSD_CHN_TOKEN) == 0)
	{
		int i;

		for(i = 0; i < HI_OSD_REG_NUM; i++)
		{
			stOsdCfg.u8EnableUsrOsd[i] = 0;
		}
	}

	gOnvifDevCb.pMsgProc(NULL, HI_SET_PARAM_MSG, HI_OSD_PARAM, 0,
	                     (char*)&stOsdCfg, sizeof(HI_DEV_OSD_CFG_S));
	__FUN_END("\n");
	return SOAP_FAULT;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//onvif 2.6 add function


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSources(struct soap* soap,
        struct _trt__GetVideoSources* trt__GetVideoSources,
        struct _trt__GetVideoSourcesResponse* trt__GetVideoSourcesResponse)
{
	int idx;
	HI_ONVIF_VIDEO_SOURCE_S OnvifVideoSource;
	struct tt__VideoSource* VideoSources;
	struct tt__VideoSource* pVideoSources;
	__FUN_BEGIN("\n");
	CHECK_USER;
	trt__GetVideoSourcesResponse->__sizeVideoSources = ONVIF_VIDEO_SOURCE_NUM;
	ALLOC_STRUCT_NUM(pVideoSources, struct tt__VideoSource, ONVIF_VIDEO_SOURCE_NUM);

	for(idx = 0; idx < ONVIF_VIDEO_SOURCE_NUM; idx++)
	{
		HI_DEV_IMA_CFG_S stIma;
		HI_DEV_3A_CFG_S st3a;
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_IMA_PARAM, idx,
		                     (char*)&stIma, sizeof(HI_DEV_IMA_CFG_S));
		gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_3A_PARAM, idx,
		                     (char*)&st3a, sizeof(HI_DEV_3A_CFG_S));
		VideoSources = &pVideoSources[idx];
		onvif_get_video_source(idx, &OnvifVideoSource);
		ALLOC_TOKEN(VideoSources->token, OnvifVideoSource.token);
		VideoSources->Framerate = OnvifVideoSource.Framerate;
		ALLOC_STRUCT(VideoSources->Resolution, struct tt__VideoResolution);
		VideoSources->Resolution->Width = OnvifVideoSource.Resolution.Width;
		VideoSources->Resolution->Height = OnvifVideoSource.Resolution.Height;
		ALLOC_STRUCT(VideoSources->Imaging, struct tt__ImagingSettings);
		ALLOC_STRUCT(VideoSources->Imaging->BacklightCompensation,
		             struct tt__BacklightCompensation);
		VideoSources->Imaging->BacklightCompensation->Mode =
		    tt__BacklightCompensationMode__OFF;
		ALLOC_STRUCT(VideoSources->Imaging->Brightness, float);
		ALLOC_STRUCT(VideoSources->Imaging->ColorSaturation, float);
		ALLOC_STRUCT(VideoSources->Imaging->Contrast, float);
		ALLOC_STRUCT(VideoSources->Imaging->Sharpness, float);
		ALLOC_STRUCT(VideoSources->Imaging->WhiteBalance, struct tt__WhiteBalance);
		ALLOC_STRUCT(VideoSources->Extension, struct tt__VideoSourceExtension);
		ALLOC_STRUCT(VideoSources->Extension->Imaging, struct tt__ImagingSettings20);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->Brightness, float);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->ColorSaturation, float);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->Contrast, float);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->Sharpness, float);
		//ALLOC_STRUCT(VideoSources->Extension->Imaging->WideDynamicRange, struct tt__WideDynamicRange20);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->WhiteBalance,
		             struct tt__WhiteBalance20);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->WhiteBalance->CrGain, float);
		ALLOC_STRUCT(VideoSources->Extension->Imaging->WhiteBalance->CbGain, float);

		if(stIma.eSuppMask & VCT_IMA_BRIGHTNESS)
		{
			*VideoSources->Imaging->Brightness = (float)
			                                     stIma.struParam[IMA_BRIGHTNESS_TYPE].u32Value;
			*VideoSources->Extension->Imaging->Brightness = (float)
			        stIma.struParam[IMA_BRIGHTNESS_TYPE].u32Value;
		}

		if(stIma.eSuppMask & VCT_IMA_CONTRAST)
		{
			*VideoSources->Imaging->Contrast = (float)
			                                   stIma.struParam[IMA_CONTRAST_TYPE].u32Value;
			*VideoSources->Extension->Imaging->Contrast = (float)
			        stIma.struParam[IMA_CONTRAST_TYPE].u32Value;
		}

		if(stIma.eSuppMask & VCT_IMA_SATURATION)
		{
			*VideoSources->Imaging->ColorSaturation  = (float)
			        stIma.struParam[IMA_SATURATION_TYPE].u32Value;
			*VideoSources->Extension->Imaging->ColorSaturation = (float)
			        stIma.struParam[IMA_SATURATION_TYPE].u32Value;
		}

		if(stIma.eSuppMask & VCT_IMA_SHARPNESS)
		{
			*VideoSources->Imaging->Sharpness  = (float)
			                                     stIma.struParam[IMA_SHARPNESS_TYPE].u32Value;
			*VideoSources->Extension->Imaging->Sharpness = (float)
			        stIma.struParam[IMA_SHARPNESS_TYPE].u32Value;
		}

		VideoSources->Imaging->WhiteBalance->Mode = tt__WhiteBalanceMode__AUTO;
		VideoSources->Extension->Imaging->WhiteBalance->Mode =
		    tt__WhiteBalanceMode__AUTO;
#if 0

		if(stIma.eSuppMask & VCT_IMA_RED)
		{
			VideoSources->Imaging->WhiteBalance->CrGain = (float)stIma.struRed.u8Value;
			*VideoSources->Extension->Imaging->WhiteBalance->CrGain =
			    (float)stIma.struRed.u8Value;
		}

		if(stIma.eSuppMask & VCT_IMA_BLUE)
		{
			VideoSources->Imaging->WhiteBalance->CbGain = (float)stIma.struBlue.u8Value;
			*VideoSources->Extension->Imaging->WhiteBalance->CbGain =
			    (float)stIma.struBlue.u8Value;
		}

#endif
	}

	trt__GetVideoSourcesResponse->VideoSources = pVideoSources;
	__FUN_END("\n");
	return SOAP_OK;
	//__FUN_BEGIN("\n");CHECK_USER;
	//__FUN_END("\n");
	//return SOAP_FAULT;
}


/////////////////////////////////////////////////////////////////////////////////////////
/*  新增加的 onvif 2.6 函数   */
SOAP_FMAC5 int SOAP_FMAC6 __tls__GetServiceCapabilities(struct soap* soap,
        struct _tls__GetServiceCapabilities* tls__GetServiceCapabilities,
        struct _tls__GetServiceCapabilitiesResponse* tls__GetServiceCapabilitiesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__GetLayout(struct soap* soap,
        struct _tls__GetLayout* tls__GetLayout, struct _tls__GetLayoutResponse* tls__GetLayoutResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__SetLayout(struct soap* soap,
        struct _tls__SetLayout* tls__SetLayout, struct _tls__SetLayoutResponse* tls__SetLayoutResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__GetDisplayOptions(struct soap* soap,
        struct _tls__GetDisplayOptions* tls__GetDisplayOptions,
        struct _tls__GetDisplayOptionsResponse* tls__GetDisplayOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__GetPaneConfigurations(struct soap* soap,
        struct _tls__GetPaneConfigurations* tls__GetPaneConfigurations,
        struct _tls__GetPaneConfigurationsResponse* tls__GetPaneConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__GetPaneConfiguration(struct soap* soap,
        struct _tls__GetPaneConfiguration* tls__GetPaneConfiguration,
        struct _tls__GetPaneConfigurationResponse* tls__GetPaneConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__SetPaneConfigurations(struct soap* soap,
        struct _tls__SetPaneConfigurations* tls__SetPaneConfigurations,
        struct _tls__SetPaneConfigurationsResponse* tls__SetPaneConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__SetPaneConfiguration(struct soap* soap,
        struct _tls__SetPaneConfiguration* tls__SetPaneConfiguration,
        struct _tls__SetPaneConfigurationResponse* tls__SetPaneConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__CreatePaneConfiguration(struct soap* soap,
        struct _tls__CreatePaneConfiguration* tls__CreatePaneConfiguration,
        struct _tls__CreatePaneConfigurationResponse* tls__CreatePaneConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tls__DeletePaneConfiguration(struct soap* soap,
        struct _tls__DeletePaneConfiguration* tls__DeletePaneConfiguration,
        struct _tls__DeletePaneConfigurationResponse* tls__DeletePaneConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __trc__GetServiceCapabilities(struct soap* soap,
        struct _trc__GetServiceCapabilities* trc__GetServiceCapabilities,
        struct _trc__GetServiceCapabilitiesResponse* trc__GetServiceCapabilitiesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateRecording(struct soap* soap,
        struct _trc__CreateRecording* trc__CreateRecording,
        struct _trc__CreateRecordingResponse* trc__CreateRecordingResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteRecording(struct soap* soap,
        struct _trc__DeleteRecording* trc__DeleteRecording,
        struct _trc__DeleteRecordingResponse* trc__DeleteRecordingResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordings(struct soap* soap,
        struct _trc__GetRecordings* trc__GetRecordings,
        struct _trc__GetRecordingsResponse* trc__GetRecordingsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingConfiguration(struct soap* soap,
        struct _trc__SetRecordingConfiguration* trc__SetRecordingConfiguration,
        struct _trc__SetRecordingConfigurationResponse* trc__SetRecordingConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingConfiguration(struct soap* soap,
        struct _trc__GetRecordingConfiguration* trc__GetRecordingConfiguration,
        struct _trc__GetRecordingConfigurationResponse* trc__GetRecordingConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateTrack(struct soap* soap,
        struct _trc__CreateTrack* trc__CreateTrack,
        struct _trc__CreateTrackResponse* trc__CreateTrackResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteTrack(struct soap* soap,
        struct _trc__DeleteTrack* trc__DeleteTrack,
        struct _trc__DeleteTrackResponse* trc__DeleteTrackResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__GetTrackConfiguration(struct soap* soap,
        struct _trc__GetTrackConfiguration* trc__GetTrackConfiguration,
        struct _trc__GetTrackConfigurationResponse* trc__GetTrackConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__SetTrackConfiguration(struct soap* soap,
        struct _trc__SetTrackConfiguration* trc__SetTrackConfiguration,
        struct _trc__SetTrackConfigurationResponse* trc__SetTrackConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateRecordingJob(struct soap* soap,
        struct _trc__CreateRecordingJob* trc__CreateRecordingJob,
        struct _trc__CreateRecordingJobResponse* trc__CreateRecordingJobResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteRecordingJob(struct soap* soap,
        struct _trc__DeleteRecordingJob* trc__DeleteRecordingJob,
        struct _trc__DeleteRecordingJobResponse* trc__DeleteRecordingJobResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobs(struct soap* soap,
        struct _trc__GetRecordingJobs* trc__GetRecordingJobs,
        struct _trc__GetRecordingJobsResponse* trc__GetRecordingJobsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingJobConfiguration(struct soap* soap,
        struct _trc__SetRecordingJobConfiguration* trc__SetRecordingJobConfiguration,
        struct _trc__SetRecordingJobConfigurationResponse* trc__SetRecordingJobConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobConfiguration(struct soap* soap,
        struct _trc__GetRecordingJobConfiguration* trc__GetRecordingJobConfiguration,
        struct _trc__GetRecordingJobConfigurationResponse* trc__GetRecordingJobConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingJobMode(struct soap* soap,
        struct _trc__SetRecordingJobMode* trc__SetRecordingJobMode,
        struct _trc__SetRecordingJobModeResponse* trc__SetRecordingJobModeResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobState(struct soap* soap,
        struct _trc__GetRecordingJobState* trc__GetRecordingJobState,
        struct _trc__GetRecordingJobStateResponse* trc__GetRecordingJobStateResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}



/** Web service operation '__tr2__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetServiceCapabilities(struct soap* soap,
        struct _tr2__GetServiceCapabilities* tr2__GetServiceCapabilities,
        struct _tr2__GetServiceCapabilitiesResponse* tr2__GetServiceCapabilitiesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__CreateProfile' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__CreateProfile(struct soap* soap,
        struct _tr2__CreateProfile* tr2__CreateProfile,
        struct _tr2__CreateProfileResponse* tr2__CreateProfileResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetProfiles' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetProfiles(struct soap* soap,
        struct _tr2__GetProfiles* tr2__GetProfiles,
        struct _tr2__GetProfilesResponse* tr2__GetProfilesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__AddConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__AddConfiguration(struct soap* soap,
        struct _tr2__AddConfiguration* tr2__AddConfiguration,
        struct _tr2__AddConfigurationResponse* tr2__AddConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__RemoveConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__RemoveConfiguration(struct soap* soap,
        struct _tr2__RemoveConfiguration* tr2__RemoveConfiguration,
        struct _tr2__RemoveConfigurationResponse* tr2__RemoveConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__DeleteProfile' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__DeleteProfile(struct soap* soap,
        struct _tr2__DeleteProfile* tr2__DeleteProfile,
        struct _tr2__DeleteProfileResponse* tr2__DeleteProfileResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetVideoSourceConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetVideoSourceConfigurations(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetVideoSourceConfigurations,
        struct _tr2__GetVideoSourceConfigurationsResponse* tr2__GetVideoSourceConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetVideoEncoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetVideoEncoderConfigurations(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetVideoEncoderConfigurations,
        struct _tr2__GetVideoEncoderConfigurationsResponse* tr2__GetVideoEncoderConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAudioSourceConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAudioSourceConfigurations(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAudioSourceConfigurations,
        struct _tr2__GetAudioSourceConfigurationsResponse* tr2__GetAudioSourceConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAudioEncoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAudioEncoderConfigurations(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAudioEncoderConfigurations,
        struct _tr2__GetAudioEncoderConfigurationsResponse* tr2__GetAudioEncoderConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAnalyticsConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAnalyticsConfigurations(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAnalyticsConfigurations,
        struct _tr2__GetAnalyticsConfigurationsResponse* tr2__GetAnalyticsConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetMetadataConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetMetadataConfigurations(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetMetadataConfigurations,
        struct _tr2__GetMetadataConfigurationsResponse* tr2__GetMetadataConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAudioOutputConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAudioOutputConfigurations(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAudioOutputConfigurations,
        struct _tr2__GetAudioOutputConfigurationsResponse* tr2__GetAudioOutputConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAudioDecoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAudioDecoderConfigurations(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAudioDecoderConfigurations,
        struct _tr2__GetAudioDecoderConfigurationsResponse* tr2__GetAudioDecoderConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetVideoSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetVideoSourceConfiguration(struct soap* soap,
        struct _tr2__SetVideoSourceConfiguration* tr2__SetVideoSourceConfiguration,
        struct tr2__SetConfigurationResponse* tr2__SetVideoSourceConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetVideoEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetVideoEncoderConfiguration(struct soap* soap,
        struct _tr2__SetVideoEncoderConfiguration* tr2__SetVideoEncoderConfiguration,
        struct tr2__SetConfigurationResponse* tr2__SetVideoEncoderConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetAudioSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetAudioSourceConfiguration(struct soap* soap,
        struct _tr2__SetAudioSourceConfiguration* tr2__SetAudioSourceConfiguration,
        struct tr2__SetConfigurationResponse* tr2__SetAudioSourceConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetAudioEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetAudioEncoderConfiguration(struct soap* soap,
        struct _tr2__SetAudioEncoderConfiguration* tr2__SetAudioEncoderConfiguration,
        struct tr2__SetConfigurationResponse* tr2__SetAudioEncoderConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetMetadataConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetMetadataConfiguration(struct soap* soap,
        struct _tr2__SetMetadataConfiguration* tr2__SetMetadataConfiguration,
        struct tr2__SetConfigurationResponse* tr2__SetMetadataConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetAudioOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetAudioOutputConfiguration(struct soap* soap,
        struct _tr2__SetAudioOutputConfiguration* tr2__SetAudioOutputConfiguration,
        struct tr2__SetConfigurationResponse* tr2__SetAudioOutputConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetAudioDecoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetAudioDecoderConfiguration(struct soap* soap,
        struct _tr2__SetAudioDecoderConfiguration* tr2__SetAudioDecoderConfiguration,
        struct tr2__SetConfigurationResponse* tr2__SetAudioDecoderConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetVideoSourceConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetVideoSourceConfigurationOptions(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetVideoSourceConfigurationOptions,
        struct _tr2__GetVideoSourceConfigurationOptionsResponse*
        tr2__GetVideoSourceConfigurationOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetVideoEncoderConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetVideoEncoderConfigurationOptions(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetVideoEncoderConfigurationOptions,
        struct _tr2__GetVideoEncoderConfigurationOptionsResponse*
        tr2__GetVideoEncoderConfigurationOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAudioSourceConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAudioSourceConfigurationOptions(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAudioSourceConfigurationOptions,
        struct _tr2__GetAudioSourceConfigurationOptionsResponse*
        tr2__GetAudioSourceConfigurationOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAudioEncoderConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAudioEncoderConfigurationOptions(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAudioEncoderConfigurationOptions,
        struct _tr2__GetAudioEncoderConfigurationOptionsResponse*
        tr2__GetAudioEncoderConfigurationOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetMetadataConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetMetadataConfigurationOptions(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetMetadataConfigurationOptions,
        struct _tr2__GetMetadataConfigurationOptionsResponse* tr2__GetMetadataConfigurationOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAudioOutputConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAudioOutputConfigurationOptions(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAudioOutputConfigurationOptions,
        struct _tr2__GetAudioOutputConfigurationOptionsResponse*
        tr2__GetAudioOutputConfigurationOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetAudioDecoderConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetAudioDecoderConfigurationOptions(struct soap* soap,
        struct tr2__GetConfiguration* tr2__GetAudioDecoderConfigurationOptions,
        struct _tr2__GetAudioDecoderConfigurationOptionsResponse*
        tr2__GetAudioDecoderConfigurationOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetVideoEncoderInstances' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetVideoEncoderInstances(struct soap* soap,
        struct _tr2__GetVideoEncoderInstances* tr2__GetVideoEncoderInstances,
        struct _tr2__GetVideoEncoderInstancesResponse* tr2__GetVideoEncoderInstancesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetStreamUri' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetStreamUri(struct soap* soap,
        struct _tr2__GetStreamUri* tr2__GetStreamUri,
        struct _tr2__GetStreamUriResponse* tr2__GetStreamUriResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__StartMulticastStreaming' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__StartMulticastStreaming(struct soap* soap,
        struct tr2__StartStopMulticastStreaming* tr2__StartMulticastStreaming,
        struct tr2__SetConfigurationResponse* tr2__StartMulticastStreamingResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__StopMulticastStreaming' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__StopMulticastStreaming(struct soap* soap,
        struct tr2__StartStopMulticastStreaming* tr2__StopMulticastStreaming,
        struct tr2__SetConfigurationResponse* tr2__StopMulticastStreamingResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetSynchronizationPoint' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetSynchronizationPoint(struct soap* soap,
        struct _tr2__SetSynchronizationPoint* tr2__SetSynchronizationPoint,
        struct _tr2__SetSynchronizationPointResponse* tr2__SetSynchronizationPointResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetSnapshotUri' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetSnapshotUri(struct soap* soap,
        struct _tr2__GetSnapshotUri* tr2__GetSnapshotUri,
        struct _tr2__GetSnapshotUriResponse* tr2__GetSnapshotUriResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetVideoSourceModes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetVideoSourceModes(struct soap* soap,
        struct _tr2__GetVideoSourceModes* tr2__GetVideoSourceModes,
        struct _tr2__GetVideoSourceModesResponse* tr2__GetVideoSourceModesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetVideoSourceMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetVideoSourceMode(struct soap* soap,
        struct _tr2__SetVideoSourceMode* tr2__SetVideoSourceMode,
        struct _tr2__SetVideoSourceModeResponse* tr2__SetVideoSourceModeResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetOSDs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetOSDs(struct soap* soap,  struct _tr2__GetOSDs* tr2__GetOSDs,
        struct _tr2__GetOSDsResponse* tr2__GetOSDsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetOSDOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetOSDOptions(struct soap* soap,
        struct _tr2__GetOSDOptions* tr2__GetOSDOptions,
        struct _tr2__GetOSDOptionsResponse* tr2__GetOSDOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetOSD' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetOSD(struct soap* soap,	struct _tr2__SetOSD* tr2__SetOSD,
                                        struct tr2__SetConfigurationResponse* tr2__SetOSDResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__CreateOSD' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__CreateOSD(struct soap* soap,
        struct _tr2__CreateOSD* tr2__CreateOSD, struct _tr2__CreateOSDResponse* tr2__CreateOSDResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__DeleteOSD' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__DeleteOSD(struct soap* soap,
        struct _tr2__DeleteOSD* tr2__DeleteOSD,
        struct tr2__SetConfigurationResponse* tr2__DeleteOSDResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetMasks' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetMasks(struct soap* soap,  struct _tr2__GetMasks* tr2__GetMasks,
        struct _tr2__GetMasksResponse* tr2__GetMasksResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__GetMaskOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__GetMaskOptions(struct soap* soap,
        struct _tr2__GetMaskOptions* tr2__GetMaskOptions,
        struct _tr2__GetMaskOptionsResponse* tr2__GetMaskOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__SetMask' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__SetMask(struct soap* soap,  struct _tr2__SetMask* tr2__SetMask,
        struct tr2__SetConfigurationResponse* tr2__SetMaskResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__CreateMask' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__CreateMask(struct soap* soap,
        struct _tr2__CreateMask* tr2__CreateMask, struct _tr2__CreateMaskResponse* tr2__CreateMaskResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tr2__DeleteMask' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tr2__DeleteMask(struct soap* soap,
        struct _tr2__DeleteMask* tr2__DeleteMask,
        struct tr2__SetConfigurationResponse* tr2__DeleteMaskResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}




/** Web service operation '__tev__Unsubscribe' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__Unsubscribe__(struct soap* soap,
        struct _wsnt__Unsubscribe* wsnt__Unsubscribe,
        struct _wsnt__UnsubscribeResponse* wsnt__UnsubscribeResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}



SOAP_FMAC5 int SOAP_FMAC6 __tds__GetStorageConfigurations(struct soap* soap,
        struct _tds__GetStorageConfigurations* tds__GetStorageConfigurations,
        struct _tds__GetStorageConfigurationsResponse* tds__GetStorageConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tds__CreateStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateStorageConfiguration(struct soap* soap,
        struct _tds__CreateStorageConfiguration* tds__CreateStorageConfiguration,
        struct _tds__CreateStorageConfigurationResponse* tds__CreateStorageConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tds__GetStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetStorageConfiguration(struct soap* soap,
        struct _tds__GetStorageConfiguration* tds__GetStorageConfiguration,
        struct _tds__GetStorageConfigurationResponse* tds__GetStorageConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tds__SetStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetStorageConfiguration(struct soap* soap,
        struct _tds__SetStorageConfiguration* tds__SetStorageConfiguration,
        struct _tds__SetStorageConfigurationResponse* tds__SetStorageConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tds__DeleteStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteStorageConfiguration(struct soap* soap,
        struct _tds__DeleteStorageConfiguration* tds__DeleteStorageConfiguration,
        struct _tds__DeleteStorageConfigurationResponse* tds__DeleteStorageConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tds__GetGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetGeoLocation(struct soap* soap,
        struct _tds__GetGeoLocation* tds__GetGeoLocation,
        struct _tds__GetGeoLocationResponse* tds__GetGeoLocationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tds__SetGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetGeoLocation(struct soap* soap,
        struct _tds__SetGeoLocation* tds__SetGeoLocation,
        struct _tds__SetGeoLocationResponse* tds__SetGeoLocationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
/** Web service operation '__tds__DeleteGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteGeoLocation(struct soap* soap,
        struct _tds__DeleteGeoLocation* tds__DeleteGeoLocation,
        struct _tds__DeleteGeoLocationResponse* tds__DeleteGeoLocationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}








SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingOptions(struct soap* soap,
        struct _trc__GetRecordingOptions* trc__GetRecordingOptions,
        struct _trc__GetRecordingOptionsResponse* trc__GetRecordingOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __trc__ExportRecordedData(struct soap* soap,
        struct _trc__ExportRecordedData* trc__ExportRecordedData,
        struct _trc__ExportRecordedDataResponse* trc__ExportRecordedDataResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __trc__StopExportRecordedData(struct soap* soap,
        struct _trc__StopExportRecordedData* trc__StopExportRecordedData,
        struct _trc__StopExportRecordedDataResponse* trc__StopExportRecordedDataResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetExportRecordedDataState(struct soap* soap,
        struct _trc__GetExportRecordedDataState* trc__GetExportRecordedDataState,
        struct _trc__GetExportRecordedDataStateResponse* trc__GetExportRecordedDataStateResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __tanae__GetAnalyticsModuleOptions(struct soap* soap,
        struct _tan__GetAnalyticsModuleOptions* tan__GetAnalyticsModuleOptions,
        struct _tan__GetAnalyticsModuleOptionsResponse* tan__GetAnalyticsModuleOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __tanre__GetRuleOptions(struct soap* soap,
        struct _tan__GetRuleOptions* tan__GetRuleOptions,
        struct _tan__GetRuleOptionsResponse* tan__GetRuleOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __tetpps__Seek(struct soap* soap, struct _tev__Seek* tev__Seek,
        struct _tev__SeekResponse* tev__SeekResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __tetpps__Unsubscribe(struct soap* soap,
        struct _wsnt__Unsubscribe* wsnt__Unsubscribe,
        struct _wsnt__UnsubscribeResponse* wsnt__UnsubscribeResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}



SOAP_FMAC5 int SOAP_FMAC6 __tad__GetServiceCapabilities(struct soap* soap,
        struct _tad__GetServiceCapabilities* tad__GetServiceCapabilities,
        struct _tad__GetServiceCapabilitiesResponse* tad__GetServiceCapabilitiesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__DeleteAnalyticsEngineControl(struct soap* soap,
        struct _tad__DeleteAnalyticsEngineControl* tad__DeleteAnalyticsEngineControl,
        struct _tad__DeleteAnalyticsEngineControlResponse* tad__DeleteAnalyticsEngineControlResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__CreateAnalyticsEngineControl(struct soap* soap,
        struct _tad__CreateAnalyticsEngineControl* tad__CreateAnalyticsEngineControl,
        struct _tad__CreateAnalyticsEngineControlResponse* tad__CreateAnalyticsEngineControlResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__SetAnalyticsEngineControl(struct soap* soap,
        struct _tad__SetAnalyticsEngineControl* tad__SetAnalyticsEngineControl,
        struct _tad__SetAnalyticsEngineControlResponse* tad__SetAnalyticsEngineControlResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetAnalyticsEngineControl(struct soap* soap,
        struct _tad__GetAnalyticsEngineControl* tad__GetAnalyticsEngineControl,
        struct _tad__GetAnalyticsEngineControlResponse* tad__GetAnalyticsEngineControlResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetAnalyticsEngineControls(struct soap* soap,
        struct _tad__GetAnalyticsEngineControls* tad__GetAnalyticsEngineControls,
        struct _tad__GetAnalyticsEngineControlsResponse* tad__GetAnalyticsEngineControlsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetAnalyticsEngine(struct soap* soap,
        struct _tad__GetAnalyticsEngine* tad__GetAnalyticsEngine,
        struct _tad__GetAnalyticsEngineResponse* tad__GetAnalyticsEngineResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetAnalyticsEngines(struct soap* soap,
        struct _tad__GetAnalyticsEngines* tad__GetAnalyticsEngines,
        struct _tad__GetAnalyticsEnginesResponse* tad__GetAnalyticsEnginesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__SetVideoAnalyticsConfiguration(struct soap* soap,
        struct _tad__SetVideoAnalyticsConfiguration* tad__SetVideoAnalyticsConfiguration,
        struct _tad__SetVideoAnalyticsConfigurationResponse* tad__SetVideoAnalyticsConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__SetAnalyticsEngineInput(struct soap* soap,
        struct _tad__SetAnalyticsEngineInput* tad__SetAnalyticsEngineInput,
        struct _tad__SetAnalyticsEngineInputResponse* tad__SetAnalyticsEngineInputResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetAnalyticsEngineInput(struct soap* soap,
        struct _tad__GetAnalyticsEngineInput* tad__GetAnalyticsEngineInput,
        struct _tad__GetAnalyticsEngineInputResponse* tad__GetAnalyticsEngineInputResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetAnalyticsEngineInputs(struct soap* soap,
        struct _tad__GetAnalyticsEngineInputs* tad__GetAnalyticsEngineInputs,
        struct _tad__GetAnalyticsEngineInputsResponse* tad__GetAnalyticsEngineInputsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetAnalyticsDeviceStreamUri(struct soap* soap,
        struct _tad__GetAnalyticsDeviceStreamUri* tad__GetAnalyticsDeviceStreamUri,
        struct _tad__GetAnalyticsDeviceStreamUriResponse* tad__GetAnalyticsDeviceStreamUriResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetVideoAnalyticsConfiguration(struct soap* soap,
        struct _tad__GetVideoAnalyticsConfiguration* tad__GetVideoAnalyticsConfiguration,
        struct _tad__GetVideoAnalyticsConfigurationResponse* tad__GetVideoAnalyticsConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__CreateAnalyticsEngineInputs(struct soap* soap,
        struct _tad__CreateAnalyticsEngineInputs* tad__CreateAnalyticsEngineInputs,
        struct _tad__CreateAnalyticsEngineInputsResponse* tad__CreateAnalyticsEngineInputsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__DeleteAnalyticsEngineInputs(struct soap* soap,
        struct _tad__DeleteAnalyticsEngineInputs* tad__DeleteAnalyticsEngineInputs,
        struct _tad__DeleteAnalyticsEngineInputsResponse* tad__DeleteAnalyticsEngineInputsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tad__GetAnalyticsState(struct soap* soap,
        struct _tad__GetAnalyticsState* tad__GetAnalyticsState,
        struct _tad__GetAnalyticsStateResponse* tad__GetAnalyticsStateResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __timg__GetPresets(struct soap* soap,
        struct _timg__GetPresets* timg__GetPresets,
        struct _timg__GetPresetsResponse* timg__GetPresetsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __timg__GetCurrentPreset(struct soap* soap,
        struct _timg__GetCurrentPreset* timg__GetCurrentPreset,
        struct _timg__GetCurrentPresetResponse* timg__GetCurrentPresetResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __timg__SetCurrentPreset(struct soap* soap,
        struct _timg__SetCurrentPreset* timg__SetCurrentPreset,
        struct _timg__SetCurrentPresetResponse* timg__SetCurrentPresetResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __tse__GetServiceCapabilities(struct soap* soap,
        struct _tse__GetServiceCapabilities* tse__GetServiceCapabilities,
        struct _tse__GetServiceCapabilitiesResponse* tse__GetServiceCapabilitiesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__GetRecordingSummary(struct soap* soap,
        struct _tse__GetRecordingSummary* tse__GetRecordingSummary,
        struct _tse__GetRecordingSummaryResponse* tse__GetRecordingSummaryResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__GetRecordingInformation(struct soap* soap,
        struct _tse__GetRecordingInformation* tse__GetRecordingInformation,
        struct _tse__GetRecordingInformationResponse* tse__GetRecordingInformationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__GetMediaAttributes(struct soap* soap,
        struct _tse__GetMediaAttributes* tse__GetMediaAttributes,
        struct _tse__GetMediaAttributesResponse* tse__GetMediaAttributesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__FindRecordings(struct soap* soap,
        struct _tse__FindRecordings* tse__FindRecordings,
        struct _tse__FindRecordingsResponse* tse__FindRecordingsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__GetRecordingSearchResults(struct soap* soap,
        struct _tse__GetRecordingSearchResults* tse__GetRecordingSearchResults,
        struct _tse__GetRecordingSearchResultsResponse* tse__GetRecordingSearchResultsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__FindEvents(struct soap* soap,
        struct _tse__FindEvents* tse__FindEvents, struct _tse__FindEventsResponse* tse__FindEventsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__GetEventSearchResults(struct soap* soap,
        struct _tse__GetEventSearchResults* tse__GetEventSearchResults,
        struct _tse__GetEventSearchResultsResponse* tse__GetEventSearchResultsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__FindPTZPosition(struct soap* soap,
        struct _tse__FindPTZPosition* tse__FindPTZPosition,
        struct _tse__FindPTZPositionResponse* tse__FindPTZPositionResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__GetPTZPositionSearchResults(struct soap* soap,
        struct _tse__GetPTZPositionSearchResults* tse__GetPTZPositionSearchResults,
        struct _tse__GetPTZPositionSearchResultsResponse* tse__GetPTZPositionSearchResultsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__GetSearchState(struct soap* soap,
        struct _tse__GetSearchState* tse__GetSearchState,
        struct _tse__GetSearchStateResponse* tse__GetSearchStateResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__EndSearch(struct soap* soap,
        struct _tse__EndSearch* tse__EndSearch, struct _tse__EndSearchResponse* tse__EndSearchResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__FindMetadata(struct soap* soap,
        struct _tse__FindMetadata* tse__FindMetadata,
        struct _tse__FindMetadataResponse* tse__FindMetadataResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tse__GetMetadataSearchResults(struct soap* soap,
        struct _tse__GetMetadataSearchResults* tse__GetMetadataSearchResults,
        struct _tse__GetMetadataSearchResultsResponse* tse__GetMetadataSearchResultsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trv__GetServiceCapabilities(struct soap* soap,
        struct _trv__GetServiceCapabilities* trv__GetServiceCapabilities,
        struct _trv__GetServiceCapabilitiesResponse* trv__GetServiceCapabilitiesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trv__GetReceivers(struct soap* soap,
        struct _trv__GetReceivers* trv__GetReceivers,
        struct _trv__GetReceiversResponse* trv__GetReceiversResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trv__GetReceiver(struct soap* soap,
        struct _trv__GetReceiver* trv__GetReceiver,
        struct _trv__GetReceiverResponse* trv__GetReceiverResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trv__CreateReceiver(struct soap* soap,
        struct _trv__CreateReceiver* trv__CreateReceiver,
        struct _trv__CreateReceiverResponse* trv__CreateReceiverResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trv__DeleteReceiver(struct soap* soap,
        struct _trv__DeleteReceiver* trv__DeleteReceiver,
        struct _trv__DeleteReceiverResponse* trv__DeleteReceiverResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trv__ConfigureReceiver(struct soap* soap,
        struct _trv__ConfigureReceiver* trv__ConfigureReceiver,
        struct _trv__ConfigureReceiverResponse* trv__ConfigureReceiverResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trv__SetReceiverMode(struct soap* soap,
        struct _trv__SetReceiverMode* trv__SetReceiverMode,
        struct _trv__SetReceiverModeResponse* trv__SetReceiverModeResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trv__GetReceiverState(struct soap* soap,
        struct _trv__GetReceiverState* trv__GetReceiverState,
        struct _trv__GetReceiverStateResponse* trv__GetReceiverStateResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __trp__GetServiceCapabilities(struct soap* soap,
        struct _trp__GetServiceCapabilities* trp__GetServiceCapabilities,
        struct _trp__GetServiceCapabilitiesResponse* trp__GetServiceCapabilitiesResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trp__GetReplayUri(struct soap* soap,
        struct _trp__GetReplayUri* trp__GetReplayUri,
        struct _trp__GetReplayUriResponse* trp__GetReplayUriResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trp__GetReplayConfiguration(struct soap* soap,
        struct _trp__GetReplayConfiguration* trp__GetReplayConfiguration,
        struct _trp__GetReplayConfigurationResponse* trp__GetReplayConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trp__SetReplayConfiguration(struct soap* soap,
        struct _trp__SetReplayConfiguration* trp__SetReplayConfiguration,
        struct _trp__SetReplayConfigurationResponse* trp__SetReplayConfigurationResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __tan__GetRuleOptions(struct soap* soap,
        struct _tan__GetRuleOptions* tan__GetRuleOptions,
        struct _tan__GetRuleOptionsResponse* tan__GetRuleOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tan__GetAnalyticsModuleOptions(struct soap* soap,
        struct _tan__GetAnalyticsModuleOptions* tan__GetAnalyticsModuleOptions,
        struct _tan__GetAnalyticsModuleOptionsResponse* tan__GetAnalyticsModuleOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __tptz__GeoMove(struct soap* soap, struct _tptz__GeoMove* tptz__GeoMove,
        struct _tptz__GeoMoveResponse* tptz__GeoMoveResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetRelayOutputOptions(struct soap* soap,
        struct _tmd__GetRelayOutputOptions* tmd__GetRelayOutputOptions,
        struct _tmd__GetRelayOutputOptionsResponse* tmd__GetRelayOutputOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetDigitalInputConfigurationOptions(struct soap* soap,
        struct _tmd__GetDigitalInputConfigurationOptions* tmd__GetDigitalInputConfigurationOptions,
        struct _tmd__GetDigitalInputConfigurationOptionsResponse*
        tmd__GetDigitalInputConfigurationOptionsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetDigitalInputConfigurations(struct soap* soap,
        struct _tmd__SetDigitalInputConfigurations* tmd__SetDigitalInputConfigurations,
        struct _tmd__SetDigitalInputConfigurationsResponse* tmd__SetDigitalInputConfigurationsResponse)
{
	TRACE("%s in \n", __FUNCTION__);
	CHECK_USER;
	TRACE("%s out \n", __FUNCTION__);
	return 0;
}





////////////////////////////////////////////////////////////////////////////////////////



//#endif
