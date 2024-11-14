/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cgray <cgray@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/07 15:14:51 by cgray             #+#    #+#             */
/*   Updated: 2024/11/14 16:53:02 by cgray            ###   ########.fr       */
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
int Server::PART(User *user, std::stringstream &command){return (0);}
