/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/01 13:03:09 by cgray             #+#    #+#             */
/*   Updated: 2024/11/05 17:09:45 by cgray            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(){}
Server::Server(const Server &src){*this = src;}
Server	&Server::operator = (const Server &src){return (*this);}
Server::Server(int port, std::string password) : _port(port), _password(password)
{
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
		perror("recv");
		// throw std::runtime_error("Error in recv()");
	else if (bytes == 0)
	{
		std::cout << "client disconnected\n";
		close(user->get_fd());
		return (-1);
	}

	std::cout << "buf: " << buf;
	// _msg.append(buf);
	// if (_msg.find_first_of("\n") == _msg.npos)
	// 	return (0);
	// std::cout << "msg: " << _msg << "\n";

	return (0);
}

int	Server::get_epoll_socket(){return (_epoll_socket);}
int	Server::get_server_socket(){return (_server_socket);}
