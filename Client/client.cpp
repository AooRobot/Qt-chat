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
		QString::fromLocal8Bit("�˳�����"),
		QString::fromLocal8Bit("�Ƿ�����������˳�?"),
		QMessageBox::Yes | QMessageBox::No);

	if (button == QMessageBox::No)
	{
		event->ignore();   //�����˳��źţ��������ִ��
	}
	else if (button == QMessageBox::Yes)
	{
		if (client_.get() != nullptr)
		{
			if (!ioservice_.stopped())
			{
				qDebug() << QString::fromLocal8Bit("Debug:׼���ر�");
				client_->close();

				ioservice_.stop();
			}
		}

		event->accept();  //�����˳��ź�,�����˳�
	}
}

void Client::slotConnectServer()
{

	if (client_.get() == nullptr)
	{
		tcp::resolver resolver_(ioservice_);   //������

		tcp::resolver::query query(ui.lineEdit->text().toStdString(), ui.lineEdit_2->text().toStdString());   //��ѯ��

		tcp::resolver::iterator iterator = resolver_.resolve(query);     //��Ҫ����  �Ƿ��п���

		client_ = boost::make_shared<chat_client>(ioservice_, iterator);

		work_ = boost::make_shared<io_service::work>(ioservice_);     //<------------------û�й���    io_serviceҲ���ᱻ����   ����״̬
																	  //ioservice.run();   //������

		QObject::connect(client_.get(), SIGNAL(singleRecvmsg()), this, SLOT(slotRecvmsg()));

		boost::thread t(boost::bind(&boost::asio::io_service::run, &ioservice_));
		//boost::thread t([]() { ioservice_.run()  })

		t.detach();    //������
	}
	
}

void Client::slotSendServer()
{
	if (client_.get() != nullptr)
	{
		if (!ioservice_.stopped())
		{
			//��������
			QByteArray buf = ui.textEdit_2->toPlainText().toLocal8Bit();

// 			qDebug() << QString::fromLocal8Bit("Debug:����������:") 
// 					 << QString::fromLocal8Bit(buf.data());
			

		 	msg_.set_body_length(buf.size());
		 
			memcpy(msg_.body(), buf.data(),buf.size());

		 	msg_.encode_header();    //���ð�ͷ����
		 
		 	client_->send(msg_);
		}
	}

}

void Client::slotRecvmsg()
{
	std::string str = client_->recv_message();

	ui.textEdit->append( QString::fromLocal8Bit(str.c_str()) );
}