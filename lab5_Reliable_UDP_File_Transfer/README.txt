RDT3.0 Stop-and-Wait over UDP (Client/Server)

Files:
- client.c : UDP sender implementing rdt3.0 stop-and-wait with seq {0,1}, checksum, timeout+retransmit
- server.c : UDP receiver implementing rdt3.0 receiver FSM with expected seq, duplicate handling, ACK resend

Build:
gcc -Wall -Wextra -O0 -g client.c -o client
gcc -Wall -Wextra -O0 -g server.c -o server

Run:
Terminal 1:
./server <port> <outfile>

Terminal 2:
./client <server_ip> <port> <srcfile>

Example:
./server 9000 out.txt
./client 127.0.0.1 9000 in.txt

Protocol:
Header: seq_ack (int), len (int), cksum (int)
Data: up to 10 bytes per packet
Final packet: len=0 indicates end-of-file

Checksum:
XOR of bytes across (Header + len bytes of data), with cksum field set to 0 during computation

Loss/Corruption Simulation:
Client:
- drops sending packet with probability 20%
- corrupts checksum with probability 20%
Server:
- drops ACK with probability 20%
(optional) server may drop received data packet if enabled in code

Timeout:
Client uses select() with 1-second timeout; retransmits on timeout or bad/wrong ACK.

Retry limit:
Client gives up after 3 failed attempts for a packet (as per template).