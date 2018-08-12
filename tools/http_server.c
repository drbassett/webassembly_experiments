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

static b32 socketReceive(const char* context, SOCKET socket, uword byteCount, void* bytes, uword* bytesReceived) {
	*bytesReceived = 0;
	int receivedLength = recv(socket, bytes, byteCount, 0);
	if (receivedLength == SOCKET_ERROR) {
		fprintf(stderr, "[%s] recv() failed: %d\n", context, WSAGetLastError());
		return FALSE;
	}
	*bytesReceived = receivedLength;
	return TRUE;
}

static b32 socketSendCString(const char* context, SOCKET socket, const char* string) {
	uword stringLength = strlen(string) + 1;
	return socketSend(context, socket, stringLength, string);
}

static b32 socketReceiveCString(
const char* context, SOCKET socket, uword bufferCapacity, u8* buffer, uword* stringLength) {
	uword bufferLength = 0;
	for (;;) {
		if (bufferCapacity == 0) {
			fprintf(stderr, "[%s] Buffer overflow on recv()\n", context);
			return FALSE;
		}

		uword bytesReceived;
		if (!socketReceive(context, socket, bufferCapacity, buffer + bufferLength, &bytesReceived)) {
			return FALSE;
		}
		assert(bytesReceived <= bufferCapacity);
		bufferCapacity -= bytesReceived;

		u64 nextLength = bufferLength + bytesReceived;
		for (u64 i = bufferLength; i < nextLength; ++i) {
			if (buffer[i] == '\0') {
				*stringLength = bufferLength;
				return TRUE;
			}
		}
		bufferLength = nextLength;
	}
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

	char* receivedChars[1024];
	uword maxCharCount = ArrayCount(receivedChars);

	uword bytesReceived = 0;
	if (!socketReceive("Client", sock, maxCharCount, receivedChars, &bytesReceived)) {
		return 1;
	}
	printf("[Client] Received reponse from server.\n");

//TODO print response from server

	if (shutdown(sock, SD_SEND) == SOCKET_ERROR) {
		fprintf(stderr, "[Client] shutdown() failed: %d\n", WSAGetLastError());
		return 1;
	}

	closesocket(sock);

	printf("[Client] Done.\n");
	return 0;
}

inline static void skipSpaces(const char** cursor, const char* end) {
	for (;;) {
		if (*cursor == end || **cursor != ' ') {
			return;
		}
		*cursor += 1;
	}
}

typedef struct HttpBuffer {
	u8* data;
	uword capacity;
	uword size;
	SOCKET socket;
	const char* name;
} HttpBuffer;

static void httpBufferInit(HttpBuffer* buffer, const char* name, SOCKET socket) {
	ClearValueToZero(*buffer);
	buffer->socket = socket;
	buffer->name = name;
}

static void httpBufferGrow(HttpBuffer* buffer) {
	uword newCapacity = buffer->capacity == 0 ? 1 : buffer->capacity * 2;
	buffer->data = checkOutOfMemory(realloc(buffer->data, newCapacity));
	buffer->capacity = newCapacity;
}

static b32 httpBufferReadBytes(HttpBuffer* buffer) {
	uword remainingCapacity = buffer->capacity - buffer->size;
	if (remainingCapacity == 0) {
		httpBufferGrow(buffer);
	}
	remainingCapacity = buffer->capacity - buffer->size;
	int receivedLength = recv(buffer->socket, buffer->data + buffer->size, remainingCapacity, 0);
	if (receivedLength == SOCKET_ERROR) {
		fprintf(stderr, "[%s] recv() failed: %d\n", buffer->name, WSAGetLastError());
		return FALSE;
	}
	buffer->size += receivedLength;
	return TRUE;
}

static void httpBufferDiscardBytes(HttpBuffer* buffer, uword byteCount) {
//TODO this isn't the most efficient way to discard information - a ring buffer would avoid copies
	assert(byteCount <= buffer->size);
	uword newSize = buffer->size - byteCount;
	memmove(buffer->data, buffer->data + byteCount, newSize);
	buffer->size = newSize;
}

static b32 httpReadLine(HttpBuffer* buffer, uword* cursor, uword* endOfLine) {
	for (;;) {
		while (*cursor < buffer->size) {
			if (buffer->size - *cursor < 2) {
				break;
			}
			char thisChar = buffer->data[*cursor];
			char nextChar = buffer->data[*cursor + 1];
			if ((thisChar == '\r') & (nextChar == '\n')) {
				*endOfLine = *cursor;
				*cursor += 2;
				return TRUE;
			}
			*cursor += 1;
		}
		if (!httpBufferReadBytes(buffer)) {
			return FALSE;
		}
	}
}

typedef struct HttpOption {
	StringSlice key;
	StringSlice value;
} HttpOption;

static b32 httpReadOption(HttpBuffer* buffer, uword* cursor, HttpOption* option) {
	uword lineBeginOffset = *cursor;
	uword lineEndOffset = *cursor;
	if (!httpReadLine(buffer, cursor, &lineEndOffset)) {
		return FALSE;
	}
	StringSlice line = {
		buffer->data + lineBeginOffset, buffer->data + lineEndOffset};
	uword lineLength = lineEndOffset - lineBeginOffset;

	const char* lineCursor = line.begin;
	for (;;) {
		// did not find a colon - assume the key is the entire line, and the
		// value is empty. This also handles the blank line marking the end of
		// the HTTP header.
		if (lineCursor == line.end) {
			option->key = line;
			option->value.begin = NULL;
			option->value.end = NULL;
			return TRUE;
		}
		char c = *lineCursor;
		++lineCursor;
		if (c == ':') {
			option->key = stringSlice(line.begin, lineCursor - 1);
			break;
		}
	}
	skipSpaces(&lineCursor, line.end);
	option->value.begin = lineCursor;
	option->value.end = line.end;
	return TRUE;
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

	SOCKET clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		fprintf(stderr, "[Server] accept() failed: %d\n", WSAGetLastError());
		return 1;
	}

	printf("[Server] Connected to client.\n");

//TODO need timeouts on waiting for data - is this built in to the socket somewhere?

//TODO this buffer could grow indefinitely if given a bad packet stream. Add some protections against this.
	HttpBuffer buffer;
	httpBufferInit(&buffer, "Server", clientSocket);

//TODO browsers do not tell us to shut down the connection, so we get stuck in this loop
	for (;;) {

//TODO buffer until the end of the request prior to doing any parsing - this will simplify things

		uword cursor = 0;

		uword requestLineBegin = cursor;
		uword requestLineEnd = cursor;
		if (!httpReadLine(&buffer, &cursor, &requestLineEnd)) {
			return 1;
		}
		uword requestLineLength = requestLineEnd - requestLineBegin;

		const char* lineCursor = buffer.data + requestLineBegin;
		const char* lineEnd = lineCursor + requestLineLength;
		uword lineLength = requestLineLength;
		if (lineLength >= 3 && (
				((lineCursor[0] == 'g') | (lineCursor[0] == 'G')) &
				((lineCursor[1] == 'e') | (lineCursor[1] == 'E')) &
				((lineCursor[2] == 't') | (lineCursor[2] == 'T')))) {
			lineCursor += 3;
			skipSpaces(&lineCursor, lineEnd);
			uword urlBeginOffset = (uword) (lineCursor - buffer.data);
			for (;;) {
				if (lineCursor == lineEnd || *lineCursor == ' ') {
					break;
				}
				++lineCursor;
			}
			uword urlEndOffset = (uword) (lineCursor - buffer.data);
			printf(
				"[Server] Received GET request: '%.*s'\n",
				StringSlicePrintf(stringSlice(buffer.data + urlBeginOffset, buffer.data + urlEndOffset)));

			// pointers are invalidated upon reading more data from the HTTP buffer
			lineCursor = NULL;
			lineEnd = NULL;

			// read through the rest of the HTTP options
			for (;;) {
				HttpOption option;
				if (!httpReadOption(&buffer, &cursor, &option)) {
					return 1;
				}
				// check for blank option - this marks the end of the HTTP header
				if (stringSliceEmpty(option.key)) {
					break;
				}
				uword valueLength = stringSliceLength(option.value);
				printf("    %.*s: %.*s\n", StringSlicePrintf(option.key), StringSlicePrintf(option.value));
			}

//TODO log entire GET request header

			StringSlice url = {buffer.data + urlBeginOffset, buffer.data + urlEndOffset};
			if (stringSliceEmpty(url) ||
					stringSliceEqualsCString(&url, "/") ||
					stringSliceEqualsCString(&url, "/index.html")) {
				printf("[Server] Sending index.html to client\n");
				const char* body = index_html;
				uword bodyLength = strlen(index_html);
				char header[1024];
				int headerLength = sprintf(
					header,
					"HTTP/1.1 200 OK\r\n"
					"Connection: close\r\n"
					"Content-Length: %llu\r\n"
					"Content-Type: text/html\r\n"
					"\r\n",
					(unsigned long long) bodyLength);
				socketSend("Server", clientSocket, headerLength, header);
				socketSend("Server", clientSocket, bodyLength, body);
			} else {
				printf(
					"[Server] Sending 404 response for request 'GET %.*s'\n",
					StringSlicePrintf(url));
				const char* response =
					"HTTP/1.1 404 NOT FOUND\r\n"
					"Connection: close\r\n"
					"Content-Length: 14\r\n"
					"Content-Type: text/plain\r\n"
					"\r\n"
					"404 NOT FOUND\n";
				uword responseLength = strlen(response);
				socketSend("Server", clientSocket, responseLength, response);
			}
			httpBufferDiscardBytes(&buffer, cursor);
		} else {
//TODO send response stating the request was invalid, and close connection
			StringSlice requestLine = {
				buffer.data + requestLineBegin,
				buffer.data + requestLineBegin + requestLineLength};
			fprintf(
				stderr, "[Server] Unknown HTML request: %.*s\n",
				StringSlicePrintf(requestLine));
			return 1;
		}
	}

	printf("[Server] Closing client socket.\n");
	if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) {
		fprintf(stderr, "[Server] shutdown() failed: %d\n", WSAGetLastError());
		return 1;
	}
	closesocket(clientSocket);

	printf("[Server] Closing listen socket.\n");
	closesocket(listenSocket);

	printf("[Server] Done.\n");
	return 0;
}

int main() {
	WSADATA wsaData;
	int wsResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsResult != 0) {
		fprintf(stderr, "WSAStartup() failed: %d\n", wsResult);
		return 1;
	}

//#define TEST_CLIENT

#ifdef TEST_CLIENT
	HANDLE clientThread = CreateThread(NULL, 0, runClient, NULL, 0, NULL);
#endif

	int mainResult = runServer();

#ifdef TEST_CLIENT
	WaitForSingleObject(clientThread, INFINITE);
	DWORD clientExitCode;
	GetExitCodeThread(clientThread, &clientExitCode);
	CloseHandle(clientThread);
	if (clientExitCode != 0)
	{
		mainResult = 1;
	}
#endif

	WSACleanup();
	return mainResult;
}
