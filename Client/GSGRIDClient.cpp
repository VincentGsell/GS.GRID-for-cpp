//---------------------------------------------------------------------------

#include <sstream>
#include <string>
//#include <vector>
//#include <sstream>
#include <algorithm>
#include <stdint.h>

#include <locale>
#include <codecvt>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <ws2tcpip.h>

#include "GSStream.h"
#include "GSGRIDClient.h"




std::string toUtf8(const std::wstring &str)
{
	std::string ret;
	int len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0, NULL, NULL);
	if (len > 0)
	{
		ret.resize(len);
		WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.length(), &ret[0], len, NULL, NULL);
	}
	return ret;
}

GSGRIDClient::GSGRIDClient()
{
	Transport = new(GSTransportTCP);
	Protocol = new(GSProtocolKissB);
}

GSGRIDClient::~GSGRIDClient()
{
	delete Transport;
	delete Protocol;
}


string GSGRIDClient::lastError()
{
	return _lastError;
}


bool GSGRIDClient::connect(string IP, int port, string user, string password)
{
	bool response = false;
	char localBuffer[1024];

	//<---TGRIDProtocol_KB_SRV_NEGOCIATE_HALF_RESPONSE

	uint32_t err;
	uint32_t l;
	if (Transport->connect(IP, port, &err))
	{
		GSMemoryStream* buf = Protocol->TGRIDProtocol_KB_CLT_NEGOCIATE();
		Transport->send((char*)buf->data(), buf->size(), false);
		delete buf;

		Transport->receive((char*)localBuffer,&l);
		GSMemoryStream received;
		received.loadFromBuffer(localBuffer, l);
		TGRIDProtocol_KB_ConnResp* a = Protocol->TGRIDProtocol_KB_CLT_NEGOCIATE_ServerResponse(received);
		if (a->command == TKBCltCommand::_connect)
		{
			_lastError = a->statusError;
			if (a->status) //connection OK
			{
				GSMemoryStream* buf = Protocol->TGRIDProtocol_KB_CLT_NEGOCIATE_UP();
				Transport->send((char*)buf->data(),buf->size());
				delete buf;
				
				Transport->receive((char*)localBuffer,&l);
				received.clear();
				received.loadFromBuffer(localBuffer, l);
				delete a;
				a = Protocol->TGRIDProtocol_KB_CLT_NEGOCIATE_ServerResponse(received);
				_lastError = a->statusError;
				response = (a->command == TKBCltCommand::_connectup) && (a->status);
				if (a->command != TKBCltCommand::_connectup)
				{
					throw "A1 protocolError";
				}
			}
		}
		else
		{
			throw "A0 protocolError";
		}
		delete a;
	}
	else
	{
		std::stringstream abc;
		abc << err;
		_lastError = "Connect error (code " + abc.str() + ")"; 
	}

	return response;
};


TGRIDProtocol_KB_SRV_PROCESS_API_INFO* GSGRIDClient::infos()
{
	uint32_t  len;
	char localBuffer[2048];
	GSMemoryStream* request = new(GSMemoryStream);
	GSMemoryStream receive;

	request = Protocol->TGRIDProtocol_KB_CLT_PROCESS_SPL_API(Protocol->CST_COMMANDID_GRID_API_SrvInfo, NULL);
	Transport->send((char*)request->data(), request->size());
	delete request;
	Transport->receive((char*)localBuffer, &len);

	receive.loadFromBuffer(localBuffer, len);

	TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE* a = new(TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE);
	a->load(receive);

	TGRIDProtocol_KB_SRV_PROCESS_API_INFO* b = new(TGRIDProtocol_KB_SRV_PROCESS_API_INFO);
	receive.clear();
	receive.loadFromStream(a->resultPayload);
	delete a;
	
	b->load(receive);
	return b;
};


void GSGRIDClient::InternalGetCommandAndParse(bool untilReachCommand,
	TKBCltCommand_FromServer commandToReach,
	uint32_t timeOut)
{
	uint32_t  len;
	char localBuffer[2048];
	GSMemoryStream receive;

	Transport->receive((char*)localBuffer, &len);

	receive.loadFromBuffer(localBuffer, len);

	receive.seekStart();
	TKBCltCommand_FromServer header = TKBCltCommand_FromServer(receive.readByte());

	switch (header)
	{
	case TKBCltCommand_FromServer::_connect_resp:;
	case TKBCltCommand_FromServer::_connectup_resp:;
	case TKBCltCommand_FromServer::_process_rpc_simple_srvinfo:;
	case TKBCltCommand_FromServer::_process_rpc_simple_srvinfocpulevel:;
	case TKBCltCommand_FromServer::_process_rpc_simple_KV:;
	case TKBCltCommand_FromServer::_process_rpc_simple_InstantPythonVersion:;
	case TKBCltCommand_FromServer::_process_rpc_simple_InstantPythonRun:;
	default: throw "invalid protocol";
	}

}


double GSGRIDClient::infoCPULevel()
{
	GSMemoryStream* request = new(GSMemoryStream);
	request = Protocol->TGRIDProtocol_KB_CLT_PROCESS_SPL_API(Protocol->CST_COMMANDID_GRID_API_SrvInfoCpuLevel, NULL);
	Transport->send((char*)request->data(), request->size());
	delete request;

	InternalGetCommandAndParse(true, TKBCltCommand_FromServer::_process_rpc_simple_srvinfocpulevel, 250);
	return 0.0;
};

