#pragma once

#include "net_common.h"
#include "net_session.h"
#include "..\Client\cli_msg.h"


namespace net
{
	enum class clientSysMsg
	{
		nickname
	};

	class Client
	{
	public:
		Client()
		{}

		~Client()
		{
			Disconnect();
		}

		bool Connect(const std::string &serverIP, const uint16_t port, HWND hwndDlg)
		{
			try
			{
				clientWindow = hwndDlg;

				asio::ip::tcp::resolver resolver(m_context);
				asio::ip::tcp::resolver::results_type endpoints =
					resolver.resolve(serverIP, std::to_string(port));

				m_Session = std::make_shared<session>(ownership::client, m_context,
														asio::ip::tcp::socket(m_context),
														0, m_IncomingMsgQueue);

				m_Session->ConnectToServer(endpoints);

				m_asioThreads = std::thread([this]() { m_context.run(); });

				m_loopThread = std::thread(&Client::ProcessIncomingMsg, this);
			}
			catch (std::exception &e)
			{
				std::cerr << "Connect Error: " << e.what() << "\n";
				return false;
			}
			return true;
		}

		void Disconnect()
		{
			if (m_Session == nullptr)
				return;

			m_Session->Disconnect();

			m_context.stop();

			if (m_asioThreads.joinable())
				m_asioThreads.join();

			m_Session.reset();

			if (m_loopThread.joinable())
				m_loopThread.join();
		}

		void SendMsg(const message &msg)
		{
			if (m_Session->IsConnected())
				m_Session->sendMsg(msg);
			else
				m_Session.reset();
		}

		void SetNickname(wchar_t *nickname)
		{
			m_nickname = nickname;
			m_Session->SetNickname(nickname);
		}

		void ProcessIncomingMsg()
		{
			while (1)
			{
				m_IncomingMsgQueue.wait();

				MsgWithSender item = m_IncomingMsgQueue.pop_front();

				message temp;

				switch (item.msg.header.ID)
				{
				case net::msgID::text:
					bufferSize = item.msg.header.size + sizeof(wchar_t);
					buffer = (uint8_t *)malloc(bufferSize);
					item.msg.GetData(buffer);
					buffer[bufferSize-2] = '\0';
					buffer[bufferSize-1] = '\0';
					PostMessage(clientWindow, cliCmd_text, 0, (LPARAM)buffer);
					break;

				case net::msgID::connected:
					PostMessage(clientWindow, cliCmd_connected, 0, 0);
					uint32_t id;
					item.msg.GetData((uint8_t *)&id);
					m_Session->SetID(id);
					temp.header.ID = msgID::mynickname;
					temp.FillData(m_nickname.size() * sizeof(wchar_t), (uint8_t *)(m_nickname.data()));
					SendMsg(temp);
					break;

				case net::msgID::welcomeNew:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.GetData((uint8_t *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					PostMessage(clientWindow, cliCmd_newguy, 0, (LPARAM)p);
					break;
				}

				case net::msgID::updatelist:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.GetData((uint8_t *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					PostMessage(clientWindow, cliCmd_updatelist, 0, (LPARAM)p);
					break;
				}

				case net::msgID::leave:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.GetData((uint8_t *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					PostMessage(clientWindow, cliCmd_leave, 0, (LPARAM)p);
					break;
				}

				case net::msgID::exit:
					temp.header.ID = msgID::exit;
					SendMsg(temp);
					return;
				}
			}
		}

		void SysCmd(int msg, LPARAM lParam)
		{
			switch (msg)
			{
			case cliCmd_exit:
				m_IncomingMsgQueue.push_front({ {msgID::exit}, nullptr });
				break;
			}
			
		}

	private:
		asio::io_context m_context;
		std::thread m_asioThreads, m_loopThread;
		std::shared_ptr<session> m_Session;
		msgQueue<MsgWithSender> m_IncomingMsgQueue;
		size_t bufferSize = 0;
		uint8_t *buffer;
		HWND clientWindow;
		std::wstring m_nickname;
	};
}