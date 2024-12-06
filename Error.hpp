#pragma once

#include "User.hpp"
#include "Channel.hpp"
// class User;
// class Channel;

int	NOSUCHNICK(std::string nick, User *user);
int	NONICKNAMEGIVEN(std::string nick, User *user);
int	ERRONEUSNICKNAME(std::string nick, User *user);
int	NICKNAMEINUSE(std::string nick, User *user);
int	ALREADYREGISTERED(User *user);
int	NEEDMOREPARAMS(std::string command, User *user);
int	PASSWORDMISMATCH(User *user);
int	NOTREGISTERED(User *user);
int	NOOPERHOST(std::string username, User *user);
int	NOSUCHCHANNEL(std::string channel_name, User *user);
int	NOTONCHANNEL(Channel *channel, User *user);
int	CHANOPRIVSNEEDED(Channel *channel, User *user);
int	USERNOTINCHANNEL(std::string target_nick, std::string channel_name, User *user);
int	CANTKICKYOURSELF(std::string channel_name, User *user);
int	NOORIGIN(User *user);
int	NORECIPIENT(std::string command, User *user);
int	NOTEXTTOSEND(std::string message, User *user);
int	CHANNELISFULL(Channel *channel, User *user);
int	INVITEONLYCHAN(Channel *channel, User *user);
int	BADCHANNELKEY(std::string key, Channel *channel, User *user);
