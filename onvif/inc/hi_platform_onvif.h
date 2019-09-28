#ifndef __HI_PLATFORM_ONVIF_H__
#define __HI_PLATFORM_ONVIF_H__

#include "hi_pf.h"

//#if (HI_SUPPORT_PLATFORM && PLATFORM_ONVIF)

#define LARGE_INFO_LENGTH		1024
#define MACH_ADDR_LENGTH		32
#define INFO_LENGTH				1024
#define SMALL_INFO_LENGTH		512

#define HI_PLATFORM_ONVIF_SUBVERSION 14

#define DISC_SVR_PORT       3702
#define INTER_SVR_PORT      8000
#define DEF_SLEEP_TIME      5
#define MAX_SUPP_INTER_THRD 4

#define EXTERNAL_SVR_PORT   8000   //Use web route to 8000 port , set EXTERNAL_SVR_PORT = 80. 


extern PF_CB_S gOnvifDevCb;

typedef struct _HI_ONVIF_SOCKET_S_
{
	int eth0Socket;
	int wifiSocket;
} HI_ONVIF_SOCKET_S, *LPHI_ONVIF_SOCKET_S;

HI_ONVIF_SOCKET_S stOnvifSocket;

void* hi_onvif_PresetTour_Thread1(void* argv);//20140619 zj

void hi_onvif_hello();

void hi_onvif_bye(int nType);

int hi_onvif_get_init_state();


int hi_onvif_message_proc(int sock, char* query);


///////  ONVIF interface //////////
int onvif_get_version(char* ver);

int onvif_register_cb(PF_CB_S* pCb);

int onvif_init();

int onvif_uninit();

int onvif_run();

int onvif_stop();

int onvif_get_param(void* args, int* pLen);

int onvif_set_param(void* args, int nLen);


//#endif
#endif

