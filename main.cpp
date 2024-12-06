#include "Server.hpp"

#include "signal.h"
#include <cerrno>

#ifdef DEBUG
// #warning "DEBUG is on"
#endif


bool	server_on = false;

void	signal_handler(int signal)
{
	if (signal == SIGINT)
		server_on = false;
}

int	arguments(int ac, char **av)
{
	if (ac != 3)
		return (std::cerr << RED << "Error: bad arguments\n" << RST
					<< "usage: ./ircserv <port> <password>\n", 1);

	int			port = atoi(av[1]);
	std::string	password = av[2];

	if (port < 1024 || port > 65535 || password.empty() || password.length() > 30)
		return (std::cerr << RED << "Error: arguments out of range\n" << RST
					<< "\t1024 < port < 65535\n\t1 < password_len < 30\n", 1);
	return (0);
}

int	main(int ac, char **av)
{
	if (arguments(ac, av))
		return (1);

	// for logging
	#ifdef DEBUG
    _logfile.open("server.log", std::ios_base::trunc);
    if (!_logfile.is_open()) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }
	#endif

	int			port = atoi(av[1]);
	std::string	password = av[2];

	struct sigaction	action;
	action.sa_handler = signal_handler;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask); //not 100% sure this is legal -- not in allowed functions but idk why it would be disallowed
	sigaction(SIGINT, &action, 0);

	try
	{
		//server constructor
		Server				server(port, password);
		struct epoll_event	events[10];

		server_on = true;
		while (server_on == true)
		{
			int	nfds = epoll_wait(server.get_epoll_socket(), events, 10, -1);
			if (nfds == -1 && errno != EINTR)
				throw std::runtime_error("epoll_wait error!");

			for (int i = 0; i < nfds; ++i)
			{
				//see if fd is read or write?

				User *user = reinterpret_cast<User *>(events[i].data.ptr);

				if (user->get_fd() == server.get_server_socket())
					server.new_connection();
				else if (server.client_message(user) == -1)
				{
					#ifdef DEBUG
					LOG("Server shutting down.");
					_logfile.close();
					#endif

					server_on = false;
				}
			}
		}
	}
	catch (std::exception &e){ return (std::cerr << e.what() << "\n", 1); }
}
