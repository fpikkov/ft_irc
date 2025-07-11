#include "Channels.hpp"
#include "constants.hpp"

Channel::Channel(const std::string& name) :
	_name(name),
	_inviteOnly(false),
	_topicLocked(false),
	_userLimit(irc::MAX_CHANNELS)
{};

//Getters

const	std::string& Channel::getName					()	const	{ return _name; }
const	std::string& Channel::getTopic					()	const	{ return _topic; }
const	std::unordered_set<int>& Channel::getMembers	()	const	{ return _members; }
const	std::unordered_set<int>& Channel::getOperators	()	const	{ return _operators; }
bool	Channel::isInviteOnly							()	const	{ return _inviteOnly; }
bool	Channel::isTopicLocked							()	const	{ return _topicLocked; }
const	std::optional<std::string>& Channel::getKey		()	const	{ return _key; }
int		Channel::getUserLimit							()	const	{ return _userLimit; }


//Setters

void	Channel::setTopic(const std::string& topic)					{ _topic = topic;}
void	Channel::setInviteOnly(bool inviteonly)						{ _inviteOnly = inviteonly; }
void	Channel::setTopicLocked(bool topiclocked)					{ _topicLocked = topiclocked; }
void	Channel::setUserLimit(int limit)							{ _userLimit = limit; }
void	Channel::setKey(const std::string& key)						{ _key = key; }

//Membership management

// If clientFd was not already in _members, it is added, and the function returns true.
// If clientFd was already in _members, nothing changes, and the function returns false.
bool	Channel::addMember(int clientFd)
{
	auto result = _members.insert(clientFd);// The insert method returns a std::pair (std::pair<iterator, bool> insert(const value_type& value);)
	return result.second;
}

bool	Channel::addOperator(int clientFd)
{
	auto result = _operators.insert(clientFd);
	return result.second;
}

void	Channel::removeMember(int clientFd)							{ _members.erase(clientFd); }
void	Channel::removeOperator(int clientFd)						{ _operators.erase(clientFd); }
// If clientFd is present, the iterator returned will not be equal to _operators.end() and the function returns true.
// If clientFd is not present, the iterator will be equal to _operators.end() and the function returns false.
bool	Channel::isOperator(int clientFd)							{ return _operators.find(clientFd) != _operators.end(); }
void	Channel::invite(int clientFd)								{ _invited.insert(clientFd); }
bool	Channel::isInvited(int clientFd)							{ return _invited.find(clientFd) != _invited.end(); }
void	Channel::removeInvite(int clientFd)							{ _invited.erase(clientFd); }

// Helpers

bool	Channel::isFull() const										{ return _userLimit > 0 && _members.size() >= static_cast<size_t>(_userLimit); }
bool	Channel::isEmpty() const									{ return _members.empty(); }
bool	Channel::isMember(int clientFd) const
{
	for (auto& it : _members)
	{
		if (it == clientFd)
			return true;
	}
	return false;
}
