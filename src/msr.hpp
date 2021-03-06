#pragma once

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/bind.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <sstream>

#include "server_base.hpp"

// Micro Server for Reveal.js

namespace msr {
	using boost::asio::ip::tcp;

	struct request_data {
		std::string method, uri;
		bool invalid;
	};

	inline boost::posix_time::ptime parse_time(const std::string &str)
	{
		boost::posix_time::ptime time;

		std::istringstream sstr;

		sstr.str(str);

		try {
			auto facet_rfc1123 = new boost::posix_time::time_facet("%a, %d %b %Y %H:%M:%S GMT");

			sstr.imbue(std::locale(sstr.getloc(), facet_rfc1123));
			sstr >> time;
		} catch (std::ios_base::failure &) {
		}

		try {
			auto facet_rfc850 = new boost::posix_time::time_facet("%A, %d-%b-%y %H:%M:%S GMT");

			sstr.imbue(std::locale(sstr.getloc(), facet_rfc850));
			sstr >> time;
		} catch (std::ios_base::failure &) {
		}

		try {
			auto facet_asctime = new boost::posix_time::time_facet("%a %b %e %H:%M:%S %Y");

			sstr.imbue(std::locale(sstr.getloc(), facet_asctime));
			sstr >> time;
		} catch (std::ios_base::failure &) {
			return boost::posix_time::ptime();
		}

		return time;
	}

	inline std::string format_time(const boost::posix_time::ptime &time)
	{
		auto facet_rfc1123 = new boost::posix_time::time_facet("%a, %d %b %Y %H:%M:%S GMT");

		std::stringstream sstr;

		sstr.imbue(std::locale(sstr.getloc(), facet_rfc1123));
		sstr << time;

		return sstr.str();
	}

	inline std::string content_type(const std::string &ext)
	{
		if (ext == ".html" || ext == ".htm") {
			return "text/html";
		} else if (ext == ".css") {
			return "text/css";
		} else if (ext == ".js") {
			return "application/javascript";
		} else if (ext == ".rtf") {
			return "application/rtf";
		} else if (ext == ".xml") {
			return "text/xml";
		} else if (ext == ".txt" || ext == ".md") {
			return "text/plain";
		} else if (ext == ".jpg" || ext == ".jpeg") {
			return "image/jpeg";
		} else if (ext == ".gif") {
			return "image/gif";
		} else if (ext == ".png") {
			return "image/png";
		} else if (ext == ".tiff") {
			return "image/tiff";
		} else if (ext == ".woff") {
			return "application/x-font-woff";
		} else if (ext == ".pdf") {
			return "application/pdf";
		} else if (ext == ".svg") {
			return "image/svg+xml";
		}

		// unknown or not implemented
		return "application/octet-stream";
	}

	class tcp_connection :
		public boost::enable_shared_from_this<tcp_connection>
	{
		tcp::socket socket_;
		std::array<char, 4096> recv_buffer_;
		std::string buffer_;
		request_data request_;
		boost::xpressive::sregex regex_req_line_, regex_header_;
		boost::filesystem::path root_;
		enum class state {
			wait_for_request_line,
			wait_for_headers,
			reception_completed,
		}state_;

		tcp_connection(boost::asio::io_service& io_service, const std::string &root)
			: socket_(io_service)
			, root_(boost::filesystem::absolute(root))
		{
			using namespace boost::xpressive;

			// https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html
			// METHOD SP Request-URI SP HTTP-Version
			// ([^ ]+) (?:https?://[^/]+)?([^?]*)[^ ]* .*
			regex_req_line_ = (s1 = +~as_xpr(' ')) >> ' ' >> !("http" >> !as_xpr('s') >> "://" >> +~as_xpr('/')) >> (s2 = *~as_xpr('?')) >> *~as_xpr(' ') >> ' ' >> *_;
		}

		void handle_write(boost::shared_ptr<std::vector<char> > /* sendbuf */, size_t /* len */) {}

		void handle_receive(const boost::system::error_code& error, size_t len)
		{
			if (!error || error == boost::asio::error::message_size) {
				// std::string message(recv_buffer_.data(), len);
				// std::cout << message << std::endl;

				// parse request message
				buffer_.append(recv_buffer_.data(), len);

				if (state_ == state::wait_for_request_line) {
					auto pos = buffer_.find("\r\n");

					if (pos == std::string::npos) {
						return;
					}

					std::string line = buffer_.substr(0, pos);
					buffer_.erase(0, pos + 1);

					boost::xpressive::smatch what;

					if (regex_search(line, what, regex_req_line_)) {
						if (what[1] != "GET") {
							// not implemented
							request_.method = what[1];
							state_ = state::reception_completed;
						} else {
							request_.method = what[1];
							request_.uri = what[2];
							state_ = state::wait_for_headers;
						}
					} else {
						// invalid request line or invalid regex
						request_.invalid = true;
						state_ = state::reception_completed;
					}
				}

				if (state_ == state::wait_for_headers) {
					// not implemented
					state_ = state::reception_completed;
				}

				if (state_ == state::reception_completed) {
					std::string status_line;
					std::string headers;
					std::string response;

					headers += "Connection: close\r\n";
					headers += "Date: " + format_time(boost::posix_time::second_clock::universal_time()) + "\r\n";
					headers += "Server: Disclose Microserver\r\n";
					headers += "Connection: close\r\n";

					boost::shared_ptr<std::vector<char> > sendbuf(new std::vector<char>);
					auto pbuf = sendbuf.get();

					if (request_.invalid) {
						status_line = "HTTP/1.1 400 Bad Request\r\n";
						headers += "Content-Length: 0\r\n\r\n";
						pbuf->insert(pbuf->begin(), status_line.begin(), status_line.end());
						pbuf->insert(pbuf->end() - 1, headers.begin(), headers.end());
					} else {
						if (request_.method != "GET") {
							status_line = "HTTP/1.0 501 Not Implemented\r\n";
							headers += "Content-Length: 0\r\n\r\n";
							pbuf->insert(pbuf->begin(), status_line.begin(), status_line.end());
							pbuf->insert(pbuf->end() - 1, headers.begin(), headers.end());
						} else {
							boost::system::error_code error;
							auto path =canonical(absolute(root_ / request_.uri), error);

							if (error.value() == boost::system::errc::success) {
								if (is_directory(path)) {
									error.assign(boost::system::errc::is_a_directory, boost::system::generic_category());
									for (auto &f : { ".html", ".htm" }) {
										if (exists((path / "index").replace_extension(f))) {
											error.assign(boost::system::errc::success, boost::system::generic_category());
											path = (path / "index").replace_extension(f);
											break;
										}
									}
								}
							}

							if (error.value() != boost::system::errc::success || !exists(path)) {
								std::string message;

								if (is_directory(path)) {
									message = (path / L"index.html").string() + " not found";
								} else {
									message = path.string() + " not found";
								}

								status_line = "HTTP/1.1 404 Not Found\r\n";
								headers += "Content-Length: " + std::to_string(message.size()) + "\r\n\r\n";

								pbuf->insert(pbuf->begin(), status_line.begin(), status_line.end());
								pbuf->insert(pbuf->end() - 1, headers.begin(), headers.end());
								pbuf->insert(pbuf->end() - 1, message.begin(), message.end());
							} else {
								auto last_modified = boost::posix_time::from_time_t(last_write_time(path));

								status_line = "HTTP/1.1 200 OK\r\n";
								headers += "Last-Modified: " + format_time(last_modified) + "\r\n";
								headers += "Content-Type: " + content_type(path.extension().string()) + "\r\n";
								headers += "Content-Length: " + std::to_string(file_size(path)) + "\r\n\r\n";
								pbuf->insert(pbuf->begin(), status_line.begin(), status_line.end());
								pbuf->insert(pbuf->end() - 1, headers.begin(), headers.end());

								std::ifstream ifs(path.wstring(), std::ios::binary);
								char buffer[200];

								while (ifs) {
									ifs.read(buffer, 200);
									pbuf->insert(pbuf->end() - 1, buffer, buffer + ifs.gcount());
								}
							}
						}
					}

					// std::cout << "status line\n" << status_line << std::endl;
					// std::cout << "headers\n" << headers << std::endl;

					async_write(socket_, boost::asio::buffer(*sendbuf),
						boost::bind(&tcp_connection::handle_write, shared_from_this(), sendbuf, len));

					start_receive();
				}
			}
		}

		void start_receive() {
			state_ = state::wait_for_request_line;
			request_ = {};
			buffer_.clear();
			socket_.async_receive(
				boost::asio::buffer(recv_buffer_),
				boost::bind(&tcp_connection::handle_receive, shared_from_this(),
					boost::asio::placeholders::error, _2));
		}

	public:
		typedef boost::shared_ptr<tcp_connection> pointer;

		static pointer create(boost::asio::io_service& io_service, const std::string &root) {
			return pointer(new tcp_connection(io_service, root));
		}

		tcp::socket& socket() {
			return socket_;
		}

		void start() {
			// std::cout << socket_.remote_endpoint().address() << ":" << socket_.remote_endpoint().port() << std::endl;

			start_receive();
		}
	};

	class tcp_server : public server_base {
		tcp::acceptor acceptor_;
		boost::filesystem::path root_;
		tcp_connection::pointer connection_;

		void start_accept() {
			connection_ = tcp_connection::create(acceptor_.get_io_service(), root_.string());

			acceptor_.async_accept(connection_->socket(),
				boost::bind(&tcp_server::handle_accept, this, connection_,
					boost::asio::placeholders::error));

			// std::cout << "port: " << acceptor_.local_endpoint().port() << std::endl;
		}

		void handle_accept(tcp_connection::pointer new_connection,
			const boost::system::error_code& error) {
			if (!error) {
				new_connection->start();
				start_accept();
			}
		}

	public:
		tcp_server(boost::asio::io_service &io_service)
			: acceptor_(io_service, tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 0))
		{
		}

		bool start(const boost::filesystem::path &root) override
		{
			if (connection_)
				return false;

			root_ = root;

			try {
				start_accept();
			} catch (std::exception &) {
				return false;
			}

			return true;
		}

		void stop() override
		{
			connection_.reset();
			acceptor_.close();
		}

		unsigned short get_port() override
		{
			return acceptor_.local_endpoint().port();
		}

		const boost::filesystem::path &get_document_root() override
		{
			return root_;
		}
	};
}
