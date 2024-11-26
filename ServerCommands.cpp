#include "Server.hpp"
#include "Log.hpp"
#include <string>
#define VALID_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]{}\\|^`–-_"



/* CAP LS --supposed to send server's capabilities to client
 	can be ignored - https://ircv3.net/specs/extensions/capability-negotiation.html */
int Server::CAP(User *user, std::stringstream &command)
{
	// std::string buf;
	// command >> buf;
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
		reply(user, "", "431", "", ":No nickname given"); //ERR_NONICKNAMEGIVEN
		return 1;
	}
	else if (nick.length() > 30)
		nick = nick.substr(0, 30);
	else if (nick.find_first_not_of(VALID_CHARS) != nick.npos || isdigit(nick[0]))
	{
		reply(user, "", "432", "", ":Erroneus nickname"); //ERR_ERRONEUSNICKNAME
		return 1;
	}
	// check for nick in use
	for (std::vector<User *>::iterator it = _users.begin(); it != _users.end(); ++it)
	{
		if ((*it)->get_nick() == nick && (*it) != user)
		{
			reply(user, "", "433", nick, ":Nickname is already in use"); //ERR_NICKNAMEINUSE
		}
	}
	// check is user already registered
	if (user->get_reg())
	{
		std::string nick_msg = ":" + user->get_prefix() + " NICK :" + nick + "\n";
		// send msg to all users
		for (std::vector<User *>::iterator it = _users.begin(); it != _users.end(); ++it)
			send((*it)->get_fd(), nick_msg.c_str(), nick_msg.length(), 0);
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
		reply(user, "", "462", "", ":You may not reregister"); //ERR_NICKNAMEINUSE
		return 1;
	}

	std::string username, hostname, servername, realname;
	command >> username >> hostname >> servername;

	getline(command, realname);
	// remove leading colon
	if (!realname.empty() && realname[0] == ':')
		realname = realname.substr(1);
	user->set_realname(realname);

	if (username.empty() || realname.empty())
	{
		Server::reply(user, "", "461", "USER", ":Not enough parameters"); // ERR_NEEDMOREPARAMS
		return 1;
	}
	if (!user->get_auth() && !Server::get_password().empty())
	{
		Server::reply(user, "", "464", "", ":Password required"); // ERR_PASSWDMISMATCH
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
		Server::reply(user, "", "464", user->get_nick(), ":Password incorrect"); //ERR_PASSWDMISMATCH
	else if (user->get_auth() == false)
		user->set_auth(true);
	else
		Server::reply(user, "", "", user->get_nick(), ":User already authenticated."); //not standard irc error -- might need to be removed/changed to std error
	// std::cout << "PASS successful, auth: " << user->get_auth() << "\n";
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

int Server::KICK(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", ":You have not registered");
		return 1;
	}

	std::string channel_name, target_nick, comment;
	command >> channel_name >> target_nick;
	std::getline(command, comment);

	// Checking for parameters
	if (channel_name.empty() || target_nick.empty())
	{
		reply(user, "", "461", "KICK", ":Not enough parameters"); // ERR_NEEDMOREPARAMS
		return 1;
	}

	// getting the channel
	Channel *channel = get_channel(channel_name);
	if (!channel)
	{
		reply(user, "", "403", channel_name, ":No such channel"); // ERR_NOSUCHCHANNEL
		return 1;
	}

	// Checking if the user issuing the command is in the channel
	if (!channel->is_member(user))
	{
		reply(user, "", "442", channel_name, ":You're not on that channel"); // ERR_NOTONCHANNEL
		return 1;
	}

	// Checking if the user is a channel operator
	if (!channel->is_operator(user)) 
	{
		reply(user, "", "482", channel_name, ":You're not a channel operator"); // ERR_CHANOPRIVSNEEDED
		return 1;
	}

	// Find the target user
	User *target = NULL;
	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
	{
		if ((*it)->get_nick() == target_nick)
		{
			target = *it;
			break;
		}
	}

	if (!target)
	{
		reply(user, "", "441", target_nick + " " + channel_name, ":They aren't on that channel"); // ERR_USERNOTINCHANNEL
		return 1;
	}
	// cant kick yourself
	if (target == user)
	{
		reply(user, "", "485", channel_name, ":You cannot kick yourself");
		return 1;
	}

	std::string kick_reason = comment.empty() ? "Kicked by operator" : comment;
	std::stringstream part_command;
	part_command << channel_name << " :" << kick_reason;
	
	PART(target, part_command);

	// Notify all channel members about the kick
	std::string kick_msg = ":" + user->get_prefix() + " KICK " + channel_name + " " + target_nick + " :" + kick_reason + "\n";
	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
	{
		send((*it)->get_fd(), kick_msg.c_str(), kick_msg.length(), 0);
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
		reply(user, "", "409", "", ":No origin specified"); // ERR_NOORIGIN
		return 1;
	}
	// maybe need to include a prefix here 
	reply(user, "", "PONG", "", response);
	return (0);
}

int Server::INVITE(User *user, std::stringstream &command){return (0);}

// https://dd.ircdocs.horse/refs/commands/topic
int Server::TOPIC(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", ":You have not registered");
		return 1;
	}

	std::string channel_name;
	command >> channel_name;

	if (channel_name.empty())
	{
		reply(user, "", "461", "TOPIC", ":Not enough parameters");
		return 1;
	}

	Channel *channel = get_channel(channel_name);
	if (!channel)
	{
		reply(user, "", "403", channel_name, ":No such channel");
		return 1;
	}

	if (!channel->is_member(user))
	{
		reply(user, "", "442", channel_name, ":You're not on that channel");
		return 1;
	}

	std::string topic;
	getline(command, topic);

	if (topic.empty())
	{
		// if without topic parameter, sends the current topic
		if (!channel->get_topic().empty())
			reply(user, "", "332", channel_name, ":" + channel->get_topic());
		else
			reply(user, "", "331", channel_name, ":No topic is set");
	}
	else
	{
		// check if mode is topic settable by operator only
		if (channel->get_mode('t') && !channel->is_operator(user))
		{
			reply(user, "", "482", channel_name, ":You're not a channel operator");
			return 1;
		}

		channel->set_topic(topic);

		// notify users
		std::string notify = ":" + user->get_nick() + "TOPIC" + channel_name + " " + topic + "\n";
		for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
			send((*it)->get_fd(), notify.c_str(), notify.length(), 0);
	}
	//:dan!d@localhost TOPIC #v3 :topic
	reply(user, user->get_prefix(), "TOPIC", channel->get_name(), ":" + topic);
	std::cout << "Topic in channel " << channel->get_name() << " changed to " << channel->get_topic() << "\n";
	return (0);
}

int Server::MODE(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", ":You have not registered");
		return 1;
	}

	std::string target;
	command >> target;

	if (target.empty())
	{
		reply(user, "", "461", "MODE", ":Not enough parameters");
		return 1;
	}

	if (target[0] == '#') // target is a channel
	{
		Channel *channel = get_channel(target);
		if (!channel)
		{
			reply(user, "", "403", target, ":No such channel");
			return 1;
		}
		if (!channel->is_operator(user))
		{
			reply(user, "", "482", target, ":You're not a channel operator");
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
			char c = modes[i];
			if (c == '+')
				add = true;
			else if (c == '-')
				add = false;
			else
				channel->set_mode(c, add);
		}

		// notify users
		std::string notify = ":" + user->get_nick() + "MODE" + target + " " + modes + "\n";
		for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
			send((*it)->get_fd(), notify.c_str(), notify.length(), 0);
	}
	// handle unknown flags here?

	return (0);
}


int Server::WHO(User *user, std::stringstream &command){return (0);}

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
		reply(user, "", "451", "", ":You have not registered");
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
			std::string reply = ":localhost 322 " + user->get_nick() + " " + channel->get_name() + " ";
			
			ss << channel->get_members().size();
			reply += ss.str() + " :";

			if (!channel->get_topic().empty())
				reply += channel->get_topic();
			else
				reply += "No topic";

			reply += "\n";
			// Send the RPL_LIST message (RPL means reply code btw)
			send(user->get_fd(), reply.c_str(), reply.length(), 0);
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
				std::string reply = ":localhost 322 " + user->get_nick() + " " + channel->get_name() + " ";
				ss << channel->get_members().size();
				reply += ss.str() + " :";


				if (!channel->get_topic().empty())
					reply += channel->get_topic();
				else
					reply += "No topic";

				reply += "\n";

				// Send the RPL_LIST message
				send(user->get_fd(), reply.c_str(), reply.length(), 0);
			} else
			{
				// if Channel not found
				std::string error = ":localhost 403 " + user->get_nick() + " " + single_channel + " :No such channel\n";
				send(user->get_fd(), error.c_str(), error.length(), 0);
			}
		}
	}

	// End the list with RPL_LISTEND
	std::string list_end = ":localhost 323 " + user->get_nick() + " :End of /LIST\n";
	send(user->get_fd(), list_end.c_str(), list_end.length(), 0);
	return (0);
}

int Server::PRIVMSG(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", ":You have not registered");
		return 1;
	}

	std::string target, message;
	// getting the target (nickname or channel name)
	command >> target;

	// no recipient error
	if(target.empty())
	{
		reply(user, "", "411", "", ":No recipient given (PRIVMSG)");
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
		reply(user, "", "412", "", ":No text to send");
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
		reply(user, "", "401", target, ":No such nick/channel");
		return 1;
	}

	// constructing the message
	std::string privmsg = ":" + user->get_nick() + " PRIVMSG " + target + " :" + message + "\n";

	// sending it
	if (!target_channel)
		send(target_user->get_fd(), privmsg.c_str(), privmsg.length(), 0);
	else //sending to each member in the channel
	{
		for (std::set<User *>::iterator it = target_channel->get_members().begin(); it != target_channel->get_members().end(); ++it)
		{
			if (*it != user) //send to everyone but yourself
				send((*it)->get_fd(), privmsg.c_str(), privmsg.length(), 0);
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
		reply(user, "", "451", "", ":You have not registered");
		return 1;
	}

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
	user->join_channel(channel);

	// send join message to the user and members of channel
	std::string join_msg = ":" + user->get_nick() + " JOIN " + name + "\n";
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
	LOG("User " << user->get_nick() << " added to channel " << channel->get_name());
	return 0;
}

// https://dd.ircdocs.horse/refs/commands/part
int Server::PART(User *user, std::stringstream &command)
{
	if (user->get_reg() == false)
	{
		reply(user, "", "451", "", ":You have not registered");
		return 1;
	}

	std::string name, reason;
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

	//get reason for parting
	getline(command, reason);
	// send PART message to users in channel
	std::string part_msg = ":" + user->get_nick() + " PART " + name + reason +"\n";
	for (std::set<User *>::iterator it = channel->get_members().begin(); it != channel->get_members().end(); ++it)
		send((*it)->get_fd(), part_msg.c_str(), part_msg.length(), 0);
	//:d!d@localhost PART #irctest :reason
	// LOG("Prefix: " << user->get_prefix());
	reply(user, user->get_prefix(), "PART", channel->get_name(), reason);
	LOG("User " << user->get_nick() << " removed from channel " << channel->get_name());

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
