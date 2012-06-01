/*
* Copyright (c) 2012, Sangoma Technologies
* Kapil Gupta <kgupta@sangoma.com>
* All rights reserved.
* 
* <Insert license here>
*/

/* INCLUDES *******************************************************************/
#include "mod_megaco.h"
/******************************************************************************/

/* DEFINES ********************************************************************/

/* FUNCTION PROTOTYPES ********************************************************/
int mgco_mg_gen_config(void);
int mgco_mu_gen_config(void);
int mgco_tucl_gen_config(void);
int mgco_mu_ssap_config(int idx);
int mgco_mg_tsap_config(int idx);
int mgco_mg_enble_debug(void);
int mgco_mg_ssap_config(int idx);
int mgco_mg_peer_config(int idx);
int mgco_mg_tpt_server_config(int idx);
int mgco_tucl_sap_config(int idx);

int mgco_mg_tsap_bind_cntrl(int idx);
int mgco_mg_tsap_enable_cntrl(int idx);
int mgco_mg_ssap_cntrl(int idx);
int mgco_mu_ssap_cntrl(int idx);
int mgco_mg_tpt_server(int idx);
int sng_mgco_tucl_shutdown();
int sng_mgco_mg_shutdown();
int sng_mgco_mg_ssap_stop(int sapId);
int sng_mgco_mg_tpt_server_stop(int idx);
int sng_mgco_mg_app_ssap_stop(int idx);

switch_status_t sng_mgco_stack_gen_cfg();

/******************************************************************************/

/* FUNCTIONS ******************************************************************/

switch_status_t sng_mgco_init(sng_isup_event_interface_t* event)
{
	uint32_t major, minor, build;

	switch_assert(event);

	/* initalize sng_mg library */
	sng_isup_init_gen(event);

	/* print the version of the library being used */
	sng_isup_version(&major, &minor, &build);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"Loaded LibSng-MEGACO %d.%d.%d\n", major, minor, build);

	/* start up the stack manager */
	if (sng_isup_init_sm()) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"Failed to start Stack Manager\n");
		return SWITCH_STATUS_FALSE;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"Started Stack Manager!\n");
	}

	if (sng_isup_init_tucl()) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"Failed to start TUCL\n");
		return SWITCH_STATUS_FALSE;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"Started TUCL!\n");
	}

	if (sng_isup_init_mg()) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"Failed to start MG\n");
		return SWITCH_STATUS_FALSE;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"Started MG!\n");
	}

	if (sng_isup_init_mu()) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"Failed to start MU\n");
		return SWITCH_STATUS_FALSE;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"Started MU!\n");
	}


	/* gen config for all the layers of megaco */
	return sng_mgco_stack_gen_cfg();
}

/*****************************************************************************************************************/
switch_status_t sng_mgco_stack_shutdown()
{
	/* shutdown MG */
	sng_mgco_mg_shutdown();

	/* shutdown TUCL */
	sng_mgco_tucl_shutdown();

	/* free MEGACO Application */
	sng_isup_free_mu();

	/* free MEGACO */
	sng_isup_free_mg();

	/* free TUCL */
	sng_isup_free_tucl();

	/* free SM */
	sng_isup_free_sm();

	/* free gen */
	sng_isup_free_gen();

	return SWITCH_STATUS_SUCCESS;
}

/*****************************************************************************************************************/
switch_status_t sng_mgco_stack_gen_cfg()
{
	if(mgco_mg_gen_config()) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"MG Gen Config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"MG Gen Config SUCCESS \n");	
	}

	if(mgco_mu_gen_config()) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"MU(MG-Application) Gen Config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"MU(MG-Application) Gen Config SUCCESS \n");	
	}

	if(mgco_tucl_gen_config()) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR," TUCL Gen Config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," TUCL Gen Config SUCCESS \n");	
	}

	return SWITCH_STATUS_SUCCESS;
}


/*****************************************************************************************************************/

switch_status_t sng_mgco_cfg(const char* profilename)
{
	int idx   = 0x00;

	switch_assert(profilename);

	GET_MG_CFG_IDX(profilename, idx);

	if(!idx || (idx == MAX_MG_PROFILES)){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR," No MG configuration found against profilename[%s]\n",profilename);
		return SWITCH_STATUS_FALSE;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," Starting MG configuration for idx[%d] against profilename[%s]\n", idx, profilename);

	if(mgco_tucl_sap_config(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR," mgco_tucl_sap_config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," mgco_tucl_sap_config SUCCESS \n");	
	}


	if(mgco_mu_ssap_config(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR," mgco_mu_ssap_config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," mgco_mu_ssap_config SUCCESS \n");	
	}


	if(mgco_mg_tsap_config(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR," mgco_mg_tsap_config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," mgco_mg_tsap_config SUCCESS \n");	
	}

	if(mgco_mg_ssap_config(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " mgco_mg_ssap_config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " mgco_mg_ssap_config SUCCESS \n");	
	}

	if(mgco_mg_peer_config(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " mgco_mg_peer_config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " mgco_mg_peer_config SUCCESS \n");	
	}

	if(mgco_mg_tpt_server_config(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " mgco_mg_tpt_server_config FAILED \n");	
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " mgco_mg_tpt_server_config SUCCESS \n");	
	}

	return SWITCH_STATUS_SUCCESS;
}

/*****************************************************************************************************************/

switch_status_t sng_mgco_start(const char* profilename)
{
	int idx   = 0x00;

	switch_assert(profilename);

	GET_MG_CFG_IDX(profilename, idx);

	if(!idx || (idx == MAX_MG_PROFILES)){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR," No MG configuration found against profilename[%s]\n",profilename);
		return SWITCH_STATUS_FALSE;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," Starting MG stack for idx[%d] against profilename[%s]\n", idx, profilename);

	if(mgco_mu_ssap_cntrl(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " mgco_mu_ssap_cntrl FAILED \n");
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " mgco_mu_ssap_cntrl SUCCESS \n");
	}

	if(mgco_mg_tsap_bind_cntrl(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " mgco_mg_tsap_bind_cntrl FAILED \n");
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " mgco_mg_tsap_bind_cntrl SUCCESS \n");
	}

	if(mgco_mg_ssap_cntrl(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " mgco_mg_ssap_cntrl FAILED \n");
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " mgco_mg_ssap_cntrl SUCCESS \n");
	}

	if(mgco_mg_tsap_enable_cntrl(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " mgco_mg_tsap_enable_cntrl FAILED \n");
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " mgco_mg_tsap_enable_cntrl SUCCESS \n");
	}

	return SWITCH_STATUS_SUCCESS;
}

/*****************************************************************************************************************/

switch_status_t sng_mgco_stop(const char* profilename)
{
	int idx   = 0x00;
	sng_mg_cfg_t* mgCfg  = NULL; 

	switch_assert(profilename);

	GET_MG_CFG_IDX(profilename, idx);

	if(!idx || (idx == MAX_MG_PROFILES)){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR," No MG configuration found against profilename[%s]\n",profilename);
		return SWITCH_STATUS_FALSE;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," Stopping MG stack for idx[%d] against profilename[%s]\n", idx, profilename);

	mgCfg  = &megaco_globals.g_mg_cfg.mgCfg[idx];

	/* MG STOP is as good as deleting that perticular mg(virtual mg instance) data from megaco stack */
	/* currently we are not supporting enable/disable MG stack */

	if(sng_mgco_mg_ssap_stop(mgCfg->id)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " sng_mgco_mg_ssap_stop FAILED \n");
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " sng_mgco_mg_ssap_stop SUCCESS \n");
	}

	if(sng_mgco_mg_tpt_server_stop(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " sng_mgco_mg_tpt_server_stop FAILED \n");
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " sng_mgco_mg_tpt_server_stop SUCCESS \n");
	}

	if(sng_mgco_mg_app_ssap_stop(idx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " sng_mgco_mg_app_ssap_stop FAILED \n");
		return SWITCH_STATUS_FALSE;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " sng_mgco_mg_app_ssap_stop SUCCESS \n");
	}

	return SWITCH_STATUS_SUCCESS;
}

/*****************************************************************************************************************/
int sng_mgco_mg_app_ssap_stop(int idx)
{
        MuMngmt         mgMngmt;
        Pst             pst;              /* Post for layer manager */
        MuCntrl         *cntrl;

        memset(&mgMngmt, 0, sizeof(mgMngmt));

        cntrl = &(mgMngmt.t.cntrl);

        /* initalize the post structure */
        smPstInit(&pst);

        /* insert the destination Entity */
        pst.dstEnt = ENTMU;

        /*fill in the specific fields of the header */
        mgMngmt.hdr.msgType         = TCNTRL;
        mgMngmt.hdr.entId.ent       = ENTMG;
        mgMngmt.hdr.entId.inst      = S_INST;
        mgMngmt.hdr.elmId.elmnt     = STSSAP;
        mgMngmt.hdr.elmId.elmntInst1     = GET_MU_SAP_ID(idx);

        cntrl->action       = ADEL;
        cntrl->subAction    = SAELMNT;

        return(sng_cntrl_mu(&pst, &mgMngmt));
}
/*****************************************************************************************************************/

int sng_mgco_mg_ssap_stop(int sapId)
{
	Pst pst;
	MgMngmt cntrl;

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(MgCntrl));

	smPstInit(&pst);

	pst.dstEnt = ENTMG;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTMG;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STSSAP;       /* SSAP */
	cntrl.hdr.elmId.elmntInst1 = sapId;      /* sap id */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action    	= ADEL;
	cntrl.t.cntrl.subAction    	= SAELMNT;
	cntrl.t.cntrl.spId 		= sapId;
	return (sng_cntrl_mg (&pst, &cntrl));
}

/*****************************************************************************************************************/
int sng_mgco_mg_tpt_server_stop(int idx)
{
        MgMngmt         mgMngmt;
        Pst             pst;              /* Post for layer manager */
        MgCntrl         *cntrl;
        MgTptCntrl *tptCntrl = &mgMngmt.t.cntrl.s.tptCntrl;
        CmInetIpAddr   ipAddr = 0;
	sng_mg_cfg_t* mgCfg  = &megaco_globals.g_mg_cfg.mgCfg[idx];

        cntrl = &(mgMngmt.t.cntrl);

        memset(&mgMngmt, 0, sizeof(mgMngmt));

        /* initalize the post structure */
        smPstInit(&pst);

        /* insert the destination Entity */
        pst.dstEnt = ENTMG;

	tptCntrl->transportType = GET_TPT_TYPE(idx);
        
	tptCntrl->serverAddr.type =  CM_INET_IPV4ADDR_TYPE;
	tptCntrl->serverAddr.u.ipv4TptAddr.port = mgCfg->port;
	if(ROK == cmInetAddr((S8*)mgCfg->my_ipaddr, &ipAddr))
	{
		tptCntrl->serverAddr.u.ipv4TptAddr.address = ntohl(ipAddr);
	}

        /*fill in the specific fields of the header */
        mgMngmt.hdr.msgType         = TCNTRL;
        mgMngmt.hdr.entId.ent       = ENTMG;
        mgMngmt.hdr.entId.inst      = S_INST;
        mgMngmt.hdr.elmId.elmnt     = STSERVER;

        cntrl->action       = ADEL;
        cntrl->subAction    = SAELMNT;

        return(sng_cntrl_mg(&pst, &mgMngmt));
}
/*****************************************************************************************************************/

int mgco_mg_tsap_bind_cntrl(int idx)
{
        MgMngmt         mgMngmt;
        Pst             pst;              /* Post for layer manager */
        MgCntrl         *cntrl;

        memset(&mgMngmt, 0, sizeof(mgMngmt));

        cntrl = &(mgMngmt.t.cntrl);

        /* initalize the post structure */
        smPstInit(&pst);

        /* insert the destination Entity */
        pst.dstEnt = ENTMG;

        /*fill in the specific fields of the header */
        mgMngmt.hdr.msgType         = TCNTRL;
        mgMngmt.hdr.entId.ent       = ENTMG;
        mgMngmt.hdr.entId.inst      = S_INST;
        mgMngmt.hdr.elmId.elmnt     = STTSAP;

        cntrl->action       = ABND_ENA;
        cntrl->subAction    = SAELMNT;
        cntrl->spId 	    = GET_TPT_ID(idx);

        return(sng_cntrl_mg(&pst, &mgMngmt));
}

/*****************************************************************************************************************/

int mgco_mg_tsap_enable_cntrl(int idx)
{
        MgMngmt         mgMngmt;
        Pst             pst;              /* Post for layer manager */
        MgCntrl         *cntrl;

        memset(&mgMngmt, 0, sizeof(mgMngmt));

        cntrl = &(mgMngmt.t.cntrl);

        /* initalize the post structure */
        smPstInit(&pst);

        /* insert the destination Entity */
        pst.dstEnt = ENTMG;

        /*fill in the specific fields of the header */
        mgMngmt.hdr.msgType         = TCNTRL;
        mgMngmt.hdr.entId.ent       = ENTMG;
        mgMngmt.hdr.entId.inst      = S_INST;
        mgMngmt.hdr.elmId.elmnt     = STTSAP;

        cntrl->action       = AENA;
        cntrl->subAction    = SAELMNT;
        cntrl->spId 	    = GET_TPT_ID(idx);

        return(sng_cntrl_mg(&pst, &mgMngmt));
}

/*****************************************************************************************************************/

int mgco_mg_tpt_server(int idx)
{
        MgMngmt         mgMngmt;
        Pst             pst;              /* Post for layer manager */
        MgCntrl         *cntrl;
        MgTptCntrl *tptCntrl = &mgMngmt.t.cntrl.s.tptCntrl;
        CmInetIpAddr   ipAddr = 0;
	sng_mg_cfg_t* mgCfg  = &megaco_globals.g_mg_cfg.mgCfg[idx];

        cntrl = &(mgMngmt.t.cntrl);

        memset(&mgMngmt, 0, sizeof(mgMngmt));

        /* initalize the post structure */
        smPstInit(&pst);

        /* insert the destination Entity */
        pst.dstEnt = ENTMG;

	tptCntrl->transportType = GET_TPT_TYPE(idx);
        
	tptCntrl->serverAddr.type =  CM_INET_IPV4ADDR_TYPE;
	tptCntrl->serverAddr.u.ipv4TptAddr.port = mgCfg->port;
	if(ROK == cmInetAddr((S8*)mgCfg->my_ipaddr, &ipAddr))
	{
		tptCntrl->serverAddr.u.ipv4TptAddr.address = ntohl(ipAddr);
	}

        /*fill in the specific fields of the header */
        mgMngmt.hdr.msgType         = TCNTRL;
        mgMngmt.hdr.entId.ent       = ENTMG;
        mgMngmt.hdr.entId.inst      = S_INST;
        mgMngmt.hdr.elmId.elmnt     = STSERVER;

        cntrl->action       = AENA;
        cntrl->subAction    = SAELMNT;
        cntrl->spId = (SpId)0x01;

        return(sng_cntrl_mg(&pst, &mgMngmt));
}

/*****************************************************************************************************************/

int mgco_mu_ssap_cntrl(int idx)
{
        MuMngmt         mgMngmt;
        Pst             pst;              /* Post for layer manager */
        MuCntrl         *cntrl;

        memset(&mgMngmt, 0, sizeof(mgMngmt));

        cntrl = &(mgMngmt.t.cntrl);

        /* initalize the post structure */
        smPstInit(&pst);

        /* insert the destination Entity */
        pst.dstEnt = ENTMU;

        /*fill in the specific fields of the header */
        mgMngmt.hdr.msgType         = TCNTRL;
        mgMngmt.hdr.entId.ent       = ENTMG;
        mgMngmt.hdr.entId.inst      = S_INST;
        mgMngmt.hdr.elmId.elmnt     = STSSAP;
        mgMngmt.hdr.elmId.elmntInst1     = GET_MU_SAP_ID(idx);

        cntrl->action       = ABND_ENA;
        cntrl->subAction    = SAELMNT;

        return(sng_cntrl_mu(&pst, &mgMngmt));
}

/*****************************************************************************************************************/

int mgco_mg_ssap_cntrl(int idx)
{
        MgMngmt         mgMngmt;
        Pst             pst;              /* Post for layer manager */
        MgCntrl         *cntrl;

        memset(&mgMngmt, 0, sizeof(mgMngmt));

        cntrl = &(mgMngmt.t.cntrl);

        /* initalize the post structure */
        smPstInit(&pst);

        /* insert the destination Entity */
        pst.dstEnt = ENTMG;

        /*fill in the specific fields of the header */
        mgMngmt.hdr.msgType         = TCNTRL;
        mgMngmt.hdr.entId.ent       = ENTMG;
        mgMngmt.hdr.entId.inst      = S_INST;
        mgMngmt.hdr.elmId.elmnt     = STSSAP;

        cntrl->action       = AENA;
        cntrl->subAction    = SAELMNT;
        cntrl->spId = (SpId)1;

        return(sng_cntrl_mg(&pst, &mgMngmt));
}
/******************************************************************************/
                                                                                                       
int mgco_mg_enble_debug()
{
	MgMngmt    mgMngmt;
	Pst          pst;              /* Post for layer manager */
	MgCntrl*    cntrl;

	memset(&mgMngmt, 0, sizeof(mgMngmt));
	cntrl = &mgMngmt.t.cntrl;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTMG;
	mgMngmt.hdr.msgType         = TCFG;
	mgMngmt.hdr.entId.ent       = ENTHI;
	mgMngmt.hdr.entId.inst      = S_INST;
	mgMngmt.hdr.elmId.elmnt     = STGEN;

	cntrl->action  		        = AENA;
	cntrl->subAction                = SADBG;
	cntrl->s.dbg.genDbgMask    = 0xfffffdff;

	return(sng_cntrl_mg(&pst, &mgMngmt));
}

/******************************************************************************/
int mgco_tucl_gen_config(void)
{
	HiMngmt cfg;
	Pst     pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTHI;

	/* clear the configuration structure */
	memset(&cfg, 0, sizeof(cfg));

	/* fill in the post structure */
	set_dest_sm_pst(&cfg.t.cfg.s.hiGen.lmPst);
	/*fill in the specific fields of the header */
	cfg.hdr.msgType         = TCFG;
	cfg.hdr.entId.ent       = ENTHI;
	cfg.hdr.entId.inst      = S_INST;
	cfg.hdr.elmId.elmnt     = STGEN;

	cfg.t.cfg.s.hiGen.numSaps               = HI_MAX_SAPS;            		/* number of SAPs */
	cfg.t.cfg.s.hiGen.numCons               = HI_MAX_NUM_OF_CON;          		/* maximum num of connections */
	cfg.t.cfg.s.hiGen.numFdsPerSet          = HI_MAX_NUM_OF_FD_PER_SET;     	/* maximum num of fds to use per set */
	cfg.t.cfg.s.hiGen.numFdBins             = HI_MAX_NUM_OF_FD_HASH_BINS;   	/* for fd hash lists */
	cfg.t.cfg.s.hiGen.numClToAccept         = HI_MAX_NUM_OF_CLIENT_TO_ACCEPT; 	/* clients to accept simultaneously */
	cfg.t.cfg.s.hiGen.permTsk               = TRUE;         		 	/* schedule as perm task or timer */
	cfg.t.cfg.s.hiGen.schdTmrVal            = HI_MAX_SCHED_TMR_VALUE;            	/* if !permTsk - probably ignored */
	cfg.t.cfg.s.hiGen.selTimeout            = HI_MAX_SELECT_TIMEOUT_VALUE;     	/* select() timeout */

	/* number of raw/UDP messages to read in one iteration */
	cfg.t.cfg.s.hiGen.numRawMsgsToRead      = HI_MAX_RAW_MSG_TO_READ;
	cfg.t.cfg.s.hiGen.numUdpMsgsToRead      = HI_MAX_UDP_MSG_TO_READ;

	/* thresholds for congestion on the memory pool */
	cfg.t.cfg.s.hiGen.poolStrtThr           = HI_MEM_POOL_START_THRESHOLD;
	cfg.t.cfg.s.hiGen.poolDropThr           = HI_MEM_POOL_DROP_THRESHOLD;
	cfg.t.cfg.s.hiGen.poolStopThr           = HI_MEM_POOL_STOP_THRESHOLD;

	cfg.t.cfg.s.hiGen.timeRes               = SI_PERIOD;        /* time resolution */

#ifdef HI_SPECIFY_GENSOCK_ADDR
	cfg.t.cfg.s.hiGen.ipv4GenSockAddr.address = CM_INET_INADDR_ANY;
	cfg.t.cfg.s.hiGen.ipv4GenSockAddr.port  = 0;                /* DAVIDY - why 0? */
#ifdef IPV6_SUPPORTED
	cfg.t.cfg.s.hiGen.ipv6GenSockAddr.address = CM_INET_INADDR6_ANY;
	cfg.t.cfg.s.hiGen.ipv4GenSockAddr.port  = 0;
#endif
#endif

	return(sng_cfg_tucl(&pst, &cfg));
}

/******************************************************************************/

int mgco_tucl_sap_config(int idx)
{
	HiMngmt cfg;
	Pst     pst;
	HiSapCfg  *pCfg;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTHI;

	/* clear the configuration structure */
	memset(&cfg, 0, sizeof(cfg));

	/* fill in the post structure */
	set_dest_sm_pst(&cfg.t.cfg.s.hiGen.lmPst);
	/*fill in the specific fields of the header */
	cfg.hdr.msgType         = TCFG;
	cfg.hdr.entId.ent       = ENTHI;
	cfg.hdr.entId.inst      = S_INST;
	cfg.hdr.elmId.elmnt     = STTSAP;

	pCfg = &cfg.t.cfg.s.hiSap;

	pCfg->spId 		= GET_TPT_ID(idx); 
	pCfg->uiSel 		= 0x00;  /*loosley coupled */
	pCfg->flcEnb 		= TRUE;
	pCfg->txqCongStrtLim 	= HI_SAP_TXN_QUEUE_CONG_START_LIMIT;
	pCfg->txqCongDropLim 	= HI_SAP_TXN_QUEUE_CONG_DROP_LIMIT;
	pCfg->txqCongStopLim 	= HI_SAP_TXN_QUEUE_CONG_STOP_LIMIT;
	pCfg->numBins 		= 10;

	pCfg->uiMemId.region 	= S_REG;
	pCfg->uiMemId.pool   	= S_POOL;
	pCfg->uiPrior 	     	= PRIOR0;
	pCfg->uiRoute        	= RTESPEC;

	return(sng_cfg_tucl(&pst, &cfg));
}

/******************************************************************************/

int mgco_mg_gen_config(void)
{
	MgMngmt    mgMngmt;
	MgGenCfg    *cfg;
	Pst          pst;              /* Post for layer manager */

	memset(&mgMngmt, 0, sizeof(mgMngmt));
	cfg   = &(mgMngmt.t.cfg.c.genCfg);

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTMG;

	/* fill in the post structure */
	set_dest_sm_pst(&mgMngmt.t.cfg.c.genCfg.lmPst);
	/*fill in the specific fields of the header */
	mgMngmt.hdr.msgType         = TCFG;
	mgMngmt.hdr.entId.ent       = ENTMG;
	mgMngmt.hdr.entId.inst      = S_INST;
	mgMngmt.hdr.elmId.elmnt     = STGEN;


	/*----------- Fill General Configuration Parameters ---------*/
	cfg->maxSSaps = (U16)MG_MAX_SSAPS;
	cfg->maxTSaps = (U32)MG_MAX_TSAPS;
	cfg->maxServers = (U16)MG_MAX_SERVERS;
	cfg->maxConn = (U32)10;
	cfg->maxTxn = (U16)MG_MAX_OUTSTANDING_TRANSACTIONS;
	cfg->maxPeer = (U32)MG_MAX_PEER;
	cfg->resThUpper = (Status)7;
	cfg->resThLower = (Status)3;
#if (defined(GCP_MGCP) || defined(TDS_ROLL_UPGRADE_SUPPORT))   
	cfg->timeResTTL = (Ticks)10000;
#endif

	cfg->timeRes = (Ticks)10;
	cfg->reCfg.rspAckEnb = MG_LMG_GET_RSPACK_MGCO;
	cfg->numBlks = (U32)MG_NUM_BLK;
	cfg->maxBlkSize = (Size)MG_MAXBLKSIZE;
	cfg->numBinsTxnIdHl = (U16)149;
	cfg->numBinsNameHl = (U16)149;
	cfg->entType = LMG_ENT_GW;
	cfg->numBinsTptSrvrHl = (U16)149;
	cfg->indicateRetx = TRUE;  /* Assume environment to be lossy */
	cfg->resOrder = LMG_RES_IPV4; /* IPV4 only */

#ifdef CM_ABNF_MT_LIB
	cfg->firstInst = 1;
	cfg->edEncTmr.enb = FALSE;
	cfg->edEncTmr.val = (U16)50;
	cfg->edDecTmr.enb = TRUE;
	cfg->edDecTmr.val = (U16)50;
	cfg->noEDInst = 1;
#endif /* CM_ABNF_MT_LIB */

#ifdef GCP_CH
	cfg->numBinsPeerCmdHl = 20;
	cfg->numBinsTransReqHl = 50;
	cfg->numBinsTransIndRspCmdHl = 50;
#endif /* GCP_CH */

#ifdef GCP_MG
	cfg->maxMgCmdTimeOut.enb =TRUE;
	cfg->maxMgCmdTimeOut.val =20;
#endif /* GCP_MG */

#ifdef GCP_MG
	cfg->maxMgCmdTimeOut.enb =TRUE;
	cfg->maxMgCmdTimeOut.val =20;
#endif /* GCP_MG */

#ifdef GCP_MGC
	cfg->maxMgcCmdTimeOut.enb =TRUE;
	cfg->maxMgcCmdTimeOut.val =20;
#endif /* GCP_MG */

#if (defined(GCP_MGCO) && (defined GCP_VER_2_1))
	cfg->reCfg.segRspTmr.enb = TRUE;
	cfg->reCfg.segRspTmr.val = (U16)50;
	cfg->reCfg.segRspAckTmr.enb = TRUE;
	cfg->reCfg.segRspAckTmr.val = (U16)25;
#endif

#ifdef GCP_PKG_MGCO_ROOT
	cfg->limit.pres.pres = PRSNT_NODEF;
	cfg->limit.mgcOriginatedPendingLimit = 20000;
	cfg->limit.mgOriginatedPendingLimit  = 20000;
#endif /* GCP_PKG_MGCO_ROOT */

	return(sng_cfg_mg(&pst, &mgMngmt));
}

/******************************************************************************/

int mgco_mu_gen_config(void)
{
	MuMngmt      mgmt;
	MuGenCfg    *cfg;
	Pst          pst;              /* Post for layer manager */

	memset(&mgmt, 0, sizeof(mgmt));
	cfg   = &(mgmt.t.cfg);

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTMU;

	/*fill in the specific fields of the header */
	mgmt.hdr.msgType         = TCFG;
	mgmt.hdr.entId.ent       = ENTMU;
	mgmt.hdr.entId.inst      = S_INST;
	mgmt.hdr.elmId.elmnt     = STGEN;

	return(sng_cfg_mu(&pst, &mgmt));
}

/******************************************************************************/

int mgco_mu_ssap_config(int idx)
{
	MuMngmt      mgmt;
	MuSAP_t      *cfg;
	Pst          pst;              /* Post for layer manager */

	memset(&mgmt, 0, sizeof(mgmt));
	cfg   = &(mgmt.t.sapCfg);

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTMU;

	/*fill in the specific fields of the header */
	mgmt.hdr.msgType         = TCFG;
	mgmt.hdr.entId.ent       = ENTMU;
	mgmt.hdr.entId.inst      = S_INST;
	mgmt.hdr.elmId.elmnt     = STSSAP;

	/* fill lower layer i.e. MG PST */ 
	cfg->ssapId 		= GET_MU_SAP_ID(idx); 						/* SSAP ID */ 
	cfg->spId 		= GET_MU_SAP_ID(idx);  		        			/* SSAP ID */ 

	cfg->mem.region 	= S_REG; 
	cfg->mem.pool 	        = S_POOL;
	cfg->dstProcId          = SFndProcId();
	cfg->dstEnt             = ENTMG; 
	cfg->dstInst            = S_INST; 
	cfg->dstPrior     	= PRIOR0; 
	cfg->dstRoute     	= RTESPEC; 
	cfg->selector       	= 0x00; /* Loosely coupled */ 

	return(sng_cfg_mu(&pst, &mgmt));
}

/******************************************************************************/

int mgco_mg_ssap_config(int idx)
{
	MgMngmt       mgMngmt;
	MgSSAPCfg    *pCfg;
	Pst          pst;              /* Post for layer manager */
	CmInetIpAddr ipAddr;
	int len = 0x00;
	sng_mg_cfg_t* mgCfg  = &megaco_globals.g_mg_cfg.mgCfg[idx];

	memset(&mgMngmt, 0, sizeof(mgMngmt));
	pCfg   = &(mgMngmt.t.cfg.c.sSAPCfg);

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTMG;

	/*fill in the specific fields of the header */
	mgMngmt.hdr.msgType         = TCFG;
	mgMngmt.hdr.entId.ent       = ENTMG;
	mgMngmt.hdr.entId.inst      = S_INST;
	mgMngmt.hdr.elmId.elmnt     = STSSAP;

	/* FILL SAP config */

	pCfg->sSAPId 		= mgCfg->id;  		/* SSAP ID */ 
	pCfg->sel 		= 0x00 ; 				/* Loosely coupled */ 
	pCfg->memId.region 	= S_REG; 
	pCfg->memId.pool 	= S_POOL;
	pCfg->prior 		= PRIOR0; 
	pCfg->route 		= RTESPEC;

	pCfg->protocol 		= mgCfg->protocol_type;

	pCfg->startTxnNum = 50;
	pCfg->endTxnNum   = 60;

	pCfg->initReg = TRUE;
	pCfg->mwdTimer = (U16)10;

	pCfg->minMgcoVersion = LMG_VER_PROF_MGCO_H248_1_0;
	switch(mgCfg->protocol_version)
	{
		case 1:
			pCfg->maxMgcoVersion = LMG_VER_PROF_MGCO_H248_1_0;
			break;
		case 2:
			pCfg->maxMgcoVersion = LMG_VER_PROF_MGCO_H248_2_0;
			break;
		case 3:
			pCfg->maxMgcoVersion = LMG_VER_PROF_MGCO_H248_3_0;
			break;
		default:
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Protocol version[%d] \n",mgCfg->protocol_version);
			return SWITCH_STATUS_FALSE;
	}

	pCfg->userInfo.pres.pres = PRSNT_NODEF;
	pCfg->userInfo.id.pres 	 = NOTPRSNT;
	pCfg->userInfo.mid.pres = PRSNT_NODEF;
	pCfg->userInfo.dname.namePres.pres = PRSNT_NODEF;

	pCfg->userInfo.mid.len = (U8)strlen((char*)mgCfg->mid);
	strncpy((char*)pCfg->userInfo.mid.val, (char*)mgCfg->mid, MAX_MID_LEN);

	len = (U32)strlen((char*)mgCfg->my_domain);
	memcpy( (U8*)(pCfg->userInfo.dname.name),
			(CONSTANT U8*)(mgCfg->my_domain), len );
	pCfg->userInfo.dname.name[len] = '\0';

	pCfg->userInfo.dname.netAddr.type = CM_TPTADDR_IPV4;
	memset(&ipAddr,'\0',sizeof(ipAddr));
	if(ROK == cmInetAddr((S8*)mgCfg->my_ipaddr,&ipAddr))
	{
		pCfg->userInfo.dname.netAddr.u.ipv4NetAddr = ntohl(ipAddr);
	}

	pCfg->reCfg.initRetxTmr.enb = TRUE;
	pCfg->reCfg.initRetxTmr.val = MG_INIT_RTT;
	pCfg->reCfg.provRspTmr.enb = TRUE;
	pCfg->reCfg.provRspTmr.val = (U16)50; /* In timer resolution */
	pCfg->reCfg.provRspDelay = 2;
	pCfg->reCfg.atMostOnceTmr.enb = TRUE;
	pCfg->reCfg.atMostOnceTmr.val = (U16)30;

	return(sng_cfg_mg(&pst, &mgMngmt));
}

/******************************************************************************/

int mgco_mg_tsap_config(int idx)
{
	MgMngmt    mgMngmt;
	/* local variables */
	MgTSAPCfg    *cfg;
	Pst          pst;              /* Post for layer manager */
	sng_mg_cfg_t* mgCfg  = &megaco_globals.g_mg_cfg.mgCfg[idx];

	memset(&mgMngmt, 0, sizeof(mgMngmt));
	cfg   = &(mgMngmt.t.cfg.c.tSAPCfg);

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTMG;

	/*fill in the specific fields of the header */
	mgMngmt.hdr.msgType         = TCFG;
	mgMngmt.hdr.entId.ent       = ENTMG;
	mgMngmt.hdr.entId.inst      = S_INST;
	mgMngmt.hdr.elmId.elmnt     = STTSAP;

	/* FILL TSAP config */
	cfg->tSAPId 	= mgCfg->id;
	cfg->spId   	= GET_TPT_ID(idx);
	cfg->provType 	= GET_TPT_TYPE(idx);

	/* FILL TUCL Information */
	cfg->memId.region = S_REG; 
	cfg->memId.pool   = S_POOL; 
	cfg->dstProcId    = SFndProcId();
	cfg->dstEnt       = ENTHI; 
	cfg->dstInst      = S_INST; 
	cfg->dstPrior     = PRIOR0; 
	cfg->dstRoute     = RTESPEC; 
	cfg->dstSel       = 0x00; /* Loosely coupled */ 
	cfg->bndTmrCfg.enb = TRUE;
	cfg->bndTmrCfg.val = 5; /* 5 seconds */


	/* Disable DNS as of now */
	cfg->reCfg.idleTmr.enb = FALSE;
        cfg->reCfg.dnsCfg.dnsAccess = LMG_DNS_DISABLED;
        cfg->reCfg.dnsCfg.dnsAddr.type = CM_TPTADDR_IPV4;
        cfg->reCfg.dnsCfg.dnsAddr.u.ipv4TptAddr.port = (U16)53;
        cfg->reCfg.dnsCfg.dnsAddr.u.ipv4TptAddr.address = (CmInetIpAddr)MG_DNS_IP;

        cfg->reCfg.dnsCfg.dnsRslvTmr.enb = FALSE;
        cfg->reCfg.dnsCfg.dnsRslvTmr.val = 60; /* 60 sec */
        cfg->reCfg.dnsCfg.maxRetxCnt = 4;

        cfg->reCfg.tMax = 1000;
        cfg->reCfg.tptParam.type = CM_TPTPARAM_SOCK;
        cfg->reCfg.tptParam.u.sockParam.listenQSize = 5;
        cfg->reCfg.tptParam.u.sockParam.numOpts = 0;


	return(sng_cfg_mg(&pst, &mgMngmt));
}

/******************************************************************************/

int mgco_mg_peer_config(int idx)
{
	MgMngmt    	mgMngmt;
	MgGcpEntCfg    *cfg;
	Pst          	pst;              /* Post for layer manager */
	U32            peerIdx = 0;
	CmInetIpAddr   ipAddr = 0;
	sng_mg_cfg_t*  mgCfg  = &megaco_globals.g_mg_cfg.mgCfg[idx];
	sng_mg_peer_t*  mgPeer = &megaco_globals.g_mg_cfg.mgPeer.peers[mgCfg->peer_id];

	memset(&mgMngmt, 0, sizeof(mgMngmt));
	cfg   = &(mgMngmt.t.cfg.c.mgGcpEntCfg);

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTMG;

	/*fill in the specific fields of the header */
	mgMngmt.hdr.msgType         = TCFG;
	mgMngmt.hdr.entId.ent       = ENTMG;
	mgMngmt.hdr.entId.inst      = S_INST;
	mgMngmt.hdr.elmId.elmnt     = STGCPENT;

	cfg->numPeer 			= megaco_globals.g_mg_cfg.mgPeer.total_peer;
	cfg->peerCfg[peerIdx].sSAPId 	= mgCfg->id;        /* SSAP ID */;
	cfg->peerCfg[peerIdx].port 	= mgPeer->port;
	cfg->peerCfg[peerIdx].tsapId 	= GET_TPT_ID(idx);

	cfg->peerCfg[peerIdx].mtuSize = MG_MAX_MTU_SIZE;


	cfg->peerCfg[peerIdx].peerAddrTbl.count = 1;
	cfg->peerCfg[peerIdx].peerAddrTbl.netAddr[0].type =
		CM_NETADDR_IPV4;

	if(ROK == cmInetAddr((S8*)&mgPeer->ipaddr[0],&ipAddr))
	{
		cfg->peerCfg[peerIdx].peerAddrTbl.netAddr[0].u.ipv4NetAddr = ntohl(ipAddr);
	}
	else
	{
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "cmInetAddr failed \n");
		cfg->peerCfg[peerIdx].peerAddrTbl.count = 0;
	}

#ifdef GCP_MG
	cfg->peerCfg[peerIdx].transportType  = GET_TPT_TYPE(idx);
	cfg->peerCfg[peerIdx].encodingScheme = GET_ENCODING_TYPE(idx); 
	cfg->peerCfg[peerIdx].mgcPriority = 0;
	cfg->peerCfg[peerIdx].useAHScheme = FALSE;
	cfg->peerCfg[peerIdx].mid.pres = PRSNT_NODEF;
	cfg->peerCfg[peerIdx].mid.len = strlen((char*)mgPeer->mid);
	cmMemcpy((U8 *)cfg->peerCfg[peerIdx].mid.val, 
		(CONSTANT U8*)(char*)mgPeer->mid, 
		 cfg->peerCfg[peerIdx].mid.len);

#endif /* GCP_MG */

	return(sng_cfg_mg(&pst, &mgMngmt));
}

/******************************************************************************/

int mgco_mg_tpt_server_config(int idx)
{
	MgMngmt    	mgMngmt;
	MgTptSrvrCfg    *cfg;
	Pst          	pst;              /* Post for layer manager */
	CmInetIpAddr   ipAddr = 0;
	sng_mg_cfg_t*  mgCfg  = &megaco_globals.g_mg_cfg.mgCfg[idx];
	int srvIdx = 0;

	memset(&mgMngmt, 0, sizeof(mgMngmt));
	cfg   = &(mgMngmt.t.cfg.c.tptSrvrCfg);

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTMG;

	/*fill in the specific fields of the header */
	mgMngmt.hdr.msgType         = TCFG;
	mgMngmt.hdr.entId.ent       = ENTMG;
	mgMngmt.hdr.entId.inst      = S_INST;
	mgMngmt.hdr.elmId.elmnt     = STSERVER;

	cfg->count = 1;
	cfg->srvr[srvIdx].isDefault = TRUE;
	cfg->srvr[srvIdx].sSAPId = mgCfg->id;
	cfg->srvr[srvIdx].tSAPId = GET_TPT_ID(idx);
	cfg->srvr[srvIdx].protocol = mgCfg->protocol_type;
	cfg->srvr[srvIdx].transportType = GET_TPT_TYPE(idx); 
	cfg->srvr[srvIdx].encodingScheme = GET_ENCODING_TYPE(idx);

	cfg->srvr[srvIdx].tptParam.type = CM_TPTPARAM_SOCK;
	cfg->srvr[srvIdx].tptParam.u.sockParam.listenQSize = 5;
	cfg->srvr[srvIdx].tptParam.u.sockParam.numOpts = 0;
	cfg->srvr[srvIdx].lclTptAddr.type = CM_TPTADDR_IPV4;
	cfg->srvr[srvIdx].lclTptAddr.u.ipv4TptAddr.port = mgCfg->port;
	if(ROK == cmInetAddr((S8*)mgCfg->my_ipaddr, &ipAddr))
	{
		cfg->srvr[srvIdx].lclTptAddr.u.ipv4TptAddr.address = ntohl(ipAddr);
	}

	return(sng_cfg_mg(&pst, &mgMngmt));
}

/******************************************************************************/
int sng_mgco_tucl_shutdown()
{
        Pst pst;
        HiMngmt cntrl;

        memset((U8 *)&pst, 0, sizeof(Pst));
        memset((U8 *)&cntrl, 0, sizeof(HiMngmt));

        smPstInit(&pst);

        pst.dstEnt = ENTHI;

        /* prepare header */
        cntrl.hdr.msgType     = TCNTRL;         /* message type */
        cntrl.hdr.entId.ent   = ENTHI;          /* entity */
        cntrl.hdr.entId.inst  = 0;              /* instance */
        cntrl.hdr.elmId.elmnt = STGEN;       /* General */

        cntrl.hdr.response.selector    = 0;
        cntrl.hdr.response.prior       = PRIOR0;
        cntrl.hdr.response.route       = RTESPEC;
        cntrl.hdr.response.mem.region  = S_REG;
        cntrl.hdr.response.mem.pool    = S_POOL;

        cntrl.t.cntrl.action    = ASHUTDOWN;

        return (sng_cntrl_tucl (&pst, &cntrl));
}
/******************************************************************************/
int sng_mgco_mg_shutdown()
{
        Pst pst;
        MgMngmt cntrl;

        memset((U8 *)&pst, 0, sizeof(Pst));
        memset((U8 *)&cntrl, 0, sizeof(MgCntrl));

        smPstInit(&pst);

        pst.dstEnt = ENTMG;

        /* prepare header */
        cntrl.hdr.msgType     = TCNTRL;         /* message type */
        cntrl.hdr.entId.ent   = ENTMG;          /* entity */
        cntrl.hdr.entId.inst  = 0;              /* instance */
        cntrl.hdr.elmId.elmnt = STGEN;       /* General */

        cntrl.hdr.response.selector    = 0;
        cntrl.hdr.response.prior       = PRIOR0;
        cntrl.hdr.response.route       = RTESPEC;
        cntrl.hdr.response.mem.region  = S_REG;
        cntrl.hdr.response.mem.pool    = S_POOL;

        cntrl.t.cntrl.action    = ASHUTDOWN;
        cntrl.t.cntrl.subAction    = SAELMNT;

        return (sng_cntrl_mg (&pst, &cntrl));
}
/******************************************************************************/
void handle_mg_alarm(Pst *pst, MgMngmt *usta)
{
	U16 ret;
	int len = 0x00;
	char prBuf[10024];

	memset(&prBuf[0], 0, sizeof(prBuf));

	len = len + sprintf(prBuf+len,"MG Status Indication: received with Category = %d, Event = %d, Cause = %d \n",
			usta->t.usta.alarm.category, usta->t.usta.alarm.event, 
			usta->t.usta.alarm.cause);

	len = len + sprintf(prBuf+len, "Category ( ");

	switch (usta->t.usta.alarm.category)
	{
		case LCM_CATEGORY_PROTOCOL:
			{
				len = len + sprintf(prBuf+len, "protocol related ");
				break;
			}
		case LCM_CATEGORY_INTERFACE:
			{
				len = len + sprintf(prBuf+len, "interface related ");
				break;
			}
		case LCM_CATEGORY_INTERNAL:
			{
				len = len + sprintf(prBuf+len, "internal ");
				break;
			}
		case LCM_CATEGORY_RESOURCE:
			{
				len = len + sprintf(prBuf+len, "system resources related ");
				break;
			}
		case LCM_CATEGORY_PSF_FTHA:
			{
				len = len + sprintf(prBuf+len, "fault tolerance / high availability PSF related ");
				break;
			}
		case LCM_CATEGORY_LYR_SPECIFIC:
			{
				len = len + sprintf(prBuf+len, "MGCP related ");
				break;
			}
		default:
			{
				len = len + sprintf(prBuf+len, "unknown: %d", (int)(usta->t.usta.alarm.category));
				break;
			}
	}
	len = len + sprintf(prBuf+len, ") ");

	len = len + sprintf(prBuf+len, " Event ( ");
	switch (usta->t.usta.alarm.event)
	{
		case LMG_EVENT_TSAP_RECVRY_SUCCESS:
			{
				len = len + sprintf(prBuf+len, "TSAP recovery success");
				break;
			}
		case LMG_EVENT_TSAP_RECVRY_FAILED:
			{
				len = len + sprintf(prBuf+len, "TSAP recovery failed");
				break;
			}
		case LCM_EVENT_UI_INV_EVT:
			{
				len = len + sprintf(prBuf+len, "upper interface invalid event");
				break;
			}
		case LCM_EVENT_LI_INV_EVT:
			{
				len = len + sprintf(prBuf+len, "lower interface invalid event");
				break;
			}
		case LCM_EVENT_PI_INV_EVT:
			{
				len = len + sprintf(prBuf+len, "peer interface invalid event");
				break;
			}
		case LCM_EVENT_INV_EVT:
			{
				len = len + sprintf(prBuf+len, "general invalid event");
				break;
			}
		case LCM_EVENT_INV_STATE:
			{
				len = len + sprintf(prBuf+len, "invalid internal state");
				break;
			}
		case LCM_EVENT_INV_TMR_EVT:
			{
				len = len + sprintf(prBuf+len, "invalid timer event");
				break;
			}
		case LCM_EVENT_MI_INV_EVT:
			{
				len = len + sprintf(prBuf+len, "management interface invalid event");
				break;
			}
		case LCM_EVENT_BND_FAIL:
			{
				len = len + sprintf(prBuf+len, "bind failure");
				break;
			}
		case LCM_EVENT_NAK:
			{
				len = len + sprintf(prBuf+len, "destination nack");
				break;
			}
		case LCM_EVENT_TIMEOUT:
			{
				len = len + sprintf(prBuf+len, "timeout");
				break;
			}
		case LCM_EVENT_BND_OK:
			{
				len = len + sprintf(prBuf+len, "bind ok");
				break;
			}
		case LCM_EVENT_SMEM_ALLOC_FAIL:
			{
				len = len + sprintf(prBuf+len, "static memory allocation failed");
				break;
			}
		case LCM_EVENT_DMEM_ALLOC_FAIL:
			{
				len = len + sprintf(prBuf+len, "dynamic mmemory allocation failed");
				break;
			}
		case LCM_EVENT_LYR_SPECIFIC:
			{
				len = len + sprintf(prBuf+len, "MGCP specific");
				break;
			}
		default:
			{
				len = len + sprintf(prBuf+len, "unknown event %d", (int)(usta->t.usta.alarm.event));
				break;
			}
		case LMG_EVENT_HIT_BNDCFM:
			{
				len = len + sprintf(prBuf+len, "HIT bind confirm");
				break;
			}
		case LMG_EVENT_HIT_CONCFM:
			{
				len = len + sprintf(prBuf+len, "HIT connect confirm");
				break;
			}
		case LMG_EVENT_HIT_DISCIND:
			{
				len = len + sprintf(prBuf+len, "HIT disconnect indication");
				break;
			}
		case LMG_EVENT_HIT_UDATIND:
			{
				len = len + sprintf(prBuf+len, "HIT unit data indication");
				break;
			}
		case LMG_EVENT_MGT_BNDREQ:
			{
				len = len + sprintf(prBuf+len, "MGT bind request");
				break;
			}
		case LMG_EVENT_PEER_CFG_FAIL:
			{
				len = len + sprintf(prBuf+len, "Peer Configuration Failed");
				break;
			}
		case LMG_EVENT_MGT_UBNDREQ:
			{
				len = len + sprintf(prBuf+len, "MGT unbind request");
				break;
			}
		case LMG_EVENT_MGT_MGCPTXNREQ:
			{
				len = len + sprintf(prBuf+len, "MGT MGCP transaction request");
				break;
			}
		case LMG_EVENT_MGT_MGCPTXNIND:
			{
				len = len + sprintf(prBuf+len, "MGT MGCP transaction indication");
				break;
			}

		case LMG_EVENT_PEER_ENABLED:
			{
				len = len + sprintf(prBuf+len, "gateway enabled");
				break;
			}
		case LMG_EVENT_PEER_DISCOVERED:
			{
				len = len + sprintf(prBuf+len, "gateway discovered , notified entity");
				break;
			}
		case LMG_EVENT_PEER_REMOVED:
			{
				len = len + sprintf(prBuf+len, "gateway removed");
				break;
			}
		case LMG_EVENT_RES_CONG_ON:
			{
				len = len + sprintf(prBuf+len, "resource congestion ON");
				break;
			}
		case LMG_EVENT_RES_CONG_OFF:
			{
				len = len + sprintf(prBuf+len, "resource congestion OFF");
				break;
			}
		case LMG_EVENT_TPTSRV:
			{
				len = len + sprintf(prBuf+len, "transport service");
				break;
			}
		case LMG_EVENT_SSAP_ENABLED:
			{
				len = len + sprintf(prBuf+len, "SSAP enabled");
				break;
			}
		case LMG_EVENT_NS_NOT_RESPONDING:
			{
				len = len + sprintf(prBuf+len, "name server not responding");
				break;
			}
		case LMG_EVENT_TPT_FAILED:
			{
				len = len + sprintf(prBuf+len, "transport failure");
				break;
			}
	}

	len = len + sprintf(prBuf+len, " ) ");

	len = len + sprintf(prBuf+len, " cause ( ");
	switch (usta->t.usta.alarm.cause)
	{
		case LCM_CAUSE_UNKNOWN:
			{
				len = len + sprintf(prBuf+len, "unknown");
				break;
			}
		case LCM_CAUSE_OUT_OF_RANGE:
			{
				len = len + sprintf(prBuf+len, "out of range");
				break;
			}
		case LCM_CAUSE_INV_SAP:
			{
				len = len + sprintf(prBuf+len, "NULL/unknown sap");
				break;
			}
		case LCM_CAUSE_INV_SPID:
			{
				len = len + sprintf(prBuf+len, "invalid service provider");
				break;
			}
		case LCM_CAUSE_INV_SUID:
			{
				len = len + sprintf(prBuf+len, "invalid service user");
				break;
			}
		case LCM_CAUSE_INV_NETWORK_MSG:
			{
				len = len + sprintf(prBuf+len, "invalid network message");
				break;
			}
		case LCM_CAUSE_DECODE_ERR:
			{
				len = len + sprintf(prBuf+len, "message decoding problem");
				break;
			}
		case LCM_CAUSE_USER_INITIATED:
			{
				len = len + sprintf(prBuf+len, "user initiated");
				break;
			}
		case LCM_CAUSE_MGMT_INITIATED:
			{
				len = len + sprintf(prBuf+len, "management initiated");
				break;
			}
		case LCM_CAUSE_INV_STATE: /* cause and event! */
			{
				len = len + sprintf(prBuf+len, "invalid state");
				break;
			}
		case LCM_CAUSE_TMR_EXPIRED: /* cause and event! */
			{
				len = len + sprintf(prBuf+len, "timer expired");
				break;
			}
		case LCM_CAUSE_INV_MSG_LENGTH:
			{
				len = len + sprintf(prBuf+len, "invalid message length");
				break;
			}
		case LCM_CAUSE_PROT_NOT_ACTIVE:
			{
				len = len + sprintf(prBuf+len, "protocol layer not active");
				break;
			}
		case LCM_CAUSE_INV_PAR_VAL:
			{
				len = len + sprintf(prBuf+len, "invalid parameter value");
				break;
			}
		case LCM_CAUSE_NEG_CFM:
			{
				len = len + sprintf(prBuf+len, "negative confirmation");
				break;
			}
		case LCM_CAUSE_MEM_ALLOC_FAIL:
			{
				len = len + sprintf(prBuf+len, "memory allocation failure");
				break;
			}
		case LCM_CAUSE_HASH_FAIL:
			{
				len = len + sprintf(prBuf+len, "hashing failure");
				break;
			}
		case LCM_CAUSE_LYR_SPECIFIC:
			{
				len = len + sprintf(prBuf+len, "MGCP specific");
				break;
			}
		default:
			{
				len = len + sprintf(prBuf+len, "unknown %d", (int)(usta->t.usta.alarm.cause));
				break;
			}
		case LMG_CAUSE_TPT_FAILURE: /* make up your mind - cause or event? */
			{
				len = len + sprintf(prBuf+len, "transport failure");
				break;
			}
		case LMG_CAUSE_NS_NOT_RESPONDING:
			{
				len = len + sprintf(prBuf+len, "name server not responding");
				break;
			}
	}
	len = len + sprintf(prBuf+len, "  ) ");

	len = len + sprintf(prBuf+len, "  Alarm parameters ( ");
	ret = smmgGetAlarmInfoField(&usta->t.usta);
	switch (ret)
	{
		case SMMG_UNKNOWNFIELD:
			{
				len = len + sprintf(prBuf+len, "invalid ");

				break;
			}
		case SMMG_PEERINFO:
			{
				/* 
				 * Invoke the new function for printing the MgPeerInfo &
				 * delete all print code here 
				 */
				smmgPrntPeerInfo(&(usta->t.usta.alarmInfo.u.peerInfo));
				break;
			}
		case SMMG_SAPID:
			{
				len = len + sprintf(prBuf+len, "SAP ID %d\n", (int)(usta->t.usta.alarmInfo.u.sapId));
				break;
			}
		case SMMG_MEM:
			{
				len = len + sprintf(prBuf+len, "memory region %d pool %d\n",
						(int)(usta->t.usta.alarmInfo.u.mem.region),
						(int)(usta->t.usta.alarmInfo.u.mem.pool));

				break;
			}
		case SMMG_SRVSTA:
			{
				smmgPrntSrvSta(&usta->t.usta.alarmInfo.u.srvSta);
				break;
			}
		case SMMG_PEERSTA:
			{
				smmgPrntPeerSta(&usta->t.usta.alarmInfo.u.peerSta);
				break;
			}
		case SMMG_SSAPSTA:
			{
				smmgPrntSsapSta(&usta->t.usta.alarmInfo.u.ssapSta);
				break;
			}
		case SMMG_PARID:
			{
				len = len + sprintf(prBuf+len,  "parameter type: ");
				switch (usta->t.usta.alarmInfo.u.parId.parType)
				{
					case LMG_PAR_TPTADDR: len = len + sprintf(prBuf+len, "transport address"); break;
					case LMG_PAR_MBUF:    len = len + sprintf(prBuf+len, "message buffer"); break;
					case LMG_PAR_CHOICE:  len = len + sprintf(prBuf+len, "choice"); break;
					case LMG_PAR_SPID:    len = len + sprintf(prBuf+len, "spId"); break;
					default:              len = len + sprintf(prBuf+len, "unknown"); break;
				}

				len = len + sprintf(prBuf+len, ", value %d\n", 
						(int)(usta->t.usta.alarmInfo.u.parId.u.sapId));

				break;
			}
		case SMMG_NOT_APPL:
			{
				len = len + sprintf(prBuf+len, "not applicable\n");
				break;
			}

			/*TODO*/
	}
	len = len + sprintf(prBuf+len, "  ) ");
	len = len + sprintf(prBuf+len, "  \n ");
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s \n", prBuf);
}

/*****************************************************************************************************************************/
void handle_tucl_alarm(Pst *pst, HiMngmt *sta)
{
	/* To print the general information */
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Recieved a status indication from TUCL layer \n\n");
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " Category = %d , event = %d , cause = %d\n", 
			sta->t.usta.alarm.category, 
			sta->t.usta.alarm.event, sta->t.usta.alarm.cause);

	switch(sta->t.usta.alarm.event)
	{
		case LCM_EVENT_INV_EVT: 
			{ 
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," [HI_USTA]: LCM_EVENT_INV_EVT with type (%d)\n\n",
						sta->t.usta.info.type);
				break;
			}
		case LHI_EVENT_BNDREQ:
			{ 
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," [HI_USTA]: LHI_EVENT_BNDREQ with type (%d) spId (%d)\n\n",
						sta->t.usta.info.type, sta->t.usta.info.spId);
				break;
			}
		case LHI_EVENT_SERVOPENREQ:
		case LHI_EVENT_DATREQ:
		case LHI_EVENT_UDATREQ:
		case LHI_EVENT_CONREQ:
		case LHI_EVENT_DISCREQ:
#if(defined(HI_TLS) && defined(HI_TCP_TLS)) 
		case LHI_EVENT_TLS_ESTREQ:
#endif
			{
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO," [HI_USTA]: partype (%d) type(%d)\n\n",
						sta->t.usta.info.inf.parType, sta->t.usta.info.type);
				break;
			}
		case LCM_EVENT_DMEM_ALLOC_FAIL:
		case LCM_EVENT_SMEM_ALLOC_FAIL:
			{
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, " [HI_USTA]: MEM_ALLOC_FAIL with region(%d) pool (%d) type(%d)\n\n",
						sta->t.usta.info.inf.mem.region, sta->t.usta.info.inf.mem.pool,
						sta->t.usta.info.type);
				break;
			}
		default:
			break;
	}

}   /* handle_sng_tucl_alarm */
/******************************************************************************/

int sng_mgco_mg_get_status(int elemId, MgMngmt* cfm, int mg_cfg_idx)
{
	Pst pst;
	MgMngmt cntrl;
	sng_mg_cfg_t*  mgCfg  = &megaco_globals.g_mg_cfg.mgCfg[mg_cfg_idx];
	sng_mg_peer_t*  mgPeer = &megaco_globals.g_mg_cfg.mgPeer.peers[mgCfg->peer_id];
	CmInetIpAddr   ipAddr = 0;

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(MgCntrl));

	smPstInit(&pst);

	pst.dstEnt = ENTMG;

	/* prepare header */
	/*cntrl.hdr.msgType     = TCNTRL;  */       /* message type */
	cntrl.hdr.entId.ent   = ENTMG;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = elemId;       /* General */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	switch(elemId)
	{
		case STGCPENT:
			{
				cntrl.t.ssta.s.mgPeerSta.peerId.pres = PRSNT_NODEF;
				cntrl.t.ssta.s.mgPeerSta.peerId.val = mgCfg->peer_id;

				cntrl.t.ssta.s.mgPeerSta.mid.pres = PRSNT_NODEF;
				cntrl.t.ssta.s.mgPeerSta.mid.len  = strlen((char*)mgPeer->mid);
				cmMemcpy((U8 *)cntrl.t.ssta.s.mgPeerSta.mid.val, 
						(CONSTANT U8*)(char*)mgPeer->mid, 
					 	cntrl.t.ssta.s.mgPeerSta.mid.len);	
				break;
			}
		case STSSAP:
			{
				cntrl.t.ssta.s.mgSSAPSta.sapId = mgCfg->id; 
				break;
			}
		case STTSAP:
			{
				cntrl.t.ssta.s.mgTSAPSta.tSapId = GET_TPT_ID(mg_cfg_idx);
				break;
			}
		case STSERVER:
			{
				cntrl.t.ssta.s.mgTptSrvSta.tptAddr.type =  CM_INET_IPV4ADDR_TYPE;
				cntrl.t.ssta.s.mgTptSrvSta.tptAddr.u.ipv4TptAddr.port = ntohl(ipAddr);
				if(ROK == cmInetAddr((S8*)mgCfg->my_ipaddr, &ipAddr))
				{
					cntrl.t.ssta.s.mgTptSrvSta.tptAddr.u.ipv4TptAddr.address = ntohl(ipAddr);
				}


				break;
			}
		default:
			break;
	}

	return (sng_sta_mg (&pst, &cntrl, cfm));
}
/******************************************************************************/
switch_status_t megaco_profile_status(switch_stream_handle_t *stream, const char* profilename)
{
	int idx   = 0x00;
	int len   = 0x00;
	MgMngmt   cfm;
	char 	  prntBuf[1024];

	switch_assert(profilename);

	memset((U8 *)&cfm, 0, sizeof(cfm));
	memset((char *)&prntBuf, 0, sizeof(prntBuf));

	GET_MG_CFG_IDX(profilename, idx);

	if(!idx || (idx == MAX_MG_PROFILES)){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR," No MG configuration found against profilename[%s]\n",profilename);
		return SWITCH_STATUS_FALSE;
	}

	/*stream->write_function(stream, "Collecting MG Profile[%s] status... \n",profilename);*/

	/* Fetch data from Trillium MEGACO Stack 	*
	 * SystemId - Software version information 	*
	 * SSAP     - MG SAP Information 		*
	 * TSAP     - MG Transport SAP Information 	*
	 * Peer     - MG Peer Information 		*
	 * TPT-Server - MG Transport Server information *
	 */ 

#if 0
	/* get System ID */
	sng_mgco_mg_get_status(STSID, &cfm, idx);
	stream->write_function(stream, "***********************************************\n");
	stream->write_function(stream, "**** TRILLIUM MEGACO Software Information *****\n");
	stream->write_function(stream, "Version 	  = %d \n", cfm.t.ssta.s.systemId.mVer);
	stream->write_function(stream, "Version Revision  = %d \n", cfm.t.ssta.s.systemId.mRev);
	stream->write_function(stream, "Branch  Version   = %d \n", cfm.t.ssta.s.systemId.bVer);
	stream->write_function(stream, "Branch  Revision  = %d \n", cfm.t.ssta.s.systemId.bRev);
	stream->write_function(stream, "Part    Number    = %d \n", cfm.t.ssta.s.systemId.ptNmb);
	stream->write_function(stream, "***********************************************\n");
#endif

	/* MG Peer Information */
	sng_mgco_mg_get_status(STGCPENT, &cfm, idx);
	smmgPrntPeerSta(&cfm.t.ssta.s.mgPeerSta);

	/* MG Peer Information */
	sng_mgco_mg_get_status(STSSAP, &cfm, idx);
	smmgPrntSsapSta(&cfm.t.ssta.s.mgSSAPSta);

	/* MG Transport SAP Information */
	sng_mgco_mg_get_status(STTSAP, &cfm, idx);
	len = len + sprintf(prntBuf+len,"***********************************************\n"); 
	len = len + sprintf(prntBuf+len,"**********MG TRANSPORT SAP Information**********\n");
	len = len + sprintf(prntBuf+len,"TSAP status:\n");
	len = len + sprintf(prntBuf+len,"state = %d, number of listeners %u\n",
			(int)(cfm.t.ssta.s.mgTSAPSta.state),
			(unsigned int)(cfm.t.ssta.s.mgTSAPSta.numServers));
	len = len + sprintf(prntBuf+len,"***********************************************\n"); 
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"%s\n",prntBuf); 

	/* MG Transport Server Information */
	sng_mgco_mg_get_status(STSERVER, &cfm, idx);
	smmgPrntSrvSta(&cfm.t.ssta.s.mgTptSrvSta);

	return SWITCH_STATUS_SUCCESS;
}
/******************************************************************************/
