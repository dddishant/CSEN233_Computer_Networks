# Lab 3 - TCP Client/Server File Transfer

## Files
- tcp_client.c : TCP client requests a file and saves it locally
- tcp_server_single.c : Single-client TCP server that sends requested file
- tcp_server_concurrent.c : Concurrent TCP server using pthreads (one thread per client)

## Compile
gcc tcp_client.c -o tcp_client
gcc tcp_server_single.c -o tcp_server_single
# Lab 3 - TCP Client/Server File Transfer

## Files
- tcp_client.c : TCP client requests a file and saves it locally
- tcp_server_single.c : Single-client TCP server that sends requested file
- tcp_server_concurrent.c : Concurrent TCP server using pthreads (one thread per client)

## Compile
gcc tcp_client.c -o tcp_client
# Lab 3 - TCP Client/Server File Transfer

## Files
- tcp_client.c : TCP client requests a file and saves it locally
- tcp_server_single.c : Single-client TCP server that sends requested file
- tcp_server_concurrent.c : Concurrent TCP server using pthreads (one thread per client)

## Compile
gcc tcp_client.c -o tcp_client
gcc tcp_server_single.c -o tcp_server_single
gcc tcp_server_concurrent.c -o tcp_server_concurrent -lpthread

## Run (example port 6000)
### Server (single)
./tcp_server_single 6000

### Server (concurrent)
./tcp_server_concurrent 6000

### Client
./tcp_client 127.0.0.1 6000 testfile.txt downloaded_testfile.txt
