#include "Server.hpp"
#include "Log.hpp"
#include <string>
#define VALID_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]{}\\|^`–-_"
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
	if (nick.empty())
	{
		reply(user, "", "431", "", "No nickname given"); //ERR_NONICKNAMEGIVEN
		return 1;
	}
	else if (nick.length() > 30)
		nick = nick.substr(0, 30);
	else if (nick.find_first_not_of(VALID_CHARS) != nick.npos || isdigit(nick[0]))
	{
		reply(user, "", "432", "", "Erroneus nickname"); //ERR_ERRONEUSNICKNAME
		return 1;
	}
	// check for nick in use
	for (std::vector<User *>::iterator it = _users.begin(); it != _users.end(); ++it)
	{
		if ((*it)->get_nick() == nick && (*it) != user)
		{
			reply(user, "", "433", nick, "Nickname is already in use"); //ERR_NICKNAMEINUSE
		}
	}
	// check is user already registered
	if (user->get_reg())
	{
		std::string nick_msg = ":" + user->get_prefix() + " NICK :" + nick + "\n";
		// send msg to all users
		for (std::vector<User *>::iterator it = _users.begin(); it != _users.end(); ++it)
			reply((*it), user->get_prefix(), "NICK", nick, "");
			// send((*it)->get_fd(), nick_msg.c_str(), nick_msg.length(), 0);
	}

	user->set_nick(nick);

	// if we have both USER and NICK set, and user is authenticated, register the client
	if (!user->get_reg() && !user->get_user().empty() && user->get_auth())
	{
		register_client(user);
	}
	// std::cout << "nick successful: " << user->get_nick() << "\n";
	return (0);
}

// https://modern.ircdocs.horse/#user-message
int Server::USER(User *user, std::stringstream &command)
{
	if (user->get_reg())
	{
		reply(user, "", "462", "", "You may not reregister"); //ERR_NICKNAMEINUSE
		return 1;
	}
	std::string username, hostname, servername, realname;
	command >> username >> username >> hostname >> servername;

	getline(command, realname);
	// remove leading colon
	if (!realname.empty() && realname[0] == ':')
		realname = realname.substr(1);
	user->set_realname(realname);

	if (username.empty() || realname.empty())
	{
		Server::reply(user, "", "461", "USER", "Not enough parameters"); // ERR_NEEDMOREPARAMS
		return 1;
	}
	if (!user->get_auth() && !Server::get_password().empty())
	{
		Server::reply(user, "", "464", "", "Password required"); // ERR_PASSWDMISMATCH
		return 1;
	}

	user->set_user(username);
	user->set_host(hostname);
	user->set_realname(realname);

	// if NICK and USER are set and user is authenticated, register the user
	if (!user->get_reg() && !user->get_nick().empty() && user->get_auth())
	{
		register_client(user);
	}
	LOG("user registered as: " << user->get_user() << " with nick: " << user->get_nick());
	return 0;
}

int Server::PASS(User *user, std::stringstream &command)
{
	std::string	password;
	command >> password;
	if (Server::get_password() != password)
		Server::reply(user, "", "464", user->get_nick(), "Password incorrect"); //ERR_PASSWDMISMATCH
	else if (user->get_auth() == false)
		user->set_auth(true);
	else
		Server::reply(user, "", "", user->get_nick(), "User already authenticated."); //not standard irc error -- might need to be removed/changed to std error
	// std::cout << "PASS successful, auth: " << user->get_auth() << "\n";
	return (0);
}
int Server::QUIT(User *user, std::stringstream &command)
{
	std::cout << YEL << "Client disconnected\n" << RST;
	Server::remove_user(user);
	(void)command;

	//TODO remove user's channels in server as well

	//clear user's msg buffer
	_msg.clear();
	return (0);
}
int Server::KILL(User *user, std::stringstream &command)
{
	(void)command;
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

	if (user->get_reg() == false)
		return (reply(user, "", "451", "", "You have not registered"), 1); //ERR_NOTREGISTERED
	if (username.empty() || password.empty())
		return (reply(user, "", "461", "KICK", "Not enough parameters"), 1); //ERR_NEEDMOREPARAMS
	if (user->get_user() != username)
		return (reply(user, "", "491", "", "No O-lines for your host"), 1);//ERR_NOOPERHOST
	if (password != OPER_PASS)
		return (reply(user, "", "464", user->get_nick(), "Password incorrect"), 1);//ERRPASSWDMISMATCH

	for (std::set<Channel *>::iterator it = user->get_channels().begin(); it != user->get_channels().end(); ++it)
	{
		if ((*it)->is_operator(user))
			(*it)->add_operator(user);
	}
	user->set_op(true);
	return (reply(user, "", "381", "", "You are now an IRC operator"), 0);
}

int Server::KICK(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", "You have not registered");
		return 1;
	}

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
	{
		reply(user, "", "461", "KICK", "Not enough parameters"); // ERR_NEEDMOREPARAMS
		return 1;
	}

	// getting the channel
	Channel *channel = get_channel(channel_name);
	if (!channel)
	{
		reply(user, "", "403", channel_name, "No such channel"); // ERR_NOSUCHCHANNEL
		return 1;
	}

	// Checking if the user issuing the command is in the channel
	if (!channel->is_member(user))
	{
		reply(user, "", "442", channel_name, "You're not on that channel"); // ERR_NOTONCHANNEL
		return 1;
	}

	// Checking if the user is a channel operator
	if (!channel->is_operator(user))
	{
		reply(user, "", "482", channel_name, "You're not a channel operator"); // ERR_CHANOPRIVSNEEDED
		return 1;
	}

	// Find the target user
	User *target = get_user_from_nick(target_nick);

	if (!target)
	{
		reply(user, "", "441", target_nick + " " + channel_name, "They aren't on that channel"); // ERR_USERNOTINCHANNEL
		return 1;
	}
	// cant kick yourself
	if (target == user)
	{
		reply(user, "", "485", channel_name, "You cannot kick yourself");
		return 1;
	}

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
	{
		reply(user, "", "409", "", "No origin specified"); // ERR_NOORIGIN
		return 1;
	}
	// maybe need to include a prefix here
	reply(user, "", "PONG", "", response);
	return (0);
}

//https://modern.ircdocs.horse/#invite-message
// INVITE <nick> <channel>
int Server::INVITE(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
		return (reply(user, "", "451", "", "You have not registered"), 1); //ERR_NOTREGISTERED

	std::string channel_name, target_nick;
	command  >> target_nick >> channel_name;

	// Checking for parameters
	if (channel_name.empty() || target_nick.empty())
	{
		reply(user, "", "461", "INVITE", "Not enough parameters"); // ERR_NEEDMOREPARAMS
		return 1;
	}

	// getting the channel
	Channel *channel = get_channel(channel_name);
	if (!channel)
	{
		reply(user, "", "403", channel_name, "No such channel"); // ERR_NOSUCHCHANNEL
		return 1;
	}

	// inviter needs to be operator
	if(!channel->is_operator(user))
	{
		reply(user, "", "482", channel_name, "You're not a channel operator");
	}

	// Find the target user
	User *target = get_user_from_nick(target_nick);

	if (!target)
	{
		reply(user, "", "401", target_nick, "No such nick");
		return 1;
	}
	// adding to inv list
	channel->add_invite(target);

	// notify inviter
	reply(user, "", "341", channel_name, target->get_nick() + " :Invited");

	// notify the invitee (is this even a word lol)
	std::string invite_msg = ":" + user->get_prefix() + " INVITE " + target_nick + " :" + channel_name + "\n";
	// send(target->get_fd(), invite_msg.c_str(), invite_msg.length(), 0);
	reply(target, user->get_prefix(), "", "INVITE " + target->get_nick(), channel->get_name());
	std::cout << "User " << target->get_nick() << " has been invited to the " << channel_name << "\n";
	return (0);
}

/* //ACCEPT <invited_server>
int Server::ACCEPT(User *user, std::stringstream &command)
{
	int	ret = 0;
	std::string	channel_name;
	command >> channel_name;

	Channel *channel = get_channel(channel_name);
	if (channel && channel->is_invited(user))
	{
		std::stringstream ss(channel->get_name());
		ret = JOIN(user, ss);
	}
	else
		return 1;
	return (ret);
} */

// https://dd.ircdocs.horse/refs/commands/topic
int Server::TOPIC(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", "You have not registered");
		return 1;
	}

	std::string channel_name;
	command >> channel_name;

	if (channel_name.empty())
	{
		reply(user, "", "461", "TOPIC", "Not enough parameters");
		return 1;
	}

	Channel *channel = get_channel(channel_name);
	if (!channel)
	{
		reply(user, "", "403", channel_name, "No such channel");
		return 1;
	}

	if (!channel->is_member(user))
	{
		reply(user, "", "442", channel_name, "You're not on that channel");
		return 1;
	}

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
		if (channel->get_mode('t') && !channel->is_operator(user))
		{
			reply(user, "", "482", channel_name, "You're not a channel operator");
			return 1;
		}
		channel->set_topic(topic);

		// notify users
		std::string notify = ":" + user->get_nick() + "TOPIC" + channel_name + " " + topic + "\n";
		for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
			reply((*it), user->get_prefix(), "TOPIC", channel->get_name(), topic);
			// send((*it)->get_fd(), notify.c_str(), notify.length(), 0);
	}

	//:dan!d@localhost TOPIC #v3 :topic
	// reply(user, user->get_prefix(), "TOPIC", channel->get_name(), ":" + topic);
	std::cout << "Topic in channel " << channel->get_name() << " changed to " << channel->get_topic() << "\n";
	return (0);
}

int Server::MODE(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", "You have not registered");
		return 1;
	}

	std::string target, param;
	command >> target;

	if (target.empty())
	{
		reply(user, "", "461", "MODE", "Not enough parameters");
		return 1;
	}

	if (target[0] == '#') // target is a channel
	{
		Channel *channel = get_channel(target);
		if (!channel)
		{
			reply(user, "", "403", target, "No such channel");
			return 1;
		}
		if (!channel->is_operator(user))
		{
			reply(user, "", "482", target, "You're not a channel operator");
			return 1;
		}

		std::string modes;
		command >> modes;

		if (modes.empty())
		{
			// if arguments empty, send current modes
			reply(user, "", "324", target, channel->str_modes());
			return 0;
		}

		bool add = true;
		for (size_t i = 0; i < modes.length(); ++i)
		{
			char mode = modes[i];
			if (mode == '+')
				add = true;
			else if (mode == '-')
				add = false;
			else if (mode == 'k')
			{
				if(add)
				{
					command >> param;
					if(param.empty())
					{
						reply(user, "", "461", "MODE", "Not enough parameters for +k");
						return 1;
					}
					channel->set_mode('k', true);
					channel->set_password(param);
					LOG("Mode +k enabled with password: " << param);
				}
				else
				{
					channel->set_mode('k', false);
					channel->set_password("");
					LOG("Mode +k disabled, password cleared");
				}
			}
			else
				channel->set_mode(mode, add);
		}

		// notify users
		// std::string notify = ":" + user->get_nick() + "MODE" + target + " " + modes + "\n";
		for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
			reply((*it), user->get_prefix(), "MODE", target, modes);
			// send((*it)->get_fd(), notify.c_str(), notify.length(), 0);
	}
	// handle unknown flags here?

	return (0);
}

// WHO <mask>
//	-where mask is either a channel name or an exact nick (mask pattern not supported)
int Server::WHO(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
		return (reply(user, "", "451", "", "You have not registered"), 1); //ERR_NOTREGISTERED
	std::string mask;
	command >> mask;

	if (mask.empty())
		return (reply(user, "", "461", "KICK", "Not enough parameters"), 1); //ERR_NEEDMOREPARAMS

	Channel *target_channel = get_channel(mask);
	User	*target_user = NULL;
	if (!target_channel)
		target_user = get_user_from_nick(mask);
	if (!target_user && target_channel)
	{
		for (std::set<User *>::iterator it = target_channel->get_members().begin(); it != target_channel->get_members().end(); ++it)
			reply(user, "", "352", " ", target_channel->get_name() + " " + (*it)->get_prefix());	//RPL_WHOREPLY (352)
				//i don't think this is the right format, functioning right, but irssi isn't displaying
	}
	if (!target_channel && target_user)
	{
		reply(user, "", "352", target_user->get_prefix(), target_user->get_realname());	//RPL_WHOREPLY (352)
		//i don't think this is the right format, functioning right, but irssi isn't displaying
	}

	reply(user, "", "315", "ft_irc", "End of WHO list");//RPL_ENDOFWHO (315)
	return (0);
}

// DELETE WITH COMMIT ?
// so it seems like it works as it should, one small "issue" is that
// if you are in the channel and write /list it looks lie it doesnt do anything
// but it does, it prompted you to write -yes, but outside of the channel
// so if you write /list -yes  it will print the channels outside the channel
// so when you leave it will show the list
// I believe that this is a expected behaviour, but we could output something
// inside the channel if we wanted
int Server::LIST(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", "You have not registered");
		return 1;
	}

	std::string channel_name;
	command >> channel_name;

	if (channel_name.empty())
	{
		// if no specific channel requested, listing all channels
		for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
			Channel *channel = it->second;
			std::stringstream ss;

			// here i construct the message
			// std::string reply = ":localhost 322 " + user->get_nick() + " " + channel->get_name() + " ";

			// ss << channel->get_members().size();
			// reply += ss.str() + " :";

			// if (!channel->get_topic().empty())
			// 	reply += channel->get_topic();
			// else
			// 	reply += "No topic";

			// reply += "\n";
			// Send the RPL_LIST message (RPL means reply code btw)
			reply(user, "", "322", channel->get_name(), channel->get_topic());
			// send(user->get_fd(), reply.c_str(), reply.length(), 0);
		}
	} else
	{
		// if specific channel requested
		std::stringstream ss(channel_name);
		std::string single_channel;

		while (std::getline(ss, single_channel, ','))
		{
			Channel *channel = get_channel(single_channel);
			if (channel)
			{
				// if the # is missing
				// i think that this is a correct behaviour, but if not
				// simply delete the if statement
				if (single_channel[0] != '#')
					single_channel = "#" + single_channel;
				// Format the RPL_LIST reply
				// std::string reply = ":localhost 322 " + user->get_nick() + " " + channel->get_name() + " ";
				// ss << channel->get_members().size();
				// reply += ss.str() + " :";


				// if (!channel->get_topic().empty())
				// 	reply += channel->get_topic();
				// else
				// 	reply += "No topic";

				// reply += "\n";

				// Send the RPL_LIST message
				// send(user->get_fd(), reply.c_str(), reply.length(), 0);
				reply(user, "", "322", channel->get_name(), channel->get_topic());
			} else
			{
				// if Channel not found
				std::string error = ":localhost 403 " + user->get_nick() + " " + single_channel + " :No such channel\n";
				reply(user, "", "403", single_channel, "No such channel");
				// send(user->get_fd(), error.c_str(), error.length(), 0);
			}
		}
	}

	// End the list with RPL_LISTEND
	std::string list_end = ":localhost 323 " + user->get_nick() + " :End of /LIST\n";
	reply(user, "", "323", "", "End of LIST");
	// send(user->get_fd(), list_end.c_str(), list_end.length(), 0);
	return (0);
}

int Server::PRIVMSG(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", "You have not registered");
		return 1;
	}

	std::string target, message;
	// getting the target (nickname or channel name)
	command >> target;

	// no recipient error
	if(target.empty())
	{
		reply(user, "", "411", "", "No recipient given (PRIVMSG)");
		return 1;
	}
	// getting the rest of the message
	getline(command, message);
	// trimming the : from message
	if(!message.empty() && message[0] == ' ')
		message.erase(0, 1);
	if (!message.empty() && message[0] == ':')
		message = message.substr(1);

	// No message to send error
	if(message.empty())
	{
		reply(user, "", "412", "", "No text to send");
		return 1;
	}
	// finding the target (nickname or channel name)
	Channel *target_channel = get_channel(target);
	User *target_user = NULL;
	if (!target_channel)
	{
		for (std::vector<User *>::iterator it = _users.begin(); it != _users.end(); ++it)
		{
			if((*it)->get_nick() == target)
			{
				target_user = *it;
				break;
			}
		}
	}

	// no target found error
	if(!target_user && !target_channel)
	{
		reply(user, "", "401", target, "No such nick/channel");
		return 1;
	}

	// constructing the message
	std::string privmsg = ":" + user->get_nick() + " PRIVMSG " + target + " :" + message + "\n";

	// sending it
	if (!target_channel)
		reply(target_user, user->get_prefix(), "PRIVMSG", target, message);
		// send(target_user->get_fd(), privmsg.c_str(), privmsg.length(), 0);
	else //sending to each member in the channel
	{
		if (target_channel->is_member(user))
		{
			for (std::set<User *>::iterator it = target_channel->get_members().begin(); it != target_channel->get_members().end(); ++it)
			{
				if (*it != user) //send to everyone but yourself
					reply((*it), user->get_prefix(), "PRIVMSG", target_channel->get_name(), message);
					// send((*it)->get_fd(), privmsg.c_str(), privmsg.length(), 0);
			}
		}
	}
	// std::cout << "PRIVMSG successful\n";
	return (0);
}


// https://dd.ircdocs.horse/refs/commands/join
int Server::JOIN(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", "You have not registered");
		return 1;
	}

	std::string name, password;
	command >> name;
	command >> password;

	LOG("Parsed JOIN command: Channel name: [" + name + "], Password: [" + password + "]");
	if (name.empty())
	{
		// ERR_NEEDMOREPARAMS
		reply(user, "", "461", "JOIN", "Not enough parameters");
		return 1;
	}


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
	if (channel->get_mode('k') && channel->has_password() && channel->get_password() != password)
	{
		LOG("Checking password for channel: " << channel->get_name());
    	LOG("Expected password: [" << channel->get_password() << "], Provided password: [" << password << "]");
		// Format:	<source> 475 <target> <channel> :Cannot join channel, you need the correct key (+k)
		reply(user, "", "475", user->get_nick() + " " + channel->get_name(), "Cannot join channel (+k) - you need the correct key");
		if(new_channel)
			remove_channel(name);
		return 1;
	}
	if (channel->get_mode('i') && !channel->is_invited(user) && !channel->is_operator(user))
	{
		reply(user, "", "473", user->get_nick() + " " + channel->get_name(), "Cannot join channel (+i)"); // ERR_INVITEONLYCHAN
		// Clean up the channel if it was created but unused
		if (new_channel && channel->get_members().empty() && channel->get_operators().empty())
		{
			LOG("Cleaning up unused invite-only channel: " << name);
			remove_channel(name);
		}
		return 1;
	}


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
		reply(user, "", "332", name, channel->get_topic()); // RPL_TOPIC
	else
		reply(user, "", "331", name, "No topic set"); // RPL_NOTOPIC

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
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", "You have not registered");
		return 1;
	}

	std::string name, reason;
	command >> name;
	if (name.empty())
	{
		// ERR_NEEDMOREPARAMS
		reply(user, "", "461", "PART", "Not enough parameters");
		return 1;
	}

	Channel *channel = get_channel(name);
	if (!channel)
	{
		reply(user, "", "403", name, "No such channel"); // ERR_NOSUCHCHANNEL
		return 1;
	}
	if (!channel->is_member(user))
	{
		reply(user, "", "442", name, "You're not on that channel"); // ERR_NOTONCHANNEL
		return 1;
	}


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
		// send((*it)->get_fd(), part_msg.c_str(), part_msg.length(), 0);
	//:d!d@localhost PART #irctest :reason
	// reply(user, user->get_prefix(), "PART", channel->get_name(), reason);
	std::cout << "user " << user->get_nick() << " removed from channel " << channel->get_name() << "\n";
	// LOG("Prefix: " << user->get_prefix());
	// LOG("User " << user->get_nick() << " removed from channel " << channel->get_name());

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
	{
		// ERR_NEEDMOREPARAMS
		reply(user, "", "461", "REMOVE_CHANNEL", "Not enough parameters");
		return 1;
	}

	Channel *channel = get_channel(channel_name);
	if(!channel)
	{
		reply(user, "", "403", channel_name, "No such channel");// ERR_NOSUCHCHANNEL
		return 1;
	}
	if(!channel->is_operator(user))
	{
		// its from here if you were wondering :D https://datatracker.ietf.org/doc/html/rfc2812
		reply(user, "", "482", channel_name, "You're not a channel operator"); // ERR_CHANOPRIVSNEEDED
		return 1;
	}

	remove_channel(channel_name);
	std::cout << "Channel " << channel_name << " removed successfully." << std::endl;
	return 0;
}
