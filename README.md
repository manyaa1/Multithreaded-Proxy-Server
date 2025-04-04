# Multithreaded-Proxy-Server

# 🔗 Multithreaded HTTP Proxy Server (Windows, Winsock)

This is a **multithreaded HTTP proxy server** built using **C** and **Winsock (Windows sockets)**. It handles multiple client requests concurrently and includes a built-in **LRU (Least Recently Used) cache** to store frequently accessed responses, improving performance and reducing external server load.

---

## Features

-  Handles multiple simultaneous client requests using threads.
-  Implements an efficient **LRU caching** mechanism.
-  Caches entire HTTP GET responses (headers + body).
-  Logs cache hits and misses.
-  Supports standard HTTP GET requests via browsers or tools like `curl`.
-  Compatible with **Windows OS** using **Winsock** API.

---

##  How It Works

1. Server starts and listens for connections on a specified port (e.g., `8081`).
2. For each incoming connection:
   - A new thread is spawned to handle the request.
   - The request is parsed to extract the target URL.
   - If the URL is **found in the cache**, the response is served from cache (cache hit).
   - If not, the request is **forwarded** to the remote server, and the response is cached (cache miss).
3. The cache uses LRU policy to evict the least recently used entries when full.

---

##  Example Usage

```bash
# Start the proxy server (make sure to build the project first)
proxy.exe

# Use curl to test with proxy
curl -x http://127.0.0.1:8081 http://example.com
curl -x http://127.0.0.1:8081 http://neverssl.com

###  Clone the Repository

```bash
git clone https://github.com/yourusername/multithreaded-http-proxy-server.git
cd multithreaded-http-proxy-server
Replace username with your username

```
##  Build Instructions
1.Open the project in Visual Studio.

2.Ensure you have C++ support and Windows Desktop Development tools installed.

3.Link the ws2_32.lib library:

Go to Project Properties → Linker → Input → Additional Dependencies → Add ws2_32.lib

4.Build the project.

###  Run the Proxy Server

After building the project:

```bash

 gcc proxy_server_with_cache.c -o proxy_server_with_cache.exe -lws2_32 -mconsole
./proxy_server_with_cache.exe

```
The socked has been made and the proxy server is now listening for the GET reuqest from client:

![image](https://github.com/user-attachments/assets/6d8ba43e-d564-4b6e-8176-7cb7916eb56c)


###  Test with curl
```bash
curl -x http://127.0.0.1:8081 http://example.com
curl -x http://127.0.0.1:8081 http://neverssl.com

```
The proxy_log.text file will log all the cache hits and cache misses as shown:

![image](https://github.com/user-attachments/assets/e17532b6-3106-4836-a683-4460ecc2a9cb)

The server returns the html page in reponse to the client request being made as shown:

![image](https://github.com/user-attachments/assets/a9dc8fbf-d1f3-4612-ac63-00d30593474e)

### FINAL NOTE
This project was built to demonstrate multithreading, socket programming, and caching techniques using C++ on Windows. Feel free to explore, improve, and adapt it for your learning or networking experiments!





