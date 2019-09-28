#ifndef _HI_ONVIF_SERVER_H_
#define _HI_ONVIF_SERVER_H_

#define ETH_NAME "eth0"
#define ETH_NAME1 "ra0"
#define LINK_LOCAL_ADDR "169.254.166.174"


#define ALLOC_TOKEN(val, token) {									\
	val = (char *) soap_malloc(soap, sizeof(char)*ONVIF_TOKEN_LEN); \
	if(val == NULL) 												\
	{																\
		printf("malloc err\r\n");									\
		return SOAP_FAULT;											\
	}																\
	memset(val, 0, sizeof(char)*ONVIF_TOKEN_LEN); 					\
	strcpy(val, token); 											\
	}


int _dn_Bye_send(struct soap* soap);

#endif

