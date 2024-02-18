# Adria Server Library (ASR)

ASR is a portable cross-platform humble and lightweight library to create TCP/UDP servers (and clients) using our beloved C++, currently supported as-is on Windows and Linux using Clang, haven't tested MacOS (yet).

Includes support for a variety of useful features such as:

- Channel management
- Message schemas!
- Stream noise tolerance
- Event buses
- Buffered I/O
- ... And more (to come)

## Compilation

Depending on your platform, rename the `Makefile.linux` or `Makefile.win` to just `Makefile`, ensure to have Clang build tools installed.

Compile by running `make all` and libasr will be available in the `lib` folder.

## Examples

There are some examples of plain UDP/TCP servers in the `examples` folder, however the big example that includes handlers, channels, schemas is the "custom_protocol".

Run `make custom_protocol_server` to start the server in port 1000. Then in another terminal window run `make custom_protocol_client` to start the client.

The client will some messages to the server, and the server will reply as well, after a couple of these the client will disconnect.

Every one second a memory/channel reporter will print a message on screen showing how many memory blocks and channels are active (or were handled overall).
