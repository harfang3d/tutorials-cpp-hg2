// HARFANG(R) Copyright (C) 2022 NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

// Create textured material with pipeline shader
#include <foundation/clock.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include <engine/assets.h>
#include <engine/render_pipeline.h>
#include <engine/scene.h>
#include <engine/scene_forward_pipeline.h>
#include <engine/forward_pipeline.h>
#include <engine/create_geometry.h>

int main() {
	// create window.
	hg::InputInit();
	hg::WindowSystemInit();

	int res_x = 1280, res_y = 720;

	hg::Window* win = hg::RenderInit("Modify material pipeline shader uniforms", res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4);

	// access compiled resources
	hg::AddAssetsFolder("resources_compiled");

	// create forward pipeline and resources.
	hg::ForwardPipeline pipeline = hg::CreateForwardPipeline();
	hg::PipelineResources res = hg::PipelineResources();

	// create models
	bgfx::VertexLayout vtx_layout = hg::VertexLayoutPosFloatNormUInt8();

	hg::Model cube_mdl = hg::CreateCubeModel(vtx_layout, 1, 1, 1);
	hg::ModelRef cube_ref = res.models.Add("cube", cube_mdl);
	
	hg::Model ground_mdl = hg::CreateCubeModel(vtx_layout, 100, 0.01, 100);
	hg::ModelRef ground_ref = res.models.Add("ground", ground_mdl);

	// create materials
	hg::PipelineProgramRef shader = hg::LoadPipelineProgramRefFromAssets("core/shader/default.hps", res, hg::GetForwardPipelineInfo());

	hg::Material mat_cube = hg::CreateMaterial(shader, "uDiffuseColor", hg::Vec4::One, "uSpecularColor", hg::Vec4::One);
	hg::Material mat_ground = hg::CreateMaterial(shader, "uDiffuseColor", hg::Vec4::One, "uSpecularColor", hg::Vec4(0.1f, 0.1f, 0.1f));

	// setup scene
	hg::Scene scene;

	hg::Node cam = hg::CreateCamera(scene, hg::Mat4LookAt(hg::Vec3(-1.30f, 0.27f, -2.47f), hg::Vec3(0.f, 0.5f, 0.f)), 0.01f, 1000.f);
	scene.SetCurrentCamera(cam);

	hg::CreateLinearLight(scene, hg::TransformationMat4(hg::Vec3(0.f, 2.f, 0.f), hg::Deg3(27.5f, -97.6f, 16.6f)), hg::ColorI(64, 64, 64), hg::ColorI(64, 64, 64), 10);
	hg::CreateSpotLight(scene, hg::TransformationMat4(hg::Vec3(5.f, 4.f, -5.f), hg::Deg3(19.f, -45.f, 0.f)), 0, hg::Deg(5.f), hg::Deg(30.f), hg::ColorI(255, 255, 255), hg::ColorI(255, 255, 255), 10, hg::LST_Map, 0.0001f);
	
	hg::Node cube_node = hg::CreateObject(scene, hg::TranslationMat4(hg::Vec3(0.f, 0.5f, 0.f)), cube_ref, { mat_cube });
	hg::CreateObject(scene, hg::TranslationMat4(hg::Vec3::Zero), ground_ref, { mat_ground });

	// material update states
	bool mat_has_texture = false;
	int64_t mat_update_delay = 0;

	hg::TextureRef texture_ref = hg::LoadTextureFromAssets("textures/squares.png", 0, res);
	
	// main loop
	hg::Keyboard keyboard;
	while (!keyboard.Pressed(hg::K_Escape) && hg::IsWindowOpen(win)) {
		hg::time_ns dt = hg::tick_clock();  // tick clock, retrieve elapsed clock since last call

		// update keyboard devices
		keyboard.Update();

		mat_update_delay -= dt;

		if (mat_update_delay <= 0) {
			// set or remove cube node material texture
			hg::Material& mat = cube_node.GetObject().GetMaterial(0);

			if (mat_has_texture) {
				hg::SetMaterialTexture(mat, "uDiffuseMap", hg::InvalidTextureRef, 0);
			}
			else {
				hg::SetMaterialTexture(mat, "uDiffuseMap", texture_ref, 0);
			}

			// update the pipeline shader variant according to the material uniform values
			hg::UpdateMaterialPipelineProgramVariant(mat, res);

			// reset delayand flip flag
			mat_update_delay = mat_update_delay + hg::time_from_sec(1);
			mat_has_texture = !mat_has_texture;
		}
		
		scene.Update(dt);

		bgfx::ViewId view_id = 0;
		hg::SceneForwardPipelinePassViewId views;
		hg::SubmitSceneToPipeline(view_id, scene, hg::iRect(0, 0, res_x, res_y), true, pipeline, res, views);

		bgfx::frame();
		hg::UpdateWindow(win);
	}

	hg::RenderShutdown();
	hg::DestroyWindow(win);

	return EXIT_SUCCESS;
}