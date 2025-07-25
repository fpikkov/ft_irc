#!/bin/bash

# Configuration

IRC_HOST="localhost"
IRC_PORT="6667"
PASSWORD="pass"
NICKNAME="testNick"
USERNAME="testUser"
HOSTNAME="testHost"
SERVERNAME="testServer"
REALNAME="testReal"
CHANNEL="#testchannel"

# Colors
RED="\033[1;31m"
GREEN="\033[1;32m"
CYAN="\033[1;36m"
YELLOW="\033[1;33m"
CLEAR="\033[0m"

# Associative array of test commands
declare -A tests=(
	["join"]="JOIN $CHANNEL"
	["part"]="PART $CHANNEL"
	["part_reason"]="PART $CHANNEL :Just because"
	["message"]="PRIVMSG $CHANNEL :Hello world"
	["notify"]="NOTICE $CHANNEL :Notice me senpai"
)

# Create named pipes
PIPE_IN=$(mktemp -u)
PIPE_OUT=$(mktemp -u)
mkfifo "$PIPE_IN" "$PIPE_OUT"

# Cleanup
cleanup()
{
	rm -f "$PIPE_IN" "$PIPE_OUT"
	kill $NC_PID $READER_PID 2>/dev/null
	exit 0
}
trap cleanup EXIT INT TERM

# Run netcat
netcat "$IRC_HOST" "$IRC_PORT" < "$PIPE_IN" > "$PIPE_OUT" & NC_PID=$!

# Display server response
{
	while IFS= read -r line; do
		echo "Server> $line"
	done < "$PIPE_OUT"
} & READER_PID=$!

# Open pipe for writing
exec 3> "$PIPE_IN"

# Launch message
echo -e "${CYAN}---------------- IRC Client test script ----------------${CLEAR}"
echo -e "${CYAN}You may send your own commands or use templates${CLEAR}"
for key in "${!tests[@]}"; do
	echo -e "	${YELLOW}$key${CLEAR} - ${tests[$key]}"
done
echo -e "${CYAN}Other commands: 'register', 'quit', 'exit'${CLEAR}"

# Helper command exxecution function
execute_command()
{
	local command="$1"

	while IFS= read -r line; do
		if [[ -n "$line" ]]; then
			printf "%s\r\n" "$line" >&3
			sleep 0.2
		fi
	done <<< "$(echo -e "$command")"
}

# Adds CRLF to the server
send_command()
{
	local command="$1"
	printf "%s\r\n" "$command" >&3
	sleep 0.2
}

# Delay client
sleep 0.5

# Command loop
while true; do
	echo -n "IRC> "
	read -r input

	case "$input" in
		"quit" | "exit")
			send_command "QUIT :$NICKNAME leaving"
			break
			;;
		"register")
			send_command "PASS $PASSWORD"
			send_command "NICK $NICKNAME"
			send_command "USER $USERNAME $HOSTNAME $SERVERNAME $REALNAME"
			;;
		"")
			;;
		*)
			if [[ -n "${tests[$input]}" ]]; then
				execute_command "${tests[$input]}"
			else
				send_command "$input"
			fi
			;;
	esac
done


echo -e "\n${RED}Exiting...${CLEAR}"
exec  3>&-
sleep 1
