/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/04 16:02:25 by cgray             #+#    #+#             */
/*   Updated: 2024/11/04 16:14:27 by cgray            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "User.hpp"

User::User(){}
User::User(const User &src) : _nick(src._nick), _user(src._user), _host(src._host){*this = src;}
User &User::operator = (const User &src){return (*this);}
User::User(std::string nick, std::string user, std::string host) : _nick(nick), _user(user), _host(host)
{

}
User::~User()
{
	std::cout << "removing user: " << _user << "\n";
	if (_fd != -1)
		close(_fd);
}

int	User::get_fd() {return _fd;}
void	User::set_fd(int fd) {_fd = fd;}
