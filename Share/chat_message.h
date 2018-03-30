#pragma once

class chat_message
{
public:
	enum 
	{
		header_length_ = 4,   //��ͷ  4�ֽڱ�ʾָ�����ܳ���

		max_body_lenght_ = 512    //�������ݳ���
	};

	chat_message()
		:body_lenght_(0)
	{
		memset(data_, 0, sizeof(data_));
	}

	char* data()    //�����޸�����
	{
		return data_;
	}

	const char* data() const    //ֻ�Ƕ�����
	{
		return data_;
	}

	char* body()
	{
		return data_ + header_length_;
	}

	const char* body() const
	{
		return data_ + header_length_;
	}

	std::size_t header_lenght() const  //��֪��ͷ��С
	{
		return header_length_;
	}

	std::size_t boby_lenght() const
	{
		return body_lenght_;
	}
	
	void set_body_length(std::size_t new_length)
	{
		body_lenght_ = new_length;

		if (body_lenght_ > max_body_lenght_)  //��ȫ���
		{
			body_lenght_ = max_body_lenght_;
		}
		
	}

	std::size_t length()
	{
		return body_lenght_ + header_length_;
	}

	bool decode_header()
	{
		body_lenght_ = *(int*)data_;
		if (body_lenght_ > max_body_lenght_)
		{
			body_lenght_ = 0;   //�Ƿ�����
			return false;     
		}
		return true;
	}
	
	bool encode_header()   
	{
		*(int*)data_ = body_lenght_;
		return true;
	}
protected:
private:
	std::size_t body_lenght_;         //��Ч����

	char data_[max_body_lenght_ + header_length_];    //ͨ��Ϊ���հ��ͷ����ٶȿ죬���ǲ�ȡ����������
													//����Ҳ������������������ݣ�����ֱ�Ӳ��ð�ͷָ������Ч���� 
													//�����������հ���ʱ�򣬿���ֱ��֪����Ч�����Ƕ��٣������������
};