/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/01 13:03:06 by cgray             #+#    #+#             */
/*   Updated: 2024/11/05 17:12:52 by cgray            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"


int	main(int ac, char **av)
{
	if (ac != 3)
		return (std::cerr << "Error: bad arguments\n"
					<< "usage: ./ircserv <port> <password>\n", 1);
	//if port is not number/out of range, 1024<port<65535
	//bad password? 1 < len(password) < 30


	try
	{
		//server constructor
		Server				server(atoi(av[1]), av[2]);
		struct epoll_event	events[10];
		int					server_on = 1;


		while (server_on)
		{
			int	nfds = epoll_wait(server.get_epoll_socket(), events, 10, -1);
			if (nfds == -1)
				throw std::runtime_error("epoll_wait error!");

			for (int i = 0; i < nfds; ++i)
			{
				//see if fd is read or write?

				User *user = reinterpret_cast<User *>(events[i].data.ptr);

				if (user->get_fd() == server.get_server_socket())
					server.new_connection();
				else if (server.client_message(user) == -1)
				{
					epoll_ctl(server.get_epoll_socket(), EPOLL_CTL_DEL, events[i].data.fd, NULL);
				}
			}
		}
	}
	catch (std::exception &e)
	{
		return (std::cerr << e.what() << "\n", 1);
	}
}
