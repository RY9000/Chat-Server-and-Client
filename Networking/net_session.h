#pragma once

#include "net_common.h"
#include "net_message.h"



namespace net
{
	enum class ownership
	{
		server,
		client
	};


	class session : public std::enable_shared_from_this<session>
	{
	public:
		session(ownership o, asio::io_context& asioContext, asio::ip::tcp::socket socket,
			uint32_t id, msgQueue<MsgWithSender>& qIn)
			: m_asioContext(asioContext), m_socket(std::move(socket)), m_IncomingMsgQueue(qIn)
		{
			m_owner = o;
			m_ID = id;
		}

		~session()
		{
			m_socket.close();
		}

		void ConnectToServer(asio::ip::tcp::resolver::results_type& endpoints)
		{
			asio::async_connect(m_socket, endpoints,
				[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
				{
					if (!ec)
					{
						recvValidation();
					}
				});
		}

		void ValidateClient()
		{
			m_validation = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
			m_validation = encryptVali(m_validation, 0);
			sendValidation();
		}

		void sendValidation()
		{
			asio::async_write(m_socket, asio::buffer(&m_validation, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_owner == ownership::client)
							recvHeader();
						else
							recvValidation();
					}
				});
		}

		void recvValidation()
		{
			asio::async_read(m_socket, asio::buffer(&m_validationRecvd, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_owner == ownership::server)
						{
							m_validation = encryptVali(m_validation, 1);
							if (m_validationRecvd == m_validation)
							{
								std::cout << "Client " << m_ID << " Validated.\n";
								recvHeader();
								message temp;
								temp.header.ID = msgID::connected;
								temp.FillData(sizeof(m_ID), (uint8_t*)&m_ID);
								sendMsg(temp);
							}
							else
							{
								std::cout << "Client " << m_ID << " Validation failed.\n";
								DeleteMyself();
							}
						}
						else
						{
							m_validation = encryptVali(m_validationRecvd, 1);
							sendValidation();
						}
					}
				});
		}

		void sendMsg(const message& msg)
		{

			asio::post(m_asioContext, [this, msg]()
				{
					size_t sz = m_OutgoingMsgQueue.size();
					m_OutgoingMsgQueue.push_back(msg);
					if (0 == sz)
						sendHeader();
				});
		}

		void sendHeader()
		{
			asio::async_write(m_socket, asio::buffer(&m_OutgoingMsgQueue.front().header,
				sizeof(messageHeader)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_OutgoingMsgQueue.front().header.size > 0)
							sendBody();
						else
						{
							m_OutgoingMsgQueue.pop_front();

							if (0 != m_OutgoingMsgQueue.size())
								sendHeader();
						}
					}
					else
					{
						std::cout << "Sending header to client " << m_ID << " fails. " << ec.message() << "\n";
					}
				});
		}

		void sendBody()
		{
			asio::async_write(m_socket, asio::buffer(m_OutgoingMsgQueue.front().body.data(),
				m_OutgoingMsgQueue.front().header.size),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_OutgoingMsgQueue.pop_front();

						if (0 != m_OutgoingMsgQueue.size())
							sendHeader();
					}
					else
					{
						std::cout << "Sending body to client " << m_ID << " fails. " << ec.message() << "\n";
					}
				});
		}

		void recvHeader()
		{
			asio::async_read(m_socket, asio::buffer(&m_msgReceived.header,
				sizeof(messageHeader)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_msgReceived.body.resize(m_msgReceived.header.size);

						if (m_msgReceived.header.size > 0)
							recvBody();
						else
							AddToIncomingMsgQueue();
					}
					else
					{
						std::cout << "Receiving header from client " << m_ID << " fails. " << ec.message() << "\n";
					}
				});
		}

		void recvBody()
		{
			asio::async_read(m_socket, asio::buffer(m_msgReceived.body.data(),
				m_msgReceived.header.size),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
						AddToIncomingMsgQueue();
					else
					{
						std::cout << "Receiving body from client " << m_ID << " fails. " << ec.message() << "\n";
					}
				});
		}

		void AddToIncomingMsgQueue()
		{
			m_IncomingMsgQueue.push_back({ m_msgReceived, this->shared_from_this() });

			recvHeader();
		}



		bool IsConnected()
		{
			return m_socket.is_open();
		}

		void Disconnect()
		{
			if (IsConnected())
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
			else if(i==1)
			{
				vali = vali ^ 0x1357924680ABCFED;
				vali = (vali + 0x1234567890ACB) % 0xFFF1010101010101;
				return vali ^ 0xABCDEF9870123456;
			}
			return 0;
		}

		void SetID(uint32_t id)
		{
			m_ID = id;
		}

		void SetNickname(wchar_t* nn)
		{
			m_Nickname = nn;
		}

		wchar_t* GetNickname()
		{
			if (m_Nickname != L"")
				return m_Nickname.data();
			else
				return NULL;
		}

		void DeleteMyself()
		{
			message temp;
			temp.header.ID = msgID::deleteme;
			temp.header.size = 0;
			m_IncomingMsgQueue.push_back({ temp, this->shared_from_this() });
		}

		bool isValidated()
		{
			return validated;
		}

		void setValidated()
		{
			validated = true;
		}

	private:
		ownership m_owner;

		asio::io_context& m_asioContext;

		asio::ip::tcp::socket m_socket;

		uint32_t m_ID;

		msgQueue<MsgWithSender>& m_IncomingMsgQueue;

		msgQueue<message> m_OutgoingMsgQueue;

		message m_msgReceived;

		uint64_t m_validation, m_validationRecvd;

		std::wstring m_Nickname;

		bool validated = false;
	};
}