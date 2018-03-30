#include "server.h"
#include <QMessageBox>
#include <QDebug>
#include <QCloseEvent>


Server::Server(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	connect(ui.connectButton, SIGNAL(clicked()), this, SLOT(slotStartServer()));    //绑定函数
	ui.lineEdit->setText("8888");
}

Server::~Server()
{

}

void Server::closeEvent(QCloseEvent *event)
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
	else if(button == QMessageBox::Yes)
	{
		if (server_.get() != nullptr)
		{
			if (server_->is_running())
			{
				qDebug() << QString::fromLocal8Bit("Debug:准备关闭");
				ioservice_.stop();
			}
		}

		event->accept();  //接受退出信号,程序退出
	}
}

void Server::slotStartServer()           //ui按钮执行完就会被析构所以要将对象存储在堆上
{
	std::thread t(     //开工作线程
		[&]()
		{
			tcp::endpoint endpoint(tcp::v4(), ui.lineEdit->text().toInt());    //ipv4   比较重要，怕丢失用拷贝

			server_ = boost::make_shared<chat_server>(ioservice_, endpoint);

			//参数1  发送者  参数2  扳机  参数3  接收者  参数4  响应函数
			QObject::connect(server_.get(), SIGNAL(singleError()), this, SLOT(slotErrorHandle()));
			QObject::connect(server_.get(), SIGNAL(singleConnect()), this, SLOT(slotConnectHandle()));

			server_->start_accept();

			ioservice_.run();     //启动服务
		}
	);

	t.detach();   //与主线程剥离
}

void Server::slotErrorHandle()
{
	//ui.textEdit->setText(QString::fromLocal8Bit("有错误发生!"));
}

void Server::slotConnectHandle()
{
	std::string msg = server_->connect_message();

	ui.textEdit->append(QString::fromLocal8Bit(msg.c_str()));
}