// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <foundation/clock.h>
#include <foundation/rand.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/projection.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include <engine/assets.h>
#include <engine/font.h>
#include <engine/scene.h>
#include <engine/scene_systems.h>
#include <engine/scene_forward_pipeline.h>
#include <engine/scene_bullet3_physics.h>
#include <engine/create_geometry.h>
#include <engine/forward_pipeline.h>

#include <list>

int main(int narg, const char **args) {
	// Create window
	const int width = 1920, height = 1090;

	hg::InputInit();
	hg::WindowSystemInit();

	hg::Window *window = hg::NewWindow(width, height);
	if (!hg::RenderInit(window)) {
		return EXIT_FAILURE;
	}
	bgfx::reset(width, height, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X8);

	// Create vertex layout for cubes and spheres (position and normal).
	bgfx::VertexLayout vs_decl;
	vs_decl.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Uint8, true, true).end();

	// Create sphere and cube geometries.
	hg::PipelineResources resources;

	hg::ModelRef sphere_ref = resources.models.Add("sphere", hg::CreateSphereModel(vs_decl, 0.5f, 12, 24));
	hg::ModelRef cube_ref = resources.models.Add("cube", hg::CreateCubeModel(vs_decl, 1.f, 1.f, 1.f));

	// Load default pipeline shader and create and simple material.
	hg::AddAssetsFolder("resources_compiled");

	hg::PipelineProgramRef prg = hg::LoadPipelineProgramRefFromAssets("core/shader/default.hps", resources, hg::GetForwardPipelineInfo());

	auto create_material = [prg](const hg::Vec4& diffuse, const hg::Vec4& specular, const hg::Vec4& self) -> hg::Material {
		hg::Material mat = hg::CreateMaterial(prg, "uDiffuseColor", diffuse, "uSpecularColor", specular);
		hg::SetMaterialValue(mat, "uSelfColor", self);
		return mat;
	};

	hg::Material ground_mat = create_material(hg::Vec4(0.5f, 0.5f, 0.5f), hg::Vec4(0.1f, 0.1f, 0.1f), hg::Vec4::Zero);
	hg::Material walls_mat = create_material(hg::Vec4(0.5f, 0.5f, 0.5f), hg::Vec4(0.1f, 0.1f, 0.1f), hg::Vec4::Zero);
	hg::Material objects_mat = create_material(hg::Vec4(0.5f, 0.5f, 0.5f), hg::Vec4::One, hg::Vec4::Zero);

	// Add a camera and a point light to the scene.
	hg::Scene scene;
	scene.canvas.color = hg::ColorI(22, 56, 76);
	scene.environment.fog_color = scene.canvas.color;
	scene.environment.fog_near = 20;
	scene.environment.fog_far = 80;

	// Light, camera
	hg::Mat4 camera_mtx = hg::TransformationMat4(hg::Vec3(0, 20.f, -30.f), hg::Deg3(30.f, 0.f, 0.f));
	hg::Node camera = hg::CreateCamera(scene, camera_mtx, 0.01f, 5000.f);

	hg::Node light = hg::CreateLinearLight(scene, hg::TransformationMat4(hg::Vec3::Zero, hg::Deg3(30, 59, 0)), hg::Color(1, 0.8f, 0.7f), hg::Color(1, 0.8f, 0.7f), 10, hg::LST_Map, 0.002f, hg::Vec4(50, 100, 200, 400));
	hg::Node back_light = hg::CreatePointLight(scene, hg::TranslationMat4(hg::Vec3(0.f, 10.f, 10.f)), 100.f, hg::ColorI(94, 155, 228), hg::ColorI(94, 255, 228));
	
	scene.SetCurrentCamera(camera);

	// Create a board.
	// -- bottom
	hg::ModelRef mdl_ref = resources.models.Add("ground", hg::CreateCubeModel(vs_decl, 100, 1, 100));
	hg::CreatePhysicCube(scene, hg::Vec3(30.f, 1.f, 30.f), hg::TranslationMat4(hg::Vec3(0.f, -.5f, 0.f)), mdl_ref, { ground_mat }, 0.f);
	
	mdl_ref = resources.models.Add("wall_lr", hg::CreateCubeModel(vs_decl, 1, 11, 32));
	// -- left wall
	hg::CreatePhysicCube(scene, hg::Vec3(1.f, 11.f, 32.f), hg::TranslationMat4(hg::Vec3(-15.5f, -.5f, 0.f)), mdl_ref, { walls_mat }, 0.f);
	// -- right wall
	hg::CreatePhysicCube(scene, hg::Vec3(1.f, 11.f, 32.f), hg::TranslationMat4(hg::Vec3(15.5f, -.5f, 0.f)), mdl_ref, { walls_mat }, 0.f);
	
	mdl_ref = resources.models.Add("wall_tb", hg::CreateCubeModel(vs_decl, 32, 11, 1));
	// -- bottom wall
	hg::CreatePhysicCube(scene, hg::Vec3(32.f, 11.f, 1.f), hg::TranslationMat4(hg::Vec3(0.f, -.5f, -15.5f)), mdl_ref, { walls_mat }, 0.f);
	// -- top wall
	hg::CreatePhysicCube(scene, hg::Vec3(32.f, 11.f, 1.f), hg::TranslationMat4(hg::Vec3(0.f, -.5f, 15.5f)), mdl_ref, { walls_mat }, 0.f);

	// Create physic system.
	hg::SceneClocks clocks;
	hg::SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromFile(scene);

	// Load and create all the resources needed to display text.
	hg::Font font = hg::LoadFontFromAssets("font/default.ttf", 32);
	bgfx::ProgramHandle font_program = hg::LoadProgramFromAssets("core/shader/font.vsb", "core/shader/font.fsb");

	hg::RenderState text_render_state = hg::ComputeRenderState(hg::BM_Alpha, hg::DT_Always, hg::FC_Disabled);
	std::vector<hg::UniformSetValue> text_uniform_values = { hg::MakeUniformSetValue("u_color", hg::Vec4(1.f, 1.f, .5f, 1.f)) };

	// Create scene rendering pipeline.
	hg::ForwardPipeline pipeline = hg::CreateForwardPipeline();
	hg::iRect viewport = hg::MakeRectFromWidthHeight(0, 0, width, height);

	// This list will hold the node reference of the objects we will create.
	std::list<hg::NodeRef> node_refs;

	hg::reset_clock();
	while(1) {
		// Fetch keyboard key states.
		hg::KeyboardState state = hg::ReadKeyboard();
		if (state.key[hg::K_Escape]) {
			break;
		}

		if (state.key[hg::K_S]) {
			// Add 8 new objects onto the scene.
			for (int i = 0; i < 8; ++i) {
				hg::SetMaterialValue(objects_mat, "uDiffuseColor", hg::Vec4(hg::FRand(), hg::FRand(), hg::FRand(), 1.f));

				hg::Node node;
				hg::Mat4 mtx = hg::TranslationMat4(hg::RandomVec3(hg::Vec3(-10.f, 18.f, -10.f), hg::Vec3(10.f, 18.f, 10.f)));
				if (hg::Rand() % 2) {
					node = CreatePhysicCube(scene, hg::Vec3::One, mtx, cube_ref, { objects_mat });
				} else {
					node = CreatePhysicSphere(scene, 0.5f, mtx, sphere_ref, { objects_mat });
				}
				physics.NodeCreatePhysicsFromFile(node);

				// scene.StartTrackingNodeDynamicCollisionEvents(node.ref);
				node_refs.push_back(node.ref);
			}
			hg::log(hg::format("%1 nodes").arg(scene.GetNodes().size()));

		} else if (state.key[hg::K_D]) {
			// Delete the first 8 objects added to the scene.
			for (int i = 0; i < 8; ++i)
				if (!node_refs.empty()) {
					const auto ref = node_refs.front();
					node_refs.pop_front();
					scene.DestroyNode(ref);
				}

			size_t destroyed_components = scene.GarbageCollect();
			size_t destroyed_physics = physics.GarbageCollect(scene);
			hg::log(hg::format("Destroyed components: %1, destroyed physics: %2").arg(destroyed_components).arg(destroyed_physics));
		} 

		// Update physics.
		hg::SceneUpdateSystems(scene, clocks, hg::tick_clock(), physics, hg::time_from_ms(16), 3);

		// Display scene.
		bgfx::ViewId view_id = 0;
		hg::SceneForwardPipelinePassViewId views;
		hg::SubmitSceneToPipeline(view_id, scene, viewport, 1.f, pipeline, resources, views);
		
		// Prints key usage and the number of active objects in a text overlay.
		hg::SetView2D(view_id, 0, 0, width, height, -1, 1, BGFX_CLEAR_DEPTH, hg::Color::Black, 1, 0);
		hg::DrawText(view_id, font, "S: Add object - D : Destruct object", font_program, "u_tex", 0, hg::Mat4::Identity, hg::Vec3(460, height - 60, 0), hg::DTHA_Left, hg::DTVA_Bottom, text_uniform_values, {}, text_render_state);
		hg::DrawText(view_id, font, hg::format("%1 Object").arg(node_refs.size()), font_program, "u_tex", 0, hg::Mat4::Identity, hg::Vec3(width - 200, height - 60, 0), hg::DTHA_Left, hg::DTVA_Bottom, text_uniform_values, {}, text_render_state);

		bgfx::frame();

		hg::UpdateWindow(window);
	}

	hg::RenderShutdown();
	hg::InputShutdown();
	hg::DestroyWindow(window);
	return EXIT_SUCCESS;
}
