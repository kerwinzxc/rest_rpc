#pragma once

namespace timax { namespace rpc 
{
	template <typename Decode>
	connection<Decode>::connection(server_ptr server, boost::asio::io_service& io_service, std::size_t timeout_milli)
		: server_(server)
		, socket_(io_service)
		, head_(), data_(), read_buf_()
		, message_{ boost::asio::buffer(head_), boost::asio::buffer(data_) }
		, timer_(io_service)
		, timeout_milli_(timeout_milli)
	{

	}

	template <typename Decode>
	void connection<Decode>::start()
	{
		set_no_delay();
		read_head();
	}

	template <typename Decode>
	tcp::socket& connection<Decode>::socket()
	{
		return socket_;
	}

	template <typename Decode>
	void connection<Decode>::response(const char* data, size_t size, result_code code)
	{
		if (size > MAX_BUF_LEN - HEAD_LEN)
			throw std::overflow_error("the size is too big");

		auto self(this->shared_from_this());
		head_t h = { (int16_t)code, 0, static_cast<int>(size) };
		message_[0] = boost::asio::buffer(&h, HEAD_LEN);
		message_[1] = boost::asio::buffer((char*)data, size);

		boost::asio::async_write(socket_, message_, [this, self](boost::system::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				g_succeed_count++;
				read_head();
			}
			else
			{
				SPD_LOG_ERROR(ec.message().c_str());
			}
		});
	}

	template <typename Decode>
	void connection<Decode>::read_head()
	{
		reset_timer();
		auto self(this->shared_from_this());
		boost::asio::async_read(socket_, boost::asio::buffer(head_), [this, self](boost::system::error_code ec, std::size_t length)
		{
			if (!socket_.is_open())
			{
				cancel_timer();
				return;
			}

			if (!ec)
			{
				head_t h = *(head_t*)(head_.data());
				//const int body_len = i & int{-1};
				//const int type = i << 32;
				if (h.len > 0 && h.len < MAX_BUF_LEN)
				{
					read_body(h);
					return;
				}

				if (h.len == 0) //nobody, just head.
				{
					read_head();
				}
				else
				{
					SPD_LOG_ERROR("invalid body len {}", h.len);
					cancel_timer();
				}
			}
			else
			{
				server_->remove_sub_conn(self.get());
				SPD_LOG_ERROR(ec.message().c_str());
				cancel_timer();
			}
		});
	}

	template <typename Decode>
	void connection<Decode>::read_body(head_t const& head)
	{
		auto self(this->shared_from_this());
		data_.resize(head.len);
		boost::asio::async_read(socket_, boost::asio::buffer(data_, head.len), [this, head, self](boost::system::error_code ec, std::size_t length)
		{
			cancel_timer();

			if (!socket_.is_open())
				return;

			if (!ec)
			{
				server_->callback(self, data_.data(), length);
			}
			else
			{
				SPD_LOG_ERROR(ec.message().c_str());
			}
		});
	}

	template <typename Decode>
	void connection<Decode>::reset_timer()
	{
		if (timeout_milli_ == 0)
			return;

		auto self(this->shared_from_this());
		timer_.expires_from_now(boost::posix_time::milliseconds(timeout_milli_));
		timer_.async_wait([this, self](const boost::system::error_code& ec)
		{
			if (!socket_.is_open())
			{
				return;
			}

			if (ec)
			{
				SPD_LOG_INFO(ec.message().c_str());
				return;
			}

			SPD_LOG_INFO("connection timeout");

			close();
		});
	}

	template <typename Decode>
	void connection<Decode>::cancel_timer()
	{
		if (timeout_milli_ == 0)
			return;
	
		timer_.cancel();
	}

	template <typename Decode>
	void connection<Decode>::close()
	{
		boost::system::error_code ignored_ec;
		socket_.close(ignored_ec);
	}

	template <typename Decode>
	void connection<Decode>::set_no_delay()
	{
		boost::asio::ip::tcp::no_delay option(true);
		boost::system::error_code ec;
		socket_.set_option(option, ec);
	}
} }