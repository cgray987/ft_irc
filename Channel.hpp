/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fvonsovs <fvonsovs@student.42prague.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/13 15:28:40 by fvonsovs          #+#    #+#             */
/*   Updated: 2024/11/13 15:53:26 by fvonsovs         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>
#include <set>
#include "User.hpp"

class User;

class Channel 
{
	private:
		std::string _name;
		std::string _topic;
		std::set<User *> _members;
		std::set<User *> _operators;
		std::set<char> _modes; 

	public:
		Channel();
		Channel(const std::string &name);
		~Channel();

		// getters
		const std::string& get_name() const;
		const std::string& get_topic() const;
		const std::set<User*> &get_members() const;
		const std::set<User*> &get_operators() const;

		// setters
		void add_member(User* user);
		void remove_member(User* user);
		bool is_member(User* user) const;
		void set_topic(const std::string& topic);

		// ops
		void add_operator(User* user);
		void remove_operator(User* user);
		bool is_operator(User* user) const;

		// modes
		void set_mode(char mode, bool value);
		bool get_mode(char mode) const;

};