/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: khlavaty <khlavaty@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/07 15:14:51 by cgray             #+#    #+#             */
/*   Updated: 2024/11/17 02:14:22 by khlavaty         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

#define VALID_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]{}\\|^`–-_"

/* CAP LS --supposed to send server's capabilities to client
	--unsure if we can just ignore */
int Server::CAP(User *user, std::stringstream &command)
{
	std::string buf;
	command >> buf;
	return (0);
}

int Server::NICK(User *user, std::stringstream &command)
{
	std::string	nick;
	bool		valid = true;

	command >> nick;
	if (nick.empty())
	{
		Server::reply(user, "", "431", "", ":No nickname given"); //ERR_NONICKNAMEGIVEN
		valid = false;
	}
	else if (nick.length() > 30)
		nick = nick.substr(0, 30);
	else if (nick.find_first_not_of(VALID_CHARS) != nick.npos
		|| isdigit(nick[0]))
	{
		Server::reply(user, "", "432", "", ":Erroneus nickname"); //ERR_ERRONEUSNICKNAME
		valid = false;
	}
	//ERR_NICKNAMEINUSE
	//set nickname
	if (valid == true)
	{
		//need to add to channels?
		reply(user, user->get_prefix(), "NICK", ":" + nick, "");
		user->set_nick(nick);
	}
	return (0);
}

int Server::USER(User *user, std::stringstream &command)
{
	if (user->get_auth() == false)
	{
		std::cout << RED << "Disconnected " << user->get_nick() << "due to auth failure\n" << RST;
	}
	std::string	username;
	command >> username;
	user->set_user(username);
	return (0);
}

int Server::PASS(User *user, std::stringstream &command)
{
	std::string	password;
	command >> password;
	if (Server::get_password() != password)
		Server::reply(user, "", "464", user->get_nick(), ":Password incorrect"); //ERR_PASSWDMISMATCH
	else if (user->get_auth() == false)
		user->set_auth(true);
	else
		Server::reply(user, "", "", user->get_nick(), ":User already authenticated."); //not standard irc error -- might need to be removed
	return (0);
}
int Server::QUIT(User *user, std::stringstream &command)
{
	std::cout << YEL << "Client disconnected\n" << RST;
	Server::remove_user(user);

	//TODO remove user's channels in server as well

	//clear user's msg buffer
	_msg.clear();
	return (0);
}
int Server::KILL(User *user, std::stringstream &command)
{
	if (user->get_op() == true)
	{
		std::cout << RED << "Server killed.\n" << RST;
		return (-1);
	}
	else
		Server::reply(user, "", "", user->get_nick(), ":Need op privleges to kill server");
	return (0);

}
int Server::OPER(User *user, std::stringstream &command){return (0);}
int Server::KICK(User *user, std::stringstream &command){return (0);}
int Server::PING(User *user, std::stringstream &command)
{
	reply(user, "", "PONG", "", "ft_irc");
	return (0);
}
int Server::INVITE(User *user, std::stringstream &command){return (0);}
int Server::TOPIC(User *user, std::stringstream &command){return (0);}
int Server::MODE(User *user, std::stringstream &command){return (0);}
int Server::WHO(User *user, std::stringstream &command){return (0);}
int Server::LIST(User *user, std::stringstream &command){return (0);}
int Server::PRIVMSG(User *user, std::stringstream &command){return (0);}


// https://dd.ircdocs.horse/refs/commands/join
int Server::JOIN(User *user, std::stringstream &command)
{
	std::string name;
	command >> name;

	if (name.empty())
	{
		// ERR_NEEDMOREPARAMS
		reply(user, "", "461", "JOIN", ":Not enough parameters");
		return 1;
	}

	Channel *channel = get_channel(name);
	if (!channel)
	{
		// create if doesnt exist
		channel = create_channel(name);
		channel->add_operator(user);

		// TODO: set default modes here
	}

	// TODO: add a check for invite-only

	if (channel->is_member(user))
		return 0;

	channel->add_member(user);
	// added so i can remove a channel as a testuser
	if (channel->get_operators().empty())
	{
		channel->add_operator(user);
		std::cout << user->get_nick() << " has been made a channel operator for " << name << std::endl;
	}
	user->join_channel(channel);

	// send join message to the user and members of channel
	std::string join_msg = ":" + user->get_nick() + " JOIN " + name + "\r\n";
	send(user->get_fd(), join_msg.c_str(), join_msg.length(), 0);

	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
	{
		if (*it != user)
			send((*it)->get_fd(), join_msg.c_str(), join_msg.length(), 0);
	}

	// send channel topic
	if (!channel->get_topic().empty())
		reply(user, "", "332", name, ":" + channel->get_topic()); // RPL_TOPIC
	else
		reply(user, "", "331", name, ":No topic set"); // RPL_NOTOPIC

	// send users in channel
	std::string members;
	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
		members = members + (*it)->get_nick() + " ";

	reply (user, "", "353", "= " + name, members); // RPL_NAMREPLY
	reply(user, "", "366", name, ":End of /NAMES list"); // RPL_ENDOFNAMES
	return 0;
}

// https://dd.ircdocs.horse/refs/commands/part
int Server::PART(User *user, std::stringstream &command)
{
	std::string name;
	command >> name;

	if (name.empty())
	{
		// ERR_NEEDMOREPARAMS
		reply(user, "", "461", "PART", ":Not enough parameters");
		return 1;
	}

	Channel *channel = get_channel(name);
	if (!channel)
	{
		reply(user, "", "403", name, ":No such channel"); // ERR_NOSUCHCHANNEL
		return 1;
	}
	if (!channel->is_member(user))
	{
		reply(user, "", "442", name, ":You're not on that channel"); // ERR_NOTONCHANNEL
		return 1;
	}

	channel->remove_member(user);
	user->leave_channel(channel);

	// send PART message to users in channel
	std::string part_msg = ":" + user->get_nick() + " PART " + name + "\r\n";
	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
		send((*it)->get_fd(), part_msg.c_str(), part_msg.length(), 0);

	// remove empty channel
	if (channel->get_members().empty())
		remove_channel(name);

	return (0);
}

int Server::REMOVE_CHANNEL(User *user, std::stringstream &command)
{
	std::string channel_name;
	command >> channel_name;
	
	if (channel_name.empty())
	{
		// ERR_NEEDMOREPARAMS
		reply(user, "", "461", "REMOVE_CHANNEL", ":Not enough parameters");
		return 1;
	}
	
	Channel *channel = get_channel(channel_name);
	if(!channel)
	{
		reply(user, "", "403", channel_name, ":No such channel");// ERR_NOSUCHCHANNEL
		return 1;
	}
	if(!channel->is_operator(user))
	{
		// its from here if you were wondering :D https://datatracker.ietf.org/doc/html/rfc2812
		reply(user, "", "482", channel_name, ":You're not a channel operator"); // ERR_CHANOPRIVSNEEDED
		return 1;
	}

	remove_channel(channel_name);
	std::cout << "Channel " << channel_name << " removed successfully." << std::endl;
	return 0;
}