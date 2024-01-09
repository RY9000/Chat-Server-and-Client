#pragma once

#include "NetCommon.h"

namespace net
{
	enum class MsgID:int
	{
		CONNECTED,
		MY_NICKNAME,
		UPDATE_LIST,
		WELCOME_NEW,
		TEXT,
		EXIT,  // I am leaving
		LEAVE, // Someone else has left
		STOP,
		DELETE_ME
	};


	struct MessageHeader
	{
		MsgID ID;
		size_t size = 0;
	};


	struct Message
	{
		MessageHeader header{};
		std::vector<BYTE> body;

		void fillData(size_t size, const BYTE *data)
		{
			body.assign(data, data + size);
			header.size = body.size();
		}

		void getData(BYTE *data)
		{
			std::memcpy(data, body.data(), header.size);
		}
	};


	class Session;
	struct MsgWithSender
	{
		Message msg;
		std::shared_ptr<Session> sender;
	};


	template<typename T>
	class MsgQueue
	{
	public:
		MsgQueue()
		{
			m_queSize = 0;
		}

		MsgQueue(const MsgQueue &) = delete;

		~MsgQueue()
		{
			clear();
		}

		const T &front()
		{
			std::unique_lock lock(m_mutexQue);
			return m_q.front();
		}

		T popFront()
		{
			std::unique_lock lock(m_mutexQue);
			m_cv.wait(lock, [this] { return m_queSize > 0; });
			--m_queSize;
			T temp = std::move(m_q.front());
			m_q.pop_front();
			return temp;
		}

		void pushFront(const T &msg)
		{
			std::unique_lock lock(m_mutexQue);
			m_q.emplace_front(std::move(msg));
			++m_queSize;
			lock.unlock();
			m_cv.notify_one();
			return;
		}

		void pushBack(const T &msg)
		{
			std::unique_lock lock(m_mutexQue);
			m_q.emplace_back(std::move(msg));
			++m_queSize;
			lock.unlock();
			m_cv.notify_one();
			return;
		}

		int size()
		{
			std::unique_lock lock(m_mutexQue);
			return m_queSize;
		}

		void clear()
		{
			std::unique_lock lock(m_mutexQue);
			m_q.clear();
		}

	private:
		int m_queSize;
		std::deque<T> m_q;
		std::condition_variable m_cv;
		std::mutex m_mutexQue;
	};
}