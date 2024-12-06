#include "Error.hpp"
#include "Server.hpp"
#define VALID_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]{}\\|^`–-_"


int	NOSUCHNICK(std::string nick, User *user)
{
	reply(user, "", "401", nick, "NO such nick");
	return 1;
}

int	NONICKNAMEGIVEN(std::string nick, User *user)
{
	if (nick.empty())
	{
		reply(user, "", "431", "", "No nickname given"); //ERR_NONICKNAMEGIVEN
		return 1;
	}
	return 0;
}

int	ERRONEUSNICKNAME(std::string nick, User *user)
{
	if (nick.find_first_not_of(VALID_CHARS) != nick.npos || isdigit(nick[0]))
	{
		reply(user, "", "432", "", "Erroneus nickname"); //ERR_ERRONEUSNICKNAME
		return 1;
	}
	return 0;
}

int	NICKNAMEINUSE(std::string nick, User *user)
{
	reply(user, "", "433", "* " + nick, "Nickname is already in use"); //ERR_NICKNAMEINUSE
	return (1);
}

int	ALREADYREGISTERED(User *user)
{
	if (user->get_reg())
	{
		reply(user, "", "462", "", "You may not reregister"); //ERR_ALREADYREGISTERED
		return 1;
	}
	return 0;
}

int	NEEDMOREPARAMS(std::string command, User *user)
{
	reply(user, "", "461", command, "Not enough parameters"); // ERR_NEEDMOREPARAMS
	return (1);
}

int	PASSWORDMISMATCH(User *user)
{
	reply(user, "", "464", "", "Password incorrect"); // ERR_PASSWDMISMATCH
	return 1;
}

int	NOTREGISTERED(User *user)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", "You have not registered");
		return 1;
	}
	return 0;
}

int	NOOPERHOST(std::string username, User *user)
{
	if (user->get_user() != username)
	{
		reply(user, "", "491", "", "No O-lines for your host");//ERR_NOOPERHOST
		return 1;
	}
	return 0;
}

int	NOSUCHCHANNEL(std::string channel_name, User *user)
{
	reply(user, "", "403", channel_name, "No such channel"); // ERR_NOSUCHCHANNEL
	return 1;
}

int	NOTONCHANNEL(Channel *channel, User *user)
{
	if (!channel->is_member(user))
	{
		reply(user, "", "442", channel->get_name(), "You're not on that channel"); // ERR_NOTONCHANNEL
		return 1;
	}
	return 0;
}

int	CHANOPRIVSNEEDED(Channel *channel, User *user)
{
	if (!channel->is_operator(user))
	{
		reply(user, "", "482", channel->get_name(), "You're not a channel operator"); // ERR_CHANOPRIVSNEEDED
		return 1;
	}
	return 0;
}

int	USERNOTINCHANNEL(std::string target_nick, std::string channel_name, User *user)
{
	reply(user, "", "441", target_nick + " " + channel_name, "They aren't on that channel"); // ERR_USERNOTINCHANNEL
	return 1;
}

int	CANTKICKYOURSELF(std::string channel_name, User *user)
{
	reply(user, "", "", channel_name, "You cannot kick yourself");
	return 1;
}

int	NOORIGIN(User *user)
{
	reply(user, "", "409", "", "No origin specified"); // ERR_NOORIGIN
	return 1;
}

int	NORECIPIENT(std::string command, User *user)
{
	reply(user, "", "411", "", "No recipient given (" + command + ")"); //NORECIPIENT
	return 1;
}

int	NOTEXTTOSEND(std::string message, User *user)
{
	if(message.empty())
	{
		reply(user, "", "412", "", "No text to send");
		return 1;
	}
	return 0;
}

int	CHANNELISFULL(Channel *channel, User *user)
{
	if (channel->get_mode('l') && channel->get_members().size() >= channel->get_user_limit())
	{
		reply(user, "", "471", user->get_nick() + " " + channel->get_name(), "Cannot join channel (+l)"); // ERR_CHANNELISFULL
		return 1;
	}
	return 0;
}

int	INVITEONLYCHAN(Channel *channel, User *user)
{
	if (channel->get_mode('i') && !channel->is_invited(user) && !channel->is_operator(user))
	{
		reply(user, "", "473", user->get_nick() + " " + channel->get_name(), "Cannot join channel (+i)"); // ERR_INVITEONLYCHAN
		return 1;
	}
	return 0;
}

int	BADCHANNELKEY(std::string key, Channel *channel, User *user)
{
	if (channel->get_mode('k') && channel->get_key() != key)
	{
		reply(user, "", "475", user->get_nick() + " " + channel->get_name(), "Cannot join channel (+k)");
		return 1;
	}
	return (0);
}
