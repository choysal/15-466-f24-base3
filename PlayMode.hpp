#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	struct Note {
		uint8_t note = 0;
		//0 = lowC, 1 = midE, 2 = midG, 3 = highC
		bool done = false;
		bool correct = false;
	};

	std::array<Note, 4> song;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *hip = nullptr;
	Scene::Transform *upper_leg = nullptr;
	Scene::Transform *lower_leg = nullptr;
	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;
	uint8_t score = 0;

	glm::vec3 get_leg_tip_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;
	std::shared_ptr< Sound::PlayingSample > low_c;
	std::shared_ptr< Sound::PlayingSample > high_c;
	std::shared_ptr< Sound::PlayingSample > mid_e;
	std::shared_ptr< Sound::PlayingSample > mid_g;
	std::shared_ptr< Sound::PlayingSample > low_c_c;
	std::shared_ptr< Sound::PlayingSample > high_c_c;
	std::shared_ptr< Sound::PlayingSample > mid_e_c;
	std::shared_ptr< Sound::PlayingSample > mid_g_c;
	std::shared_ptr< Sound::PlayingSample > wrong;
	std::shared_ptr< Sound::PlayingSample > correct;



	bool playing = true;
	bool completed = false;
	float note_elapsed = 0.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
