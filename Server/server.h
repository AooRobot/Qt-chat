#ifndef SERVER_H
#define SERVER_H

#include <QtWidgets/QMainWindow>
#include "ui_server.h"
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

#include "chat_server.hpp"

class Server : public QMainWindow
{
	Q_OBJECT

public:
	Server(QWidget *parent = 0);
	~Server();

private:
	Ui::ServerClass ui;
	

private slots:     //槽函数   (消息)
	void slotStartServer();

	void slotErrorHandle();

	void slotConnectHandle();

	void closeEvent(QCloseEvent *event);
private:
	io_service ioservice_;          //前摄器定义一个就够
	boost::shared_ptr<chat_server> server_;
};

#endif // SERVER_H
