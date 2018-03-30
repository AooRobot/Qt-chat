#pragma once

#include <iostream>
#include <deque>
#include <set>
#include <thread>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include "../Share/chat_message.h"

#include <QObject>
#include <QDebug>

using namespace boost::asio;
using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_participant   //提供接口的方法类
{
public:
	virtual ~chat_participant() {}       //有虚函数就必须是虚析构
	virtual void deliver(const chat_message& msg) = 0;
protected:
private:
};

class chat_room    //all users in this room
{
public:
	void join_room(boost::shared_ptr<chat_participant> participant)
	{
		participants_.insert(participant);       //把用户放到容器里，加入到用户管理器里

		std::cout << "有用户进入聊天室" << std::endl;

		std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
			boost::bind(&chat_participant::deliver, participant, _1)  //[&](const chat_message& msg){ participant->deliver(msg) }
		);
	}

	void leave(boost::shared_ptr<chat_participant> participants)
	{
		std::cout << "有用户离开聊天室" << std::endl;
		participants_.erase(participants);
	}

	void deliver(const chat_message& msg)
	{
		recent_msgs_.push_back(msg);     //把聊天内容存入从底部压入

		while (recent_msgs_.size() > max_recent_msgs)
		{
			recent_msgs_.pop_front();     //如果消息对列满了，就把最早的那一条扔掉
		}

		std::for_each(participants_.begin(), participants_.end(),
			boost::bind(&chat_participant::deliver, _1, boost::ref(msg))  //_1是for_each遍历出来的participan(参与者)  //双层引用会被折叠一次,用ref解决   //把这条消息分发给每个用户
		);
	}
protected:
private:
	std::set<boost::shared_ptr<chat_participant> > participants_;  //储存用户的容器
	enum
	{
		max_recent_msgs = 100   // 历史消息限长
	};
	chat_message_queue recent_msgs_;   //聊天记录
};

class chat_session : public chat_participant,            //多重继承
	                 public boost::enable_shared_from_this<chat_session>    //为了使指示计数加一,防止 new_session 结束的太快被析构
					 
{
public:
	chat_session(io_service& ioservice, chat_room& room)
		:socket_(ioservice)
		, room_(room)
	{

	}

	~chat_session()
	{
		std::cout << "欢迎下次光临" << std::endl;
	}

	void start()
	{
		room_.join_room(shared_from_this());    //加入聊天室,采用shared_from_this对其引用计数+1

												//开始接受数据
		boost::asio::async_read(socket_,
			boost::asio::buffer(read_msg_.data(), chat_message::header_length_),
			boost::bind(&chat_session::read_handle_header, shared_from_this(), _1)      //shared_from_this  不用this是因为异步执行 会被析构
		);
	}

	void read_handle_header(const boost::system::error_code& error)
	{

		if (!error && read_msg_.decode_header())
		{
			std::cout << "输出总长度：" << read_msg_.length() << std::endl;

			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.body(), read_msg_.boby_lenght()),
				boost::bind(&chat_session::read_handle_body, shared_from_this(), _1)
			);
		}
		else
		{
			room_.leave(shared_from_this());
		}

	}
	void read_handle_body(const boost::system::error_code& error)
	{
		if (!error)
		{
			std::string str(read_msg_.body(), read_msg_.boby_lenght());

			qDebug() << QString::fromLocal8Bit("Debug:内容为:")
					 << QString::fromLocal8Bit(str.c_str())
					 << QString::fromLocal8Bit(" 长度为:")
					 << read_msg_.boby_lenght();

			room_.deliver(read_msg_);  //投递消息给聊天室(将消息发给很多人)

			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.data(), chat_message::header_length_),
				boost::bind(&chat_session::read_handle_header, shared_from_this(), _1)      //shared_from_this  不用this是因为异步执行 会被析构
			);
		}
		else
		{
			room_.leave(shared_from_this());
		}
	}

	virtual void deliver(const chat_message& msg)
	{
		bool write_in_progress = !write_msgs_.empty();

		write_msgs_.push_back(msg);    //把要发送的内容压入到发送队列里

		if (!write_in_progress)
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
				boost::bind(&chat_session::write_handle, shared_from_this(), _1)
			);
		}
	}

	void write_handle(const boost::system::error_code& error)
	{
		if (!error)
		{
			write_msgs_.pop_front();

			if (!write_msgs_.empty())
			{
				boost::asio::async_write(socket_,
					boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
					boost::bind(&chat_session::write_handle, shared_from_this(), _1)
				);
			}
		}
		else
		{
			room_.leave(shared_from_this());
		}
	}

	tcp::socket& socket()
	{
		return socket_;
	}
protected:
private:
	tcp::socket socket_;     //
	chat_message_queue write_msgs_;
	chat_room& room_;
	chat_message read_msg_;
};

class chat_server :public QObject
{
	Q_OBJECT  //浸入式  对扩展开放,对修改关闭

	//信号
	Q_SIGNAL void singleConnect();
	Q_SIGNAL void singleError();   //不需要实现的函数
	
public:
	chat_server(io_service& ioservice, tcp::endpoint endpoint)
		:io_service_(ioservice)
		, acceptor_(ioservice, endpoint)    //构造接受器
	{
		
	}

	void start_accept()   //异步接收器
	{
		//服务器启动成功
		
		//为连接的客户端产生一个结点
		boost::shared_ptr<chat_session> new_session(boost::make_shared<chat_session>(io_service_, room_));

		//异步接受客户
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&chat_server::accept_handle, this, new_session, _1));   //必须携带this 因为是异步接受

																				//速度很快，很快就结束
	}


	bool is_running()
	{
		return io_service_.stopped() ? false : true;
	}

	//异步发生
	void accept_handle(boost::shared_ptr<chat_session> session, const boost::system::error_code& error)     //异步接受看不见this
	{
		//不吃了，走了 
		if (!error)
		{
			static boost::format strInfo("IP地址:%1% 端口号:%2%");    //避免频繁的构造析构,使用全局变量

			strInfo % session->socket().remote_endpoint().address().to_string();
			strInfo % session->socket().remote_endpoint().port();

			connect_msgs_.push_back(strInfo.str().c_str());

			emit singleConnect();    //触发消息槽

			session->start();   //滚动起来
		}
		start_accept();   //接受动作滚动
	}

	std::string connect_message()
	{
		static std::string str;

		str = connect_msgs_.front();

		connect_msgs_.pop_front();
		return str;
	}

protected:
private:
	io_service& io_service_;        //谷歌命名规范

	tcp::acceptor acceptor_;       //接受器(欢迎客户) 绑定端口

	chat_room room_;

	std::deque<std::string> connect_msgs_;
};

