# Advanced Chat Application

A multi-client chat application implemented from scratch in C using raw Berkeley sockets — built as two parallel implementations, one over TCP and one over UDP, to compare connection-oriented and connectionless approaches to the same problem.

## Features

### TCP Version
- Multi-client server (up to 10 concurrent clients) using `select()` for I/O multiplexing
- Login-based authentication
- Public broadcast messaging
- Private messaging between users (`/msg`)
- File transfer between clients, base64-encoded over the socket (`/file`)
- User status updates — online / away / busy
- Message history (last *N* messages, on request)
- Join/leave notifications with timestamps

### UDP Version
- Multi-client connectionless chat server
- Join / leave handling without a persistent connection
- Public broadcast messaging
- User status updates
- Message history

## Tech Stack

- C
- POSIX sockets (`sys/socket.h`, `netinet/in.h`, `arpa/inet.h`)
- `select()` for I/O multiplexing

## Project Structure

```
24i2008-AbdulRaffay-A02-B/
├── 24i2008-AbdulRaffay-A02-TCP-B/
│   ├── server.c
│   └── client.c
├── 24i2008-AbdulRaffay-A02-UDP-B/
│   ├── server.c
│   └── client.c
├── Screenshots/
└── Report/
    └── 24i2008-AbdulRaffay-Report-B.pdf
```

## Getting Started

### TCP
```bash
cd 24i2008-AbdulRaffay-A02-TCP-B
gcc server.c -o server
gcc client.c -o client

./server          # in one terminal
./client           # in another terminal (repeat for more clients)
```
Log in first with `/login <username> <password>` using one of the demo credentials defined in `server.c`, then type `/help` to see available commands.

### UDP
```bash
cd 24i2008-AbdulRaffay-A02-UDP-B
gcc server.c -o server
gcc client.c -o client

./server
./client            # enter a username when prompted, repeat for more clients
```
Type `/help` after joining to see available commands.

## Notes

- Both server/client pairs run on `127.0.0.1` by default — for testing across machines, update the server address in `client.c`.
- The TCP server's login credentials are hardcoded for demo purposes; this is a class assignment, not a production auth system.

## Author

**Abdul Raffay** — BS Cyber Security, FAST University Islamabad
