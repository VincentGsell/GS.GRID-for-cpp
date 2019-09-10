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

	delete _INFO_API_CACHE;
	delete _QUERYRESP;
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

	_INFO_API_CACHE->clear();
	InternalGetCommandAndParse(true, TKBCltCommand_FromServer::_process_rpc_simple_srvinfo);
	if (!_QUERYRESP->status)
		throw _QUERYRESP->statusInfo;
	return _INFO_API_CACHE;

}


double GSGRIDClient::infoCPULevel()
{
	GSMemoryStream* request = new(GSMemoryStream);
	request = Protocol->TGRIDProtocol_KB_CLT_PROCESS_SPL_API(Protocol->CST_COMMANDID_GRID_API_SrvInfoCpuLevel, NULL);
	Transport->send((char*)request->data(), request->size());
	delete request;

	InternalGetCommandAndParse(true, TKBCltCommand_FromServer::_process_rpc_simple_srvinfocpulevel);
	return _INFO_CPUVALUE;
}

string GSGRIDClient::instantPythonVersion()
{
	GSMemoryStream* request = new(GSMemoryStream);
	request = Protocol->TGRIDProtocol_KB_CLT_PROCESS_SPL_API(Protocol->CST_COMMANDID_GRID_API_InstantPythonVersion, NULL);
	Transport->send((char*)request->data(), request->size());
	delete request;

	InternalGetCommandAndParse(true, TKBCltCommand_FromServer::_process_rpc_simple_InstantPythonVersion);
	return FSplProcessStep_PythonVersion;
}

string GSGRIDClient::instantPythonRun(string code)
{
	GSMemoryStream* request = new(GSMemoryStream);
	GSMemoryStream* codeStream = new(GSMemoryStream);
	codeStream->writeRawString(code);
	request = Protocol->TGRIDProtocol_KB_CLT_PROCESS_SPL_API(Protocol->CST_COMMANDID_GRID_API_InstantPythonRun, codeStream);
	Transport->send((char*)request->data(), request->size());
	delete request;
	delete codeStream;

	InternalGetCommandAndParse(true, TKBCltCommand_FromServer::_process_rpc_simple_InstantPythonRun);
	if (_QUERYRESP->status)
		return FSplProcessStep_PythonRun;
	else
		return _QUERYRESP->statusInfo;
}


bool GSGRIDClient::internalSendMessage(const string channel, GSMemoryStream* payLoad)
{
	GSMemoryStream* mess = Protocol->TGRIDProtocol_KB_CLT_BUS_CMD(TKBCltBusCmd::_sendmsg, channel, payLoad);
	uint32_t sended = Transport->send((char*)mess->data(), mess->size());
	return (sended == mess->size());
	delete mess;
}

bool GSGRIDClient::sendMessage(const string channel, GSMemoryStream* payLoad)
{
	return internalSendMessage(channel, payLoad);
}



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

	_QUERYRESP->clear();
	_QUERYRESP->load(receive);
	receive.clear();

	if (_QUERYRESP->status)
	{
		TKBCltCommand_FromServer header = TKBCltCommand_FromServer(_QUERYRESP->header);

		switch (header)
		{
		case TKBCltCommand_FromServer::_connect_resp:break;
		case TKBCltCommand_FromServer::_connectup_resp:break;
		case TKBCltCommand_FromServer::_process_rpc_simple_srvinfo:
		{
			receive.loadFromStream(_QUERYRESP->resultPayload);
			_INFO_API_CACHE->load(receive);
			break;
		};
		case TKBCltCommand_FromServer::_process_rpc_simple_srvinfocpulevel:
		{
			receive.loadFromStream(_QUERYRESP->resultPayload);
			receive.seekStart();
			_INFO_CPUVALUE = receive.readDouble();
			break;
		};
		case TKBCltCommand_FromServer::_process_rpc_simple_KV:break;
		case TKBCltCommand_FromServer::_process_rpc_simple_InstantPythonVersion:
		{
			receive.loadFromStream(_QUERYRESP->resultPayload);
			receive.seekStart();
			FSplProcessStep_PythonVersion = receive.readRawString();
			break;
		};
		case TKBCltCommand_FromServer::_process_rpc_simple_InstantPythonRun: 
		{
			receive.loadFromStream(_QUERYRESP->resultPayload);
			receive.seekStart();
			FSplProcessStep_PythonRun = receive.readRawString();
			break; 
		}
		default: throw "protocol error" ;
		}

	}
		
}

