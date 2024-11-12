/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/01 13:03:06 by cgray             #+#    #+#             */
/*   Updated: 2024/11/12 16:00:59 by cgray            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "signal.h"

bool	server_on = false; //unsure if this is legal

void	signal_handler(int signal)
{
	if (signal == SIGINT)
		server_on = false;
}

int	main(int ac, char **av)
{
	if (ac != 3)
		return (std::cerr << RED << "Error: bad arguments\n" << RST
					<< "usage: ./ircserv <port> <password>\n", 1);
	int	port = atoi(av[1]);
	std::string	password = av[2];

	if (port < 1024 || port > 65535 || password.empty() || password.length() > 30)
		return (std::cerr << RED << "Error: arguments out of range\n" << RST
					<< "\t1024 < port < 65535\n\t1 < password_len < 30\n", 1);

	struct sigaction	act;
	act.sa_handler = signal_handler;
	act.sa_flags = 0;
	sigaction(SIGINT, &act, 0);

	try
	{
		//server constructor
		Server				server(port, password);
		struct epoll_event	events[10];

		server_on = true;
		while (server_on == true)
		{
			int	nfds = epoll_wait(server.get_epoll_socket(), events, 10, -1);
			if (nfds == -1)
				throw std::runtime_error("epoll_wait error!"); //TODO sometimes this is called when sending SIGINT

			for (int i = 0; i < nfds; ++i)
			{
				//see if fd is read or write?

				User *user = reinterpret_cast<User *>(events[i].data.ptr);

				if (user->get_fd() == server.get_server_socket())
					server.new_connection();
				else if (server.client_message(user) == -1)
				{
					server_on = false;
				}
			}
		}
	}
	catch (std::exception &e){ return (std::cerr << e.what() << "\n", 1); }
}