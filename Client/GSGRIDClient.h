//---------------------------------------------------------------------------
#ifndef GSGRIDClientH
#define GSGRIDClientH
//---------------------------------------------------------------------------

#include <string>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>

#include "GSTransportTCP.h"
#include "GSProtocolKissB.h"

using namespace std;


class GSGRIDClient
{
	protected:
		TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE* _QUERYRESP = new(TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE);
		TGRIDProtocol_KB_SRV_PROCESS_API_INFO* _INFO_API_CACHE = new(TGRIDProtocol_KB_SRV_PROCESS_API_INFO);
		std::string FSplProcessStep_PythonVersion = u8"";
		std::string FSplProcessStep_PythonRun = u8"";
		double _INFO_CPUVALUE = 0;

		string _lastError = "";

		void InternalGetCommandAndParse(bool untilReachCommand,
			TKBCltCommand_FromServer commandToReach,
			uint32_t timeOut = 2500);
		bool internalSendMessage(const string channel, GSMemoryStream* payLoad);
public:
		GSGRIDClient();
		~GSGRIDClient();
		
		bool connect(string IP, int port, string user="admin", string password = "admin");
		
		TGRIDProtocol_KB_SRV_PROCESS_API_INFO* infos();
		double infoCPULevel();
		string instantPythonVersion();
		string instantPythonRun(string code);
		bool sendMessage(const string channel, GSMemoryStream* payLoad);


		GSTransportTCP* Transport;
		GSProtocolKissB* Protocol;

		string lastError();
};

#endif

