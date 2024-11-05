/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/01 13:03:20 by cgray             #+#    #+#             */
/*   Updated: 2024/11/05 16:13:47 by cgray            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <string>
#include <sys/socket.h> //socket, bind
#include <sys/epoll.h> //epoll
#include <netinet/in.h> //sockaddr
#include <fcntl.h> //fcntl
#include <unistd.h> //open, close
#include <arpa/inet.h> //htons -- convert to network byte order
#include <vector>
#include <cstring>
#include <exception>

#include "User.hpp"

class Server
{
	private:
		int				_port;
		int				_server_socket;
		int				_epoll_socket;
		std::string		_password;

		std::vector<User *>	_users;
		User				*_server_user;
		std::string			_msg;


		//default constructor
		Server();

	public:
		//constructors
		Server(const Server &src);
		Server(int port, std::string password);

		//overloaded ops
		Server &operator = (const Server &src);

		//destructor
		~Server();

		//setters

		//getters
		int	get_epoll_socket();
		int	get_server_socket();

		//server functions
		int	client_message(User *user);
		int	new_connection();

		//error messages
};
