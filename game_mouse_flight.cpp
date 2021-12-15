// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
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

void draw_circle(bgfx::ViewId &view_id, const hg::Vec2 &center, float radius, const hg::Color &color, bgfx::ProgramHandle draw2D_program, hg::RenderState& draw2D_render_state, bgfx::VertexLayout &vtx_layout) {
	int segment_count = 32;
	float step = hg::TwoPi / segment_count;
	hg::Vec3 p0 = hg::Vec3(center.x + radius, center.y, 0.f);
	hg::Vec3 p1 = hg::Vec3::Zero;

	hg::Vertices vtx(vtx_layout, segment_count * 2 + 2);

	for (int i = 0; i <= segment_count; i++) {
		p1.x = radius * cos(i * step) + center.x; 
		p1.y = radius * sin(i * step) + center.y;
		
		vtx.Begin(2 * i).SetPos(p0).SetColor0(color).End();
		vtx.Begin(2 * i + 1).SetPos(p1).SetColor0(color).End();
		p0 = p1;

		hg::DrawLines(view_id, vtx, draw2D_program, draw2D_render_state);
	}
}

void update_plane(hg::Node &plane_node, float mouse_x_normd, float mouse_y_normd, float setting_plane_speed, float setting_plane_mouse_sensitivity) {
	hg::Transform plane_transform = plane_node.GetTransform();
	
	hg::Vec3 plane_pos = plane_transform.GetPos();
	plane_pos = plane_pos + hg::Normalize(hg::GetZ(plane_transform.GetWorld())) * setting_plane_speed;
	plane_pos.y = hg::Clamp(plane_pos.y, 0.1f, 50.f);  // floor / ceiling;

	hg::Vec3 plane_rot = plane_transform.GetRot();

	hg::Vec3 next_plane_rot = plane_rot; // make a copy of the plane rotation
	next_plane_rot.x = hg::Clamp(next_plane_rot.x + mouse_y_normd * -0.03f, -0.75f, 0.75f);
	next_plane_rot.y = next_plane_rot.y + mouse_x_normd * 0.03f;
	next_plane_rot.z = hg::Clamp(mouse_x_normd * -0.75f, -1.2f, 1.2f);

	plane_rot = plane_rot + (next_plane_rot - plane_rot) * setting_plane_mouse_sensitivity;

	plane_transform.SetPos(plane_pos);
	plane_transform.SetRot(plane_rot);
}

void update_chase_camera(hg::Node& camera_node, const hg::Vec3& target_pos, float setting_camera_chase_distance) {
	hg::Transform camera_transform = camera_node.GetTransform();
	hg::Vec3 camera_to_target = hg::Normalize(target_pos - camera_transform.GetPos());

	camera_transform.SetPos(target_pos - camera_to_target * setting_camera_chase_distance);  // camera is 'distance' away from its target
	camera_transform.SetRot(hg::ToEuler(hg::Mat3LookAt(camera_to_target)));
}

int main() {
	// Initialize input and window system.
	hg::InputInit();
	hg::WindowSystemInit();

	int res_x = 1280, res_y = 720;

	// create window.
	hg::Window* window = hg::RenderInit("Harfang - Mouse Flight", res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X8);
	if (!window) {
		hg::error("failed to create window.");
		return EXIT_FAILURE;
	}

	// create forward pipeline and resources.
	hg::PipelineResources res = hg::PipelineResources();
	hg::ForwardPipeline pipeline = hg::CreateForwardPipeline();

	// access compiled resources
	hg::AddAssetsFolder("resources_compiled");

	// 2D drawing helpers
	bgfx::VertexLayout vtx_layout = hg::VertexLayoutPosFloatColorFloat();

	bgfx::ProgramHandle draw2D_program = hg::LoadProgramFromAssets("shaders/pos_rgb");
	hg::RenderState draw2D_render_state = hg::ComputeRenderState(hg::BM_Alpha, hg::DT_Less, hg::FC_Disabled);

	// gameplay settings
	hg::Vec3 setting_camera_chase_offset = hg::Vec3(0, 0.2f, 0);
	float setting_camera_chase_distance = 1;

	float setting_plane_speed = 0.05f;
	float setting_plane_mouse_sensitivity = 0.5f;

	// setup game world
	hg::Scene scene;
	hg::LoadSceneContext load_ctx;
	hg::LoadSceneFromAssets("playground/playground.scn", scene, res, hg::GetForwardPipelineInfo(), load_ctx);

	bool success = true;
	hg::Node plane_node = hg::CreateInstanceFromAssets(scene, hg::TranslationMat4(hg::Vec3(0, 4, 0)), "paper_plane/paper_plane.scn", res, hg::GetForwardPipelineInfo(), success);
	hg::Node camera_node = hg::CreateCamera(scene, hg::TranslationMat4(hg::Vec3(0, 4, -5)), 0.01f, 1000);

	scene.SetCurrentCamera(camera_node);

	hg::Keyboard keyboard;
	hg::Mouse mouse;

	// game loop
	while(!keyboard.Down(hg::K_Escape)) {
		hg::time_ns dt = hg::tick_clock();  // tick clock, retrieve elapsed clock since last call

		// update mouse/keyboard devices
		keyboard.Update();
		mouse.Update();

		// compute ratio corrected normalized mouse position
		int mouse_x = mouse.X();
		int mouse_y = mouse.Y();

		hg::Vec2 aspect_ratio = hg::ComputeAspectRatioX(float(res_x), float(res_y));
		float mouse_x_normd = (mouse_x / float(res_x) - 0.5f) * aspect_ratio.x;
		float mouse_y_normd = (mouse_y / float(res_y) - 0.5f) * aspect_ratio.y;

		// update gameplay elements (plane & camera)
		update_plane(plane_node, mouse_x_normd, mouse_y_normd, setting_plane_speed, setting_plane_mouse_sensitivity);
		update_chase_camera(camera_node, setting_camera_chase_offset * plane_node.GetTransform().GetWorld(), setting_camera_chase_distance);

		// update scene and submit it to render pipeline
		scene.Update(dt);

		bgfx::ViewId view_id = 0;
		hg::SceneForwardPipelinePassViewId views;
		hg::SubmitSceneToPipeline(view_id, scene, hg::iRect(0, 0, int(res_x), int(res_y)), true, pipeline, res, views);

		// draw 2D GUI
		hg::SetView2D(view_id, 0, 0, res_x, res_y, -1, 1, BGFX_CLEAR_DEPTH, hg::Color::Black, 1, 0, true);
		draw_circle(view_id, hg::Vec2(float(mouse_x), float(mouse_y)), 20.f, hg::Color::White, draw2D_program, draw2D_render_state, vtx_layout); // display mouse cursor

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