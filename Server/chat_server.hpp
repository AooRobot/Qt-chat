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

class chat_participant   //�ṩ�ӿڵķ�����
{
public:
	virtual ~chat_participant() {}       //���麯���ͱ�����������
	virtual void deliver(const chat_message& msg) = 0;
protected:
private:
};

class chat_room    //all users in this room
{
public:
	void join_room(boost::shared_ptr<chat_participant> participant)
	{
		participants_.insert(participant);       //���û��ŵ���������뵽�û���������

		std::cout << "���û�����������" << std::endl;

		std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
			boost::bind(&chat_participant::deliver, participant, _1)  //[&](const chat_message& msg){ participant->deliver(msg) }
		);
	}

	void leave(boost::shared_ptr<chat_participant> participants)
	{
		std::cout << "���û��뿪������" << std::endl;
		participants_.erase(participants);
	}

	void deliver(const chat_message& msg)
	{
		recent_msgs_.push_back(msg);     //���������ݴ���ӵײ�ѹ��

		while (recent_msgs_.size() > max_recent_msgs)
		{
			recent_msgs_.pop_front();     //�����Ϣ�������ˣ��Ͱ��������һ���ӵ�
		}

		std::for_each(participants_.begin(), participants_.end(),
			boost::bind(&chat_participant::deliver, _1, boost::ref(msg))  //_1��for_each����������participan(������)  //˫�����ûᱻ�۵�һ��,��ref���   //��������Ϣ�ַ���ÿ���û�
		);
	}
protected:
private:
	std::set<boost::shared_ptr<chat_participant> > participants_;  //�����û�������
	enum
	{
		max_recent_msgs = 100   // ��ʷ��Ϣ�޳�
	};
	chat_message_queue recent_msgs_;   //�����¼
};

class chat_session : public chat_participant,            //���ؼ̳�
	                 public boost::enable_shared_from_this<chat_session>    //Ϊ��ʹָʾ������һ,��ֹ new_session ������̫�챻����
					 
{
public:
	chat_session(io_service& ioservice, chat_room& room)
		:socket_(ioservice)
		, room_(room)
	{

	}

	~chat_session()
	{
		std::cout << "��ӭ�´ι���" << std::endl;
	}

	void start()
	{
		room_.join_room(shared_from_this());    //����������,����shared_from_this�������ü���+1

												//��ʼ��������
		boost::asio::async_read(socket_,
			boost::asio::buffer(read_msg_.data(), chat_message::header_length_),
			boost::bind(&chat_session::read_handle_header, shared_from_this(), _1)      //shared_from_this  ����this����Ϊ�첽ִ�� �ᱻ����
		);
	}

	void read_handle_header(const boost::system::error_code& error)
	{

		if (!error && read_msg_.decode_header())
		{
			std::cout << "����ܳ��ȣ�" << read_msg_.length() << std::endl;

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

			qDebug() << QString::fromLocal8Bit("Debug:����Ϊ:")
					 << QString::fromLocal8Bit(str.c_str())
					 << QString::fromLocal8Bit(" ����Ϊ:")
					 << read_msg_.boby_lenght();

			room_.deliver(read_msg_);  //Ͷ����Ϣ��������(����Ϣ�����ܶ���)

			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.data(), chat_message::header_length_),
				boost::bind(&chat_session::read_handle_header, shared_from_this(), _1)      //shared_from_this  ����this����Ϊ�첽ִ�� �ᱻ����
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

		write_msgs_.push_back(msg);    //��Ҫ���͵�����ѹ�뵽���Ͷ�����

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
	Q_OBJECT  //����ʽ  ����չ����,���޸Ĺر�

	//�ź�
	Q_SIGNAL void singleConnect();
	Q_SIGNAL void singleError();   //����Ҫʵ�ֵĺ���
	
public:
	chat_server(io_service& ioservice, tcp::endpoint endpoint)
		:io_service_(ioservice)
		, acceptor_(ioservice, endpoint)    //���������
	{
		
	}

	void start_accept()   //�첽������
	{
		//�����������ɹ�
		
		//Ϊ���ӵĿͻ��˲���һ�����
		boost::shared_ptr<chat_session> new_session(boost::make_shared<chat_session>(io_service_, room_));

		//�첽���ܿͻ�
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&chat_server::accept_handle, this, new_session, _1));   //����Я��this ��Ϊ���첽����

																				//�ٶȺܿ죬�ܿ�ͽ���
	}


	bool is_running()
	{
		return io_service_.stopped() ? false : true;
	}

	//�첽����
	void accept_handle(boost::shared_ptr<chat_session> session, const boost::system::error_code& error)     //�첽���ܿ�����this
	{
		//�����ˣ����� 
		if (!error)
		{
			static boost::format strInfo("IP��ַ:%1% �˿ں�:%2%");    //����Ƶ���Ĺ�������,ʹ��ȫ�ֱ���

			strInfo % session->socket().remote_endpoint().address().to_string();
			strInfo % session->socket().remote_endpoint().port();

			connect_msgs_.push_back(strInfo.str().c_str());

			emit singleConnect();    //������Ϣ��

			session->start();   //��������
		}
		start_accept();   //���ܶ�������
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
	io_service& io_service_;        //�ȸ������淶

	tcp::acceptor acceptor_;       //������(��ӭ�ͻ�) �󶨶˿�

	chat_room room_;

	std::deque<std::string> connect_msgs_;
};

