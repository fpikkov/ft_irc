# ft_irc 💬  
*A modern, lightweight IRC server in C++*  

`ft_irc` is a **multi-client, non-blocking IRC server**  
It speaks the **classic IRC protocol**, so you can connect with your favorite IRC client (tested with **irssi**) to join channels, chat, and relive the golden IRC days — in real time.

This repository implements the core mandatory features of the 42 ft_irc project while emphasizing robustness, clean architecture, clear limits, and strong logging capabilities.


## ✨ Features at a Glance  
- **Single `poll()` loop** — no forking, fully non-blocking I/O  
- **Real IRC messaging** — supports `PRIVMSG`, `NOTICE`, `JOIN`, `PART`, `TOPIC`, `INVITE`, `KICK`, `MODE`  
- **Authentication** — `PASS`, `NICK`, `USER` (with timeouts & validation)  
- **Channel support** — operators, invites, topics, limits, passwords  
- **Connection liveness** — PING/PONG handling & timeouts  
- **Robust framing** — aggregates TCP fragments, enforces protocol limits  
- **Colorized logging** — for easier debugging & monitoring  
- **Graceful shutdown** — handles `SIGINT`/`SIGQUIT`  
 

## 🚀 Quickstart  

### 1. Build  
```bash
make          # default build
make release  # optimized build
make clean    # remove objects
make fclean   # full clean
```

### 2. Run the server  
```bash
./build/ircserv <port> <password>
```

### 3. Connect with client
```bash
# With netcat (basic test)
nc 127.0.0.1 <port>

# With irssi
irssi -c localhost -p <port> -w <password>

# With the provided script (bash)
./client_test.sh
```

### 4. Authenticate client
```irc
PASS <password>
NICK <your_nick>
USER <username> 0 * :<realname>
```

### 5. Shutter down the server  
```bash
CTRL + C
```

## 📖 Example Flow
```bash
> PASS hunter2
> NICK alice
> USER alice 0 * :Alice Wonderland
:ircserv 001 alice :Welcome to ft_irc!
JOIN #tea
PRIVMSG #tea :Hello world!
```

## 🛠️ Project Structure  

- **`main.cpp`** → entry point, starts the server loop  
- **`Server`** → handles sockets, clients, channels, poll loop  
- **`Client`** → stores nick/user/host, buffers, states  
- **`Channel`** → tracks members, topic, modes, operators  
- **`CommandHandler`** → dispatches IRC commands  
- **`Logger`** → colorful, timestamped logs  


## 🧩 Supported Channel Modes  

- `+i` — invite-only  
- `+t` — topic lock (ops only)  
- `+k <key>` — password  
- `+o <nick>` — grant/revoke operator  
- `+l <limit>` — set/unset user limit  


## 📦 Limits (RFC-friendly defaults)  

- Max message length: **512** chars  
- Per-client buffer: **4096** bytes  
- Max channels per user: **20**  
- Nickname length: **16**  
- Username length: **16**  

*(all configurable in `include/constants.hpp`)*  


## 🪪 License  
Licensed under the **GNU GPL v3.0**. See [LICENSE](LICENSE).  

