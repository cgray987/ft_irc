#include "Server.hpp"
#define OPER_PASS "password"



/* CAP LS --supposed to send server's capabilities to client
 	can be ignored - https://ircv3.net/specs/extensions/capability-negotiation.html */
int Server::CAP(User *user, std::stringstream &command)
{
	// std::string buf;
	// command >> buf;
	(void)user;
	(void)command;
	return (0);
}

// https://modern.ircdocs.horse/#nick-message
// can ignore ERR_NICKCOLLISION
int Server::NICK(User *user, std::stringstream &command)
{
	std::string	nick;

	command >> nick;
	if (NONICKNAMEGIVEN(nick, user))
		return 1;
	else if (nick.length() > 30)
		nick = nick.substr(0, 30);
	else if (ERRONEUSNICKNAME(nick, user))
		return 1;

	// check for nick in use
	for (std::vector<User *>::iterator it = _users.begin(); it != _users.end(); ++it)
	{
		if ((*it)->get_nick() == nick && (*it) != user)
			return (NICKNAMEINUSE(nick, user));
	}
	// check is user already registered
	if (user->get_reg())
	{
		std::string nick_msg = ":" + user->get_prefix() + " NICK :" + nick + "\n";
		// send msg to all users
		for (std::vector<User *>::iterator it = _users.begin(); it != _users.end(); ++it)
			reply((*it), user->get_prefix(), "NICK", nick, "");
		user->set_nick(nick);
		return (0);
	}

	user->set_nick(nick);

	// if we have both USER and NICK set, and user is authenticated, register the client
	if (!user->get_reg() && !user->get_user().empty() && user->get_auth())
		register_client(user);
	return (0);
}

// https://modern.ircdocs.horse/#user-message
int Server::USER(User *user, std::stringstream &command)
{
	if (ALREADYREGISTERED(user))
		return (1);

	std::string username, hostname, realname;
	command >> username >> username >> hostname;

	getline(command, realname);
	// remove leading colon
	if (!realname.empty() && realname[0] == ':')
		realname = realname.substr(1);

	if (username.empty() || realname.empty())
	{
		NEEDMOREPARAMS("USER", user);
		close(user->get_fd());
		remove_user(user);
		return 1;
	}
	if (!user->get_auth() && !Server::get_password().empty())
	{
		PASSWORDMISMATCH(user);
		close(user->get_fd());
		remove_user(user);
		return 1;
	}
	user->set_user(username);
	user->set_host(hostname);
	user->set_realname(realname);

	// if NICK and USER are set and user is authenticated, register the user
	if (!user->get_reg() && !user->get_nick().empty() && user->get_auth())
		register_client(user);
	LOG("user registered as: " << user->get_user() << " with nick: " << user->get_nick());
	return 0;
}

int Server::PASS(User *user, std::stringstream &command)
{
	std::string	password;
	command >> password;
	if (password.empty() || password != get_password())
		return (PASSWORDMISMATCH(user), 1);
	if (user->get_auth() == false)
		user->set_auth(true);
	else
		ALREADYREGISTERED(user);
	return (0);
}
int Server::QUIT(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;
	std::cout << YEL << "Client disconnected\n" << RST;

	if (!user->get_channels().empty())
	{
		std::set<Channel *>::iterator it = user->get_channels().begin();
		while (!user->get_channels().empty())
		{
			Channel *channel = *it;
			std::stringstream ss(channel->get_name());
			PART(user, ss);
			if (user->get_channels().empty())
				break;
			it = user->get_channels().begin(); //this is silly, but can't do ++it for some reason, causes seg fault
		}
	}

	Server::remove_user(user);
	(void)command;

	//clear user's msg buffer
	_msg.clear();
	return (0);
}
int Server::KILL(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;
	std::string confirm;
	command >> confirm;
	if (confirm != "yes")
		return (reply(user, "", "", user->get_nick(), "Confirm KILL with 'yes'"), 1);

	if (user->get_op() == true)
	{
		std::cout << RED << "Server killed.\n" << RST;
		return (-1);
	}
	else
		reply(user, "", "", user->get_nick(), "Need op privleges to kill server");
	return (0);
}

// OPER <name> <password>
// password is seperate from the server password, and ideally should be unique to each OP
// 	-currently just using a defined password, OPER_PASS
// this command will make username an operator to any currently joined channels as well as a global op
int Server::OPER(User *user, std::stringstream &command)
{
	std::string	username, password;

	command >> username >> password;

	if (NOTREGISTERED(user))
		return (1);
	if (username.empty() || password.empty())
		return (NEEDMOREPARAMS("OPER", user), 1); //ERR_NEEDMOREPARAMS
	if (NOOPERHOST(username, user))
		return (1);
	if (password != OPER_PASS)
		return (PASSWORDMISMATCH(user), 1);

	for (std::vector<User *>::iterator it = _users.begin(); it != _users.end(); ++it)
	{
		if ((*it)->get_user() == username)
			(*it)->set_op(true);
	}
	reply(user, "", "381", "", "You are now an IRC operator"); //RPL_YOUREOPER
	return (0);
}

int Server::KICK(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	std::string channel_name, target_nick, comment;
	command >> channel_name >> target_nick;
	std::getline(command, comment);

	//irssi automatically adds the channel in the command, so if you specify it in
	//your command, it is added twice -- hence why i delete channel name from comment
	std::size_t	pos = comment.find(channel_name);
	if (pos != std::string::npos)
		comment.erase(pos, channel_name.length());
	pos = comment.find(":");
	if (pos != std::string::npos)
		comment.erase(pos, 1);



	// Checking for parameters
	if (channel_name.empty() || target_nick.empty())
		return (NEEDMOREPARAMS("KICK", user), 1);

	// getting the channel
	Channel *channel = get_channel(channel_name);
	if (!channel)
		return (NOSUCHCHANNEL(channel_name, user), 1);

	// Checking if the user issuing the command is in the channel
	if (NOTONCHANNEL(channel, user))
		return 1;

	// Checking if the user is a channel operator
	if (CHANOPRIVSNEEDED(channel, user))
		return 1;

	// Find the target user
	User *target = get_user_from_nick(target_nick);
	if (!target)
		return (USERNOTINCHANNEL(target_nick, channel_name, user), 1);
	// cant kick yourself
	if (target == user)
		return (CANTKICKYOURSELF(channel->get_name(), user), 1);

	std::string kick_reason = comment.empty() ? "Kicked by operator" : comment;
	std::stringstream part_command;
	part_command << channel_name << kick_reason;

	PART(target, part_command);

	// Notify all channel members about the kick
	std::string kick_msg = ":" + user->get_prefix() + " KICK " + channel_name + " " + target_nick + " :" + kick_reason + "\n";
	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
	{
		reply((*it), user->get_prefix(), "KICK " + channel_name, target_nick, kick_reason); //might be server prefix
		// send((*it)->get_fd(), kick_msg.c_str(), kick_msg.length(), 0);
	}

	std::cout << "User " << target->get_nick() << " was kicked from channel " << channel_name << "\n";
	return (0);
}

int Server::PING(User *user, std::stringstream &command)
{
	std::string response;

	std::getline(command, response);

	// remove leading space or :
	if (!response.empty() && response[0] == ' ')
		response.erase(0, 1);

	if (!response.empty() && response[0] == ':')
		response.erase(0, 1);

	if (response.empty())
		return (NOORIGIN(user), 1);

	// maybe need to include a prefix here
	reply(user, "", "PONG", "", response);
	return (0);
}

//https://modern.ircdocs.horse/#invite-message
// INVITE <nick> <channel>
int Server::INVITE(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	std::string channel_name, target_nick;
	command  >> target_nick >> channel_name;

	if (channel_name.empty() || target_nick.empty())
		return (NEEDMOREPARAMS("INVITE", user), 1);

	// getting the channel
	Channel *channel = get_channel(channel_name);
	if (!channel)
		return (NOSUCHCHANNEL(channel_name, user), 1);

	// inviter needs to be operator
	if (CHANOPRIVSNEEDED(channel, user))
		return 1;

	// Find the target user
	User *target = get_user_from_nick(target_nick);

	if (!target)
		return (USERNOTINCHANNEL(target_nick, channel_name, user), 1);

	// adding to inv list
	channel->add_invite(target);

	// notify inviter
	reply(user, "", "341", channel_name, target->get_nick() + " :Invited");

	// notify the invitee (is this even a word lol)
	reply(target, user->get_prefix(), "", "INVITE " + target->get_nick(), channel->get_name());
	return (0);
}

// https://dd.ircdocs.horse/refs/commands/topic
int Server::TOPIC(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	std::string channel_name;
	command >> channel_name;

	if (channel_name.empty())
		return (NEEDMOREPARAMS("TOPIC", user), 1);

	Channel *channel = get_channel(channel_name);
	if (!channel)
		return (NOSUCHCHANNEL(channel_name, user), 1);

	if (NOTONCHANNEL(channel, user))
		return 1;

	std::string topic;
	getline(command, topic);

	size_t pos = topic.find_first_not_of(" \t\n\r\f\v");
	if (pos != std::string::npos && topic[pos] == ':')
		topic.erase(0, pos + 1);

	if (topic.empty())
	{
		// if without topic parameter, sends the current topic
		if (!channel->get_topic().empty())
			reply(user, "", "332", channel_name, channel->get_topic());
		else
			reply(user, "", "331", channel_name, "No topic is set");
	}
	else
	{
		// check if mode is topic settable by operator only
		if (channel->get_mode('t') && CHANOPRIVSNEEDED(channel, user))
			return (CHANOPRIVSNEEDED(channel, user), 1);
		channel->set_topic(topic);

		// notify users
		for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
			reply((*it), user->get_prefix(), "TOPIC", channel->get_name(), topic);
	}
	return (0);
}

// MODE <channel> {[+|-]<modes>} [<mode parameters>]
int Server::MODE(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	char	c = 0;
	std::string target;
	command >> target;

	if (target.empty())
		return (NEEDMOREPARAMS("MODE", user), 1);

	if (target[0] == '#') // target is a channel
	{
		Channel *channel = get_channel(target);
		if (!channel)
			return (NOSUCHCHANNEL(target, user), 1);

		std::string modes;
		command >> modes;

		//RPL_CHANNELMODEIS
		if (modes.empty())
			return (reply(user, "", "324", user->get_nick() + " " + target, channel->str_modes()), 0);

		if (CHANOPRIVSNEEDED(channel, user))
			return 1;

		bool add = true;
		std::string mode_params; // for setting password for l mode etc
		while (!modes.empty())
		{
			c = modes[0];
			modes.erase(0, 1);

			if (c == '+')
				add = true;
			else if (c == '-')
				add = false;
			else if (c == 'i')
				channel->set_mode(c, add);
			else if (c == 'k')
			{
				std::string key;
				if (add)
				{
					command >> key;
					if (key.empty())
						return (NEEDMOREPARAMS("MODE", user), 1);
					LOG("Set key for channel " + channel->get_name() + ", " + key);
					channel->set_key(key);
				}
				else
					channel->remove_key();
			}
			else if (c == 'l')
			{
				if (add)
				{
					std::string limit;
					command >> limit;
					if (limit.empty())
						return (NEEDMOREPARAMS("MODE", user), 1);
					size_t l = stoi(limit);
					LOG("Set userlimit for channel " + channel->get_name() + ", " + limit);
					channel->set_user_limit(l);
				}
				else
					channel->remove_user_limit();
			}
			else if (c == 'o')
			{
				std::string nick;
				command >> nick;
				if (nick.empty())
					return (NEEDMOREPARAMS("MODE", user), 1);
				User *target = get_user_from_nick(nick);
				if (!target)
					return (NOSUCHNICK(nick, user), 1);
				if (add)
				{
					if (channel->is_member(target))
					{
						LOG("Set user " + target->get_nick() + " as op for channel " + channel->get_name());
						channel->add_operator(target);
					}
				}
				else
					channel->remove_operator(target);
			}
			else if (c == 't')
			{
				channel->set_mode(c, add);
				LOG((add ? "Enabled" : "Disabled") + std::string(" +t mode for channel ") + channel->get_name());
			}
			else
				reply(user, "", "472", std::string(1, c), "is unknown mode char to me");
		}

		// notify users
		std::string notify_modes = (add ? "+" : "-") + modes;
		std::string notify = user->get_prefix() + " MODE " + target + " " + notify_modes + c + "\r\n";

		for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
			send((*it)->get_fd(), notify.c_str(), notify.length(), 0);
		LOG("Set mode on channel " + channel->get_name() + notify_modes);
	}
	return (0);
}

// WHO <mask>
//	-where mask is either a channel name or an exact nick (mask pattern not supported)
int Server::WHO(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	std::string mask;
	command >> mask;

	if (mask.empty())
		return (NEEDMOREPARAMS("WHO", user), 1); //ERR_NEEDMOREPARAMS

	Channel *target_channel = get_channel(mask);
	User	*target_user = NULL;
	if (!target_channel)
		target_user = get_user_from_nick(mask);
	if (!target_user && target_channel)
	{
		for (std::set<User *>::iterator it = target_channel->get_members().begin(); it != target_channel->get_members().end(); ++it)
			reply(user, "", "352", target_channel->get_name() + " " + (*it)->get_user() + " " + (*it)->get_nick() , (*it)->get_host() + " :" + (*it)->get_realname().erase(0, 2));	//RPL_WHOREPLY (352)
			//reply(user, "", "352", "", channel + " " + (*it)->getUser() + " " + (*it)->getNick());
		reply(user, "", "315", "", "End of WHO list");
	}
	if (!target_channel && target_user)
	{
		reply(user, "", "352", "", "* " + target_user->get_user() + " " + target_user->get_nick());	//RPL_WHOREPLY (352)
		reply(user, "", "315", "", "End of WHO list");
	}
	return (0);
}

int Server::LIST(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	std::string channel_name;
	command >> channel_name;

	if (channel_name.empty())
	{
		// if no specific channel requested, listing all channels
		for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		{
			Channel *channel = it->second;
			std::stringstream ss;

			ss << channel->get_members().size();
			std::string member_count = ss.str();

			std::string topic = channel->get_topic();
			if(topic.empty())
			{
				topic = "No topic";
			}
			reply(user, "", "322", user->get_nick() + " " + channel->get_name() + " " + member_count, channel->get_topic());
		}
	}
	else
	{
		// if specific channel requested
		std::stringstream ss(channel_name);
		std::string single_channel;

		while (std::getline(ss, single_channel, ','))
		{
			if (single_channel[0] != '#')
				single_channel = "#" + single_channel;

			Channel *channel = get_channel(single_channel);
			if (channel)
			{
				std::stringstream ss;
				ss << channel->get_members().size();
				std::string member_count = ss.str();

				std::string topic = channel->get_topic();
				if(topic.empty())
				{
					topic = "No topic";
				}
				reply(user, "", "322", user->get_nick() + " " + channel->get_name() + " " + member_count, channel->get_topic());
			}
			else
				NOSUCHCHANNEL(channel_name, user);
		}
	}
	reply(user, "", "323", "", "End of LIST");
	return (0);
}

int Server::PRIVMSG(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	std::string target, message;
	// getting the target (nickname or channel name)
	command >> target;

	// no recipient error
	if(target.empty())
		return (NORECIPIENT("PRIVMSG", user), 1);

	// getting the rest of the message
	getline(command, message);
	// trimming the : from message
	if(!message.empty() && message[0] == ' ')
		message.erase(0, 1);
	if (!message.empty() && message[0] == ':')
		message = message.substr(1);

	// No message to send error
	if (NOTEXTTOSEND(message, user))
		return 1;
	// finding the target (nickname or channel name)
	Channel *target_channel = get_channel(target);
	User *target_user = NULL;
	if (!target_channel)
		target_user = get_user_from_nick(target);

	// no target found error
	if(!target_user && !target_channel)
		return (NOSUCHNICK(target, user), 1);

	// sending it
	if (!target_channel)
		reply(target_user, user->get_prefix(), "PRIVMSG", target, message);
	else if (target_channel->is_member(user))//sending to each member in the channel
	{
		for (std::set<User *>::iterator it = target_channel->get_members().begin(); it != target_channel->get_members().end(); ++it)
		{
			if (*it != user) //send to everyone but yourself
				reply((*it), user->get_prefix(), "PRIVMSG", target_channel->get_name(), message);
		}
	}
	else
		return (NOTONCHANNEL(target_channel, user), 1);
	return (0);
}

// https://dd.ircdocs.horse/refs/commands/join
int Server::JOIN(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	std::string name;
	command >> name;

	if (name.empty())
		return (NEEDMOREPARAMS("JOIN", user), 1);


	Channel *channel = get_channel(name);
	// This serves as flag for the invite only check, if its not here
	// you basically kick everybody inside of the channel while
	// trying to join and not having been invited
	bool new_channel = false;
	if (!channel)
	{
		// create if doesnt exist
		channel = create_channel(name);
		LOG("Channel " << name << " created.");
		new_channel = true;
		channel->add_member(user);
		channel->add_operator(user);
		user->join_channel(channel);
		// TODO: set default modes here
	}

	// check for l mode
	if (CHANNELISFULL(channel, user))
		return 1;

	if (INVITEONLYCHAN(channel, user))
	{
		// Clean up the channel if it was created but unused
		if (new_channel && channel->get_members().empty() && channel->get_operators().empty())
		{
			LOG("Cleaning up unused invite-only channel: " << name);
			remove_channel(name);
		}
		return 1;
	}

	// check for k mode
	std::string key;
	command >> key;

	LOG("Received key for channel " + channel->get_name() + " : " + key);
	if (BADCHANNELKEY(key, channel, user))
		return 1;

	// Add the user to the channel only if they are not already a member
	if (!channel->is_member(user))
	{
		channel->add_member(user);
		user->join_channel(channel);
	}

	// send join message to the user and members of channel
	reply(user, user->get_prefix(), "JOIN", channel->get_name(), "");

	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
	{
		if (*it != user)
			reply((*it), user->get_prefix(), "JOIN", channel->get_name(), "");
	}

	// send channel topic
	if (!channel->get_topic().empty())
		reply(user, "", "332", user->get_nick() + " " + name, channel->get_topic()); // RPL_TOPIC
	else
		reply(user, "", "331", user->get_nick() + " " + name, "No topic set"); // RPL_NOTOPIC

	// send users in channel
	std::string members;
	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
	{
		//check if member is op : they get '@' symbol
		if (channel->is_operator(*it))
			members += "@" + (*it)->get_nick() + " ";
		else
			members += (*it)->get_nick() + " ";
	}

	reply(user, "", "353", user->get_nick() + " = " + name , members); // RPL_NAMREPLY
	reply(user, "", "366", user->get_nick() + " " + name, "End of NAMES list"); // RPL_ENDOFNAMES
	LOG("User " << user->get_nick() << " added to channel " << channel->get_name());
	return 0;
}

// https://dd.ircdocs.horse/refs/commands/part
int Server::PART(User *user, std::stringstream &command)
{
	if (NOTREGISTERED(user))
		return 1;

	std::string name, reason;
	command >> name;
	if (name.empty())
		return (NEEDMOREPARAMS("PART", user), 1);

	Channel *channel = get_channel(name);
	if (!channel)
		return (NOSUCHCHANNEL(name, user), 1);
	if (NOTONCHANNEL(channel, user))
		return 1;


	//get reason for parting
	getline(command, reason);

	size_t pos = reason.find_first_not_of(" \t\n\r\f\v");
	if (pos != std::string::npos && reason[pos] == ':')
		reason.erase(0, pos + 1);


	// send PART message to users in channel
	reply(user, user->get_prefix(), "PART", channel->get_name(), reason);
	std::string part_msg = ":" + user->get_nick() + " PART " + name + reason +"\n";
	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
		if ((*it) != user)
			reply((*it), user->get_prefix(), "PART", channel->get_name(), reason);

	channel->remove_member(user);
	user->leave_channel(channel);

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
		return (NEEDMOREPARAMS("REMOVECHANNEL", user), 1);

	Channel *channel = get_channel(channel_name);
	if (!channel)
		return (NOSUCHCHANNEL(channel_name, user), 1);
	if (NOTONCHANNEL(channel, user))
		return 1;
	if (CHANOPRIVSNEEDED(channel, user))
		return 1;

	remove_channel(channel_name);
	std::cout << "Channel " << channel_name << " removed successfully." << std::endl;
	return 0;
}
