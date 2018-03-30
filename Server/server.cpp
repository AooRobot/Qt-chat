#include "server.h"
#include <QMessageBox>
#include <QDebug>
#include <QCloseEvent>


Server::Server(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	connect(ui.connectButton, SIGNAL(clicked()), this, SLOT(slotStartServer()));    //�󶨺���
	ui.lineEdit->setText("8888");
}

Server::~Server()
{

}

void Server::closeEvent(QCloseEvent *event)
{

	QMessageBox::StandardButton button;

	button = QMessageBox::question(this,
		QString::fromLocal8Bit("�˳�����"),
		QString::fromLocal8Bit("�Ƿ�����������˳�?"),
		QMessageBox::Yes | QMessageBox::No);

	if (button == QMessageBox::No)
	{
		event->ignore();   //�����˳��źţ��������ִ��
	} 
	else if(button == QMessageBox::Yes)
	{
		if (server_.get() != nullptr)
		{
			if (server_->is_running())
			{
				qDebug() << QString::fromLocal8Bit("Debug:׼���ر�");
				ioservice_.stop();
			}
		}

		event->accept();  //�����˳��ź�,�����˳�
	}
}

void Server::slotStartServer()           //ui��ťִ����ͻᱻ��������Ҫ������洢�ڶ���
{
	std::thread t(     //�������߳�
		[&]()
		{
			tcp::endpoint endpoint(tcp::v4(), ui.lineEdit->text().toInt());    //ipv4   �Ƚ���Ҫ���¶�ʧ�ÿ���

			server_ = boost::make_shared<chat_server>(ioservice_, endpoint);

			//����1  ������  ����2  ���  ����3  ������  ����4  ��Ӧ����
			QObject::connect(server_.get(), SIGNAL(singleError()), this, SLOT(slotErrorHandle()));
			QObject::connect(server_.get(), SIGNAL(singleConnect()), this, SLOT(slotConnectHandle()));

			server_->start_accept();

			ioservice_.run();     //��������
		}
	);

	t.detach();   //�����̰߳���
}

void Server::slotErrorHandle()
{
	//ui.textEdit->setText(QString::fromLocal8Bit("�д�����!"));
}

void Server::slotConnectHandle()
{
	std::string msg = server_->connect_message();

	ui.textEdit->append(QString::fromLocal8Bit(msg.c_str()));
}