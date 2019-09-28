//#include "hi_platform.h"

//#if (HI_SUPPORT_PLATFORM && PLATFORM_ONVIF)

#include "hi_struct.h"
#include "hi_param_interface.h"
#include "stdsoap2.h"
#include "soapH.h"
//#include "hi_net_wifi.h"
//#include "hi_param_proc.h"

#include "hi_onvif.h"
#include "hi_platform_onvif.h"
//#include "hi_platform_proc.h"
#include "hi_onvif_server.h"
#include "hi_onvif_proc.h"
#include "soapStub.h"
#include "hi_net_api.h"

#include   <net/if_arp.h>
#include   <arpa/inet.h>
#include   <errno.h>
#include   <net/if.h>
#include "DeviceBinding.nsmap"

#ifdef USE_WSSE_AUTH
	#include "wsseapi.h"
	#include "wsaapi.h"
#endif

#ifdef USE_DIGEST_AUTH
	#include "httpda.h"
#endif



PF_CB_S gOnvifDevCb = {0};

struct soap gIntSoap;
int onvif_intr_thr_flag;
pthread_mutex_t g_inter_mutex;

#define NUM_BROADCAST (4)        //number of times hello is broadcasted
#define NUM_TIMEOUT (5)
typedef struct _HI_ONVIF_MNG_S_
{
	int quit;
	int init;

	pthread_t helloThread;
	pthread_t probeThread;
	pthread_t serverThread;
	pthread_t helloWifiThread;
	pthread_t probeWifiThread;
	pthread_t onviftourThread;//20140619 zj
	pthread_t interacteThread[MAX_SUPP_INTER_THRD];//
	pthread_mutex_t mutex;
} HI_ONVIF_MNG_S;

static HI_ONVIF_MNG_S gOnvifMng;;

#include  <net/if.h>
#include  <linux/sockios.h>
#include  <linux/ethtool.h>
static int hi_onvif_get_netstatus()
{
	//unsigned int ETHTOOL_GLINK = 0x0000000a;/* Get link status (ethtool_value) */
	//#ifndef ETHTOOL_GLINK
	//#define ETHTOOL_GLINK	0x0000000a
	//#endif
	struct ethtool_value
	{
		unsigned int   cmd;
		unsigned int   data;
	};
	int skfd;
	struct ifreq ifr;
	struct ethtool_value edata;
	edata.cmd = ETHTOOL_GLINK;
	edata.data = 0;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name) - 1);
	ifr.ifr_data = (char*)&edata;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
	{
		return -1;
	}

	if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1)
	{
		close(skfd);
		return -1;
	}

	close(skfd);
	return edata.data;
	return 0;
}

static void* hi_onvif_hello_check(void* data)
{
	int StatusFlag = 0;
	int ChangFlag = 0;
	pthread_detach(pthread_self());
	ChangFlag = hi_onvif_get_netstatus();

	while(!gOnvifMng.quit)
	{
		StatusFlag = hi_onvif_get_netstatus();

		if(ChangFlag != StatusFlag)
		{
			ChangFlag = StatusFlag;

			if(StatusFlag)
			{
				hi_onvif_hello(HI_CARD_ETH0);
			}
		}

		usleep(500000);
	}

	return NULL;
}

#if 0 //ONVIF_OK
static void* hi_onvif_wifi_hello_check(void* data)
{
	int StatusFlag = 0;
	int ChangFlag = 0;
	pthread_detach(pthread_self());
	ChangFlag = hi_onvif_get_netstatus();

	while(!gOnvifMng.quit)
	{
		StatusFlag = hi_wifi_get_iw_netstat();

		if(ChangFlag != StatusFlag)
		{
			ChangFlag = StatusFlag;

			if(StatusFlag)
			{
				if(hi_wifi_get_flag() == 2)
				{
					hi_onvif_hello(HI_CARD_RA0);
				}
			}
		}

		usleep(500000);
	}

	return NULL;
}
#endif

static unsigned long hi_get_ip_by_interface(char* pInterfaceName)
{
	int sockfd;
	struct sockaddr_in stAddr;
	struct ifreq ifr;

	if((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		return -1;
	}

	strncpy(ifr.ifr_name, pInterfaceName, IF_NAMESIZE);
	ifr.ifr_name[IF_NAMESIZE - 1] = '\0';

	if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
	{
		return -1;
	}

	close(sockfd);
	memcpy(&stAddr, &ifr.ifr_addr, sizeof(stAddr));
	return stAddr.sin_addr.s_addr;
}

static void* hi_onvif_probe(void* data)
{
	int nSocket = 0;
	unsigned int accept_count = 0;
	struct soap* soap_udp;
	struct sockaddr_in servaddr;
	struct ip_mreq mip;
	pthread_detach(pthread_self());
	soap_udp = soap_new();
	soap_init1(soap_udp, SOAP_IO_UDP);

	if (!soap_valid_socket((nSocket = soap_bind(soap_udp, NULL,
	                                  DEFAULT_ONVIF_WSD_PORT, 100))))
	{
		soap_print_fault(soap_udp, stderr);
		__E("soap_bind err\r\n");
		soap_destroy (soap_udp);
		soap_end(soap_udp);
		soap_free(soap_udp);
		return NULL;
	}

	/* Enable Multicast Reception */
	bzero (&servaddr, sizeof (servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton (AF_INET, ONVIF_WSD_MC_ADDR, &servaddr.sin_addr);
	servaddr.sin_port = htons (DEFAULT_ONVIF_WSD_PORT);
	mip.imr_multiaddr = servaddr.sin_addr;
	mip.imr_interface.s_addr = hi_get_ip_by_interface("eth0");
	setsockopt (nSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mip, sizeof(mip));
	hi_sock_bind_interface(nSocket, "eth0");
	hi_set_sock_multicast_noloop(nSocket);

	while(!gOnvifMng.quit)
	{
		if( accept_count % 30 == 0 )
		{
			printf("soap_accept %d\n", accept_count);
		}

		accept_count++;

		if(soap_accept(soap_udp) < 0)
		{
			soap_print_fault(soap_udp, stderr);
			__E("soap_accept err\r\n");
			soap_destroy (soap_udp);
			soap_end(soap_udp);
			break;
		}

		stOnvifSocket.eth0Socket = soap_udp->master;
		soap_serve(soap_udp);
		soap_destroy (soap_udp);
		soap_end(soap_udp);
		usleep(500000);
	}

	soap_done(soap_udp);
	soap_free(soap_udp);
	return NULL;
}

#if 0 //ONVIF_OK
static void* hi_onvif_wifi_probe(void* data)
{
	int nSocket = 0;
	struct soap* soap_udp;
	struct sockaddr_in servaddr;
	struct ip_mreq mip;
	pthread_detach(pthread_self());

	while(!gOnvifMng.quit)
	{
		if(hi_wifi_get_iw_netstat() != 1)
		{
			sleep(2);
			continue;
		}

		soap_udp = soap_new();
		soap_init1(soap_udp, SOAP_IO_UDP);

		if (!soap_valid_socket((nSocket = soap_bind(soap_udp, NULL,
		                                  DEFAULT_ONVIF_WSD_PORT, 5000))))
		{
			soap_print_fault(soap_udp, stderr);
			__E("soap_bind err\r\n");
			soap_destroy (soap_udp);
			soap_end(soap_udp);
			soap_free(soap_udp);
			sleep(3);
			continue;
		}

		bzero (&servaddr, sizeof (servaddr));
		servaddr.sin_family = AF_INET;
		inet_pton (AF_INET, ONVIF_WSD_MC_ADDR, &servaddr.sin_addr);
		servaddr.sin_port = htons (DEFAULT_ONVIF_WSD_PORT);
		mip.imr_multiaddr = servaddr.sin_addr;
		mip.imr_interface.s_addr = hi_get_ip_by_interface("ra0");

		if( mip.imr_interface.s_addr < 0)
		{
			soap_destroy (soap_udp);
			soap_end(soap_udp);
			soap_free(soap_udp);
			continue;
		}

		setsockopt (nSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mip, sizeof(mip));
		/* Enable Multicast Reception */
		hi_set_sock_multicast_noloop(nSocket);
		hi_sock_bind_interface(nSocket, "ra0");

		while(!gOnvifMng.quit)
		{
			if(hi_wifi_get_iw_netstat() != 1)
			{
				soap_destroy (soap_udp);
				soap_end(soap_udp);
				break;
			}

			if(soap_accept(soap_udp) < 0)
			{
				soap_print_fault(soap_udp, stderr);
				__E("soap_accept err\r\n");
				soap_destroy (soap_udp);
				soap_end(soap_udp);
				break;
			}

			stOnvifSocket.wifiSocket = soap_udp->master;
			soap_serve(soap_udp, NULL);
			soap_destroy (soap_udp);
			soap_end(soap_udp);
		}

		soap_done(soap_udp);
		soap_free(soap_udp);
	}

	return NULL;
}
#endif
#if 0
static void* hi_onvif_server(void* data)
{
	SOAP_SOCKET sock;
	struct soap* psoap;
	pthread_detach(pthread_self());
	prctl(PR_SET_NAME, "ONVIFServer");
	psoap = soap_new();
	soap_init (psoap);
	sock = soap_bind (psoap, NULL, /*DEFAULT_ONVIF_SERVER_PORT*/8000, 5000);

	if(!soap_valid_socket(sock))
	{
		__E(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>create onvif server sock err\r\n");
		soap_print_fault(psoap, stderr);
		soap_destroy (psoap);
		soap_end (psoap);
		soap_done(psoap);
		soap_free(psoap);
		return (void*)0;
	}

	/* Run Server Loop */
	while (!gOnvifMng.quit)
	{
		/* Wait for Request */
		sock = soap_accept (psoap);

		if (!soap_valid_socket(sock))
		{
			/* TODO: Log Error! */
			/* soap_print_fault (&soap, stderr); */
			printf("Failed to accept onvif server socket.\n");
			soap_print_fault(psoap, stderr);
			soap_destroy (psoap);
			soap_end (psoap);
			break;
		}

		/* Serve Request */
		soap_serve(psoap, NULL);
		soap_destroy (psoap);
		soap_end (psoap); /* clear soap */
	}

	/* End */
	soap_done(psoap);
	soap_free(psoap);
	printf("out onvif server\r\n");
	return (void*)0;
}
#endif

void hi_onvif_hello(int nType)
{
	unsigned int nIpAddr = 0;
	struct soap* soap = NULL;
	char macaddr[MACH_ADDR_LENGTH];
	char* str;
	time_t time_n;
	struct tm* tm_t;
	HI_DEV_NET_CFG_S stNetCfg;
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0,
	                     (char*)&stNetCfg, sizeof(HI_DEV_NET_CFG_S));
	soap = soap_new();

	if(soap == NULL)
	{
		printf("[%d]soap = NULL\n", __LINE__);
		return;
	}

	soap_set_namespaces( soap, namespaces);

	if(nType)
	{
		nIpAddr = stNetCfg.struEtherCfg[1].u32IpAddr;
		memcpy(macaddr, stNetCfg.struEtherCfg[1].u8MacAddr, 6);
	}
	else
	{
		nIpAddr = stNetCfg.struEtherCfg[0].u32IpAddr;
		memcpy(macaddr, stNetCfg.struEtherCfg[0].u8MacAddr, 6);
	}

	time_n = time(NULL);
	tm_t = localtime(&time_n);
	struct SOAP_ENV__Header* header;
	header = soap_malloc(soap, sizeof(struct SOAP_ENV__Header));
	soap_default_SOAP_ENV__Header(soap,
	                              header); 					 /*setting default values for header*/
	header->wsa__MessageID = soap_malloc(soap, sizeof(char) * LARGE_INFO_LENGTH);
	header->wsa__To = soap_malloc(soap, sizeof(char) * 128);
	header->wsa__Action = soap_malloc(soap, sizeof(char) * 128);
	sprintf(header->wsa__MessageID,
	        "uuid:1319d68a-%d%d%d-%d%d-%d%d-%02X%02X%02X%02X%02X%02X",
	        tm_t->tm_wday, tm_t->tm_mday, tm_t->tm_mon,
	        tm_t->tm_year, tm_t->tm_hour, tm_t->tm_min,
	        tm_t->tm_sec, macaddr[0], macaddr[1], macaddr[2],
	        macaddr[3], macaddr[4], macaddr[5]);
	str = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";
	memcpy(header->wsa__To, str, strlen(str) + 1);
	str = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello";
	memcpy(header->wsa__Action, str, strlen(str) + 1);
	soap->header = header;
	struct wsdd__HelloType* req_hello;
	req_hello = soap_malloc(soap, sizeof(struct wsdd__HelloType));
	soap_default_wsdd__HelloType(soap, req_hello);
	req_hello->Types  = "tdn:NetworkVideoTransmitter";
	req_hello->XAddrs = (char*)soap_malloc(soap, sizeof(char) * 128);
	sprintf(req_hello->XAddrs, "http://%d.%d.%d.%d:%d/onvif/device_service",
	        0xFF & (nIpAddr >> 24), 0xFF & (nIpAddr >> 16), 0xFF & (nIpAddr >> 8),
	        0xFF & (nIpAddr),
	        stNetCfg.u16HttpPort);
	req_hello->wsa__EndpointReference.Address = (char*)soap_malloc(soap,
	        sizeof(char) * 128);
	sprintf(req_hello->wsa__EndpointReference.Address,
	        "urn:uuid:2419d68a-2dd2-21b2-a205-%02X%02X%02X%02X%02X%02X",
	        macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	req_hello->MetadataVersion = 1;
	soap_send___wsdd__Hello(soap, "soap.udp://239.255.255.250:3702/", NULL,
	                        req_hello);
	soap_destroy (soap);
	soap_end(soap);
	soap_done(soap);
	soap_free(soap);
}
/////////////////////////END/////////////////////////////

int _dn_Bye_send(struct soap* soap)
{
	char msgid[LARGE_INFO_LENGTH];
	time_t curTime;
	struct tm* ptm;
	struct tm tmTime;
	struct SOAP_ENV__Header header;
	struct wsdd__ByeType* Bye;
	HI_DEV_NET_CFG_S stNetCfg;
	memset(&header, 0, sizeof(struct SOAP_ENV__Header));
	soap_default_SOAP_ENV__Header(soap,
	                              &header);						/* setting default values for header */
	soap->header = &header;
	HI_U8* pMac = NULL;
	unsigned long ip = htonl(hi_get_sock_ip(soap->socket));
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0, (char*)&stNetCfg,
	                     sizeof(HI_DEV_NET_CFG_S));

	if(ip == stNetCfg.struEtherCfg[0].u32IpAddr)
	{
		pMac = stNetCfg.struEtherCfg[0].u8MacAddr;
	}
	else
	{
		pMac = stNetCfg.struEtherCfg[1].u8MacAddr;
	}

	curTime = time(NULL);
	ptm = localtime_r(&curTime, &tmTime);
	memset(msgid, 0, sizeof(msgid));
	sprintf(msgid, "uuid:1319d68a-%d%d%d-%d%d-%d%d-%02X%02X%02X%02X%02X%02X",
	        ptm->tm_wday, ptm->tm_mday, ptm->tm_mon,
	        ptm->tm_year, ptm->tm_hour, ptm->tm_min,
	        ptm->tm_sec, pMac[0], pMac[1], pMac[2],
	        pMac[3], pMac[4], pMac[5]);
	ALLOC_TOKEN(header.wsa__MessageID, msgid);
	ALLOC_TOKEN(header.wsa__To, "urn:schemas-xmlsoap-org:ws:2005:04:discovery");
	ALLOC_TOKEN(header.wsa__Action,
	            "http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye");
	Bye = soap_malloc(soap, sizeof(struct wsdd__ByeType));
	soap_default_wsdd__ByeType(soap, Bye);
	soap_send___wsdd__Bye(soap, "soap.udp://239.255.255.250:3702/", NULL, Bye);
	return SOAP_OK;
}


void hi_onvif_bye(int nType)
{
	struct soap* pSoap;
	pSoap = soap_new();                                                  /* creating  soap object */
	soap_init1(pSoap, SOAP_IO_UDP |
	           SO_BROADCAST);                       /* initializing soap packet */
	_dn_Bye_send(pSoap);
	soap_destroy (pSoap);
	soap_end(pSoap);
	soap_done(pSoap);
	soap_free(pSoap);
}

int hi_onvif_get_init_state()
{
	return gOnvifMng.init;
}

///////////////////////20140619 zj/////////////////////////////
void* hi_onvif_PresetTour_Thread1(void* argv)
{
	return NULL;
}
////////////////////////END////////////////////////////////

int onvif_interactive_work_proc(struct soap* soap)
{
	int ret = -1;
	fd_set r_set;
	struct timeval tv = {.tv_sec = 3, .tv_usec = 0 };
	FD_ZERO(&r_set);
	FD_SET(soap->socket, &r_set);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	ret = select(soap->socket + 1, &r_set, NULL, NULL, &tv);

	if (ret > 0)
	{
		soap->recv_timeout = 2;

		if ((ret = soap_serve(soap)) != SOAP_OK)
		{
			soap_print_fault(soap, stderr);	//bug maybe in here
			//hi_log_write(LOG_ERR, "soap_serve err ret %d\n", ret);
		}
	}
	else if (ret < 0)
	{
		//_LOG(LOG_ERR, "select sock %d err %m\n", soap->socket);
	}

	return ret;
}

void* onvif_interactive_service_thrd(void* arg)
{
	struct soap* pSoap = NULL;
	int i = 0;
	int index = (int)arg;

	if (index < 0 || index >= MAX_SUPP_INTER_THRD)
	{
		//_LOG(LOG_ERR, "=> Invalid arg %d\n", index);
		return NULL;
	}

	while (!onvif_intr_thr_flag)
	{
		pthread_mutex_lock(&g_inter_mutex);

		if (!soap_valid_socket(soap_accept(&gIntSoap)))
		{
			//_LOG(LOG_ERR, "soap_accept err count %d => %m\n", i);
			if (++i > 20)
			{
				pthread_mutex_unlock(&g_inter_mutex);
				break;
			}
		}

		i = 0;
		pSoap = soap_copy(&gIntSoap);

		if (pSoap == NULL)
		{
			pthread_mutex_unlock(&g_inter_mutex);
			//_LOG(LOG_ERR, "soap_copy err %m\n");
			continue;
		}

		pthread_mutex_unlock(&g_inter_mutex);
		onvif_interactive_work_proc(pSoap);
		soap_destroy(pSoap);
		soap_end(pSoap);
		soap_free(pSoap);
	}

	//_LOG(LOG_DEBUG, "server thrd %d exit\n");
	return NULL;
}

int hi_onvif_interactive_thr()
{
	int i;
	pthread_mutex_init(&g_inter_mutex, NULL);
	onvif_intr_thr_flag = 0;
	soap_init(&gIntSoap);//, SOAP_IO_KEEPALIVE);
#ifdef USE_WSSE_AUTH
	printf("Need soap_register_plugin wsse\n");
	soap_register_plugin(&gIntSoap, soap_wsa);
	//soap_register_plugin(&gIntSoap, soap_wsse);
#endif
#ifdef USE_DIGEST_AUTH
	printf("Need soap_register_plugin http_da\n");
	soap_register_plugin(&gIntSoap, http_da);
#endif
	soap_set_mode(&gIntSoap, SOAP_C_UTFSTRING);
	gIntSoap.bind_flags = SO_REUSEADDR;

	if (!soap_valid_socket(soap_bind(&gIntSoap, NULL, INTER_SVR_PORT, 128)))
	{
		// _LOG(LOG_ERR, "onvif_interactive_service exiting[tid:%ld]...\n", gettid());
		soap_print_fault(&gIntSoap, stderr);
		return -1;
	}

	for (i = 0; i < MAX_SUPP_INTER_THRD; i++)
	{
		if (pthread_create(&gOnvifMng.interacteThread[i],
		                   "Onv_interSvr",  /*&attr */ NULL, onvif_interactive_service_thrd,
		                   (void*)i) < 0)
		{
			//_LOG(LOG_ERR, "thread_create interactive_service thrd %d failed\n", i);
			return -2;
		}
		else
		{
			//_LOG(LOG_DEBUG, "interactive service thrd %d starting...\n", i);
		}
	}

	return 0;
}


int hi_onvif_message_proc(int sock, char* query)
{
	struct soap gsoap;
	soap_init (&gsoap);
	soap_set_mode(&gsoap, SOAP_C_UTFSTRING);
	gsoap.socket = sock;
	{
		printf("hi_onvif_message_proc query=%s. query is not use.\n", query);
	}
	//soap_serve(&gsoap, query);
	soap_serve(&gsoap);
	soap_destroy (&gsoap);
	soap_end (&gsoap); /* clear soap */
	soap_done(&gsoap);
	return 0;
}

int onvif_get_version(char* ver)
{
	return HI_SUCCESS;
}

int onvif_register_cb(PF_CB_S* pCb)
{
	memset(&gOnvifDevCb, 0, sizeof(PF_CB_S));
	memcpy(&gOnvifDevCb, pCb, sizeof(PF_CB_S));
	return HI_SUCCESS;
}

int onvif_init()
{
	HI_DEV_NET_CFG_S stNetCfg;
	__D("onvif initialize...\n");
	memset(&gOnvifMng, 0, sizeof(gOnvifMng));
	pthread_mutex_init(&gOnvifMng.mutex, NULL);
	//hi_platform_get_net_cfg(0, &stNetCfg);
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_NET_PARAM, 0, (char*)&stNetCfg,
	                     sizeof(HI_DEV_NET_CFG_S));
	onvif_server_init(stNetCfg.u16HttpPort);
	hi_onvif_hello(HI_CARD_ETH0); /* Send Hello */
	pthread_create(&gOnvifMng.helloThread, "OnvifHello", NULL, hi_onvif_hello_check,
	               NULL);
	pthread_create(&gOnvifMng.probeThread, "OnvifProbe", NULL, hi_onvif_probe,
	               NULL);
	//pthread_create(&gOnvifMng.serverThread, "ONVIFServer", NULL, hi_onvif_server, NULL);
	//pthread_create(&gOnvifMng.helloWifiThread, "ONVIFWIFIHello", NULL, hi_onvif_wifi_hello_check, NULL);
	//pthread_create(&gOnvifMng.probeWifiThread, "ONVIFWIFIProbe", NULL, hi_onvif_wifi_probe, NULL);
	pthread_create(&gOnvifMng.onviftourThread, "PresetTour", NULL,
	               hi_onvif_PresetTour_Thread1, NULL); //20140619 zj
	gOnvifMng.init = 1;
	return HI_SUCCESS;
}

int onvif_uninit()
{
	return 0;
#if 0 //ONVIF_OK
	HI_DEV_WIFI_CFG_S wifiCfg = {0};
	gOnvifMng.quit = 1;
	hi_onvif_bye(HI_CARD_ETH0);
	gOnvifDevCb.pMsgProc(NULL, HI_GET_PARAM_MSG, HI_WIFI_PARAM, 0, (char*)&wifiCfg,
	                     sizeof(HI_DEV_WIFI_CFG_S));

	if(hi_wifi_get_iw_netstat())
	{
		hi_onvif_bye(HI_CARD_RA0);
	}

	pthread_kill(gOnvifMng.helloThread, SIGKILL);
	pthread_kill(gOnvifMng.probeThread, SIGKILL);
	//pthread_kill(gOnvifMng.serverThread, SIGKILL);
	pthread_kill(gOnvifMng.helloWifiThread, SIGKILL);
	pthread_kill(gOnvifMng.probeWifiThread, SIGKILL);
#endif
	return HI_SUCCESS;
}

int onvif_run()
{
	hi_onvif_interactive_thr();
	return HI_SUCCESS;
}

int onvif_stop()
{
	return HI_SUCCESS;
}

int onvif_get_param(void* args, int* pLen)
{
	return HI_SUCCESS;
}

int onvif_set_param(void* args, int nLen)
{
	return HI_SUCCESS;
}

//#endif

