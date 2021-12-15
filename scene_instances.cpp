// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <foundation/log.h>
#include <foundation/clock.h>
#include <foundation/math.h>
#include <foundation/rand.h>
#include <foundation/projection.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include <engine/render_pipeline.h>
#include <engine/scene.h>
#include <engine/scene_forward_pipeline.h>
#include <engine/forward_pipeline.h>
#include <engine/assets.h>

#include <memory>
#include <deque>

// Biped actor
class BipedActor {
public:
	enum State {
		Idle = 0,
		Walk,
		Run
	};

	BipedActor(hg::Scene& scene, hg::PipelineResources& res, const hg::Vec3& pos);
	~BipedActor();

	void Update(hg::time_ns dt);

private:
	void StartAnim(const std::string& name);

	hg::Node node;
	hg::time_ns delay = 0;
	State state = Idle;
	hg::ScenePlayAnimRef playing_anim_ref = hg::InvalidScenePlayAnimRef;
};

BipedActor::BipedActor(hg::Scene& scene, hg::PipelineResources& res, const hg::Vec3 &pos) {
	bool success = true;
	
	node = hg::CreateInstanceFromAssets(scene, hg::Mat4::Identity, "biped/biped.scn", res, hg::GetForwardPipelineInfo(), success);
	if (success) {
		node.GetTransform().SetPosRot(pos, hg::Deg3(0.f, hg::FRand(360.f), 0.f));
		playing_anim_ref = hg::InvalidScenePlayAnimRef;
	}
	state = BipedActor::Idle;
}

BipedActor::~BipedActor() {
	if (node.scene_ref && node.scene_ref->scene) {
		node.scene_ref->scene->DestroyNode(node);
	}
}

void BipedActor::StartAnim(const std::string& name) {
	if (!(node.scene_ref && node.scene_ref->scene)) {
		return;
	}
	hg::SceneAnimRef anim = node.GetInstanceSceneAnim(name);
	if (playing_anim_ref != hg::InvalidScenePlayAnimRef) {
		node.scene_ref->scene->StopAnim(playing_anim_ref);
	}
	playing_anim_ref = node.scene_ref->scene->PlayAnim(anim, hg::ALM_Loop);
}

void BipedActor::Update(hg::time_ns dt) {
	static const char* state_names[] = {
		"idle", "walk", "run"
	};

	// check for state change
	delay = delay - dt;
	if (delay <= 0) {
		state = BipedActor::State(hg::Rand(2));
		delay = delay + hg::time_from_sec_f(hg::FRRand(2.f, 6.f)); // 2 to 6 seconds before next state change
		StartAnim(state_names[state]);
	}

	// apply motion
	float dt_sec_f = hg::time_to_sec_f(dt);

	hg::Transform transform = node.GetTransform();
	hg::Vec3 pos, rot;
	transform.GetPosRot(pos, rot);

	if (state == BipedActor::Walk) {
		pos = pos - hg::GetZ(transform.GetWorld()) * hg::Mtr(1.15f) * dt_sec_f; // 1.15 m / sec
		rot.y = rot.y + hg::Deg(50.f) * dt_sec_f;
	}
	else if (state == BipedActor::Run) {
		pos = pos - hg::GetZ(transform.GetWorld()) * hg::Mtr(4.5f) * dt_sec_f; // 4.5 m / sec
		rot.y = rot.y - hg::Deg(70.f) * dt_sec_f;
	}

	// confine actor to playground
	pos = hg::Clamp(pos, hg::Vec3(-10.f, 0.f, -10.f), hg::Vec3(10.f, 0.f, 10.f));

	transform.SetPosRot(pos, rot);
}

int main() {
	// Initialize input and window system.
	hg::InputInit();
	hg::WindowSystemInit();

	int res_x = 1280, res_y = 720;

	// create window.
	hg::Window* window = hg::RenderInit("Harfang - Scene instances", res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4);
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

	hg::LoadSceneFromAssets("playground/playground.scn", scene, res, hg::GetForwardPipelineInfo(), load_ctx);

	// spawn initial actors
	std::deque<std::unique_ptr<BipedActor>> actors;
	for (int i = 0; i < 20; i++) {
		actors.emplace_back(std::make_unique<BipedActor>(scene, res, hg::RandomVec3(hg::Vec3(-10.f, 0.f, -10.f), hg::Vec3(10.f, 0.f, 10.f))));
	}
	printf("%zu nodes in scene", scene.GetAllNodeCount());

	hg::Keyboard keyboard;

	// game loop
	while (!keyboard.Down(hg::K_Escape)) {
		hg::RenderResetToWindow(window, res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4 | BGFX_RESET_MAXANISOTROPY);

		hg::time_ns dt = hg::tick_clock();  // tick clock, retrieve elapsed clock since last call

		// update mouse/keyboard devices
		keyboard.Update();
		
		if (keyboard.Pressed(hg::K_S)) {
			actors.emplace_back(std::make_unique<BipedActor>(scene, res, hg::RandomVec3(hg::Vec3(-10.f, 0.f, -10.f), hg::Vec3(10.f, 0.f, 1.0f))));
		}

		if (keyboard.Pressed(hg::K_D)) {
			if (!actors.empty()) {
				actors.pop_front();
				scene.GarbageCollect();
			}
		}
		
		for (auto &it : actors) {
			it->Update(dt);
		}
		
		scene.Update(dt);

		hg::ViewState view_state = hg::ComputePerspectiveViewState(hg::Mat4LookAt(hg::Vec3(0.f, 10.f, -14.f), hg::Vec3(0.f, 1.f, -4.f)), hg::Deg(45.f), 0.01f, 1000.f, hg::ComputeAspectRatioX(float(res_x), float(res_y)));
		bgfx::ViewId view_id = 0;
		hg::SceneForwardPipelinePassViewId views;
		hg::SubmitSceneToPipeline(view_id, scene, hg::iRect(0, 0, res_x, res_y), view_state, pipeline, res, views);

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