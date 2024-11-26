# ft_irc
42 Project to create an IRC server in C++98 fully compatible with an official client.

## **About:**
**Mandatory part:**
- Communication between client and server has to be done via TCP/IP (v4 or v6).
- You must be able to authenticate, set a nickname, a username, join a channel, send and receive private messages using your reference client.
- All the messages sent from one client to a channel have to be forwarded to every other client that joined the channel.
- You must have operators and regular users.
- Then, you have to implement the commands that are specific to channel operators:
  - KICK - Eject a client from the channel
  - INVITE - Invite a client to a channel
  - TOPIC - Change or view the channel topic
  - MODE - Change the channel’s mode:
    - i: Set/remove Invite-only channel
	- t: Set/remove the restrictions of the TOPIC command to channel operators
	- k: Set/remove the channel key (password)
	- o: Give/take channel operator privilege
	- l: Set/remove the user limit to channel

## **Usage:**

```shell
# Clone the repository:
$> git clone --recursive https://github.com/cgray987/ft_irc.git

# Compile:
$> make

# Run with valid arguments:
$> ./ircserv <port> <password>
```
