#include "util.h"

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
		FatalError("Failed to create WebGL context: %s (%d)\n", ut_emResultToString(result), result);
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
	UtEmCheckResult(emscripten_webgl_destroy_context(context));
	return 0;
}
