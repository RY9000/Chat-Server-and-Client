#pragma once

#include "net_common.h"

namespace net
{
	enum class msgID
	{
		connected,
		mynickname,
		updatelist,
		welcomeNew,
		text,
		exit,
		leave,
		stop,
		deleteme
	};


	struct messageHeader
	{
		msgID ID{};
		size_t size = 0;
	};


	struct message
	{
		messageHeader header{};
		std::vector<uint8_t> body;


		void FillData(size_t size, uint8_t *data)
		{
			body.resize(size);
			std::memcpy(body.data(), data, size);
			header.size = body.size();
		}

		void GetData(uint8_t *data)
		{
			std::memcpy(data, body.data(), header.size);
		}
	};

	class session;
	struct MsgWithSender
	{
		message msg;
		std::shared_ptr<session> sender;
	};

	template<typename T>
	class msgQueue
	{
	public:
		msgQueue() = default;

		msgQueue(const msgQueue &) = delete;

		~msgQueue() { clear(); }

		const T & front()
		{
			std::scoped_lock lock(qMutex);
			return q.front();
		}

		T pop_front()
		{
			std::scoped_lock lock(qMutex);
			T temp = std::move(q.front());
			q.pop_front();
			return temp;
		}

		void push_front(const T &item)
		{
			std::scoped_lock lock(qMutex);
			q.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(cvMutex);
			cv.notify_one();
			return;
		}

		void push_back(const T &item)
		{
			std::scoped_lock lock(qMutex);
			q.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(cvMutex);
			cv.notify_one();
			return;
		}

		size_t size()
		{
			std::scoped_lock lock(qMutex);
			return this->q.size();
		}

		void clear()
		{
			std::scoped_lock lock(qMutex);
			q.clear();
		}

		void wait()
		{
			while (this->q.size()==0)
			{
				std::unique_lock<std::mutex> ul(cvMutex);
				cv.wait(ul);
			}
		}

	private:
		std::deque<T> q;
		std::condition_variable cv;
		std::mutex qMutex, cvMutex;
	};
}