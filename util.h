#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef void (*FExitError)();

static void defaultExitError() {
	exit(1);
}

FExitError exitError = defaultExitError;

typedef u32 b32;
#define FALSE 0
#define TRUE 1

#define StringifyHelper(X) #X
#define Stringify(X) StringifyHelper(X)

#define LogError(Message, ...) \
	fprintf(stderr, __FILE__ "@" Stringify(__LINE__) ": " Message, ##__VA_ARGS__);

#define FatalError(Message, ...) \
	LogError(Message, ##__VA_ARGS__); \
	exitError()

static const char* ut_emResultToString(EMSCRIPTEN_RESULT result) {
	switch (result) {
	case EMSCRIPTEN_RESULT_SUCCESS:             return "EMSCRIPTEN_RESULT_SUCCESS";
	case EMSCRIPTEN_RESULT_DEFERRED:            return "EMSCRIPTEN_RESULT_DEFERRED";
	case EMSCRIPTEN_RESULT_NOT_SUPPORTED:       return "EMSCRIPTEN_RESULT_NOT_SUPPORTED";
	case EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED: return "EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED";
	case EMSCRIPTEN_RESULT_INVALID_TARGET:      return "EMSCRIPTEN_RESULT_INVALID_TARGET";
	case EMSCRIPTEN_RESULT_UNKNOWN_TARGET:      return "EMSCRIPTEN_RESULT_UNKNOWN_TARGET";
	case EMSCRIPTEN_RESULT_INVALID_PARAM:       return "EMSCRIPTEN_RESULT_INVALID_PARAM";
	case EMSCRIPTEN_RESULT_FAILED:              return "EMSCRIPTEN_RESULT_FAILED";
	case EMSCRIPTEN_RESULT_NO_DATA:             return "EMSCRIPTEN_RESULT_NO_DATA";
	case EMSCRIPTEN_RESULT_TIMED_OUT:           return "EMSCRIPTEN_RESULT_TIMED_OUT";
	default:
		assert(0);
		return "?";
	}
}

#define UtEmCheckResult(FunctionCall) \
	ut__emCheckResultImpl((FunctionCall), #FunctionCall)

inline static void ut__emCheckResultImpl(EMSCRIPTEN_RESULT result, const char* functionCall) {
	if (result < 0) {
		FatalError(
			"Function call failed with error %s (%d): %s\n",
			ut_emResultToString(result), result, functionCall);
	}
}

static b32 ut_glCompileShader(const char* name, GLuint shader, const char* source) {
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		GLint logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		char* log = malloc(logLength);
		glGetShaderInfoLog(shader, logLength, NULL, log);
		LogError("Failed to compile shader: '%s'\n%.*s", name, logLength - 1, log);
		free(log);
		return FALSE;
	}
	return TRUE;
}

static b32 ut_glLinkProgram(const char* name, GLuint program, GLuint vertShader, GLuint fragShader) {
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);
	glDetachShader(program, vertShader);
	glDetachShader(program, fragShader);
	GLint linkStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		GLint logLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		char* log = malloc(logLength);
		glGetProgramInfoLog(program, logLength, NULL, log);
		LogError("Failed to link program: '%s'\n%.*s", name, logLength - 1, log);
		free(log);
		return FALSE;
	}
	return TRUE;
}
