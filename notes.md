# Notes

This document contains notes about how to work with WASM.

## Canvas Resizing

Resizing behavior for canvas elements is a mess. Each canvas has two sizes: a
display size, and a framebuffer size. If the framebuffer size does not match
the display size, the browser will stretch and squash the framebuffer pixels to
make them fit inside the canvas display size.

In Firefox, and likely in other browsers, resizing the window does not change
the framebuffer size, but it does change the display size. Additionally, in
emscripten, the resize callback is not called for a canvas element when the
display size changes. Thus, as the window is resized, the canvas may be resized
with it, but the canvas framebuffer size does not change, and we are left with
a stretched looking image.

The best we can do is register for a resize event for the overall window, and
check if the canvas needs resized then. If there are other actions that could
cause a canvas resize, and we are unable to detect them, then we are out of
luck. We could instead check for a resize in the main loop, prior to rendering
every frame; this is a robust solution, but it is inefficient.

One more important detail: a WASM application does not receive a resize
callback on startup. Thus, it is necessary to manually trigger any resize
behaviors at application startup.

The code below shows how to do get the canvas resize behavior to work using the
resize callback:

```c
static void resizeCanvas() {
	EM_ASM({
		var canvas = document.getElementById('canvas');
		var displayWidth = canvas.clientWidth;
		var displayHeight = canvas.clientHeight;
		if (canvas.width != displayWidth || canvas.height != displayHeight) {
			canvas.width = displayWidth;
			canvas.height = displayHeight;
		}
	});
	double width, height;
	const char* canvasId = "canvas"; // use the ID of the canvas element defined in the HTML
	emscripten_get_element_css_size(canvasId, &width, &height);
	i32 canvasWidth = (i32) width;
	i32 canvasHeight = (i32) height;
	glViewport(0, 0, canvasWidth, canvasHeight);
}

static EM_BOOL resizeCallback(int eventType, const EmscriptenUiEvent* uiEvent, void* userData) {
	resizeCanvas();
	return EM_TRUE;
}

int main() {
	// listen for resizes for the entire window
	emscripten_set_resize_callback(0, NULL, EM_FALSE, resizeCallback);
	// manually trigger a resize
	resizeCanvas();

	// ...

	return 0;
}
```

This next code sample shows the more robust solution of resizing directly in
the main loop:

```c
// this function is identical to the one above
static void resizeCanvas() {
	EM_ASM({
		var canvas = document.getElementById('canvas');
		var displayWidth = canvas.clientWidth;
		var displayHeight = canvas.clientHeight;
		if (canvas.width != displayWidth || canvas.height != displayHeight) {
			canvas.width = displayWidth;
			canvas.height = displayHeight;
		}
	});
	double width, height;
	const char* canvasId = "canvas"; // use the ID of the canvas element defined in the HTML
	emscripten_get_element_css_size(canvasId, &width, &height);
	i32 canvasWidth = (i32) width;
	i32 canvasHeight = (i32) height;
	glViewport(0, 0, canvasWidth, canvasHeight);
}

static void mainLoop(void* arg) {
	resizeCanvas();
	
	// ...
}

int main() {
	emscripten_set_main_loop_arg(mainLoop, NULL, 0, EM_TRUE);

	// ...

	return 0;
}
```

### Soft Fullscreen

Shortly after working through the resizing problems detailed above, I
discovered `emscripten_enter_soft_fullscreen`. This seems to be a clean
solution to the canvas resizing problem. Under the hood, this function likely
has some of the same problems I encountered, but perhaps not.
