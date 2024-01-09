#pragma once

#include "NetCommon.h"
#include "NetSession.h"


namespace net
{
	enum class ClientSysMsg:UINT
	{
		NICKNAME = WM_APP,
		CONNECTED,
		TEXT,
		NEW,
		UPDATE_LIST,
		LEAVE // Someone else has left.
	};

	class Client
	{
	public:
		Client()
		{}

		~Client()
		{
			disconnect();
		}

		bool connect(const std::string &serverIP, const uint16_t port, HWND hDlg)
		{
			try
			{
				m_clientWindow = hDlg;

				asio::ip::tcp::resolver resolver(m_context);
				asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(serverIP, std::to_string(port));

				asio::ip::tcp::socket sock(m_context);
				m_session = std::make_shared<Session>(Owner::CLIENT, m_context, sock, 0, m_incomingMsgQueue);

				m_session->connectToServer(endpoints);

				m_asioThreads = std::thread([this]() { m_context.run(); });

				m_loopThread = std::thread(&Client::processIncomingMsg, this);
			}
			catch (std::exception &e)
			{
				std::cerr << "Connect Error: " << e.what() << std::endl;
				return false;
			}
			return true;
		}

		void disconnect()
		{
			if (m_session == nullptr)
				return;

			m_session->disconnect();

			m_context.stop();

			if (m_asioThreads.joinable())
				m_asioThreads.join();

			m_session.reset();

			if (m_loopThread.joinable())
				m_loopThread.join();
		}

		void exit()
		{
			m_incomingMsgQueue.pushFront({ {MsgID::STOP}, nullptr });
		}

		void sendMsg(const Message &msg)
		{
			if (m_session->isConnected())
				m_session->sendMsg(msg);
			else
				m_session.reset();
		}

		void setNickname(wchar_t *nickname)
		{
			m_nickname = nickname;
			m_session->setNickname(nickname);
		}

		void processIncomingMsg()
		{
			while (1)
			{
				MsgWithSender item = m_incomingMsgQueue.popFront();

				Message temp;

				switch (item.msg.header.ID)
				{
				case net::MsgID::TEXT:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.getData((BYTE *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					PostMessage(m_clientWindow, (UINT)ClientSysMsg::TEXT, 0, (LPARAM)p);
					break;
				}
				case net::MsgID::CONNECTED:
				{
					PostMessage(m_clientWindow, (UINT)ClientSysMsg::CONNECTED, 0, 0);
					uint64_t id;
					item.msg.getData((BYTE *)&id);
					m_session->setID(id);
					temp.header.ID = MsgID::MY_NICKNAME;
					temp.fillData(m_nickname.size() * sizeof(wchar_t), (BYTE *)(m_nickname.data()));
					sendMsg(temp);
					break;
				}
				case net::MsgID::WELCOME_NEW:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.getData((BYTE *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					PostMessage(m_clientWindow, (UINT)ClientSysMsg::NEW, 0, (LPARAM)p);
					break;
				}
				case net::MsgID::UPDATE_LIST:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.getData((BYTE *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					PostMessage(m_clientWindow, (UINT)ClientSysMsg::UPDATE_LIST, 0, (LPARAM)p);
					break;
				}
				case net::MsgID::LEAVE:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.getData((BYTE *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					PostMessage(m_clientWindow, (UINT)ClientSysMsg::LEAVE, 0, (LPARAM)p);
					break;
				}
				case net::MsgID::STOP:
					temp.header.ID = MsgID::EXIT;
					sendMsg(temp);
					return;
				}
			}
		}


	private:
		asio::io_context m_context;
		std::thread m_asioThreads, m_loopThread;
		std::shared_ptr<Session> m_session;
		MsgQueue<MsgWithSender> m_incomingMsgQueue;
		HWND m_clientWindow;
		std::wstring m_nickname;
	};
}