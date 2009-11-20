/*
 * Copyright 2008 Arsen Chaloyan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "umcframework.h"
#include "umcsession.h"
#include "synthscenario.h"
#include "recogscenario.h"
#include "recorderscenario.h"
#include "dtmfscenario.h"
#include "unimrcp_client.h"
#include "apt_log.h"

typedef struct
{
	char                      m_SessionId[10];
	char                      m_ScenarioName[128];
	char                      m_ProfileName[128];
	const mrcp_app_message_t* m_pAppMessage;
} UmcTaskMsg;

enum UmcTaskMsgType
{
	UMC_TASK_CLIENT_MSG,
	UMC_TASK_RUN_SESSION_MSG,
	UMC_TASK_KILL_SESSION_MSG,
	UMC_TASK_SHOW_SCENARIOS_MSG,
	UMC_TASK_SHOW_SESSIONS_MSG
};

apt_bool_t UmcProcessMsg(apt_task_t* pTask, apt_task_msg_t* pMsg);
void UmcOnStartComplete(apt_task_t* pTask);
void UmcOnTerminateComplete(apt_task_t* pTask);
apt_bool_t AppMessageHandler(const mrcp_app_message_t* pAppMessage);


UmcFramework::UmcFramework() :
	m_pPool(NULL),
	m_pDirLayout(NULL),
	m_pTask(NULL),
	m_pMrcpClient(NULL),
	m_pMrcpApplication(NULL),
	m_Ready(false),
	m_pScenarioTable(NULL),
	m_pSessionTable(NULL)
{
}

UmcFramework::~UmcFramework()
{
}

bool UmcFramework::Create(apt_dir_layout_t* pDirLayout, apr_pool_t* pool)
{
	m_pDirLayout = pDirLayout;
	m_pPool = pool;

	m_pSessionTable = apr_hash_make(m_pPool);
	m_pScenarioTable = apr_hash_make(m_pPool);
	if(!CreateTask())
		return false;

	/* wait for READY state,
	   preferably cond wait object should be used */
	int attempts = 0;
	while(!m_Ready && attempts < 10)
	{
		attempts++;
		apr_sleep(500000);
	}

	return true;
}

void UmcFramework::Destroy()
{
	DestroyTask();

	m_pScenarioTable = NULL;
	m_pSessionTable = NULL;
}

bool UmcFramework::CreateMrcpClient()
{
	/* create MRCP client stack first */
	m_pMrcpClient = unimrcp_client_create(m_pDirLayout);
	if(!m_pMrcpClient)
		return false;

	/* create MRCP application to send/get requests to/from MRCP client stack */
	m_pMrcpApplication = mrcp_application_create(AppMessageHandler,this,m_pPool);
	if(!m_pMrcpApplication)
	{
		mrcp_client_destroy(m_pMrcpClient);
		m_pMrcpClient = NULL;
		return false;
	}

	/* register MRCP application to MRCP client */
	mrcp_client_application_register(m_pMrcpClient,m_pMrcpApplication,"UMC");
	/* start MRCP client stack processing */
	if(mrcp_client_start(m_pMrcpClient) == FALSE)
	{
		mrcp_client_destroy(m_pMrcpClient);
		m_pMrcpClient = NULL;
		m_pMrcpApplication = NULL;
		return false;
	}
	return true;
}

void UmcFramework::DestroyMrcpClient()
{
	if(m_pMrcpClient)
	{
		/* shutdown MRCP client stack processing first (blocking call) */
		mrcp_client_shutdown(m_pMrcpClient);
		/* destroy MRCP client stack */
		mrcp_client_destroy(m_pMrcpClient);
		m_pMrcpClient = NULL;
		m_pMrcpApplication = NULL;
	}
}

bool UmcFramework::CreateTask()
{
	apt_task_t* pTask;
	apt_task_vtable_t* pVtable;
	apt_task_msg_pool_t* pMsgPool;

	pMsgPool = apt_task_msg_pool_create_dynamic(sizeof(UmcTaskMsg),m_pPool);
	m_pTask = apt_consumer_task_create(this,pMsgPool,m_pPool);
	if(!m_pTask)
		return false;

	pTask = apt_consumer_task_base_get(m_pTask);
	apt_task_name_set(pTask,"Framework Task");
	pVtable = apt_consumer_task_vtable_get(m_pTask);
	if(pVtable) 
	{
		pVtable->process_msg = UmcProcessMsg;
		pVtable->on_start_complete = UmcOnStartComplete;
		pVtable->on_terminate_complete = UmcOnTerminateComplete;
	}

	m_Ready = false;
	apt_task_start(pTask);
	return true;
}

void UmcFramework::DestroyTask()
{
	if(m_pTask)
	{
		apt_task_t* pTask = apt_consumer_task_base_get(m_pTask);
		if(pTask)
		{
			apt_task_terminate(pTask,TRUE);
			apt_task_destroy(pTask);
		}
		m_pTask = NULL;
	}
}

UmcScenario* UmcFramework::CreateScenario(const char* pType)
{
	if(pType)
	{
		if(strcasecmp(pType,"Synthesizer") == 0)
			return new SynthScenario();
		else if(strcasecmp(pType,"Recognizer") == 0)
			return new RecogScenario();
		else if(strcasecmp(pType,"Recorder") == 0)
			return new RecorderScenario();
		else if(strcasecmp(pType,"DtmfRecognizer") == 0)
			return new DtmfScenario();
	}
	return NULL;
}

apr_xml_doc* UmcFramework::LoadDocument()
{
	apr_xml_parser* pParser = NULL;
	apr_xml_doc* pDoc = NULL;
	apr_file_t* pFD = NULL;
	apr_status_t rv;
	const char* pFilePath;

	pFilePath = apr_psprintf(m_pPool,"%s/%s",m_pDirLayout->conf_dir_path,"umcscenarios.xml");

	apt_log(APT_LOG_MARK,APT_PRIO_NOTICE,"Open Config File [%s]",pFilePath);
	rv = apr_file_open(&pFD,pFilePath,APR_READ|APR_BINARY,0,m_pPool);
	if(rv != APR_SUCCESS) 
	{
		apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"Failed to Open Config File [%s]",pFilePath);
		return NULL;
	}

	rv = apr_xml_parse_file(m_pPool,&pParser,&pDoc,pFD,2000);
	if(rv != APR_SUCCESS) 
	{
		apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"Failed to Parse Config File [%s]",pFilePath);
		pDoc = NULL;
	}

	apr_file_close(pFD);
	return pDoc;
}

bool UmcFramework::LoadScenarios()
{
	apr_xml_doc* pDoc = LoadDocument();
	if(!pDoc)
		return false;

	const apr_xml_attr* pAttr;
	const apr_xml_elem* pElem;
	const apr_xml_elem* pRoot = pDoc->root;
	if(!pRoot || strcasecmp(pRoot->name,"umcscenarios") != 0)
	{
		apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"Unknown Document");
		return FALSE;
	}
	for(pElem = pRoot->first_child; pElem; pElem = pElem->next)
	{
		if(strcasecmp(pElem->name,"scenario") != 0)
		{
			apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"Unknown Element <%s>",pElem->name);
			continue;
		}
		
		const char* pName = NULL;
		const char* pClass = NULL;
		const char* pMrcpProfile = NULL;
		for(pAttr = pElem->attr; pAttr; pAttr = pAttr->next) 
		{
			if(strcasecmp(pAttr->name,"name") == 0) 
			{
				pName = pAttr->value;
			}
			else if(strcasecmp(pAttr->name,"class") == 0) 
			{
				pClass = pAttr->value;
			}
			else if(strcasecmp(pAttr->name,"profile") == 0) 
			{
				pMrcpProfile = pAttr->value;
			}
		}

		if(pName && pClass)
		{
			apt_log(APT_LOG_MARK,APT_PRIO_INFO,"Load Scenario name [%s] class [%s]",pName,pClass);
			UmcScenario* pScenario = CreateScenario(pClass);
			if(pScenario)
			{
				pScenario->SetDirLayout(m_pDirLayout);
				pScenario->SetName(pName);
				pScenario->SetMrcpProfile(pMrcpProfile);
				if(pScenario->Load(pElem,m_pPool))
					apr_hash_set(m_pScenarioTable,pScenario->GetName(),APR_HASH_KEY_STRING,pScenario);
				else
					delete pScenario;
			}
			else
			{
				apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"No such scenario <%s>",pClass);
			}
		}
		else
		{
			apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"Missing either name or class of the scenario");
		}
	}
	return true;
}

void UmcFramework::DestroyScenarios()
{
	UmcScenario* pScenario;
	void* pVal;
	apr_hash_index_t* it = apr_hash_first(m_pPool,m_pScenarioTable);
	for(; it; it = apr_hash_next(it)) 
	{
		apr_hash_this(it,NULL,NULL,&pVal);
		pScenario = (UmcScenario*) pVal;
		if(pScenario)
		{
			pScenario->Destroy();
			delete pScenario;
		}
	}
	apr_hash_clear(m_pScenarioTable);
}

bool UmcFramework::AddSession(UmcSession* pSession)
{
	if(!pSession)
		return false;

	apr_hash_set(m_pSessionTable,pSession->GetId(),APR_HASH_KEY_STRING,pSession);
	return true;
}

bool UmcFramework::RemoveSession(UmcSession* pSession)
{
	if(!pSession)
		return false;

	apr_hash_set(m_pSessionTable,pSession->GetId(),APR_HASH_KEY_STRING,NULL);
	return true;
}

bool UmcFramework::ProcessRunRequest(const char* pScenarioName, const char* pProfileName)
{
	UmcScenario* pScenario = (UmcScenario*) apr_hash_get(m_pScenarioTable,pScenarioName,APR_HASH_KEY_STRING);
	if(!pScenario)
		return false;

	UmcSession* pSession = pScenario->CreateSession();
	if(!pSession)
		return false;

	printf("[%s]\n",pSession->GetId());
	pSession->SetMrcpProfile(pProfileName);
	pSession->SetMrcpApplication(m_pMrcpApplication);
	if(!pSession->Run())
	{
		delete pSession;
		return false;
	}

	AddSession(pSession);
	return true;
}

void UmcFramework::ProcessKillRequest(const char* id)
{
	UmcSession* pSession;
	void* pVal;
	apr_hash_index_t* it = apr_hash_first(m_pPool,m_pSessionTable);
	for(; it; it = apr_hash_next(it)) 
	{
		apr_hash_this(it,NULL,NULL,&pVal);
		pSession = (UmcSession*) pVal;
		if(pSession && strcasecmp(pSession->GetId(),id) == 0)
		{
			/* first, terminate session */
			pSession->Terminate();
			return;
		}
	}
}

void UmcFramework::ProcessShowScenarios()
{
	UmcScenario* pScenario;
	void* pVal;
	printf("%d Scenario(s)\n", apr_hash_count(m_pScenarioTable));
	apr_hash_index_t* it = apr_hash_first(m_pPool,m_pScenarioTable);
	for(; it; it = apr_hash_next(it))
	{
		apr_hash_this(it,NULL,NULL,&pVal);
		pScenario = (UmcScenario*) pVal;
		if(pScenario)
		{
			printf("[%s]\n", pScenario->GetName());
		}
	}
}

void UmcFramework::ProcessShowSessions()
{
	UmcSession* pSession;
	void* pVal;
	printf("%d Session(s)\n", apr_hash_count(m_pSessionTable));
	apr_hash_index_t* it = apr_hash_first(m_pPool,m_pSessionTable);
	for(; it; it = apr_hash_next(it)) 
	{
		apr_hash_this(it,NULL,NULL,&pVal);
		pSession = (UmcSession*) pVal;
		if(pSession)
		{
			printf("[%s] - %s\n", pSession->GetId(), pSession->GetScenario()->GetName());
		}
	}
}

void UmcFramework::RunSession(const char* pScenarioName, const char* pProfileName)
{
	apt_task_t* pTask = apt_consumer_task_base_get(m_pTask);
	apt_task_msg_t* pTaskMsg = apt_task_msg_get(pTask);
	if(!pTaskMsg) 
		return;

	pTaskMsg->type = TASK_MSG_USER;
	pTaskMsg->sub_type = UMC_TASK_RUN_SESSION_MSG;
	UmcTaskMsg* pUmcMsg = (UmcTaskMsg*) pTaskMsg->data;
	strncpy(pUmcMsg->m_ScenarioName,pScenarioName,sizeof(pUmcMsg->m_ScenarioName)-1);
	strncpy(pUmcMsg->m_ProfileName,pProfileName,sizeof(pUmcMsg->m_ProfileName)-1);
	pUmcMsg->m_pAppMessage = NULL;
	apt_task_msg_signal(pTask,pTaskMsg);
}

void UmcFramework::KillSession(const char* id)
{
	apt_task_t* pTask = apt_consumer_task_base_get(m_pTask);
	apt_task_msg_t* pTaskMsg = apt_task_msg_get(pTask);
	if(!pTaskMsg) 
		return;

	pTaskMsg->type = TASK_MSG_USER;
	pTaskMsg->sub_type = UMC_TASK_KILL_SESSION_MSG;
	
	UmcTaskMsg* pUmcMsg = (UmcTaskMsg*) pTaskMsg->data;
	strncpy(pUmcMsg->m_SessionId,id,sizeof(pUmcMsg->m_SessionId)-1);
	pUmcMsg->m_pAppMessage = NULL;
	apt_task_msg_signal(pTask,pTaskMsg);
}

void UmcFramework::ShowScenarios()
{
	apt_task_t* pTask = apt_consumer_task_base_get(m_pTask);
	apt_task_msg_t* pTaskMsg = apt_task_msg_get(pTask);
	if(!pTaskMsg) 
		return;

	pTaskMsg->type = TASK_MSG_USER;
	pTaskMsg->sub_type = UMC_TASK_SHOW_SCENARIOS_MSG;
	apt_task_msg_signal(pTask,pTaskMsg);
}

void UmcFramework::ShowSessions()
{
	apt_task_t* pTask = apt_consumer_task_base_get(m_pTask);
	apt_task_msg_t* pTaskMsg = apt_task_msg_get(pTask);
	if(!pTaskMsg) 
		return;

	pTaskMsg->type = TASK_MSG_USER;
	pTaskMsg->sub_type = UMC_TASK_SHOW_SESSIONS_MSG;
	apt_task_msg_signal(pTask,pTaskMsg);
}

apt_bool_t AppMessageHandler(const mrcp_app_message_t* pMessage)
{
	UmcFramework* pFramework = (UmcFramework*) mrcp_application_object_get(pMessage->application);
	if(!pFramework)
		return FALSE;

	apt_task_t* pTask = apt_consumer_task_base_get(pFramework->m_pTask);
	apt_task_msg_t* pTaskMsg = apt_task_msg_get(pTask);
	if(pTaskMsg) 
	{
		pTaskMsg->type = TASK_MSG_USER;
		pTaskMsg->sub_type = UMC_TASK_CLIENT_MSG;
		
		UmcTaskMsg* pUmcMsg = (UmcTaskMsg*) pTaskMsg->data;
		pUmcMsg->m_pAppMessage = pMessage;
		apt_task_msg_signal(pTask,pTaskMsg);
	}
	
	return TRUE;
}


apt_bool_t AppOnSessionUpdate(mrcp_application_t *application, mrcp_session_t *session, mrcp_sig_status_code_e status)
{
	UmcSession* pSession = (UmcSession*) mrcp_application_session_object_get(session);
	return pSession->OnSessionUpdate(status);
}

apt_bool_t AppOnSessionTerminate(mrcp_application_t *application, mrcp_session_t *session, mrcp_sig_status_code_e status)
{
	UmcSession* pSession = (UmcSession*) mrcp_application_session_object_get(session);
	if(!pSession->OnSessionTerminate(status))
		return false;

	UmcFramework* pFramework = (UmcFramework*) mrcp_application_object_get(application);
	pFramework->RemoveSession(pSession);
	delete pSession;
	return true;
}

apt_bool_t AppOnChannelAdd(mrcp_application_t *application, mrcp_session_t *session, mrcp_channel_t *channel, mrcp_sig_status_code_e status)
{
	UmcSession* pSession = (UmcSession*) mrcp_application_session_object_get(session);
	return pSession->OnChannelAdd(channel,status);
}

apt_bool_t AppOnChannelRemove(mrcp_application_t *application, mrcp_session_t *session, mrcp_channel_t *channel, mrcp_sig_status_code_e status)
{
	UmcSession* pSession = (UmcSession*) mrcp_application_session_object_get(session);
	return pSession->OnChannelRemove(channel,status);
}

apt_bool_t AppOnMessageReceive(mrcp_application_t *application, mrcp_session_t *session, mrcp_channel_t *channel, mrcp_message_t *message)
{
	UmcSession* pSession = (UmcSession*) mrcp_application_session_object_get(session);
	return pSession->OnMessageReceive(channel,message);
}

apt_bool_t AppOnTerminateEvent(mrcp_application_t *application, mrcp_session_t *session, mrcp_channel_t *channel)
{
	UmcSession* pSession = (UmcSession*) mrcp_application_session_object_get(session);
	return pSession->OnTerminateEvent(channel);
}

apt_bool_t AppOnResourceDiscover(mrcp_application_t *application, mrcp_session_t *session, mrcp_session_descriptor_t *descriptor, mrcp_sig_status_code_e status)
{
	UmcSession* pSession = (UmcSession*) mrcp_application_session_object_get(session);
	return pSession->OnResourceDiscover(descriptor,status);
}

apt_bool_t AppOnReady(mrcp_application_t *application, mrcp_sig_status_code_e status)
{
	UmcFramework* pFramework = (UmcFramework*) mrcp_application_object_get(application);
	pFramework->m_Ready = true;
	return TRUE;
}

void UmcOnStartComplete(apt_task_t* pTask)
{
	apt_consumer_task_t* pConsumerTask = (apt_consumer_task_t*) apt_task_object_get(pTask);
	UmcFramework* pFramework = (UmcFramework*) apt_consumer_task_object_get(pConsumerTask);
	
	pFramework->CreateMrcpClient();
	pFramework->LoadScenarios();
}

void UmcOnTerminateComplete(apt_task_t* pTask)
{
	apt_consumer_task_t* pConsumerTask = (apt_consumer_task_t*) apt_task_object_get(pTask);
	UmcFramework* pFramework = (UmcFramework*) apt_consumer_task_object_get(pConsumerTask);

	pFramework->DestroyMrcpClient();
	pFramework->DestroyScenarios();
}

apt_bool_t UmcProcessMsg(apt_task_t *pTask, apt_task_msg_t *pMsg)
{
	if(pMsg->type != TASK_MSG_USER)
		return FALSE;

	apt_consumer_task_t* pConsumerTask = (apt_consumer_task_t*) apt_task_object_get(pTask);
	UmcFramework* pFramework = (UmcFramework*) apt_consumer_task_object_get(pConsumerTask);
	UmcTaskMsg* pUmcMsg = (UmcTaskMsg*) pMsg->data;
	switch(pMsg->sub_type) 
	{
		case UMC_TASK_CLIENT_MSG:
		{
			static const mrcp_app_message_dispatcher_t applicationDispatcher = 
			{
				AppOnSessionUpdate,
				AppOnSessionTerminate,
				AppOnChannelAdd,
				AppOnChannelRemove,
				AppOnMessageReceive,
				AppOnReady,
				AppOnTerminateEvent,
				AppOnResourceDiscover
			};

			mrcp_application_message_dispatch(&applicationDispatcher,pUmcMsg->m_pAppMessage);
			break;
		}
		case UMC_TASK_RUN_SESSION_MSG:
		{
			if(pFramework->m_Ready)
				pFramework->ProcessRunRequest(pUmcMsg->m_ScenarioName,pUmcMsg->m_ProfileName);
			break;
		}
		case UMC_TASK_KILL_SESSION_MSG:
		{
			if(pFramework->m_Ready)
				pFramework->ProcessKillRequest(pUmcMsg->m_SessionId);
			break;
		}
		case UMC_TASK_SHOW_SCENARIOS_MSG:
		{
			if(pFramework->m_Ready)
				pFramework->ProcessShowScenarios();
			break;
		}
		case UMC_TASK_SHOW_SESSIONS_MSG:
		{
			if(pFramework->m_Ready)
				pFramework->ProcessShowSessions();
			break;
		}
	}
	return TRUE;
}
