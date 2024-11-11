/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/01 13:03:09 by cgray             #+#    #+#             */
/*   Updated: 2024/11/11 17:08:30 by cgray            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(){}
Server::Server(const Server &src){*this = src;}
Server	&Server::operator = (const Server &src){return (*this);}
Server::Server(int port, std::string password) : _port(port), _password(password)
{
	struct tm *timeinfo;
	time_t rawtime;
	timeinfo = localtime(&rawtime);
	char	buf[80];
	strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", timeinfo);
	std::string str(buf);
	_start_time = str;

	
	//getaddrinfo?
	//create server listening socket socket(domain, type, protocol)
		//--unsure if we can hardcode these with AF_INET, SOCK_STREAM, 0, or need getaddrinfo
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

	_server_user = new User("ServerUser",  "", "");
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
	delete _server_user;
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

	// std::cout << "socket: " << connection_socket << "\n";

	//set conn_fd to non-blocking (must use F_SETFL to do so)
	//might not be needed
	fcntl(connection_socket, F_SETFL, O_NONBLOCK);


	std::string	host = inet_ntoa(new_address.sin_addr);

	std::cout << "Connected to: " << host
			<< ":" << new_address.sin_port
			<< " on FD: " << connection_socket << "\n";

	//need to store this
	User *new_user = new User("",  "", host);
	_users.push_back(new_user);
	new_user->set_fd(connection_socket);

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
		perror("recv");
		return (1);
	}
		// throw std::runtime_error("Error in recv()");

	else if (bytes == 0)
	{
		std::cout << "client disconnected\n";
		close(user->get_fd());
		return (1);
	}

	std::cout << "buf: " << buf;
	_msg.append(buf);
	if (_msg.find_first_of("\n") == _msg.npos)
		return (0);
	std::cout << "msg: " << _msg << "\n";


	int ret = Server::get_command(user, _msg);


	return (ret);
}

int Server::get_command(User *user, std::string msg)
{
	std::stringstream	ss(msg);
	std::string			word;
	int					ret = 0;

	typedef int (Server::*CommandFunc)(User *, std::stringstream &);
	std::map<std::string, CommandFunc> command_map;
	command_map["PASS"] = &Server::PASS;
	command_map["QUIT"] = &Server::QUIT;
	command_map["KILL"] = &Server::KILL;
	command_map["PING"] = &Server::PING;
	command_map["LIST"] = &Server::LIST;
	command_map["MODE"] = &Server::MODE;
	command_map["INVITE"] = &Server::INVITE;
	command_map["KICK"] = &Server::KICK;
	command_map["PART"] = &Server::PART;
	command_map["PRIVMSG"] = &Server::PRIVMSG;
	command_map["WHO"] = &Server::WHO;
	command_map["NICK"] = &Server::NICK;
	command_map["TOPIC"] = &Server::TOPIC;
	command_map["USER"] = &Server::USER;
	command_map["OPER"] = &Server::OPER;

	//read each word in _msg, if word is a command, call corresponding command function
	while (ss >> word)
	{
		std::map<std::string, CommandFunc>::iterator it = command_map.find(word);
		if (it != command_map.end())
			ret = (this->*(it->second))(user, ss);
	}
	return (ret);
}

void	Server::reply(User *user, std::string prefix, std::string command,
						std::string target, std::string message)
{
	std::string	reply;
	if (!prefix.empty())
		reply.append(prefix + " ");
	// else
	// 	reply.append(_server_prefix + " ");
	reply.append(command + " ");
	if (!target.empty())
		reply.append(target);
	else
		reply.append(user->get_nick());
	if (!message.empty())
		reply.append(" " + message);
	if (reply.find_last_of("\n") == std::string::npos)
		reply.append("\n");

	int	bytes_sent = send(user->get_fd(), reply.c_str(), reply.length(), 0);
	if (bytes_sent <= 0) //TODO not sure what bytes sent==0 means, might be valid message
		std::cout << RED << "Failed to send to FD:" << user->get_fd() << ":\t" << reply << "\n" << RST;
	std::cout << CYN << "FD:" << user->get_fd() << ":\t" << reply << "\n" << RST;
}

void	Server::add_user(User *user)
{
	_users.push_back(user);
	register_client(user);

}

void	Server::remove_user(User *user)
{
	_users.erase(std::find(_users.begin(), _users.end(), user));

	//TODO will need to remove from each channel as well

	delete user;
}

void	Server::register_client(User *user)
{
	user->set_send_buf(RPL_WELCOME(user_id(user->get_nick(), user->get_user()), user->get_nick()));
	user->set_send_buf(RPL_YOURHOST(user->get_nick(), "ft_irc", "0.01"));
	user->set_send_buf(RPL_CREATED(user->get_nick(), this->get_start_time()));
	user->set_send_buf(RPL_MYINFO(user->get_nick(), "localhost", "0.01", "io", "kost", "k"));
	user->set_send_buf(RPL_ISUPPORT(user->get_nick(), "CHANNELLEN=32 NICKLEN=9 TOPICLEN=307"));
}

std::string	Server::get_password(){return (_password);}
std::string	Server::get_start_time(){return (_start_time);}
int	Server::get_epoll_socket(){return (_epoll_socket);}
int	Server::get_server_socket(){return (_server_socket);}
