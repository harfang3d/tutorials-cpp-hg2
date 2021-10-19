// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <foundation/log.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include <engine/render_pipeline.h>

int main() {
	// create window.
	hg::InputInit();
	hg::WindowSystemInit();

	int width = 1280, height = 720;

	hg::Window *window = hg::RenderInit("Harfang - Basic Loop", width, height, BGFX_RESET_VSYNC);
	if (!window) {
		hg::error("failed to create window.");
		return EXIT_FAILURE;
	}

	hg::Keyboard keyboard;
	while (!keyboard.Pressed(hg::K_Escape)) {
		keyboard.Update();

		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, hg::ColorToABGR32(hg::Color::Green), 1, 0);
		bgfx::setViewRect(0, 0, 0, width, height);

		bgfx::touch(0);  // force the view to be processed as it would be ignored since nothing is drawn to it(a clear does not count)

		bgfx::frame();
		hg::UpdateWindow(window);
	}

	hg::RenderShutdown();
	hg::DestroyWindow(window);

	return EXIT_SUCCESS;
}