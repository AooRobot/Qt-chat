#include "client.h"
#include <QDebug>
#include <QMessageBox>
#include <QCloseEvent>


Client::Client(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.connectButton, SIGNAL(clicked()), this, SLOT(slotConnectServer()));
	connect(ui.sendButton, SIGNAL(clicked()), this, SLOT(slotSendServer()));

	ui.lineEdit->setText("127.0.0.1");
	ui.lineEdit_2->setText("8888");
}

Client::~Client()
{

}

void Client::closeEvent(QCloseEvent *event)
{

	QMessageBox::StandardButton button;

	button = QMessageBox::question(this,
		QString::fromLocal8Bit("退出程序"),
		QString::fromLocal8Bit("是否结束操作并退出?"),
		QMessageBox::Yes | QMessageBox::No);

	if (button == QMessageBox::No)
	{
		event->ignore();   //忽略退出信号，程序继续执行
	}
	else if (button == QMessageBox::Yes)
	{
		if (client_.get() != nullptr)
		{
			if (!ioservice_.stopped())
			{
				qDebug() << QString::fromLocal8Bit("Debug:准备关闭");
				client_->close();

				ioservice_.stop();
			}
		}

		event->accept();  //接受退出信号,程序退出
	}
}

void Client::slotConnectServer()
{

	if (client_.get() == nullptr)
	{
		tcp::resolver resolver_(ioservice_);   //解析器

		tcp::resolver::query query(ui.lineEdit->text().toStdString(), ui.lineEdit_2->text().toStdString());   //查询器

		tcp::resolver::iterator iterator = resolver_.resolve(query);     //主要网卡  是否有空座

		client_ = boost::make_shared<chat_client>(ioservice_, iterator);

		work_ = boost::make_shared<io_service::work>(ioservice_);     //<------------------没有工作    io_service也不会被析构   待机状态
																	  //ioservice.run();   //阻塞了

		QObject::connect(client_.get(), SIGNAL(singleRecvmsg()), this, SLOT(slotRecvmsg()));

		boost::thread t(boost::bind(&boost::asio::io_service::run, &ioservice_));
		//boost::thread t([]() { ioservice_.run()  })

		t.detach();    //非阻塞
	}
	
}

void Client::slotSendServer()
{
	if (client_.get() != nullptr)
	{
		if (!ioservice_.stopped())
		{
			//发送内容
			QByteArray buf = ui.textEdit_2->toPlainText().toLocal8Bit();

// 			qDebug() << QString::fromLocal8Bit("Debug:发送内容是:") 
// 					 << QString::fromLocal8Bit(buf.data());
			

		 	msg_.set_body_length(buf.size());
		 
			memcpy(msg_.body(), buf.data(),buf.size());

		 	msg_.encode_header();    //设置包头长度
		 
		 	client_->send(msg_);
		}
	}

}

void Client::slotRecvmsg()
{
	std::string str = client_->recv_message();

	ui.textEdit->append( QString::fromLocal8Bit(str.c_str()) );
}