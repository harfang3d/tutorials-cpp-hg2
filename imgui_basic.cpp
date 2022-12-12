// HARFANG(R) Copyright (C) 2022 NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <foundation/log.h>

#include <foundation/clock.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include <engine/render_pipeline.h>
#include <engine/assets.h>
#include <engine/dear_imgui.h>

int main() {
	// create window.
	hg::InputInit();
	hg::WindowSystemInit();

	int width = 1280, height = 720;

	hg::Window* window = hg::RenderInit("Harfang - ImGui Basics", width, height, BGFX_RESET_VSYNC);
	if (!window) {
		hg::error("failed to create window.");
		return EXIT_FAILURE;
	}

	// access compiled resources
	hg::AddAssetsFolder("resources_compiled");

	// initialize ImGui 
	bgfx::ProgramHandle imgui_prg = hg::LoadProgramFromAssets("core/shader/imgui");
	bgfx::ProgramHandle imgui_img_prg = hg::LoadProgramFromAssets("core/shader/imgui_image");

	hg::ImGuiInit(10.f, imgui_prg, imgui_img_prg);

	// main Loop
	hg::Keyboard keyboard;
	while (!keyboard.Pressed(hg::K_Escape) && hg::IsWindowOpen(window)) {
		hg::time_ns dt = hg::tick_clock();  // tick clock, retrieve elapsed clock since last call

		keyboard.Update();

		hg::ImGuiBeginFrame(width, height, dt, hg::ReadMouse(), keyboard.GetState());

		if (ImGui::Begin("Window")) {
			ImGui::Text("Hello World!");
		}
		ImGui::End();

		hg::SetView2D(0, 0, 0, width, height, -1, 1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, hg::Color::Black, 1, 0);
		hg::ImGuiEndFrame(0);
		
		bgfx::frame();
		hg::UpdateWindow(window);
	}

	hg::RenderShutdown();
	hg::DestroyWindow(window);

	return EXIT_SUCCESS;
}