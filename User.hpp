/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2024/11/19 15:13:59 by cgray            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#pragma once

#include <iostream>
#include <vector>
#include <unistd.h>
#include <set>

class Channel;

class User
{
	private:
		std::string	_nick;
		std::string	_user;
		std::string	_host;
		std::string _realname;
		int			_fd;
		bool		_auth;
		bool		_op;
		bool		_registered;
		std::string	_read_buf;
		// std::string	_user_messge;

		std::set<Channel *> _channels;

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
		std::string	get_realname();

		std::string	get_prefix();
		std::string	get_read_buf();

		bool		get_auth();
		bool		get_op();
		bool		get_reg();

		//setters
		void	set_fd(int fd);
		void	set_nick(std::string nick);
		void	set_user(std::string user);
		void	set_host(std::string host);
		void	set_realname(std::string nick);
		void	set_auth(bool auth);
		void	set_reg(bool reg);

		void	set_op(bool op_status);
		void	set_send_buf(std::string buf);

		// channels
		void join_channel(Channel *channel);
		void leave_channel(Channel *channel);
		const std::set<Channel *> &get_channels() const;
};
