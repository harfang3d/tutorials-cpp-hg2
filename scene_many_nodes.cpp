// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <foundation/log.h>
#include <foundation/clock.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

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

	hg::Window* window = hg::RenderInit("Many dynamic objects", res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4);
	if (!window) {
		hg::error("failed to create window.");
		return EXIT_FAILURE;
	}

	// create forward pipeline and resources.
	hg::ForwardPipeline pipeline = hg::CreateForwardPipeline(4096); // increase shadow map resolution to 4096x4096.
	hg::PipelineResources resources = hg::PipelineResources();

	// create models.
	bgfx::VertexLayout vtx_layout = hg::VertexLayoutPosFloatNormUInt8();

	hg::ModelRef sphere_ref = resources.models.Add("sphere", hg::CreateSphereModel(vtx_layout, 0.1f, 8, 16));
	hg::ModelRef ground_ref = resources.models.Add("ground", hg::CreateCubeModel(vtx_layout, 60.f, 0.001f, 60.f));

	// create materials.
	hg::PipelineProgramRef prg = hg::LoadPipelineProgramRefFromFile("resources_compiled/core/shader/default.hps", resources, hg::GetForwardPipelineInfo());

	hg::Material sphere_mat = hg::CreateMaterial(prg, "uDiffuseColor", hg::Vec4(1, 0, 0), "uSpecularColor", hg::Vec4(1, 0.8f, 0));
	hg::Material ground_mat = hg::CreateMaterial(prg, "uDiffuseColor", hg::Vec4(1, 1, 1), "uSpecularColor", hg::Vec4(1, 1, 1));

	// setup scene, camera and light.
	hg::Scene scene;
	scene.canvas.color = hg::Color(0.1f, 0.1f, 0.1f);
	scene.environment.ambient = hg::Color(0.1f, 0.1f, 0.1f);

	hg::Node camera = hg::CreateCamera(scene, hg::TransformationMat4(hg::Vec3(15.5f, 5, -6), hg::Vec3(0.4f, -1.2f, 0)), 0.01f, 100);
	scene.SetCurrentCamera(camera);

	hg::CreateSpotLight(scene, hg::TransformationMat4(hg::Vec3(-8.8f, 21.7f, -8.8f), hg::Deg3(60, 45, 0)), 0, hg::Deg(5.f), hg::Deg(30.f), hg::Color::White, hg::Color::White, 0, hg::LST_Map, 0.000005f);
	hg::CreateObject(scene, hg::TranslationMat4(hg::Vec3(0, 0, 0)), ground_ref, {ground_mat});

	// create scene objects made of 100 by 100 spheres.
	std::vector<hg::Transform> rows;
	int count = 100;
	rows.reserve(count * count);
	for (int j = 0; j < count; j++) {
		for (int i = 0; i < count; i++) {
			hg::Vec3 position = hg::Vec3(((2.f*i)/count - 1.f) * 10.f, 0.1f, ((2.f*j)/count - 1.f) * 10.f);
			hg::Node node = hg::CreateObject(scene, hg::TranslationMat4(position), sphere_ref, { sphere_mat });		
			rows.push_back(node.GetTransform()); // store the node transform directly.
		}
	}

	// main loop.
	float angle = 0.f;
	hg::iRect viewport = hg::MakeRectFromWidthHeight(0, 0, res_x, res_y);
		
	hg::SceneForwardPipelinePassViewId views;

	hg::Keyboard keyboard;
	while (!keyboard.Pressed(hg::K_Escape)) {
		keyboard.Update();

		hg::time_ns dt = hg::tick_clock();

		// move the spheres vertically in a wave pattern.
		angle += hg::time_to_sec_f(dt);

		for (int j = 0; j < count; j++) {
			float row_y = cos(angle + j * 0.1f);
			for (int i = 0; i < count; i++) {
				hg::Transform& trs = rows[i + (j * count)];
				hg::Vec3 pos = trs.GetPos();
				pos.y = 0.1f * (row_y * sin(angle + i * 0.1f) * 6.f + 6.5f);
				trs.SetPos(pos);
			}
		}

		// update scene and send it to the forward rendering pipeline.
		scene.Update(dt);

		bgfx::ViewId view_id = 0;
		hg::SubmitSceneToPipeline(view_id, scene, viewport, 1.f, pipeline, resources, views);

		bgfx::frame();
		hg::UpdateWindow(window);
	}

	hg::RenderShutdown();
	hg::DestroyWindow(window);

	return EXIT_SUCCESS;
}