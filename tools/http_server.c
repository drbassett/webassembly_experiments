#pragma warning(push, 0)
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

typedef struct addrinfo addrinfo;
#pragma warning(pop)

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef intptr_t iword;
typedef uintptr_t uword;

typedef u32 b32;

#define FALSE 0
#define TRUE 1

#define ArrayCount(A) (sizeof(A) / sizeof((A)[0]))
#define ClearValueToZero(Value) memset(&(Value), 0, sizeof(Value))
#define ClearArrayToZero(Count, Array) memset((Array), 0, sizeof(*(Array)) * (Count))

inline static void* checkOutOfMemory(void* pointer) {
	if (!pointer) {
		fprintf(stderr, "Out of memory");
		ExitProcess(1);
	}
	return pointer;
}

inline static void* reallocSafe(void* pointer, uword newSize) {
	return checkOutOfMemory(realloc(pointer, newSize));
}

typedef struct StringSlice {
	const char* begin;
	const char* end;
} StringSlice;

// for the "%.*s" format in printf
#define StringSlicePrintf(S)  (int) stringSliceLength(S), (S).begin

inline static StringSlice stringSlice(const char* begin, const char* end) {
	StringSlice ss = {begin, end};
	return ss;
}

inline static StringSlice stringSliceFromCString(const char* cs) {
	StringSlice ss = {
		.begin = cs,
		.end = cs + strlen(cs),
	};
	return ss;
}

//TODO rename to stringSliceCount
inline static uword stringSliceLength(StringSlice ss) {
	return (uword) (ss.end - ss.begin);
}

inline static b32 stringSliceEmpty(StringSlice ss) {
	return ss.begin == ss.end;
}

static b32 stringSlicesEqual(const StringSlice* a, const StringSlice* b) {
	uword aLen = stringSliceLength(*a);
	uword bLen = stringSliceLength(*b);
	if (aLen != bLen) {
		return FALSE;
	}
	const char* ap = a->begin;
	const char* bp = b->begin;
	while (ap != a->end) {
		if (*ap != *bp) {
			return FALSE;
		}
		++ap;
		++bp;
	}
	return TRUE;
}

static b32 stringSliceEqualsCString(const StringSlice* ss, const char* cs) {
	const char* begin = ss->begin;
	for (;;) {
		b32 end1 = (begin == ss->end);
		b32 end2 = (*cs == '\0');
		if (end1 | end2) {
			return end1 & end2;
		}
		if (*begin != *cs) {
			return FALSE;
		}
		++begin;
		++cs;
	}
}

const char* serverAddress = "127.0.0.1";
const char* port = "6931";

const char* index_html =
	"<!DOCTYPE html>\n"
	"<html>\n"
	"	<head>\n"
	"		<meta charset=\"utf-8\">\n"
	"	</head>\n"
	"	<body>\n"
	"		Hello, world!\n"
	"	</body>\n"
	"</html>\n";

static b32 socketSend(const char* context, SOCKET socket, uword byteCount, const void* bytes) {
	while (byteCount > 0) {
		int bytesSent = send(socket, (char*) bytes, byteCount, 0);
		if (bytesSent == SOCKET_ERROR) {
			fprintf(stderr, "[%s] sending data failed: %d\n", context, WSAGetLastError());
			return FALSE;
		}
		byteCount -= bytesSent;
	}
	return TRUE;
}

static b32 socketReceive(
const char* context, SOCKET socket, uword maxByteCount,
void* bytes, uword* receivedByteCount, b32* connectionClosed) {
	*receivedByteCount = 0;
	*connectionClosed = TRUE;
	int receivedLength = recv(socket, bytes, maxByteCount, 0);
	if (receivedLength == SOCKET_ERROR) {
		fprintf(stderr, "[%s] recv() failed: %d\n", context, WSAGetLastError());
		return FALSE;
	}
	*connectionClosed = (receivedLength == 0);
	*receivedByteCount = receivedLength;
	return TRUE;
}

static b32 socketSendCString(const char* context, SOCKET socket, const char* string) {
	uword stringLength = strlen(string) + 1;
	return socketSend(context, socket, stringLength, string);
}

inline static void skipSpaces(const char** cursor, const char* end) {
	for (;;) {
		if (*cursor == end || **cursor != ' ') {
			return;
		}
		*cursor += 1;
	}
}

void httpServerExitError() {
//TODO maybe attempt to automatically reboot the server?
	ExitProcess(1);
}

typedef struct HttpHeader {
	uword charCount;
	const char* chars;
	const char* cursor;
} HttpHeader;

typedef struct HttpOption {
	StringSlice key;
	StringSlice value;
} HttpOption;

// Read the next line from a buffered HTTP header. If this line is the end of
// the header (a blank line), this function returns false. Otherwise, it
// returns TRUE.
static StringSlice httpHeaderNextLine(HttpHeader* header) {
	const char* lineBegin = header->cursor;
	const char* headerEnd = header->chars + header->charCount;
	for (;;) {
		uword remainingByteCount = headerEnd - header->cursor;
		assert(remainingByteCount >= 2); // if this is false, we have not been given a valid HTTP header
		if ((header->cursor[0] == '\r') & (header->cursor[1] == '\n')) {
			StringSlice line = stringSlice(lineBegin, header->cursor);
			header->cursor += 2; // advance to the start of the next line
			return line;
		}
		++header->cursor;
	}
}

static HttpOption httpHeaderNextOption(HttpHeader* header) {
	StringSlice line = httpHeaderNextLine(header);
	uword lineLength = stringSliceLength(line);

	HttpOption option;
	const char* lineCursor = line.begin;
	for (;;) {
		// No colon on this line. Set the key to the entire line; the value
		// will end up being nothing. This also handles the blank line at the
		// end of the header, without a special case.
		if (lineCursor == line.end) {
			option.key = line;
			break;
		}
		char c = *lineCursor;
		++lineCursor;
		if (c == ':') {
			option.key = stringSlice(line.begin, lineCursor - 1);
			break;
		}
	}
	skipSpaces(&lineCursor, line.end);
	option.value = stringSlice(lineCursor, line.end);
	option.value.end = line.end;
	return option;
}

typedef struct HttpBuffer {
	const char* name;
	u8* data;
	uword capacity;
	uword size;
	SOCKET socket;

//TODO could use a bitset for these flags. Since they are mutually exclusive, they could also be an enum.
	b32 error;
	b32 connectionClosed;
} HttpBuffer;

static void httpBufferInit(HttpBuffer* buffer, const char* name) {
	ClearValueToZero(*buffer);
	buffer->name = name;
}

static void httpBufferGrow(HttpBuffer* buffer) {
	uword newCapacity = (buffer->capacity == 0) ? 1024 : buffer->capacity * 2;
	buffer->data = checkOutOfMemory(realloc(buffer->data, newCapacity));
	buffer->capacity = newCapacity;
}

static void httpBufferReadBytes(HttpBuffer* buffer) {
	if (buffer->capacity == buffer->size) {
		httpBufferGrow(buffer);
	}
	uword remainingCapacity = buffer->capacity - buffer->size;
	u8* receivePointer = buffer->data + buffer->size;
	uword receivedByteCount;
	buffer->error |= !socketReceive(
		buffer->name, buffer->socket, remainingCapacity, receivePointer,
		&receivedByteCount, &buffer->connectionClosed);
	buffer->size += receivedByteCount;
}

static void httpBufferDiscardBytes(HttpBuffer* buffer, uword byteCount) {
//TODO this isn't the most efficient way to discard information - a ring buffer would avoid copies
	assert(byteCount <= buffer->size);
	uword newSize = buffer->size - byteCount;
	memmove(buffer->data, buffer->data + byteCount, newSize);
	buffer->size = newSize;
}

//TODO in case of malicious/malformed packets, add a maxByteCount parameter; if
// we do not find the end of the header before reaching this limit, log an
// error message, send an error response to the client, and close the
// connection to the client.
static void httpBufferReadHeader(HttpBuffer* buffer, HttpHeader* header) {
	uword cursor = 0;
	for (;;) {
		while (cursor < buffer->size) {
			uword remainingByteCount = buffer->size - cursor;
			if (remainingByteCount < 4) {
				break;
			}
			b32 blankLine =
				(buffer->data[cursor + 0] == '\r') &
				(buffer->data[cursor + 1] == '\n') &
				(buffer->data[cursor + 2] == '\r') &
				(buffer->data[cursor + 3] == '\n');
			if (blankLine) {
				cursor += 4;
				header->charCount = cursor;
				header->chars = buffer->data;
				header->cursor = buffer->data;
				return;
			}
			++cursor;
		}
		httpBufferReadBytes(buffer);
		if (buffer->error | buffer->connectionClosed) {
			return;
		}
	}
}

inline static void httpBufferReset(HttpBuffer* buffer) {
	buffer->size = 0;
	buffer->socket = INVALID_SOCKET;
	buffer->error = FALSE;
	buffer->connectionClosed = FALSE;
}

inline static void httpBufferDestroy(HttpBuffer* buffer) {
	free(buffer->data);
	ClearValueToZero(*buffer);
	buffer->socket = INVALID_SOCKET;
}

static b32 httpSendOkResponse(const char* context, SOCKET socket, uword contentLength, const void* content) {
	char header[1024]; //TODO prune the size of this header
	int headerLength = sprintf(
		header,
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Length: %llu\r\n"
		"Content-Type: text/html\r\n"
		"\r\n",
		(unsigned long long) contentLength);
	return
		socketSend(context, socket, headerLength, header) &&
		socketSend(context, socket, contentLength, content);
}

static b32 httpSend404Response(const char* context, SOCKET socket) {
	const char* response =
		"HTTP/1.1 404 NOT FOUND\r\n"
		"Connection: close\r\n"
		"Content-Length: 14\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"404 NOT FOUND\n";
//TODO this string length could be precomputed
	uword responseLength = strlen(response);
	return socketSend(context, socket, responseLength, response);
}

static void serviceClient(HttpBuffer* buffer, SOCKET clientSocket) {
//TODO need timeouts on waiting for data - is this built in to the socket somewhere?

	httpBufferReset(buffer);
//TODO it feels wrong to use the HTTP buffer's socket as the client socket
	buffer->socket = clientSocket;
	
	for (;;) {
		HttpHeader header;
		httpBufferReadHeader(buffer, &header);
		if (buffer->error) {
			return;
		}
		if (buffer->connectionClosed) {
			printf("[Server] Client closed connection.\n");
			break;
		}

		StringSlice requestLine = httpHeaderNextLine(&header);
		uword requestLineLength = stringSliceLength(requestLine);

		const char* requestLineCursor = requestLine.begin;
		b32 getRequest = requestLineLength >= 3 && (
			((requestLineCursor[0] == 'g') | (requestLineCursor[0] == 'G')) &
			((requestLineCursor[1] == 'e') | (requestLineCursor[1] == 'E')) &
			((requestLineCursor[2] == 't') | (requestLineCursor[2] == 'T')));
		if (getRequest) {
			requestLineCursor += 3;
			skipSpaces(&requestLineCursor, requestLine.end);
			const char* urlBegin = requestLineCursor;
			for (;;) {
				if (requestLineCursor == requestLine.end || *requestLineCursor == ' ') {
					break;
				}
				++requestLineCursor;
			}
			const char* urlEnd = requestLineCursor;
			StringSlice url = stringSlice(urlBegin, urlEnd);
			printf(
				"[Server] Received GET request: '%.*s'\n",
				StringSlicePrintf(url));

			// read through the rest of the HTTP options
			for (;;) {
				HttpOption option = httpHeaderNextOption(&header);
				// check for blank key - this marks the end of the HTTP header
				if (stringSliceEmpty(option.key)) {
					break;
				}
				printf("    %.*s: %.*s\n", StringSlicePrintf(option.key), StringSlicePrintf(option.value));
			}

			b32 indexFileRequested =
				stringSliceEmpty(url) ||
				stringSliceEqualsCString(&url, "/") ||
				stringSliceEqualsCString(&url, "/index.html");
			if (indexFileRequested) {
				printf("[Server] Sending index.html to client...\n");
				if (!httpSendOkResponse("SERVER", clientSocket, strlen(index_html), index_html)) {
					return;
				}
			} else {
				printf(
					"[Server] Sending 404 response for request 'GET %.*s'...\n",
					StringSlicePrintf(url));
				if (!httpSend404Response("Server", clientSocket)) {
					return;
				}
			}
		} else {
//TODO send response stating the request was invalid, and close connection
			fprintf(
				stderr, "[Server] Unknown HTML request: '%.*s'\n",
				StringSlicePrintf(requestLine));
			return;
		}

		httpBufferDiscardBytes(buffer, header.charCount);
	}

	// shut down the client if the connection has not yet been closed
	if (!buffer->connectionClosed) {
		printf("[Server] Shutting down client.\n");
		if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) {
			fprintf(stderr, "[Server] shutdown() failed: %d\n", WSAGetLastError());
			return;
		}
	}

	printf("[Server] Closing client socket.\n");
	if (closesocket(clientSocket) == SOCKET_ERROR) {
		fprintf(stderr, "[Server] closesocket() for client socket failed: %d\n", WSAGetLastError());
	}
}

static int runServer() {
	int wsResult;

	addrinfo addrHints = {
		.ai_flags = AI_PASSIVE,
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};
	addrinfo* addr;
	wsResult = getaddrinfo(NULL, port, &addrHints, &addr);
	if (wsResult != 0) {
		fprintf(stderr, "[Server] getaddrinfo() failed: %d\n", wsResult);
		return 1;
	}

	SOCKET listenSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		fprintf(stderr, "[Server] socket() failed: %d\n", WSAGetLastError());
		return 1;
	}

	printf("[Server] Waiting for connection request...\n");

	wsResult = bind(listenSocket, addr->ai_addr, (int) addr->ai_addrlen);
	if (wsResult != 0) {
		fprintf(stderr, "[Server] bind() failed: %d\n", wsResult);
		return 1;
	}
	freeaddrinfo(addr);

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		fprintf(stderr, "[Server] listen() failed: %d\n", WSAGetLastError());
		return 1;
	}

//TODO this buffer could grow indefinitely if given a bad packet stream. Add some protections against this.
	HttpBuffer httpBuffer;
	httpBufferInit(&httpBuffer, "Server");

//TODO provide some means of shutting down the server
	for (;;)
	{
		SOCKET clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			fprintf(stderr, "[Server] accept() failed: %d\n", WSAGetLastError());
			continue;
		}
		printf("[Server] Connected to client.\n");
		serviceClient(&httpBuffer, clientSocket);
	}

	httpBufferDestroy(&httpBuffer);

	printf("[Server] Closing listen socket.\n");
	if (closesocket(listenSocket) == SOCKET_ERROR) {
		fprintf(stderr, "[Server] closesocket() for listen socket failed: %d\n", WSAGetLastError());
	}

	printf("[Server] Done.\n");
	return 0;
}

static DWORD WINAPI runClient(void* param) {
	int wsResult;

	addrinfo addrHints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};
	addrinfo* addr; //LEAK
	wsResult = getaddrinfo(serverAddress, port, &addrHints, &addr);
	if (wsResult != 0) {
		fprintf(stderr, "[Client] getaddrinfo() failed: %d\n", wsResult);
		return 1;
	}

	SOCKET sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (sock == INVALID_SOCKET) {
		fprintf(stderr, "[Client] socket() failed: %d\n", WSAGetLastError());
		return 1;
	}

	printf("[Client] Establishing connection to server...\n");

	wsResult = connect(sock, addr->ai_addr, (int) addr->ai_addrlen);
	if (wsResult == SOCKET_ERROR) {
		fprintf(stderr, "[Client] connect() failed: %d\n", wsResult);
		return 1;
	}
	freeaddrinfo(addr);
	addr = NULL;

	printf("[Client] Connected to server.\n");

	const char* httpRequest =
		"GET /index.html\r\n"
		"Accept: text/html\r\n"
		"Accept-Charset: utf-8\r\n"
		"Accept-Encoding: gzip, deflate\r\n"
		"\r\n";
	uword stringLength = strlen(httpRequest);
	if (!socketSend("Client", sock, stringLength, (char*) httpRequest)) {
		return 1;
	}
	printf("[Client] Sent GET request to server.\n");

	HttpBuffer buffer;
	httpBufferInit(&buffer, "Client");
	buffer.socket = sock;

	HttpHeader header;
	httpBufferReadHeader(&buffer, &header);
	if (buffer.error) {
		return 1;
	}
	if (buffer.connectionClosed) {
		printf("[Client] Server closed connection before it sent response.\n");
		return 1;
	}

	printf(
		"[Client] Received reponse from server:\n%.*s",
		(int) header.charCount, header.chars);

//TODO log response from server

	if (shutdown(sock, SD_SEND) == SOCKET_ERROR) {
		fprintf(stderr, "[Client] shutdown() failed: %d\n", WSAGetLastError());
		return 1;
	}

	if (closesocket(sock) == SOCKET_ERROR) {
		fprintf(stderr, "[Client] closesocket() failed: %d\n", WSAGetLastError());
	}

	printf("[Client] Done.\n");
	return 0;
}

int main(int argc, char* argv[]) {
	WSADATA wsaData;
	int wsResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsResult != 0) {
		fprintf(stderr, "WSAStartup() failed: %d\n", wsResult);
		return 1;
	}

	b32 runTestClient = FALSE;
	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];
		if (strcmp(arg, "--test-client") == 0) {
			runTestClient = TRUE;
		} else {
			fprintf(stderr, "Unknown option '%s'\n", arg);
			return 1;
		}
	}

	HANDLE clientThread = NULL;
	if (runTestClient) {
		printf("[Client] Starting test client...\n");
		clientThread = CreateThread(NULL, 0, runClient, NULL, 0, NULL);
		if (clientThread == NULL) {
			fprintf(stderr, "Failed to create test client thread\n");
			return 1;
		}
	}

	int mainResult = runServer();

	if (runTestClient) {
		assert(clientThread);
		WaitForSingleObject(clientThread, INFINITE);
		DWORD clientExitCode;
		GetExitCodeThread(clientThread, &clientExitCode);
		CloseHandle(clientThread);
		if (clientExitCode != 0) {
			mainResult = 1;
		}
	}

	WSACleanup();
	return mainResult;
}
