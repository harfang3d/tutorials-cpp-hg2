// HARFANG(R) Copyright (C) 2022 NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

// Toyota 2JZ - GTE Engine model by Serhii Denysenko(CGTrader: serhiidenysenko8256)
// URL : https://www.cgtrader.com/3d-models/vehicle/part/toyota-2jz-gte-engine-2932b715-2f42-4ecd-93ce-df9507c67ce8

#include <foundation/clock.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include <engine/assets.h>
#include <engine/render_pipeline.h>
#include <engine/scene.h>
#include <engine/scene_forward_pipeline.h>
#include <engine/forward_pipeline.h>

int main() {
	// create window.
	hg::InputInit();
	hg::WindowSystemInit();

	int res_x = 1280, res_y = 720;

	hg::Window* win = hg::RenderInit("AAA Depth Of Field Scene", res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4);

	// access compiled resources
	hg::AddAssetsFolder("resources_compiled");

	// create forward pipeline and resources.
	hg::ForwardPipeline pipeline = hg::CreateForwardPipeline();
	hg::PipelineResources res = hg::PipelineResources();

	// load scene
	hg::Scene scene;
	hg::LoadSceneContext load_ctx;
	hg::LoadSceneFromAssets("car_engine/engine.scn", scene, res, hg::GetForwardPipelineInfo(), load_ctx);
		
	// The DOF post process is only available in the AAA pipeline
	hg::ForwardPipelineAAAConfig pipeline_aaa_config;
	hg::ForwardPipelineAAA pipeline_aaa = hg::CreateForwardPipelineAAAFromAssets("core", pipeline_aaa_config, bgfx::BackbufferRatio::Equal, bgfx::BackbufferRatio::Equal);
	pipeline_aaa_config.sample_count = 1;
	pipeline_aaa_config.dof_focus_point = 3.5f; // Distance to the focus point (in meters)
	pipeline_aaa_config.dof_focus_length = 2.f; // Depth of field (in meters); smaller values result in a narrower focused area.

	// main loop
	int frame = 0;

	hg::Keyboard keyboard;
	while (!keyboard.Pressed(hg::K_Escape) && hg::IsWindowOpen(win)) {
		hg::time_ns dt = hg::tick_clock();  // tick clock, retrieve elapsed clock since last call

		// update keyboard devices
		keyboard.Update();
		
		hg::Transform trs = scene.GetNode("engine_master").GetTransform();
		trs.SetRot(trs.GetRot() + hg::Vec3(0, hg::Deg(15.f) * hg::time_to_sec_f(dt), 0));

		bgfx::ViewId view_id = 0;
		hg::SceneForwardPipelinePassViewId views;

		scene.Update(dt);
		hg::SubmitSceneToPipeline(view_id, scene, hg::iRect(0, 0, res_x, res_y), true, pipeline, res, views, pipeline_aaa, pipeline_aaa_config, frame);

		frame = bgfx::frame();
		hg::UpdateWindow(win);
	}

	hg::RenderShutdown();
	hg::DestroyWindow(win);

	return EXIT_SUCCESS;
}