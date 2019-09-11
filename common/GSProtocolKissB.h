//KissB Binary implementation.
#pragma once
#ifndef GSPROTOCOLKISSBH
#define GSPROTOCOLKISSBH

#include "GSProtocol.h"
#include "GSStream.h"

using namespace std;

enum TKBCltCommand {
	_kcnone, 
	_connect, 
	_connectup, 
	_process_rpc_simple, 
	_process,
	_bus 
};


enum TKBCltCommand_FromServer {
	_kfnone,
	_connect_resp,
	_connectup_resp,
	_process_rpc_simple_srvinfo,
	_process_rpc_simple_srvinfocpulevel,
	_process_rpc_simple_InstantPythonRun,
	_process_rpc_simple_InstantPythonVersion,
	_process_rpc_simple_KV,
	_bus_sub,
	_bus_unsub,
	_bus_send,
	_bus_recv
};

enum TKBCltBusCmd {
	_bcnone,
	_sendmsg,
	_sub,
	_unsub,
	_openchan,
	_joinappspace
};

struct TGRIDProtocol_KB_ConnResp
{
	TKBCltCommand command;
	bool status;
	std::string statusError;
};

struct TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE
{
	TKBCltCommand_FromServer header;
	bool status;
	string statusInfo;
	GSMemoryStream* resultPayload = NULL;

	TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE()
	{
		//resultPayload = (char*)malloc(1024);
	}

	~TGRIDProtocol_KB_SRV_PROCESS_SPL_API_RESPONSE()
	{
		//free(resultPayload);
		if (resultPayload != NULL)
			delete resultPayload;
	}

	void load(GSMemoryStream& data)
	{
		data.seekStart();
		header = TKBCltCommand_FromServer(data.readByte());
		status = (data.readByte()>0);
		statusInfo = data.readString();
		resultPayload = data.readMemoryStream();
	};

	void clear()
	{
		header = TKBCltCommand_FromServer::_kfnone;
		status = false;
		statusInfo = "";
		if (resultPayload != NULL)
			resultPayload->clear();
	};
};


struct TGRIDProtocol_KB_SRV_PROCESS_API_INFO
{
	std::string ServerGenuineName;
	std::string ServerHostCPUArchitecture;
	std::string ServerHostArchitecture;
	std::string ServerHostOS;
	std::string ServerHostOSBuild;
	std::string GRIDVersion;
	std::string GRIDServerName;
	std::string GRIDServices; //Delimited list.
	std::string GRIDArch;
	std::string GRIDCompiler;

	TGRIDProtocol_KB_SRV_PROCESS_API_INFO()
	{
	}

	~TGRIDProtocol_KB_SRV_PROCESS_API_INFO()
	{
	}

	void load(GSMemoryStream& data)
	{
		data.seekStart();
		ServerGenuineName = data.readString();
		ServerHostCPUArchitecture = data.readString();
		ServerHostArchitecture = data.readString();
		ServerHostOS = data.readString();
		ServerHostOSBuild = data.readString();
		GRIDVersion = data.readString();
		GRIDServerName = data.readString();
		GRIDServices = data.readString(); //Delimited list.
		GRIDArch = data.readString();
		GRIDCompiler = data.readString();
	}

	void clear()
	{
		ServerGenuineName = "";
		ServerHostCPUArchitecture = "";
		ServerHostArchitecture = "";
		ServerHostOS = "";
		ServerHostOSBuild = "";
		GRIDVersion = "";
		GRIDServerName = "";
		GRIDServices = ""; 
		GRIDArch = "";
		GRIDCompiler = "";
	}
};



class GSProtocolKissB :	public GSProtocol
{
protected:
public:
	const string CST_SIGNATURE = u8"KissB_Grid_Protocol_V1";
	const string CST_COMMANDID_GRID_API_SrvInfo = u8"GRID.API.SrvInfo";
	const string CST_COMMANDID_GRID_API_SrvInfoCpuLevel = u8"GRID.API.SrvInfoCpuLevel";
	const string CST_COMMANDID_GRID_API_InstantPythonRun = u8"GRID.API.InstantPythonRun";
	const string CST_COMMANDID_GRID_API_InstantPythonVersion = u8"GRID.API.InstantPythonVersion";
	const string CST_COMMANDID_GRID_API_KeyValue = u8"GRID.API.KV";


	GSProtocolKissB()
	{

	}

	~GSProtocolKissB()
	{

	}

	GSMemoryStream* TGRIDProtocol_KB_CLT_NEGOCIATE()
	{
		GSMemoryStream* buf = new(GSMemoryStream);  //TGRIDProtocol_KB_CLT_NEGOCIATE--->
		buf->writeByte(TKBCltCommand::_connect);  //Command connect (TKBCltCommand)
		buf->writeString(CST_SIGNATURE); //SIgneture UTF8
		buf->writeByte(1); //VERSION MAJOR
		buf->writeByte(0); //VERSION MINOR
		buf->writeByte(0); //FORMAT (0=Binary, 1=JSON)
		buf->writeByte(0); //COMPRESSION (0=none)
		buf->writeByte(0); //CYPHERING (0=none)
		return buf;
	}

	GSMemoryStream* TGRIDProtocol_KB_CLT_NEGOCIATE_UP(std::string user = "admin", std::string password = "admin")
	{
		GSMemoryStream* buf = new(GSMemoryStream);  //TGRIDProtocol_KB_CLT_NEGOCIATE_UP--->
		buf->writeByte(TKBCltCommand::_connectup);  //Command connectup (TKBCltCommand)
		buf->writeString(user); //username
		buf->writeString(password); //password
		return buf;
	}

	TGRIDProtocol_KB_ConnResp* TGRIDProtocol_KB_CLT_NEGOCIATE_ServerResponse(GSMemoryStream& buffer)
	{
		TGRIDProtocol_KB_ConnResp* a = new(TGRIDProtocol_KB_ConnResp);
		buffer.seekStart();
		a->command = TKBCltCommand(buffer.readByte());
		a->status = (buffer.readByte() > 0);
		a->statusError = buffer.readString();
		return a;
	}


	//Simple API request build.
	GSMemoryStream* TGRIDProtocol_KB_CLT_PROCESS_SPL_API(const string& commandID, GSMemoryStream* subCall)
	{
		GSMemoryStream* buf = new(GSMemoryStream);
		buf->writeByte(TKBCltCommand::_process_rpc_simple);
		buf->writeString(commandID);
		if (subCall != NULL)
		{
			buf->writeUint64(subCall->size());
			buf->loadFromStream(subCall, false);
		}
		else
			buf->writeUint64(0);
		return buf;
	}

	GSMemoryStream* TGRIDProtocol_KB_CLT_BUS_CMD(const TKBCltBusCmd command, string channelInvolved, GSMemoryStream* MessageContent)
	{
		GSMemoryStream* buf = new(GSMemoryStream);
		buf->writeByte(TKBCltCommand::_bus);
		buf->writeByte(command);
		buf->writeString(channelInvolved);
		if (MessageContent != NULL)
		{
			buf->writeUint64(MessageContent->size());
			buf->loadFromStream(MessageContent, false);
		}
		else
			buf->writeUint64(0);
		return buf;
	}




};


#endif