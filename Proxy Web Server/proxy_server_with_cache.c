#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h> // for _beginthreadex
#include <stdarg.h> // For va_list, va_start, va_end

#pragma comment(lib, "ws2_32.lib")

#define PORT 8081
#define BUFFER_SIZE 4096
#define MAX_THREADS 10
#define MAX_CACHE_SIZE 100
#define LOG_FILE "proxy_log.txt" // Define LOG_FILE

CRITICAL_SECTION cs;

typedef struct CacheEntry {
    char url[2048];
    char *response;
    int size;
    struct CacheEntry *next;
} CacheEntry;

CacheEntry *cache_head = NULL;
int cache_count = 0;

DWORD WINAPI handle_client(LPVOID arg);

void log_message(const char *format, ...) {
    EnterCriticalSection(&cs);

    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        va_list args;
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fclose(log_file);
    }

    LeaveCriticalSection(&cs);
}
// Check cache
CacheEntry* check_cache(const char *url) {
    CacheEntry *curr = cache_head;
    while (curr) {
        if (strcmp(curr->url, url) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}
void add_to_cache(const char *url, const char *response, int size) {
    if (cache_count >= MAX_CACHE_SIZE) return;

    CacheEntry *new_entry = (CacheEntry *)malloc(sizeof(CacheEntry));
    if (!new_entry) {
        perror("Failed to allocate memory for cache entry");
        return;
    }
    strcpy(new_entry->url, url);
    new_entry->response = (char *)malloc(size);
    if (!new_entry->response) {
        perror("Failed to allocate memory for cache response");
        free(new_entry);
        return;
    }
    memcpy(new_entry->response, response, size);
    new_entry->size = size;
    new_entry->next = cache_head;
    cache_head = new_entry;
    cache_count++;
}

DWORD WINAPI handle_client(LPVOID arg) {
    SOCKET client_socket = (SOCKET)arg;
    char buffer[BUFFER_SIZE], method[16], url[2048], protocol[16];
    char host[1024], path[1024];
    int port = 80;

    int received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (received <= 0) {
        closesocket(client_socket);
        return 1;
    }

    buffer[received] = '\0';
    sscanf(buffer, "%s %s %s", method, url, protocol);

    // Parse URL
    if (strncmp(url, "http://", 7) == 0) {
        if (sscanf(url + 7, "%[^/]%s", host, path) < 1) {
            closesocket(client_socket);
            return 1;
        }
        char *colon = strchr(host, ':');
        if (colon) {
            *colon = '\0';
            port = atoi(colon + 1);
        }
    } else {
        closesocket(client_socket);
        return 1;
    }

    // Check cache first
    CacheEntry *cached = check_cache(url);
    if (cached) {
        send(client_socket, cached->response, cached->size, 0);
        log_message("[CACHE HIT] %s\n", url);
        closesocket(client_socket);
        return 0;
    }

    // Create socket to target server
    SOCKET remote_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_socket == INVALID_SOCKET) {
        printf("Failed to create remote socket: %d\n", WSAGetLastError());
        closesocket(client_socket);
        return 1;
    }

    struct hostent *remote_host = gethostbyname(host);
    if (!remote_host) {
        printf("Host not found: %s\n", host);
        closesocket(remote_socket);
        closesocket(client_socket);
        return 1;
    }

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    remote_addr.sin_addr = *((struct in_addr *)remote_host->h_addr);
    memset(&(remote_addr.sin_zero), 0, 8);

    if (connect(remote_socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        printf("Connection to remote host failed.\n");
        closesocket(remote_socket);
        closesocket(client_socket);
        return 1;
    }

    // Send request to remote
    send(remote_socket, buffer, received, 0);

    // Receive and forward response, also collect for cache
    char *response_data = NULL;
    int total_size = 0;
    int recv_result;

    do {
        recv_result = recv(remote_socket, buffer, BUFFER_SIZE, 0);
        if (recv_result > 0) {
            send(client_socket, buffer, recv_result, 0);

            char *temp = (char *)realloc(response_data, total_size + recv_result);
            if (!temp) {
                perror("Failed to reallocate memory for response data");
                free(response_data);
                closesocket(remote_socket);
                closesocket(client_socket);
                return 1;
            }
            response_data = temp;
            memcpy(response_data + total_size, buffer, recv_result);
            total_size += recv_result;
        } else if (recv_result < 0) {
            printf("Error receiving from remote server: %d\n", WSAGetLastError());
        }
    } while (recv_result > 0);

    if (total_size > 0) {
        add_to_cache(url, response_data, total_size);
        log_message("[CACHE MISS] %s\n", url);
    }

    free(response_data);
    closesocket(remote_socket);
    closesocket(client_socket);
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    printf("Proxy server listening on port %d...\n", PORT);

    InitializeCriticalSection(&cs);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        // Create a thread for each client
        DWORD thread_id;
        HANDLE thread = CreateThread(NULL, 0, handle_client, (LPVOID)client_socket, 0, &thread_id);
        if (thread == NULL) {
            printf("Failed to create thread: %d\n", GetLastError());
            closesocket(client_socket);
        } else {
            CloseHandle(thread); // we don't need to wait
        }
    }

    DeleteCriticalSection(&cs);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}