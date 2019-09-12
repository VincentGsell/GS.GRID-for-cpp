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

	//<---TGRIDProtocol_KB_SRV_NEGOCIATE_HALF_RESPONSE

	uint32_t err;
	if (Transport->connect(IP, port, &err))
	{
		GSMemoryStream* buf = Protocol->TGRIDProtocol_KB_CLT_NEGOCIATE();
		Transport->send((char*)buf->data(), buf->size(), false);
		delete buf;

		GSMemoryStream received;
		Transport->receive(received);
		received.seekStart();
		TGRIDProtocol_KB_ConnResp* a = Protocol->TGRIDProtocol_KB_CLT_NEGOCIATE_ServerResponse(received);
		if (a->command == TKBCltCommand::_connect)
		{
			_lastError = a->statusError;
			if (a->status) //connection OK
			{
				GSMemoryStream* buf = Protocol->TGRIDProtocol_KB_CLT_NEGOCIATE_UP();
				Transport->send((char*)buf->data(),buf->size());
				delete buf;
				
				Transport->receive(received);
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
	GSMemoryStream* request;
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

bool GSGRIDClient::internalSubUnsub(const string channel, const bool subscribe)
{
	TKBCltCommand_FromServer lsSide;
	GSMemoryStream* mess;
	if (subscribe)
	{
		mess = Protocol->TGRIDProtocol_KB_CLT_BUS_CMD(TKBCltBusCmd::_sub, channel, NULL);
		lsSide = TKBCltCommand_FromServer::_bus_sub;
	}
	else
	{
		mess = Protocol->TGRIDProtocol_KB_CLT_BUS_CMD(TKBCltBusCmd::_unsub, channel, NULL);
		lsSide = TKBCltCommand_FromServer::_bus_unsub;
	}
	uint32_t sended = Transport->send((char*)mess->data(), mess->size());
	delete mess;
	InternalGetCommandAndParse(true, lsSide);
	return (_QUERYBUSRESP->status);
}

bool GSGRIDClient::subscribe(const string channel)
{
	return internalSubUnsub(channel, true);
}

bool GSGRIDClient::unsubscribe(const string channel)
{
	return internalSubUnsub(channel, false);
}


bool GSGRIDClient::internalSendMessage(const string channel, GSMemoryStream* payLoad)
{
	GSMemoryStream* mess = Protocol->TGRIDProtocol_KB_CLT_BUS_CMD(TKBCltBusCmd::_sendmsg, channel, payLoad);
	uint32_t sended = Transport->send((char*)mess->data(), mess->size());
	//InternalGetCommandAndParse(true, lsSide); //No wait for return (fast), got it on checkmsg process.
	return (sended > 0);
	delete mess;
}

bool GSGRIDClient::sendMessage(const string channel, GSMemoryStream* payLoad)
{
	return internalSendMessage(channel, payLoad);
}

//To be called for example, in a (thread loop...)
//true only if msg count>0
bool GSGRIDClient::checkMsg(GSGRIDMessages& messages, uint32_t timeOut)
{
	InternalGetCommandAndParse(false, TKBCltCommand_FromServer::_kfnone, timeOut);
	messages.resize(internalMessages.size());
	for (int i(0); i < internalMessages.size(); i++)
	{
		messages[i].from = internalMessages[i].from;
		messages[i].channel = internalMessages[i].channel;
		messages[i].payload->loadFromStream(internalMessages[i].payload);
		messages[i].ticks = internalMessages[i].ticks;
	}
	internalMessages.clear();
	
	return ((&messages != NULL)&&(messages.size() > 0));
}



void GSGRIDClient::InternalGetCommandAndParse(bool untilReachCommand,
	TKBCltCommand_FromServer commandToReach,
	uint32_t timeOut)
{
	GSMemoryStream receive;
	bool _cond = true;
	uint32_t npos = 0;
	TKBCltCommand_FromServer serverHeader;

	do
	{
		Transport->receive(receive, timeOut);
		if (receive.size() == 0)
			exit;
		receive.seekStart();


		do
		{
			npos = receive.seekpos();
			serverHeader = TKBCltCommand_FromServer(receive.readByte());
			receive.setPosition(npos);
			_QUERYRESP->clear();

			if (untilReachCommand)
				if (_cond)
					_cond = serverHeader != commandToReach;


			switch (serverHeader)
			{
				case TKBCltCommand_FromServer::_connect_resp:break;
				case TKBCltCommand_FromServer::_connectup_resp:break;
				case TKBCltCommand_FromServer::_process_rpc_simple_srvinfo:
				{
					_QUERYRESP->load(receive);
					receive.loadFromStream(_QUERYRESP->resultPayload);
					receive.seekStart();
					_INFO_API_CACHE->load(receive);
					break;
				};
				case TKBCltCommand_FromServer::_process_rpc_simple_srvinfocpulevel:
				{
					_QUERYRESP->load(receive);
					receive.loadFromStream(_QUERYRESP->resultPayload);
					receive.seekStart();
					_INFO_CPUVALUE = receive.readDouble();
					break;
				};
				case TKBCltCommand_FromServer::_process_rpc_simple_KV:break;
				case TKBCltCommand_FromServer::_process_rpc_simple_InstantPythonVersion:
				{
					_QUERYRESP->load(receive);
					receive.loadFromStream(_QUERYRESP->resultPayload);
					receive.seekStart();
					FSplProcessStep_PythonVersion = receive.readRawString();
					break;
				};
				case TKBCltCommand_FromServer::_process_rpc_simple_InstantPythonRun:
				{
					_QUERYRESP->load(receive);
					receive.loadFromStream(_QUERYRESP->resultPayload);
					receive.seekStart();
					FSplProcessStep_PythonRun = receive.readRawString();
					break;
				}
				case TKBCltCommand_FromServer::_bus_recv:
				{
					_QUERYBUSRESP->load(receive);
					uint32_t messagesCount = receive.readUint32();
					internalMessages.resize(messagesCount);
					for (int i(0); i < internalMessages.size(); i++)
					{
						internalMessages[i].from = receive.readString();
						internalMessages[i].channel = receive.readString();
						internalMessages[i].payload->loadFromBuffer((char*)receive.data(), receive.size());
						internalMessages[i].ticks = receive.readUint64();
					}
					break;
				}
				case TKBCltCommand_FromServer::_bus_send:
				{
					_QUERYBUSRESP->load(receive);
					//response from server about a previous send : work here for ack management.
					break;
				}
				case TKBCltCommand_FromServer::_bus_sub:
				{
					_QUERYBUSRESP->load(receive);
					break;
				}
				case TKBCltCommand_FromServer::_bus_unsub:
				{
					_QUERYBUSRESP->load(receive);
					break;
				}
				default: throw "protocol error";
			}
		} while (receive.seekpos()<receive.size());
	} while (_cond);
		
}

