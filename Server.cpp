#include "Server.hpp"
#include "Log.hpp"

Server::Server(){}
Server::Server(const Server &src){*this = src;}
Server	&Server::operator = (const Server &src){return (*this);}
Server::Server(int port, std::string password) : _port(port), _password(password)
{
	init_command_map();

	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo = localtime(&rawtime);
	char	buf[80];
	strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", timeinfo);
	std::string str(buf);
	_start_time = str;

	//create server listening socket socket(domain, type, protocol)
	_server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_socket == -1)
		throw std::runtime_error("Socket creation failure!");

	//ip address schema
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET; //always req'd
	server_address.sin_port = htons(_port); //convert port to network byte
	server_address.sin_addr.s_addr = INADDR_ANY; //binds socket to all available interfaces (=0)

	if (bind(_server_socket, reinterpret_cast<struct sockaddr *>(&server_address), sizeof(server_address)) == -1)
		throw std::runtime_error("Bind failure!");

	if (listen(_server_socket, SOMAXCONN) == -1)
		throw std::runtime_error("Listen failure!");

	//epoll socket
	struct epoll_event event;
	_epoll_socket = epoll_create1(0);
	if (_epoll_socket == -1)
		throw std::runtime_error("epoll error!");

	_server_user = new User("ServerNick", "ServerUser", "");
	_server_user->set_fd(_server_socket);
	event.events = EPOLLIN | EPOLLET;
	event.data.ptr = _server_user;

	if (epoll_ctl(_epoll_socket, EPOLL_CTL_ADD, _server_socket, &event) == -1)
	{
		close(_epoll_socket);
		throw std::runtime_error("cannot add server socket to epoll!");
	}
}

Server::~Server()
{
	std::cout << "Server Destructor\n";
	close(_server_socket);
	close(_epoll_socket);
	while (!_users.empty())
	{
		delete _users.back();
		_users.pop_back();
	}
	//delete channels?
	delete _server_user;
}

void	Server::init_command_map()
{
	typedef int (Server::*CommandFunc)(User *, std::stringstream &);
	std::map<std::string, CommandFunc> command_map;
	command_map["CAP"] = &Server::CAP;
	command_map["PASS"] = &Server::PASS;
	command_map["QUIT"] = &Server::QUIT;
	command_map["KILL"] = &Server::KILL;
	command_map["PING"] = &Server::PING;
	command_map["LIST"] = &Server::LIST;
	command_map["MODE"] = &Server::MODE;
	command_map["INVITE"] = &Server::INVITE;
	command_map["KICK"] = &Server::KICK;
	command_map["JOIN"] = &Server::JOIN;
	command_map["PART"] = &Server::PART;
	command_map["PRIVMSG"] = &Server::PRIVMSG;
	command_map["WHO"] = &Server::WHO;
	command_map["NICK"] = &Server::NICK;
	command_map["TOPIC"] = &Server::TOPIC;
	command_map["USER"] = &Server::USER;
	command_map["OPER"] = &Server::OPER;
	command_map["REMOVE_CHANNEL"] = &Server::REMOVE_CHANNEL;
	this->_command_map = command_map;
}

int	Server::new_connection()
{
	sockaddr_in	new_address;
	socklen_t	address_len = sizeof(new_address);

	int	connection_socket = accept(_server_socket,
				reinterpret_cast<struct sockaddr *>(&new_address),
				reinterpret_cast<socklen_t *>(&address_len));
	std::cout << connection_socket << "\n";
	if (connection_socket == -1)
		throw std::runtime_error("cannot connect to user");

	//set conn_fd to non-blocking (must use F_SETFL to do so)
	fcntl(connection_socket, F_SETFL, O_NONBLOCK);

	std::string	host = inet_ntoa(new_address.sin_addr);

	std::cout << CYN << "Connected to: " << host
			<< ":" << new_address.sin_port
			<< " on FD: " << connection_socket << "\n" << RST;

	//store user
	User *new_user = new User("", "", host);
	_users.push_back(new_user);
	new_user->set_fd(connection_socket);

	//might need to be somewhere else
	// Server::register_client(new_user);

	// new_user->set_auth(true); --now in PASS
	// ^^ might have to use this here instead of register_client as we are now registering in USER and NICK

	struct epoll_event	ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = new_user;

	if (epoll_ctl(_epoll_socket, EPOLL_CTL_ADD, connection_socket, &ev))
	{
		close(connection_socket);
		throw std::runtime_error("cannot add server socket to epoll!");
	}
	return (0);
}

int	Server::client_message(User *user)
{
	char	buf[1024];
	memset(buf, 0, sizeof(buf));

	int bytes = recv(user->get_fd(), buf, sizeof(buf), 0);
	if (bytes == -1)
	{
		std::cout << RED << "~Server~ recv() failure [456]\n" << RST;
		// perror("recv");
		throw std::runtime_error("Error in recv()");
		return (1);
	}

	else if (bytes == 0)
	{
		std::cout << "client disconnected\n";
		remove_user(user);
		close(user->get_fd());
		return (1);
	}

	LOG("Received raw message from FD: " << user->get_fd() << ": " << std::string(buf, bytes));
	// std::cout << "\tbuf: " << buf;
	_msg.append(buf);

	// processing each commang in _msg
	std::stringstream ss(_msg);
	std::string line;
	// LOG("Processing command: " + line + " from user " + user->get_user());

	while(std::getline(ss, line, '\n'))
	{
		if(line.empty()) continue;
		// std::cout << "\tProcessing command: " << line << std::endl;
		get_command(user, line);
	}
	return 0;
}

int Server::get_command(User *user, std::string msg)
{
	std::stringstream	ss(msg);
	std::string			word;
	int					ret = 0;
	ss >> word;

	std::map<std::string, CommandFunc>::iterator it = _command_map.find(word);
	if (it != _command_map.end())
	{
		std::stringstream params;
		params << ss.rdbuf(); // Get the remaining parameters
		LOG("Calling command function for: " + word + " params: " + params.str());
		// std::cout << "Calling command function for: " << word << " with params: " << RED << params.str() << std::endl << RST;
		ret = (this->*(it->second))(user, params);
		// clearing after processing the command
		if (ret == 0)
			_msg.clear();
	}
	else
	{
		reply(user, "", "421", word, ":Unknown command"); // ERR_UNKNOWNCOMMAND
		// clearing invalids also --can't do this because command might be valid, but not complete (nc ctrl-d from subject)
		// _msg.clear();
	}
	return ret;
}

void	Server::reply(User *user, std::string prefix, std::string command,
						std::string target, std::string message)
{
	std::string	reply;
	if (!prefix.empty())
		reply += ":" + prefix + " ";
	reply += command;
	if (!target.empty())
		reply += " " + target;
	if (!message.empty())
		reply += " :" + message;
	reply += "\n"; // correct line ending ? 

	send (user->get_fd(), reply.c_str(), reply.length(), 0);
	LOG("Sent reply to FD " << user->get_fd() << ": " << reply);
}

void	Server::add_user(User *user)
{
	_users.push_back(user);
	// register_client(user);
}

void	Server::remove_user(User *user)
{
	_users.erase(std::find(_users.begin(), _users.end(), user));
	//TODO will need to remove from each channel as well
	delete user;
}

void	Server::register_client(User *user)
{
	user->set_reg(true);
	user->set_send_buf(RPL_WELCOME(user_id(user->get_nick(), user->get_user()), user->get_nick()));
	user->set_send_buf(RPL_YOURHOST(user->get_nick(), "ft_irc", "v0.01"));
	user->set_send_buf(RPL_CREATED(user->get_nick(), this->get_start_time()));
	user->set_send_buf(RPL_MYINFO(user->get_nick(), "localhost", "0.01", "io", "kost", "k"));
	user->set_send_buf(RPL_ISUPPORT(user->get_nick(), "CHANNELLEN=32 NICKLEN=9 TOPICLEN=307"));
	LOG("Registered user: " + user->get_user() + "@" + user->get_host() + " known as " + user->get_nick());
	Server::send_server_response(user, user->get_read_buf());
}

void	Server::send_server_response(User *user, std::string send_buf)
{
	std::istringstream	buf(send_buf);
	std::string			reply;

	send(user->get_fd(), send_buf.c_str(), send_buf.size(), 0);
	while (getline(buf, reply))
		std::cout << "Server Message to client [" << user->get_fd() << "]: " << BLU << reply << "\n" << RST;
}

std::string	Server::get_password(){return (_password);}
std::string	Server::get_start_time(){return (_start_time);}
int	Server::get_epoll_socket(){return (_epoll_socket);}
int	Server::get_server_socket(){return (_server_socket);}

// channels

Channel *Server::get_channel(const std::string &name)
{
	std::map<std::string, Channel *>::iterator it = _channels.find(name);
	if (it != _channels.end())
		return it->second;
	return NULL;
}

Channel *Server::create_channel(const std::string &name)
{
	Channel *channel = new Channel(name);
	_channels[name] = channel;
	return channel;
}

// I added basically notification for users that the channel
// is being removed
// and removing from users channel list
void Server::remove_channel(const std::string &name)
{
	std::map<std::string, Channel *>::iterator it = _channels.find(name);

	if (it != _channels.end())
	{
		Channel *channel = it->second;

		for (std::set<User *>::iterator user_it = channel->get_members().begin();
			user_it != channel->get_members().end(); ++user_it)
		{
			User *user = *user_it;
			user->leave_channel(channel);
			reply(user, "", "NOTICE", "", "Channel " + name + " has been removed.");
		}

		delete channel;
		delete it->second;
		_channels.erase(it);
	}
}
