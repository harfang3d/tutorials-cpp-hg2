// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <cmath>

#include <foundation/log.h>
#include <foundation/clock.h>
#include <foundation/math.h>
#include <foundation/projection.h>
#include <foundation/matrix3.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include <engine/render_pipeline.h>
#include <engine/scene.h>
#include <engine/scene_forward_pipeline.h>
#include <engine/forward_pipeline.h>
#include <engine/create_geometry.h>
#include <engine/assets.h>

int main() {
	// Initialize input and window system.
	hg::InputInit();
	hg::WindowSystemInit();

	int res_x = 1280, res_y = 720;

	// create window.
	hg::Window* window = hg::RenderInit("Harfang - Draw Scene to Texture", res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4);
	if (!window) {
		hg::error("failed to create window.");
		return EXIT_FAILURE;
	}

	// access compiled resources
	hg::AddAssetsFolder("resources_compiled");

	// create forward pipeline and resources.
	hg::ForwardPipeline pipeline = hg::CreateForwardPipeline();
	hg::PipelineResources res = hg::PipelineResources();

	// load host scene
	hg::Scene scene;
	hg::LoadSceneContext load_ctx;

	hg::LoadSceneFromAssets("materials/materials.scn", scene, res, hg::GetForwardPipelineInfo(), load_ctx);
	
	// create a 512x512 frame buffer to draw the scene to
	hg::FrameBuffer frame_buffer = hg::CreateFrameBuffer(512, 512, bgfx::TextureFormat::RGBA32F, bgfx::TextureFormat::D24, 4, "framebuffer");
	hg::Texture color = hg::GetColorTexture(frame_buffer);

	// create the cube model
	bgfx::VertexLayout vtx_layout = hg::VertexLayoutPosFloatTexCoord0UInt8();

	hg::Model cube_mdl = hg::CreateCubeModel(vtx_layout, 1, 1, 1);
	hg::ModelRef cube_ref = res.models.Add("cube", cube_mdl);

	// prepare the cube shader program
	bgfx::ProgramHandle cube_prg = hg::LoadProgramFromAssets("shaders/texture");

	// main loop
	float angle = 0.f;

	hg::Keyboard keyboard;

	// game loop
	while (!keyboard.Down(hg::K_Escape) && hg::IsWindowOpen(window)) {
		hg::time_ns dt = hg::tick_clock();  // tick clock, retrieve elapsed clock since last call

		keyboard.Update();

		angle = angle + hg::time_to_sec_f(dt);

		// update scene and render to the frame buffer
		scene.GetCurrentCamera().GetTransform().SetPos(hg::Vec3(0.f, 0.f, -(hg::Sin(angle) * 3.f + 4.f))); // animate the scene current camera on Z
		scene.Update(dt);

		bgfx::ViewId view_id = 0;
		hg::SceneForwardPipelinePassViewId views;
		hg::SubmitSceneToPipeline(view_id, scene, hg::iRect(0, 0, 512, 512), true, pipeline, res, views, frame_buffer.handle);

		// draw a rotating cube in immediate mode using the texture the scene was rendered to
		hg::SetViewPerspective(view_id, 0, 0, res_x, res_y, hg::TranslationMat4(hg::Vec3(0.f, 0.f, -1.8f)));
		
		std::vector<hg::UniformSetValue> val_uniforms = { hg::MakeUniformSetValue("color", hg::Vec4::One) };  // note: these could be moved out of the main loop but are kept here for readability
		std::vector<hg::UniformSetTexture> tex_uniforms = { hg::MakeUniformSetTexture("s_tex", color, 0) };
		
		hg::Mat4 trs = hg::TransformationMat4(hg::Vec3::Zero, hg::Vec3(angle * 0.1f, angle * 0.05f, angle * 0.2f));
		hg::DrawModel(view_id, cube_mdl, cube_prg, val_uniforms, tex_uniforms, &trs, 1);

		// end of frame
		bgfx::frame();
		hg::UpdateWindow(window);
	}

	hg::RenderShutdown();
	hg::DestroyWindow(window);

	hg::WindowSystemShutdown();
	hg::InputShutdown();

	return EXIT_SUCCESS;
}