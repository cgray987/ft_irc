/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/01 13:03:20 by cgray             #+#    #+#             */
/*   Updated: 2024/11/11 17:07:04 by cgray            ###   ########.fr       */
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
#include <algorithm> //find
#include <vector>
#include <map>
#include <cstring>
#include <sstream>
#include <exception>

#include "User.hpp"
#include "Colors.hpp"

# define user_id(nickname, username) (":" + nickname + "!" + username + "@localhost")

# define RPL_WELCOME(user_id, nickname) (":localhost 001 " + nickname + " :Welcome to the Internet Relay Network " + user_id + "\r\n")
# define RPL_YOURHOST(client, servername, version) (":localhost 002 " + client + " :Your host is " + servername + " (localhost), running version " + version + "\r\n")
# define RPL_CREATED(client, datetime) (":localhost 003 " + client + " :This server was created " + datetime + "\r\n")
# define RPL_MYINFO(client, servername, version, user_modes, chan_modes, chan_param_modes) (":localhost 004 " + client + " " + servername + " " + version + " " + user_modes + " " + chan_modes + " " + chan_param_modes + "\r\n")
# define RPL_ISUPPORT(client, tokens) (":localhost 005 " + client + " " + tokens " :are supported by this server\r\n")

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
		std::string			_start_time;


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
		std::string	get_password();
		std::string	get_start_time();

		//server functions
		int	client_message(User *user);
		int	new_connection();
		int	auth_user(User *user);
		int	get_command(User *user, std::string msg);
		void	reply(User *user, std::string prefix, std::string command,
						std::string target, std::string message);
		void	add_user(User *user);
		void	remove_user(User *user);
		void	register_client(User *user);

		//server commands
		// Command function implementations
		int OPER(User *user, std::stringstream &command);
		int PASS(User *user, std::stringstream &command);
		int QUIT(User *user, std::stringstream &command);
		int KILL(User *user, std::stringstream &command);
		int PING(User *user, std::stringstream &command);
		int LIST(User *user, std::stringstream &command);
		int MODE(User *user, std::stringstream &command);
		int INVITE(User *user, std::stringstream &command);
		int KICK(User *user, std::stringstream &command);
		int PART(User *user, std::stringstream &command);
		int PRIVMSG(User *user, std::stringstream &command);
		int WHO(User *user, std::stringstream &command);
		int NICK(User *user, std::stringstream &command);
		int TOPIC(User *user, std::stringstream &command);
		int USER(User *user, std::stringstream &command);

		//error messages
};
