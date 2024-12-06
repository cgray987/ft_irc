#include "User.hpp"

User::User(){}
User::User(const User &src):
		_nick(src._nick), _user(src._user), _host(src._host),
		_fd(src._fd), _auth(src._auth), _op(src._op), _registered(src._registered)
{*this = src;}
User &User::operator = (const User &src)
{
	_nick = src._nick;
	_user = src._user;
	_host = src._host;
	_fd = src._fd;
	_auth = src._auth;
	_op = src._op;
	_registered = src._registered;
	return (*this);
}

User::User(std::string nick, std::string user, std::string host):
		_nick(nick), _user(user), _host(host),
		_fd(-1), _auth(false), _op(false), _registered(false)
{

}
User::~User()
{
	std::cout << "user destructor: " << this->get_prefix()  << " on fd: " << this->get_fd() << "\n";
	if (_fd != -1)
		close(_fd);
}

int	User::get_fd() {return _fd;}
std::string	User::get_nick(){return _nick;}
std::string	User::get_user(){return _user;}
std::string	User::get_realname(){return _realname;}
std::string	User::get_host(){return _host;};
std::string	User::get_read_buf(){return _read_buf;}
std::string	User::get_prefix(){return (":" + _nick + "!" + _user + "@" + _host);}
bool	User::get_auth(){return _auth;}
bool	User::get_op(){return _op;}
bool	User::get_reg(){return _registered;};

void	User::set_host(std::string host) {_host = host;}
void	User::set_fd(int fd) {_fd = fd;}
void	User::set_auth(bool auth){_auth = auth;}
void	User::set_op(bool op_status){_op = op_status;}
void	User::set_send_buf(std::string buf){_read_buf += buf;}
void	User::set_nick(std::string nick){_nick = nick;};
void	User::set_user(std::string user){_user = user;};
void	User::set_realname(std::string realname){_realname = realname;};
void	User::set_reg(bool reg){_registered = reg;};

// channels
void User::join_channel(Channel* channel)
{
    _channels.insert(channel);
}

void User::leave_channel(Channel* channel)
{
    _channels.erase(channel);
}

const std::set<Channel*>& User::get_channels() const
{
    return _channels;
}
