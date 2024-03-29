// ConsoleApplication5.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//
#include <iostream>
#include <sstream>
#include "GSStream.h"
#include "GSProtocolKissB.h"
#include "GSGRIDClient.h"

using namespace std;

int main()
{
	std::cout << "c++ - GRID Server KissB protocol (Binary) implementation \n";
	cout << "connecting..." << endl;


	GSGRIDClient client;
	if (client.connect("127.0.0.1", 60000))
	{
		cout << "connected !" << std::endl;
		int option = 0;


		do
		{
			cout << "--- Menu" << endl;
			cout << "0 - Exit " << endl;
			cout << "1 - infos " << endl;
			cout << "2 - cpu " << endl;
			cout << "3 - python version " << endl;
			cout << "4 - python run " << endl;
			cout << "5 - subs " << endl;
			cout << "6 - send " << endl;
			cout << "7 - recv " << endl;
			cout << "enter choice " << endl;
			cin >> option;

			switch (option)
			{
				case 0:	
				{
					return 0;
					break;
				}
				case 1: 
				{
					cout << "Test get infos" << std::endl;
					TGRIDProtocol_KB_SRV_PROCESS_API_INFO* a = client.infos();
					cout << a->GRIDArch << std::endl;
					cout << a->GRIDCompiler << std::endl;
					cout << a->GRIDServerName << std::endl;
					cout << a->GRIDServices << std::endl;
					cout << a->GRIDVersion << std::endl;
					cout << a->ServerGenuineName << std::endl;
					cout << a->ServerHostArchitecture << std::endl;
					cout << a->ServerHostCPUArchitecture << std::endl;
					cout << a->ServerHostOS << std::endl;
					cout << a->ServerHostOSBuild << std::endl;
					break;
				}
				case 2: 
				{
					cout << "Test CPU level " << endl;
					cout << client.infoCPULevel() << endl;
					break;
				}
				case 3: 
				{
					cout << "Test Python version" << endl;
					cout << client.instantPythonVersion() << endl;
					cout << endl;					
					break;
				}
				case 4: 
				{
					cout << "Test Python Run" << endl;
					cout << client.instantPythonRun("def gridmain():\n print(\"hello world\".upper())") << endl;
					cout << endl;
					break;
				}
				case 5:
				{
					cout << " Subs topic" << endl;
					if (client.subscribe("test"))
						cout << "subscribted to \"test\"" << endl;
					else
						cout << "NOT subscribted to \"test\"" << endl;
					break;
				}
				case 6:
				{
					cout << "Test Messaging" << endl;
					cout << " Send 20 Messages" << endl;
					GSMemoryStream payload;
					stringstream sm("");
					for (int i = 0; i < 19; i++)
					{
						cout << "  message " << i << endl;
						sm.str("");
						sm << "Hello world " << i << endl;
						payload.clear();
						payload.writeRawString(sm.str());
						client.sendMessage("test", &payload);
					}
					cout << " Sended." << endl;
				}
				case 7:
				{
					cout << " Recv Message" << endl;
					GSGRIDMessages m;
					if (client.checkMsg(m))
					{
						cout << m.size() << " message(s) : " << endl;
						for (uint32_t i(0); i < m.size(); i++)
						{
							cout << "Message" << i + 1 << "from " << m[i].from << "payload size : " << m[i].payload->size() << endl;
						}
					}
					else
						cout << "no message" << endl;
					break;
				}
			}
				

			cout << endl << endl;
		} while (true);
	}
	else
	{
		cout << client.lastError() << std::endl;
		system("pause");
	}

}





