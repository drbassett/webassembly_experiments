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

typedef u32 b32;
#define FALSE 0
#define TRUE 1

#define LogError(Message, ...) \
	fprintf(stderr, Message, ##__VA_ARGS__);

#define FatalError(Message, ...) \
	LogError(Message, ##__VA_ARGS__); \
	exitError();

inline static void exitError() {
	exit(1);
}

static b32 compileShader(const char* name, GLuint shader, const char* source) {
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

static b32 linkProgram(const char* name, GLuint program, GLuint vertShader, GLuint fragShader) {
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

static const char* emscriptenResultToString(EMSCRIPTEN_RESULT result) {
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

#define EmscriptenCheckResult(FunctionCall) \
	emscriptenCheckResultImpl((FunctionCall), #FunctionCall)

inline static EMSCRIPTEN_RESULT emscriptenCheckResultImpl(EMSCRIPTEN_RESULT result, const char* functionCall) {
	if (result < 0) {
		FatalError(
			"Function call failed with error %s (%d): %s\n",
			emscriptenResultToString(result), result, functionCall);
	}
	return result;
}

GLuint program;

// the ID of the canvas element on the HTML page
const char* canvasId = "canvas";

static EM_BOOL canvasResizedCallback(int eventType, const void* reserved, void* userData) {
	double width, height;
	EmscriptenCheckResult(emscripten_get_element_css_size(canvasId, &width, &height));
	i32 canvasWidth = (i32) width;
	i32 canvasHeight = (i32) height;
	glViewport(0, 0, canvasWidth, canvasHeight);
	return EM_TRUE;
}

static void mainLoop(void* arg) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main() {
	EmscriptenWebGLContextAttributes contextAttribs = {
		.alpha = EM_TRUE,
		.depth = EM_TRUE,
		.stencil = EM_FALSE,
		.antialias = EM_TRUE,
		.premultipliedAlpha = EM_TRUE,
		.preserveDrawingBuffer = EM_FALSE,
		.preferLowPowerToHighPerformance = EM_FALSE,
		.failIfMajorPerformanceCaveat = EM_FALSE,
		.majorVersion = 2,
		.minorVersion = 0,
		.enableExtensionsByDefault = EM_FALSE,
		.explicitSwapControl = EM_FALSE,
	};
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context(canvasId, &contextAttribs);
	if (context < 0) {
		EMSCRIPTEN_RESULT result = (EMSCRIPTEN_RESULT) context;
		FatalError("Failed to create WebGL context: %s (%d)\n", emscriptenResultToString(result), result);
	}
	emscripten_webgl_make_context_current(context);

	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	program = glCreateProgram();

	const char* vertShaderSource =
		"#version 300 es\n"
		"\n"
		"out mediump vec3 vertColor;\n"
		"\n"
		"void main() {\n"
		"    const vec2 positions[3] = vec2[3](\n"
		"        vec2(-0.5f, -0.5f),\n"
		"        vec2(0.5f, -0.5f),\n"
		"        vec2(0.0f, 0.5f));\n"
		"    const vec3 colors[3] = vec3[3](\n"
		"        vec3(1.0f, 0.0f, 0.0f),\n"
		"        vec3(0.0f, 1.0f, 0.0f),\n"
		"        vec3(0.0f, 0.0f, 1.0f));\n"
		"    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n"
		"    vertColor = colors[gl_VertexID];"
		"}\n";

	const char* fragShaderSource =
		"#version 300 es\n"
		"\n"
		"in mediump vec3 vertColor;\n"
		"\n"
		"out mediump vec4 fragColor;\n"
		"\n"
		"void main() {\n"
		"    fragColor = vec4(vertColor, 1.0f);\n"
		"}\n";

	b32 success =
		compileShader("triangle-vert", vertShader, vertShaderSource) &
		compileShader("triangle-frag", fragShader, fragShaderSource);
	if (!success) {
		exitError();
	}
	if (!linkProgram("triangle", program, vertShader, fragShader)) {
		exitError();
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	EmscriptenFullscreenStrategy fullscreenStrategy = {
		.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
		.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF,
		.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
		.canvasResizedCallback = canvasResizedCallback,
		.canvasResizedCallbackUserData = NULL,
	};
	emscripten_enter_soft_fullscreen(canvasId, &fullscreenStrategy);

	emscripten_set_main_loop_arg(mainLoop, NULL, 0, EM_TRUE);

	EmscriptenCheckResult(emscripten_webgl_destroy_context(context));
	return 0;
}
