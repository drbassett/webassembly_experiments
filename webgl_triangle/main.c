#include "util.h"

GLuint program;

// the ID of the canvas element on the HTML page
const char* canvasId = "canvas";

static EM_BOOL canvasResizedCallback(int eventType, const void* reserved, void* userData) {
	double width, height;
	UtEmCheckResult(emscripten_get_element_css_size(canvasId, &width, &height));
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
		FatalError("Failed to create WebGL context: %s (%d)\n", ut_emResultToString(result), result);
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
		ut_glCompileShader("triangle-vert", vertShader, vertShaderSource) &
		ut_glCompileShader("triangle-frag", fragShader, fragShaderSource);
	if (!success) {
		exitError();
	}
	if (!ut_glLinkProgram("triangle", program, vertShader, fragShader)) {
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

	UtEmCheckResult(emscripten_webgl_destroy_context(context));
	return 0;
}
