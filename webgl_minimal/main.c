#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

#define FatalError(Message, ...) \
	fprintf(stderr, Message, ##__VA_ARGS__); \
	exit(1)

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

inline static void emscriptenCheckResultImpl(EMSCRIPTEN_RESULT result, const char* functionCall) {
	if (result < 0) {
		FatalError(
			"Function call failed with error %s (%d): %s\n",
			emscriptenResultToString(result), result, functionCall);
	}
}

int main() {
	// the ID of the canvas element on the HTML page
	const char* canvasId = "canvas";

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

	EmscriptenFullscreenStrategy fullscreenStrategy = {
		.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
		.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF,
		.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
		.canvasResizedCallback = NULL,
		.canvasResizedCallbackUserData = NULL,
	};
	emscripten_enter_soft_fullscreen(canvasId, &fullscreenStrategy);

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	EmscriptenCheckResult(emscripten_webgl_destroy_context(context));
	return 0;
}
