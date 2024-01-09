#pragma once

#include "NetCommon.h"
#include "NetSession.h"


namespace net
{
	class Server
	{
	public:
		Server():m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), LISTEN_PORT))
		{
			m_asioAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			m_idCounter = 1000;
			m_numOfThreads = 1;
		}


		~Server()
		{
			stop();
		}


		bool start(int n)
		{
			if (m_loopThread.joinable())
			{
				std::cout << "Server is already running." << std::endl;
				return false;
			}

			m_numOfThreads = n;

			listen();

			for (int i = 0; i < m_numOfThreads; ++i)
				m_asioThreads.emplace_back([&]() { m_asioContext.run(); });

			m_loopThread = std::thread(&Server::processIncomingMsg, this);

			std::cout << "Server Started." << std::endl;
			return true;
		}


		bool stop()
		{
			if (!m_loopThread.joinable())
			{
				std::cout << "Server is not running." << std::endl;
				return false;
			}

			m_incomingMsgQueue.pushFront({ {MsgID::STOP}, nullptr });

			if (m_loopThread.joinable())
				m_loopThread.join();

			m_asioContext.stop();

			for (int i = 0; i < m_numOfThreads; ++i)
				if (m_asioThreads[i].joinable())
					m_asioThreads[i].join();

			m_asioThreads.clear();

			{
				std::scoped_lock lock(m_mutexClients);

				for (auto client : m_clients)
				{
					client->disconnect();
					client.reset();
				}

				m_clients.clear();
			}

			m_incomingMsgQueue.clear();
			m_asioContext.reset();

			std::cout << "Server Stopped.\n";
			return true;
		}


	private:
		void listen()
		{
			m_asioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket)
				{
					if (!ec)
					{
						std::shared_ptr<Session> newClient =
							std::make_shared<Session>(Owner::SERVER, m_asioContext, socket, ++m_idCounter, m_incomingMsgQueue);

						{
							std::scoped_lock lock(m_mutexClients);
							auto result = m_clients.insert(newClient);
							(*result.first)->validateClient();
						}

					}
					else
						std::cout << "Accept new connection error: " << ec.message() << std::endl;

					listen();
				});
		}


		void sendMsgToOne(const Message &msg, std::shared_ptr<Session> thisOne)
		{
			if (thisOne && thisOne->isConnected())
				thisOne->sendMsg(msg);
			else
			{
				thisOne.reset();
				{
					std::scoped_lock lock(m_mutexClients);
					m_clients.erase(thisOne);
				}
			}
		}


		void sendMsgToOthers(const Message &msg, std::shared_ptr<Session> notThisOne)
		{
			std::scoped_lock lock(m_mutexClients);
			for (auto client : m_clients)
			{
				if (client != notThisOne && client->isValidated())
				{
					if (client->isConnected())
						client->sendMsg(msg);
					else
					{
						client.reset();
						m_clients.erase(client);
					}
				}
			}
		}


		void processIncomingMsg()
		{
			while (1)
			{
				MsgWithSender item = m_incomingMsgQueue.popFront();

				Message temp;

				switch (item.msg.header.ID)
				{
				case MsgID::TEXT:
				{
					sendMsgToOthers(item.msg, item.sender);
					break;
				}
				case MsgID::MY_NICKNAME:
				{
					wchar_t *p = (wchar_t *)malloc(item.msg.header.size + sizeof(wchar_t));
					item.msg.getData((BYTE *)p);
					p[item.msg.header.size / sizeof(wchar_t)] = L'\0';
					item.sender->setNickname(p);
					item.sender->setValidated();
					item.msg.header.ID = MsgID::WELCOME_NEW;
					sendMsgToOthers(item.msg, item.sender);
					free(p);

					size_t len = 0;
					p = getAllNicknames(&len, item.sender);
					if (0 == len)
						break;
					temp.header.ID = MsgID::UPDATE_LIST;
					temp.fillData(len * sizeof(wchar_t), (uint8_t *)p);
					sendMsgToOne(temp, item.sender);
					free(p);
					break;
				}
				case MsgID::DELETE_ME:
				{
					{
						std::scoped_lock lock(m_mutexClients);
						m_clients.erase(item.sender);
					}
					item.sender.reset();
					break;
				}
				case MsgID::EXIT:
				{
					temp.header.ID = MsgID::LEAVE;
					temp.fillData(sizeof(wchar_t) * wcslen(item.sender->getNickname()),
						(uint8_t *)item.sender->getNickname());
					sendMsgToOthers(temp, item.sender);
					{
						std::scoped_lock lock(m_mutexClients);
						m_clients.erase(item.sender);
					}
					item.sender.reset();
					break;
				}
				case MsgID::STOP:
				{
					return;
				}
				}
			}
		}


		wchar_t *getAllNicknames(size_t *len, std::shared_ptr<Session> notThisOne)
		{
			std::scoped_lock lock(m_mutexClients);
			std::wstring allNickNames;
			for (auto client : m_clients)
			{
				if (client != notThisOne)
				{
					wchar_t *cur = client->getNickname();
					if (cur != NULL)
					{
						allNickNames += cur;
						allNickNames += L"\t";
					}
				}
			}

			*len = allNickNames.length();

			if (0 == *len)
				return nullptr;

			wchar_t *p = (wchar_t *)malloc((*len) * sizeof(wchar_t));

			wmemcpy(p, allNickNames.data(), (*len));

			return p;
		}


	private:
		asio::io_context m_asioContext;

		asio::ip::tcp::acceptor m_asioAcceptor;

		std::atomic<uint64_t> m_idCounter{ 0 };

		std::unordered_set<std::shared_ptr<Session>> m_clients;

		std::mutex m_mutexClients;

		std::vector<std::thread> m_asioThreads; // thread pool

		int m_numOfThreads; // number of threads in the thread pool

		std::thread m_loopThread; // thread which runs message loop

		MsgQueue<MsgWithSender> m_incomingMsgQueue;
	};
}