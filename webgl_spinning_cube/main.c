#include "util.h"

typedef struct Vertex {
	Vec3 position;
	ColorRgba8 color;
} Vertex;

GLuint program;
GLint unifMvp;
GLuint vertexBuffer, indexBuffer;
GLuint vao;

// the ID of the canvas element on the HTML page
const char* canvasId = "canvas";

i32 canvasWidth, canvasHeight;

static EM_BOOL canvasResizedCallback(int eventType, const void* reserved, void* userData) {
	double width, height;
	UtEmCheckResult(emscripten_get_element_css_size(canvasId, &width, &height));
	canvasWidth = (i32) width;
	canvasHeight = (i32) height;
	glViewport(0, 0, canvasWidth, canvasHeight);
	return EM_TRUE;
}

f64 lastTimeMillis;
f32 spinRadians;

static void mainLoop(void* arg) {
	f64 timeMillis = emscripten_get_now();
	f32 dtMillis = (f32) (timeMillis - lastTimeMillis);
	lastTimeMillis = timeMillis;

	f32 aspectRatio = (f32) canvasWidth / (f32) canvasHeight;

	f32 secondsPerRevolution = 5.0f;
	f32 millisecondsPerRevolution = secondsPerRevolution * 1000.0f;
	f32 revolutionsPerMillisecond = 1.0f / millisecondsPerRevolution;
	f32 radiansPerRevolution = (f32) (2.0 * PI);
	f32 radiansPerMillisecond = radiansPerRevolution * revolutionsPerMillisecond;
	f32 radiansIncrement = radiansPerMillisecond * dtMillis;
	spinRadians += radiansIncrement;

	Mat4 perspective = perspectiveM4(degToRad(90.0f), aspectRatio, 0.1f, 10.0f);
	Mat4 translate = translateM4(vec3(0.0f, 0.0f, -2.0f));
	Mat4 spin = rotationYAxisM4(spinRadians);
	Mat4 mvp = mulM4(perspective, mulM4(translate, spin));

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(program);
	glUniformMatrix4fv(unifMvp, 1, GL_TRUE, mvp.elems);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, NULL);
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
		"uniform mat4 mvp;\n"
		"\n"
		"layout(location = 0) in mediump vec3 vertexPosition;\n"
		"layout(location = 1) in mediump vec4 vertexColor;\n"
		"\n"
		"out mediump vec3 vertColor;\n"
		"\n"
		"void main() {\n"
		"    gl_Position = mvp * vec4(vertexPosition, 1.0f);\n"
		"    vertColor = vertexColor.rgb;\n"
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
	unifMvp = glGetUniformLocation(program, "mvp");
	assert(unifMvp != -1);

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	glGenBuffers(1, &vertexBuffer);
	glGenBuffers(1, &indexBuffer);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	GLuint positionIndex = 0, colorIndex = 1;
	glVertexAttribPointer(positionIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, position));
	glEnableVertexAttribArray(positionIndex);
	glVertexAttribPointer(colorIndex, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, color));
	glEnableVertexAttribArray(colorIndex);
	glBindVertexArray(0);

	const Vertex cubeVertices[8] = {
		{ .position = {-0.5f, -0.5f, -0.5f}, .color = {  0,   0,   0, 255} },
		{ .position = {-0.5f, -0.5f,  0.5f}, .color = {  0,   0, 255, 255} },
		{ .position = {-0.5f,  0.5f, -0.5f}, .color = {  0, 255,   0, 255} },
		{ .position = {-0.5f,  0.5f,  0.5f}, .color = {  0, 255, 255, 255} },
		{ .position = { 0.5f, -0.5f, -0.5f}, .color = {255,   0,   0, 255} },
		{ .position = { 0.5f, -0.5f,  0.5f}, .color = {255,   0, 255, 255} },
		{ .position = { 0.5f,  0.5f, -0.5f}, .color = {255, 255,   0, 255} },
		{ .position = { 0.5f,  0.5f,  0.5f}, .color = {255, 255, 255, 255} },
	};
	const u16 cubeIndices[36] = {
		0, 1, 3,
		6, 0, 2,
		5, 0, 4,
		6, 4, 0,
		0, 3, 2,
		5, 1, 0,
		3, 1, 5,
		7, 4, 6,
		4, 7, 5,
		7, 6, 2,
		7, 2, 3,
		7, 3, 5,
	};

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	EmscriptenFullscreenStrategy fullscreenStrategy = {
		.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
		.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF,
		.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
		.canvasResizedCallback = canvasResizedCallback,
		.canvasResizedCallbackUserData = NULL,
	};
	emscripten_enter_soft_fullscreen(canvasId, &fullscreenStrategy);

	spinRadians = 0.0f;
	lastTimeMillis = emscripten_get_now();
	emscripten_set_main_loop_arg(mainLoop, NULL, 0, EM_TRUE);

	UtEmCheckResult(emscripten_webgl_destroy_context(context));
	return 0;
}
