#include "Channel.hpp"

// constructors
Channel::Channel(): _name("unnammed channel") {}

Channel::~Channel() {}

Channel::Channel(const std::string &name): _name(name) {}

// getters
const std::string& Channel::get_name() const {return _name;}

const std::string& Channel::get_topic() const {return _topic;}

const std::set<User*>& Channel::get_members() const {return _members;}

const std::set<User*>& Channel::get_operators() const {return _operators;}

// setters
void Channel::add_member(User* user)
{
    _members.insert(user);
}

void Channel::remove_member(User* user)
{
    _members.erase(user);
    _operators.erase(user);
}

bool Channel::is_member(User* user) const
{
    return _members.find(user) != _members.end();
}

void Channel::set_topic(const std::string& topic)
{
    _topic = topic;
}

// ops
void Channel::add_operator(User* user)
{
    if (is_member(user))
        _operators.insert(user);
}

void Channel::remove_operator(User* user)
{
    _operators.erase(user);
}

bool Channel::is_operator(User* user) const
{
    return _operators.find(user) != _operators.end();
}

// modes
void Channel::set_mode(char mode, bool value)
{
    if (value == true)
        _modes.insert(mode);
    else
        _modes.erase(mode);
}

bool Channel::get_mode(char mode) const
{
    return _modes.find(mode) != _modes.end();
}

// return current channel modes as string
std::string Channel::str_modes() const
{
	std::string ret = "+";
	for (std::set<char>::const_iterator it = _modes.begin(); it != _modes.end(); ++it)
		ret += *it;
	return ret;
}
