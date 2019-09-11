//---------------------------------------------------------------------------
#ifndef GSGRIDClientH
#define GSGRIDClientH
//---------------------------------------------------------------------------

#include <string>
#include <vector>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>

#include "GSTransportTCP.h"
#include "GSProtocolKissB.h"

using namespace std;


struct GSGRIDMessage
{
	string from;
	string channel;
	GSMemoryStream* payload;
	uint64_t ticks;

	GSGRIDMessage()
	{
		from = "";
		channel = "";
		ticks = 0;
		payload = new(GSMemoryStream);
	}
	~GSGRIDMessage()
	{ 
		delete payload;
	}
};

typedef vector<GSGRIDMessage> GSGRIDMessages;

class GSGRIDClient
{
	protected:
		TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE* _QUERYRESP = new(TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE);
		TGRIDProtocol_KB_SRV_PROCESS_API_INFO* _INFO_API_CACHE = new(TGRIDProtocol_KB_SRV_PROCESS_API_INFO);
		std::string FSplProcessStep_PythonVersion = u8"";
		std::string FSplProcessStep_PythonRun = u8"";
		double _INFO_CPUVALUE = 0;
		GSGRIDMessages internalMessages;
		char* sockLocalBuffer; 


		string _lastError = "";

		void InternalGetCommandAndParse(bool untilReachCommand,
			TKBCltCommand_FromServer commandToReach,
			uint32_t timeOut = 2500);
		bool internalSendMessage(const string channel, GSMemoryStream* payLoad);
		bool internalSubUnsub(const string channel,const bool subscribe = true);
public:
		GSGRIDClient();
		~GSGRIDClient();
		
		bool connect(string IP, int port, string user="admin", string password = "admin");
		
		TGRIDProtocol_KB_SRV_PROCESS_API_INFO* infos();
		double infoCPULevel();
		string instantPythonVersion();
		string instantPythonRun(string code);
		bool sendMessage(const string channel, GSMemoryStream* payLoad);
		bool subscribe(const string channel);
		bool unsubscribe(const string channel);
		bool checkMsg(GSGRIDMessages& messages, uint32_t timeOut = 0);

		GSTransportTCP* Transport;
		GSProtocolKissB* Protocol;

		string lastError();
};

#endif

