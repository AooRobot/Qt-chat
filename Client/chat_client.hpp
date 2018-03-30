#pragma once

#include <iostream>
#include <deque>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "../Share/chat_message.h"

#include <QObject>
using namespace boost::asio;
using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;    //双端队列

class chat_client : public QObject
{
	Q_OBJECT

	Q_SIGNAL void singleRecvmsg();
public:
	chat_client(io_service& ioservice, tcp::resolver::iterator endpoint_iterator)
		:io_service_(ioservice)
		, socket_(ioservice)
	{
		boost::asio::async_connect(socket_, endpoint_iterator,
			boost::bind(&chat_client::connect_handle, this, _1));
	}



	tcp::socket& socket()
	{
		return socket_;
	}

	void send(chat_message& msg)   //不在这拷贝
	{

		//do_write(msg);
		io_service_.post(boost::bind(&chat_client::do_write, this, msg));
	}

	void do_write(chat_message msg)   //用它拷贝 ---害怕信息丢失
	{
		bool write_in_progress = !write_msgs_.empty();
		write_msgs_.push_back(msg);    //生产者

		if (!write_in_progress)
		{
			//发包函数
			boost::asio::async_write(socket_,
				boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
				boost::bind(&chat_client::write_handle, this, _1)
			);
		}
	}

	void write_handle(const boost::system::error_code& error)
	{
		//这里能够被执行的时候,有一条内容被发送了(成功与否不清楚)
		if (!error)
		{
			write_msgs_.pop_front();   //消费掉

			if (!write_msgs_.empty())
			{
				//继续挂接异步发送动作     滚动发包函数
				boost::asio::async_write(socket_,
					boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
					boost::bind(&chat_client::write_handle, this, _1)
				);
			}
		}
		else
		{
			do_close();
		}
	}

	void do_close()
	{
		socket_.close();   //关闭套接字
	}

	void close()
	{
		io_service_.post(boost::bind(&chat_client::do_close, this));   //不紧急的关闭方式，将消息投递给系统，速度也不会太差，但又不会太紧急
	}

	std::string recv_message()
	{
		static std::string str;

		str = read_msgs_.front();

		read_msgs_.pop_front();
		return str;
	}

protected:
	void connect_handle(const boost::system::error_code& error)
	{
		//读取或者发送数据
		if (!error)
		{
			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.data(), chat_message::header_length_),
				boost::bind(&chat_client::read_handle_header, this, _1)
			);
		}

	}

	void read_handle_header(const boost::system::error_code& error)     //拆包头
	{
		if (!error && read_msg_.decode_header())
		{
			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.body(), read_msg_.boby_lenght()),
				boost::bind(&chat_client::read_handle_body, this, _1)
			);
		}
		else
		{
			do_close();
		}
	}

	void read_handle_body(const boost::system::error_code& error)
	{
		if (!error)
		{
			std::cout.write(read_msg_.body(), read_msg_.boby_lenght());
			std::cout << "\n";

			std::string str( read_msg_.body() ,read_msg_.boby_lenght());

			read_msgs_.push_back(str);

			emit singleRecvmsg();

			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.data(), chat_message::header_length_),
				boost::bind(&chat_client::read_handle_header, this, _1)
			);
		}
		else
		{
			do_close();
		}
	}

private:
	io_service& io_service_;
	tcp::socket socket_;

	chat_message read_msg_;
	chat_message_queue write_msgs_;

	std::deque<std::string> read_msgs_;   //群聊天
};
