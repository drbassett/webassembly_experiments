#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef u32 b32;
#define FALSE 0
#define TRUE 1

#define StringifyHelper(X) #X
#define Stringify(X) StringifyHelper(X)

// use a lot of digits to support constant propagation
#define PI 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862

typedef void (*FExitError)();

static void defaultExitError() {
	exit(1);
}

FExitError exitError = defaultExitError;

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

inline static f32 degToRad(f32 deg) {
	return deg * ((f32) (PI / 180.0));
}

inline static f32 radToDeg(f32 rad) {
	return rad * ((f32) (180.0 / PI));
}

static void sincosf(f32 radians, f32* s, f32* c) {
	*s = sinf(radians);
	*c = cosf(radians);
}

typedef struct Vec3 {
	f32 x, y, z;
} Vec3;

typedef struct Vec4 {
	f32 x, y, z, w;
} Vec4;

typedef struct ColorRgba8 {
	u8 r, g, b, a;
} ColorRgba8;

typedef struct ColorRgbF32 {
	f32 r, g, b;
} ColorRgbF32;

typedef struct ColorRgbaF32 {
	f32 r, g, b, a;
} ColorRgbaF32;

inline static Vec3 vec3(f32 x, f32 y, f32 z) {
	Vec3 v = {x, y, z};
	return v;
}

inline static Vec4 vec4(f32 x, f32 y, f32 z, f32 w) {
	Vec4 v = {x, y, z, w};
	return v;
}

inline static f32 dotV4(Vec4 a, Vec4 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

typedef struct Mat4 {
	f32 elems[16];
} Mat4;

inline static u32 indexM4(u32 r, u32 c) {
	assert(r <= 3);
	assert(c <= 3);
	return r * 4 + c;
}

inline static f32 getM4(const Mat4* m, u32 r, u32 c) {
	return m->elems[indexM4(r, c)];
}

inline static void setM4(Mat4* m, u32 r, u32 c, f32 value) {
	m->elems[indexM4(r, c)] = value;
}

inline static Mat4 mat4(
		f32 m00, f32 m01, f32 m02, f32 m03,
		f32 m10, f32 m11, f32 m12, f32 m13,
		f32 m20, f32 m21, f32 m22, f32 m23,
		f32 m30, f32 m31, f32 m32, f32 m33) {
	Mat4 m = {{
		m00, m01, m02, m03,
		m10, m11, m12, m13,
		m20, m21, m22, m23,
		m30, m31, m32, m33,
	}};
	return m;
}

inline static Vec4 rowM4(const Mat4* m, u32 row) {
	Vec4 v = {
		getM4(m, row, 0),
		getM4(m, row, 1),
		getM4(m, row, 2),
		getM4(m, row, 3),
	};
	return v;
}

inline static Vec4 columnM4(const Mat4* m, u32 column) {
	Vec4 v = {
		getM4(m, 0, column),
		getM4(m, 1, column),
		getM4(m, 2, column),
		getM4(m, 3, column),
	};
	return v;
}

static Mat4 mulM4(Mat4 a, Mat4 b) {
	Vec4 r0 = rowM4(&a, 0);
	Vec4 r1 = rowM4(&a, 1);
	Vec4 r2 = rowM4(&a, 2);
	Vec4 r3 = rowM4(&a, 3);
	Vec4 c0 = columnM4(&b, 0);
	Vec4 c1 = columnM4(&b, 1);
	Vec4 c2 = columnM4(&b, 2);
	Vec4 c3 = columnM4(&b, 3);
	return mat4(
		dotV4(r0, c0),  dotV4(r0, c1),  dotV4(r0, c2),  dotV4(r0, c3),
		dotV4(r1, c0),  dotV4(r1, c1),  dotV4(r1, c2),  dotV4(r1, c3),
		dotV4(r2, c0),  dotV4(r2, c1),  dotV4(r2, c2),  dotV4(r2, c3),
		dotV4(r3, c0),  dotV4(r3, c1),  dotV4(r3, c2),  dotV4(r3, c3));
}

inline static Mat4 translateM4(Vec3 v) {
	return mat4(
		1.0f, 0.0f, 0.0f, v.x,
		0.0f, 1.0f, 0.0f, v.y,
		0.0f, 0.0f, 1.0f, v.z,
		0.0f, 0.0f, 0.0f, 1.0f);
}

inline static Mat4 rotationXAxisM4(f32 radians) {
	f32 s, c;
	sincosf(radians, &s, &c);
	return mat4(
		1.0f,  0.0f,  0.0f,  0.0f,
		0.0f,  c,     -s,    0.0f,
		0.0f,  s,     c,     0.0f,
		0.0f,  0.0f,  0.0f,  1.0f);
}

inline static Mat4 rotationYAxisM4(f32 radians) {
	f32 s, c;
	sincosf(radians, &s, &c);
	return mat4(
		c,     0.0f,  -s,    0.0f,
		0.0f,  1.0f,  0.0f,  0.0f,
		s,     0.0f,  c,     0.0f,
		0.0f,  0.0f,  0.0f,  1.0f);
}

inline static Mat4 rotationZAxisM4(f32 radians) {
	f32 s, c;
	sincosf(radians, &s, &c);
	return mat4(
		c,     -s,    0.0f,  0.0f,
		s,     c,     0.0f,  0.0f,
		0.0f,  0.0f,  1.0f,  0.0f,
		0.0f,  0.0f,  0.0f,  1.0f);
}

inline static Mat4 perspectiveM4(f32 fieldOfView, f32 aspectRatio, f32 near, f32 far) {
	f32 t = tanf(0.5f * fieldOfView);
	f32 m00 = 1.0f / (aspectRatio * t);
	f32 m11 = 1.0f / t;
	f32 m22 = -(far + near) / (far - near);
	f32 m23 = -2.0f * far * near / (far - near);
	return mat4(
		m00,  0.0f, 0.0f,  0.0f,
		0.0f, m11,  0.0f,  0.0f,
		0.0f, 0.0f, m22,   m23,
		0.0f, 0.0f, -1.0f, 0.0f);
}
