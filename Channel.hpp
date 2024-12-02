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
		std::set<User *> _invited_users; // added for tracking invited users
		size_t _userlimit; // for l mode
		std::string _key; // for k mode

	public:
		Channel();
		Channel(const std::string &name);
		~Channel();

		// getters
		const std::string& get_name() const;
		const std::string& get_topic() const;
		const std::set<User*> &get_members() const;
		const std::set<User*> &get_operators() const;
		const std::set<User *> &get_invitees() const;
		const std::string &get_key() const;
		size_t get_user_limit() const;

		// setters
		void add_member(User* user);
		void remove_member(User* user);
		bool is_member(User* user) const;
		void set_topic(const std::string &topic);

		// ops
		void add_operator(User* user);
		void remove_operator(User* user);
		bool is_operator(User* user) const;

		// modes
		void set_mode(char mode, bool value);
		bool get_mode(char mode) const;
    	std::string str_modes() const;

		void set_key(const std::string &key);
		void remove_key();
		void set_user_limit(size_t limit);
		void remove_user_limit();

		// invites
		void add_invite(User* user);
		bool is_invited(User* user) const;
		void remove_invite(User* user);

	
};
