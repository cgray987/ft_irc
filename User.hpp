/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/04 16:02:28 by cgray             #+#    #+#             */
/*   Updated: 2024/11/12 15:53:41 by cgray            ###   ########.fr       */
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
		bool		_auth;
		bool		_op;
		std::string	_read_buf;

		User();
	public:
		User(std::string nick, std::string user, std::string host);
		User(const User &src);
		User &operator = (const User &src);
		~User();

		//getters
		int			get_fd();
		std::string	get_nick();
		std::string	get_user();
		std::string	get_host();
		std::string	get_prefix();
		std::string	get_read_buf();

		bool		get_auth();
		bool		get_op();


		//setters
		void	set_fd(int fd);
		void	set_nick(std::string nick);
		void	set_user(std::string user);
		void	set_host(std::string host);
		void	set_auth(bool auth);
		void	set_op(bool op_status);
		void	set_send_buf(std::string buf);
};
