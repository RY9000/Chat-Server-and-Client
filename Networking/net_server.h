#pragma once

#include "net_common.h"
#include "net_session.h"
#include "..\Server\srv_msg.h"


namespace net
{
	class Server
	{
	public:
		Server(asio::ip::port_type port_num, uint32_t num) :
			m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_num))
		{
			numOfThreads = num;
		}

		~Server()
		{
			Stop();
		}

		void Start()
		{
			Listen();

			for (uint32_t i = 1; i <= numOfThreads; i++)
				m_asioThreads.emplace_back([&]() { m_asioContext.run(); });

			m_loopThread = std::thread(&Server::ProcessIncomingMsg, this);

			std::cout << "*** Server Started. ***\n";

			return;
		}

		void Stop()
		{
			m_IncomingMsgQueue.push_front({ {msgID::stop}, nullptr });

			m_asioContext.stop();

			std::scoped_lock lock(cliMutex);

			for (auto client : m_clients)
			{
				client->Disconnect();
				client.reset();
			}

			m_clients.clear();

			for (uint32_t i = 0; i < numOfThreads; i++)
				if (m_asioThreads[i].joinable())
					m_asioThreads[i].join();

			m_asioThreads.clear();

			if (m_loopThread.joinable())
				m_loopThread.join();

			m_IncomingMsgQueue.clear();

			m_asioContext.reset();
			std::cout << "*** Server Stopped. ***\n";
		}

		void Listen()
		{
			m_asioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket)
			{
				if (!ec)
				{
					std::shared_ptr<session> newClient =
						std::make_shared<session>(ownership::server, m_asioContext,
												  std::move(socket), IDCounter++,
												  m_IncomingMsgQueue);

					newClient->ValidateClient();

					std::scoped_lock lock(cliMutex);

					m_clients.insert(std::move(newClient));

				}
				else
					std::cout << "Accepting new connection error: " << ec.message() << std::endl;

				Listen();
			});
		}

		void SendMsgToOne(const message &msg, std::shared_ptr<session> ThisOne)
		{
			if (ThisOne && ThisOne->IsConnected())
				ThisOne->sendMsg(msg);
			else
			{
				ThisOne.reset();
				std::scoped_lock lock(cliMutex);
				m_clients.erase(ThisOne);
			}
		}

		void SendMsgToOthers(const message &msg, std::shared_ptr<session> notThisOne)
		{
			std::scoped_lock lock(cliMutex);
			for (auto client : m_clients)
			{
				if (client != notThisOne && client->isValidated())
				{
					if (client->IsConnected())
						client->sendMsg(msg);
					else
					{
						client.reset();
						m_clients.erase(client);
					}
				}
			}
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
				case msgID::text:
					SendMsgToOthers(item.msg, item.sender);
					break;

				case msgID::mynickname:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.GetData((uint8_t *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					item.sender->SetNickname(p);
					item.sender->setValidated();
					item.msg.header.ID = msgID::welcomeNew;
					SendMsgToOthers(item.msg, item.sender);
					free(p);

					size_t len = 0;
					p = GetAllNicknames(&len, item.sender);
					if (0 == len)
						break;
					temp.header.ID = msgID::updatelist;
					temp.FillData(len * sizeof(wchar_t), (uint8_t *)p);
					SendMsgToOne(temp, item.sender);
					free(p);
					break;
				}

				case msgID::stop:
					return;

				case msgID::deleteme:
				{
					std::scoped_lock lock(cliMutex);
					m_clients.erase(item.sender);
					item.sender.reset();
					break;
				}

				case msgID::exit:
				{
					temp.header.ID = msgID::leave;
					temp.FillData(sizeof(wchar_t) * wcslen(item.sender->GetNickname()),
						(uint8_t*)item.sender->GetNickname());
					SendMsgToOthers(temp, item.sender);
					break;
				}
				}
			}
		}

		void SysCmd()
		{
			;
		}


		wchar_t *GetAllNicknames(size_t *l, std::shared_ptr<session> notThisOne)
		{
			std::scoped_lock lock(cliMutex);
			std::wstring allNickNames;
			for (auto client : m_clients)
			{
				if (client != notThisOne)
				{
					wchar_t *cur = client->GetNickname();
					if (cur != NULL)
					{
						allNickNames += cur;
						allNickNames += L"\t";
					}
				}
			}

			*l = allNickNames.length();
			
			if (0 == *l)
				return nullptr;

			wchar_t *p = (wchar_t*)malloc( (*l) * sizeof(wchar_t) );

			wmemcpy(p, allNickNames.data(), (*l));

			return p;
		}

	private:
		asio::io_context m_asioContext;

		asio::ip::tcp::acceptor m_asioAcceptor;

		uint32_t IDCounter = 1000;

		std::unordered_set<std::shared_ptr<session>> m_clients;

		std::mutex cliMutex;

		std::vector<std::thread> m_asioThreads;

		std::thread m_loopThread;

		uint32_t numOfThreads = 1;

		msgQueue<MsgWithSender> m_IncomingMsgQueue;
	};
}