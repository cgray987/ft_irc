/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/04 16:02:28 by cgray             #+#    #+#             */
/*   Updated: 2024/11/04 16:14:54 by cgray            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <vector>
#include <unistd.h>

class User
{
	private:
		std::string	_nick;
		std::string	_user;
		std::string	_host;
		int			_fd;

		User();
	public:
		User(std::string nick, std::string user, std::string host);
		User(const User &src);
		User &operator = (const User &src);
		~User();

		int		get_fd();
		void	set_fd(int fd);
};
