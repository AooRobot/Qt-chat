#ifndef CLIENT_H
#define CLIENT_H

#include <QtWidgets/QMainWindow>
#include "ui_client.h"
#include <boost/shared_ptr.hpp>

#include "chat_client.hpp"

class Client : public QMainWindow
{
	Q_OBJECT

public:
	Client(QWidget *parent = 0);
	~Client();

private:
	Ui::ClientClass ui;

private slots:
	void slotConnectServer();
	void slotSendServer();

	void slotRecvmsg();
	void closeEvent(QCloseEvent *event);
private:
	io_service ioservice_;
	boost::shared_ptr<chat_client> client_;
	boost::shared_ptr<io_service::work> work_;
	
	chat_message msg_;
};

#endif // CLIENT_H
