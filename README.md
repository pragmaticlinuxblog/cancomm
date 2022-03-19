# LibCanComm
[![License: LGPL v2.1](https://img.shields.io/badge/license-LGPL_2.1-blue.svg)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

*LibCanComm* is a Linux shared library for convenient access to CAN communication, based on the SocketCAN kernel functionality. Its goal is to make it easier for application developers to access and exchange data with nodes on a CAN bus. The library supports both CAN classic and CAN FD message formats. *LibCanComm* embeds functionality for:

* Listing the available SocketCAN devices on the user's system.
* Connecting to a SocketCAN device.
* Transmitting and receiving CAN messages.

The library itself is developed in the C programming language. While designing the library's API, special care was taken to make it uncomplicated to create bindings for other programming languages. As such, it should be straightforward to access the *LibCanComm* shared library from different programming languages, such as Python, Java, C#, Rust, Go, Ruby, Object Pascal, etc.

Note that LibCanComm does not provide functionality related to configuring and bringing up SocketCAN devices on the user's system. The excellent [LibSocketCan](https://git.pengutronix.de/cgit/tools/libsocketcan) shared library already covers this, if you want to perform these tasks programmatically.

## Getting started

To get started with *LibCanComm*, it is recommended to download the latest stable release from the [releases](https://github.com/pragmaticlinuxblog/cancomm/releases) page. Next, read through the getting started documentation in the [user manual](https://pragmaticlinuxblog.github.io/cancomm/latest/).

## User manual

The online user manual is located at the following location:

- [https://pragmaticlinuxblog.github.io/cancomm/latest/](https://pragmaticlinuxblog.github.io/cancomm/latest/)

## Development

Development of *LibCanComm* takes place at GitHub. Feel free to contribute by submitting issues and pull requests.

* [https://github.com/pragmaticlinuxblog/cancomm](https://github.com/pragmaticlinuxblog/cancomm)

