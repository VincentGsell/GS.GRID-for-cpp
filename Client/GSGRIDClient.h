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
		string _lastError = "";

		void InternalGetCommandAndParse(bool untilReachCommand,
			TKBCltCommand_FromServer commandToReach,
			uint32_t timeOut = 2500);
public:
		GSGRIDClient();
		~GSGRIDClient();
		
		bool connect(string IP, int port, string user="admin", string password = "admin");
		
		TGRIDProtocol_KB_SRV_PROCESS_API_INFO* infos();
		double infoCPULevel();



		GSTransportTCP* Transport;
		GSProtocolKissB* Protocol;

		string lastError();
};

#endif

