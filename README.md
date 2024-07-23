# DenIM - Deniable Instant Messenger

## About
DenIM is a client-to-client instant messaging client offering enhanced deniability and perfect forward secrecy without the involvement of a third-party server.

![](./assets/images/sock.jpg)

Built using C++ and powered by Boost and Botan libraries, DenIM is an instance of usable OTR (Off-The-Record) application based on the OTR messaging protocol, accessible to a user even with no security background. 

DenIM offers an encrypted socket-based communication using the standard AES-256 encryption, with a new key derived using Diffie-Hellman(DH) Key Exchange between the parties after each instance of message exchange.

![](./assets/images/keyex.jpg)

**Key Features**

1. **Deniability for both parties**
> DenIM's underlying communication protocol is deniable with the ability of editing the sent and received messages for both of the parties participating in a session.

2. **Perfect Forward Secrecy**
> With a new 256-bit key generated after each message exchange, DenIM offers perfect forward secrecy which ensures minimal-to-no compromise against the leak of the encryption key.

## Installation

### Docker-based installation

1. Build the Docker image:
```bash
docker build -t denim_project .
```
2. Run the Docker container:
```bash
docker run -it denim_project
```
**NOTE: Make sure you have Docker installed**

### Non-Docker-based  installation

To install and modify using non-docker installation, make sure you have the following dependencies and **make** installed:
- [Boost 1.85.0](https://www.boost.org/users/history/version_1_85_0.html)
- [Botan3](https://botan.randombit.net/)
- [sqlite3](https://www.sqlite.org/cintro.html)

```bash
sudo apt-get install build-essential
```

Open the terminal in the project directory and run:
```bash
make
```

To build using CMake (if CMake is installed):
```bash
cmake --build /. --config Debug --target all -j 12 --
```

## Usage
Select the desired mode of operation

Connect with an user to start a communication session

**In-session commands**
> 1. To view the message history, enter ":v"

> 2. To edit a sent/received message, enter ":e" and then enter the particular index of the respective message

> 3. To delete a sent/received message, enter ":d" and then enter the particular index of the respective message


## Snapshots 

**DenIM's welcome screen**

![](./assets/images/welcome.jpg)

**Starting a communication session**

![](./assets/images/mode.jpg)

![](./assets/images/connect.jpg)

**An ongoing session**

![](./assets/images/session.jpg)

**Message history**

![](./assets/images/history.jpg)

**Editing a message**

![](./assets/images/edit.jpg)

**Deleting a message**

![](./assets/images/delete.jpg)

## Contributing

Pull requests are welcome. 

To implement major changes, please open an issue first to discuss about what you would like to change.

