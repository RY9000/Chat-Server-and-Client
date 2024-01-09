#pragma once

#include "NetCommon.h"
#include "NetMessage.h"



namespace net
{
	enum class Owner
	{
		SERVER, CLIENT
	};


	// Session is used by both the server and the clients, so it contains functions used by both.
	class Session: public std::enable_shared_from_this<Session>
	{
	public:
		Session(Owner owner, asio::io_context &asioContext, asio::ip::tcp::socket &socket, uint64_t id, MsgQueue<MsgWithSender> &qIn):
			m_asioContext(asioContext), m_socket(std::move(socket)), m_incomingMsgQueue(qIn)
		{
			m_owner = owner;
			m_ID = id;
			m_validated = false;
			m_validationReceived = 0;
		}

		~Session()
		{
			m_socket.close();
		}

		void connectToServer(asio::ip::tcp::resolver::results_type &endpoints)
		{
			asio::async_connect(m_socket, endpoints, [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
				{
					if (!ec)
					{
						recvValidation();
					}
					else
					{
						std::cout << "Connect to server error: " << ec.message() << std::endl;
					}
				});
		}

		void validateClient()
		{
			m_validation = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
			m_validation = encryptVali(m_validation, 0);
			sendValidation();
		}


		void sendMsg(const Message &msg)
		{
			asio::post(m_asioContext, [this, msg]()
				{
					size_t sz = m_outgoingMsgQueue.size();
					m_outgoingMsgQueue.pushBack(msg);
					if (0 == sz)
						sendHeader();
				});
		}


		void sendValidation()
		{
			asio::async_write(m_socket, asio::buffer(&m_validation, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_owner == Owner::CLIENT)
							recvHeader();
						else
							recvValidation();
					}
					else
					{
						std::cout << "Send validation error: " << ec.message() << std::endl;
					}
				});
		}

		void recvValidation()
		{
			asio::async_read(m_socket, asio::buffer(&m_validationReceived, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_owner == Owner::SERVER)
						{
							m_validation = encryptVali(m_validation, 1);
							if (m_validationReceived == m_validation)
							{
								std::cout << "Client " << m_ID << " Validated.\n";
								recvHeader();
								Message temp;
								temp.header.ID = MsgID::CONNECTED;
								temp.fillData(sizeof(m_ID), (uint8_t *)&m_ID);
								sendMsg(temp);
							}
							else
							{
								std::cout << "Client " << m_ID << " Validation failed.\n";
								deleteMe();
							}
						}
						else
						{
							m_validation = encryptVali(m_validationReceived, 1);
							sendValidation();
						}
					}
					else
					{
						std::cout << "Receive validation error: " << ec.message() << std::endl;
					}
				});
		}


		void sendHeader()
		{
			asio::async_write(m_socket, asio::buffer(&m_outgoingMsgQueue.front().header, sizeof(MessageHeader)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_outgoingMsgQueue.front().header.size > 0)
							sendBody();
						else
						{
							m_outgoingMsgQueue.popFront();

							if (0 != m_outgoingMsgQueue.size())
								sendHeader();
						}
					}
					else
					{
						std::cout << "Send header to client " << m_ID << " failed. " << ec.message() << std::endl;
					}
				});
		}

		void sendBody()
		{
			asio::async_write(m_socket, asio::buffer(m_outgoingMsgQueue.front().body.data(), m_outgoingMsgQueue.front().header.size),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_outgoingMsgQueue.popFront();

						if (0 != m_outgoingMsgQueue.size())
							sendHeader();
					}
					else
					{
						std::cout << "Send body to client " << m_ID << " failed. " << ec.message() << std::endl;
					}
				});
		}

		void recvHeader()
		{
			asio::async_read(m_socket, asio::buffer(&m_msgReceived.header, sizeof(MessageHeader)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_msgReceived.body.resize(m_msgReceived.header.size);

						if (m_msgReceived.header.size > 0)
							recvBody();
						else
							addToIncomingMsgQueue();
					}
					else
					{
						std::cout << "Receive header from client " << m_ID << " failed. " << ec.message() << std::endl;
					}
				});
		}

		void recvBody()
		{
			asio::async_read(m_socket, asio::buffer(m_msgReceived.body.data(), m_msgReceived.header.size),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
						addToIncomingMsgQueue();
					else
					{
						std::cout << "Receive body from client " << m_ID << " failed. " << ec.message() << std::endl;
					}
				});
		}

		void addToIncomingMsgQueue()
		{
			m_incomingMsgQueue.pushBack({ m_msgReceived, this->shared_from_this() });
			recvHeader();
		}


		bool isConnected()
		{
			return m_socket.is_open();
		}

		void disconnect()
		{
			if (isConnected())
			{
				m_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
			}
		}


		uint64_t encryptVali(uint64_t vali, int i)
		{
			if (i == 0)
			{
				return vali ^ 0x1357924680ABCFED;
			}
			else if (i == 1)
			{
				vali = vali ^ 0x1357924680ABCFED;
				vali = (vali + 0x1234567890ACB) % 0xFFF1010101010101;
				return vali ^ 0xABCDEF9870123456;
			}
			return 0;
		}

		void setID(uint64_t id)
		{
			m_ID = id;
		}

		void setNickname(wchar_t *nn)
		{
			m_nickname = nn;
		}

		wchar_t *getNickname()
		{
			if (m_nickname != L"")
				return m_nickname.data();
			else
				return NULL;
		}

		void deleteMe()
		{
			Message temp;
			temp.header.ID = MsgID::DELETE_ME;
			temp.header.size = 0;
			m_incomingMsgQueue.pushBack({ temp, this->shared_from_this() });
		}

		bool isValidated()
		{
			return m_validated;
		}

		void setValidated()
		{
			m_validated = true;
		}

	private:
		Owner m_owner;

		asio::io_context &m_asioContext;

		asio::ip::tcp::socket m_socket;

		uint64_t m_ID;

		MsgQueue<MsgWithSender> &m_incomingMsgQueue;

		MsgQueue<Message> m_outgoingMsgQueue;

		Message m_msgReceived;

		uint64_t m_validation, m_validationReceived;

		bool m_validated;

		std::wstring m_nickname;
	};
}