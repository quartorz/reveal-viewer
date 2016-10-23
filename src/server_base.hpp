#pragma once

#include <string>

#include <boost/filesystem/path.hpp>

class server_base {
public:
	virtual bool start(const boost::filesystem::path &root) = 0;
	virtual void stop() = 0;
	virtual unsigned short get_port() = 0;
};
