#pragma once

class chat_message
{
public:
	enum 
	{
		header_length_ = 4,   //包头  4字节表示指定接受长度

		max_body_lenght_ = 512    //最大包内容长度
	};

	chat_message()
		:body_lenght_(0)
	{
		memset(data_, 0, sizeof(data_));
	}

	char* data()    //可以修改内容
	{
		return data_;
	}

	const char* data() const    //只是读数据
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

	std::size_t header_lenght() const  //得知包头大小
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

		if (body_lenght_ > max_body_lenght_)  //安全检查
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
			body_lenght_ = 0;   //非法长度
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
	std::size_t body_lenght_;         //有效长度

	char data_[max_body_lenght_ + header_length_];    //通常为了收包和发包速度快，我们采取定长缓冲区
													//而且也不清理里面的垃圾数据，而是直接采用包头指定的有效长度 
													//这样发包和收包的时候，可以直接知道有效内容是多少，不会产生问题
};