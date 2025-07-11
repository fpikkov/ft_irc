#pragma once

#include <iostream>
#include <unordered_set>
#include <optional>

class Channel
{
	private:
		std::string					_name;
		std::string					_topic;
		std::unordered_set<int> 	_members;
		std::unordered_set<int> 	_operators;
		std::unordered_set<int> 	_invited;
		bool						_inviteOnly;
		bool						_topicLocked;
		std::optional<std::string>	_key;
		int							_userLimit;

	public:
		Channel(const std::string& name);

		//Getters
		const std::string&					getName			() const;
		const std::string&					getTopic		() const;
		const std::unordered_set<int>&		getMembers		() const;
		const std::unordered_set<int>&		getOperators	() const;
		bool								isInviteOnly	() const;
		bool								isTopicLocked	() const;
		const std::optional<std::string>&	getKey			() const;
		int									getUserLimit	() const;

		//Setters
		void	setTopic		(const std::string& topic);
		void	setInviteOnly	(bool inviteonly);
		void	setTopicLocked	(bool topiclocked);
		void	setUserLimit	(int limit);
		void	setKey			(const std::string& key);

		//Membership management
		bool	addMember		(int clientFd);
		bool	addOperator		(int clientFd);
		void	removeMember	(int clientFd);
		void	removeOperator	(int clientFd);
		bool	isOperator		(int clientFd);

		//Invite management
		void	invite			(int clientFd);
		bool	isInvited		(int clientFd);
		void	removeInvite	(int clientFd);

		//Helpers
		bool	isMember		(int clientFd) const;
		bool	isFull			() const;
		bool	isEmpty			() const;


};

// _name: Channel name (e.g., #school42).
// _topic: Channel topic (can be empty).
// _members: Set of client fds in the channel (fast lookup, no duplicates).
// _operators: Set of client fds with operator privileges.
// _invited: Set of invited client fds (for invite-only channels).
// _inviteOnly: If true, only invited users can join (+i mode).
// _topicLocked: If true, only operators can change the topic (+t mode).
// _key: Optional password for the channel (+k mode).
// _userLimit: Max users allowed (+l mode, 0 = unlimited).
