// HARFANG(R) Copyright (C) 2022 NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

// Display a scene in VR

#include <foundation/clock.h>
#include <foundation/projection.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include <engine/assets.h>
#include <engine/render_pipeline.h>
#include <engine/scene.h>
#include <engine/scene_forward_pipeline.h>
#include <engine/forward_pipeline.h>
#include <engine/create_geometry.h>
#include <engine/openvr_api.h>

static hg::Material create_material(hg::PipelineProgramRef prg_ref, const hg::Vec4 &ubc, const hg::Vec4& orm) {
	hg::Material mat;
	hg::SetMaterialProgram(mat, prg_ref);
	hg::SetMaterialValue(mat, "uBaseOpacityColor", ubc);
	hg::SetMaterialValue(mat, "uOcclusionRoughnessMetalnessColor", orm);
	return mat;
}

int main() {
	// create window.
	hg::InputInit();
	hg::WindowSystemInit();

	int res_x = 1280, res_y = 720;

	hg::Window* win = hg::RenderInit("Harfang - OpenVR Scene", res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4);

	// access compiled resources
	hg::AddAssetsFolder("resources_compiled");

	// create forward pipeline and resources.
	hg::ForwardPipeline pipeline = hg::CreateForwardPipeline();
	hg::PipelineResources res = hg::PipelineResources();

	hg::SceneForwardPipelineRenderData render_data; // this object is used by the low - level scene rendering API to share view - independent data with both eyes

	// OpenVR initialization
	if (!hg::OpenVRInit()) {
		exit(EXIT_FAILURE);
	}

	hg::OpenVREyeFrameBuffer vr_left_fb = hg::OpenVRCreateEyeFrameBuffer(hg::OVRAA_MSAA4x);
	hg::OpenVREyeFrameBuffer vr_right_fb = hg::OpenVRCreateEyeFrameBuffer(hg::OVRAA_MSAA4x);

	// create models
	bgfx::VertexLayout vtx_layout = hg::VertexLayoutPosFloatNormUInt8();

	hg::Model cube_mdl = hg::CreateCubeModel(vtx_layout, 1, 1, 1);
	hg::ModelRef cube_ref = res.models.Add("cube", cube_mdl);
	
	hg::Model ground_mdl = hg::CreateCubeModel(vtx_layout, 50, 0.01f, 50);
	hg::ModelRef ground_ref = res.models.Add("ground", ground_mdl);

	// load shader
	hg::PipelineProgramRef prg_ref = hg::LoadPipelineProgramRefFromAssets("core/shader/pbr.hps", res, hg::GetForwardPipelineInfo());

	// create scene
	hg::Scene scene;
	scene.canvas.color = hg::ColorI(255, 255, 217, 255);
	scene.environment.ambient = hg::ColorI(15, 12, 9, 255);

	hg::Node lgt = hg::CreateSpotLight(scene, hg::TransformationMat4(hg::Vec3(-8.f, 4.f, -5.f), hg::Deg3(19.f, 59.f, 0.f)), 0, hg::Deg(5.f), hg::Deg(30.f), hg::Color::White, hg::Color::White, 10, hg::LST_Map, 0.0001f);
	hg::Node back_lgt = hg::CreatePointLight(scene, hg::TranslationMat4(hg::Vec3(2.4f, 1.f, 0.5f)), 10, hg::ColorI(94, 255, 228, 255), hg::ColorI(94, 255, 228, 255), 0);

	hg::Material mat_cube = create_material(prg_ref, hg::Vec4(hg::ColorI(255, 230, 20)), hg::Vec4(1.f, 0.658f, 0.f, 1.f));
	hg::CreateObject(scene, hg::TransformationMat4(hg::Vec3(0.f, 0.5f, 0.f), hg::Deg3(0.f, 70.f, 0.f)), cube_ref, { mat_cube });

	hg::Material mat_ground = create_material(prg_ref, hg::Vec4(hg::ColorI(255, 120, 147)), hg::Vec4(1.f, 1.f, 0.1f, 1.f));
	hg::CreateObject(scene, hg::TranslationMat4(hg::Vec3::Zero), ground_ref, { mat_ground });

	// setup 2D rendering to display eyes textures
	bgfx::VertexLayout quad_layout;
	quad_layout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
		.end();

	hg::Model quad_model = hg::CreatePlaneModel(quad_layout, 1, 1, 1, 1);
	hg::RenderState quad_render_state = hg::ComputeRenderState(hg::BM_Alpha, hg::DT_Disabled, hg::FC_Disabled);

	float eye_t_size = res_x / 2.5f;
	float eye_t_x = (res_x - 2 * eye_t_size) / 6 + eye_t_size / 2;
	hg::Mat4 quad_matrix = hg::TransformationMat4(hg::Vec3::Zero, hg::Deg3(90.f, 0.f, 0.f), hg::Vec3(eye_t_size, 1.f, eye_t_size));

	bgfx::ProgramHandle tex0_program = hg::LoadProgramFromAssets("shaders/sprite");

	std::vector<hg::UniformSetValue> quad_uniform_set_value_list;
	quad_uniform_set_value_list.push_back(hg::MakeUniformSetValue("color", hg::Vec4::One));

	std::vector<hg::UniformSetTexture> quad_uniform_set_texture_list;


	// main loop
	hg::Keyboard keyboard;
	while (!keyboard.Pressed(hg::K_Escape) && hg::IsWindowOpen(win)) {
		hg::time_ns dt = hg::tick_clock();  // tick clock, retrieve elapsed clock since last call

		// update keyboard devices
		keyboard.Update();

		scene.Update(dt);

		hg::Mat4 actor_body_mtx = hg::TransformationMat4(hg::Vec3(-1.3f, .45f, -2.f), hg::Vec3::Zero);

		hg::ViewState left, right;
		hg::OpenVRState vr_state = hg::OpenVRGetState(actor_body_mtx, 0.01f, 1000.f);
		hg::OpenVRStateToViewState(vr_state, left, right);

		bgfx::ViewId vid = 0;  // keep track of the next free view id
		hg::SceneForwardPipelinePassViewId passId;

		// prepare view - independent render data once
		hg::PrepareSceneForwardPipelineCommonRenderData(vid, scene, render_data, pipeline, res, passId);
		hg::iRect vr_eye_rect(0, 0, vr_state.width, vr_state.height);

		// prepare the left eye render data then draw to its framebuffer
		hg::PrepareSceneForwardPipelineViewDependentRenderData(vid, left, scene, render_data, pipeline, res, passId);
		hg::SubmitSceneToForwardPipeline(vid, scene, vr_eye_rect, left, pipeline, render_data, res, passId, vr_left_fb.fb);

		// repare the right eye render data then draw to its framebuffer
		hg::PrepareSceneForwardPipelineViewDependentRenderData(vid, right, scene, render_data, pipeline, res, passId);
		hg::SubmitSceneToForwardPipeline(vid, scene, vr_eye_rect, right, pipeline, render_data, res, passId, vr_right_fb.fb);

		// display the VR eyes texture to the backbuffer
		bgfx::setViewRect(vid, 0, 0, res_x, res_y);
		hg::ViewState vs = hg::ComputeOrthographicViewState(hg::TranslationMat4(hg::Vec3::Zero), (float)res_y, 0.1f, 100.f, hg::ComputeAspectRatioX((float)res_x, (float)res_y));
		bgfx::setViewTransform(vid, hg::to_bgfx(vs.view).data(), hg::to_bgfx(vs.proj).data());

		quad_uniform_set_texture_list.clear();
		quad_uniform_set_texture_list.push_back(hg::MakeUniformSetTexture("s_tex", hg::OpenVRGetColorTexture(vr_left_fb), 0));
		hg::SetT(quad_matrix, hg::Vec3(eye_t_x, 0.f, 1.f));
		hg::DrawModel(vid, quad_model, tex0_program, quad_uniform_set_value_list, quad_uniform_set_texture_list, &quad_matrix, 1, quad_render_state);

		quad_uniform_set_texture_list.clear();
		quad_uniform_set_texture_list.push_back(hg::MakeUniformSetTexture("s_tex", hg::OpenVRGetColorTexture(vr_right_fb), 0));
		hg::SetT(quad_matrix, hg::Vec3(-eye_t_x, 0.f, 1.f));
		hg::DrawModel(vid, quad_model, tex0_program, quad_uniform_set_value_list, quad_uniform_set_texture_list, &quad_matrix, 1, quad_render_state);
					
		bgfx::frame();
		hg::OpenVRSubmitFrame(vr_left_fb, vr_right_fb);
		hg::UpdateWindow(win);
	}

	hg::DestroyForwardPipeline(pipeline);
	hg::RenderShutdown();
	hg::DestroyWindow(win);

	return EXIT_SUCCESS;
}