#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <thread>
#include <ctime>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("C.wav"));
});

// noise samples
Load< Sound::Sample > wrong_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("wrong.wav"));
});

Load< Sound::Sample > correct_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("correct.wav"));
});

//player samples

Load< Sound::Sample > high_c_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Ch.wav"));
});

Load< Sound::Sample > low_c_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("C.wav"));
});

Load< Sound::Sample > mid_e_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("E.wav"));
});

Load< Sound::Sample > mid_g_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("G.wav"));
});

//choir samples

Load< Sound::Sample > high_c_choir_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("ChC.wav"));
});

Load< Sound::Sample > low_c_choir_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("CC.wav"));
});

Load< Sound::Sample > mid_e_choir_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("EC.wav"));
});

Load< Sound::Sample > mid_g_choir_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("GC.wav"));
});



PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Hip.FL") hip = &transform;
		else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	}
	if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//inits song
	// for (uint32_t x = 0; x < 4; ++x) {
	// 	song[x] = Note{0,false}; //just populate with default note
	// }

	song =  {Note{0,false,false},Note{1,false,false},Note{2,false,false},Note{3,false,false}};
	

	//start music loop playing:
	// (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);
	leg_tip_loop.get()->stop();

	low_c = Sound::loop_3D(*low_c_sample, 1.0f, get_leg_tip_position(), 10.0f);
	low_c.get()->stop();

	high_c = Sound::loop_3D(*high_c_sample, 1.0f, get_leg_tip_position(), 10.0f);
	high_c.get()->stop();

	mid_e = Sound::loop_3D(*mid_e_sample, 1.0f, get_leg_tip_position(), 10.0f);
	mid_e.get()->stop();

	mid_g = Sound::loop_3D(*mid_g_sample, 1.0f, get_leg_tip_position(), 10.0f);
	mid_g.get()->stop();

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	//move sound to follow leg tip position:
	low_c->set_position(get_leg_tip_position(), 1.0f / 60.0f);
	high_c->set_position(get_leg_tip_position(), 1.0f / 60.0f);
	mid_e->set_position(get_leg_tip_position(), 1.0f / 60.0f);
	mid_g->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	

	//move camera:
	{
		bool match = false;
		bool wrong_sequence = false;

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		uint8_t player_note = 4; //set to a none note
		if (left.downs ==1){
			low_c.get()->stop();
			mid_g.get()->stop();
			high_c.get()->stop();
			// move.x =-1.0f;
			mid_e = Sound::play_3D(*mid_e_sample, 1.0f, get_leg_tip_position(), 10.0f);
			player_note = 1;
			//CHECK CORRECT NOTE
			size_t note_count = 0;
			match = false;
			wrong_sequence = false;
			while(note_count<4 && !match && !wrong_sequence){
				if(!song[note_count].correct){ //if next note
					if(player_note == song[note_count].note){ //if match
						song[note_count].correct = true;	//correct
						match = true; //exit out of check loop	
					} else {
						wrong_sequence = true; //exit loop
						wrong = Sound::play_3D(*wrong_sample, 1.0f, get_leg_tip_position(), 10.0f);
					}

				}
				note_count++;
			}
			
		} 
		if (right.downs==1){
			low_c.get()->stop();
			mid_e.get()->stop();
			high_c.get()->stop();
			// move.x = 1.0f;
			mid_g = Sound::play_3D(*mid_g_sample, 1.0f, get_leg_tip_position(), 10.0f);
			player_note = 2;
			//CHECK CORRECT NOTE
			size_t note_count = 0;
			match = false;
			wrong_sequence = false;
			while(note_count<4 && !match && !wrong_sequence){
				if(!song[note_count].correct){ //if next note
					if(player_note == song[note_count].note){ //if match
						song[note_count].correct = true;	//correct
						match = true; //exit out of check loop	
					} else {
						wrong_sequence = true; //exit loop
						wrong = Sound::play_3D(*wrong_sample, 1.0f, get_leg_tip_position(), 10.0f);
					}

				}
				

				note_count++;
			}
		} 
		if (down.downs==1){
			mid_g.get()->stop();
			mid_e.get()->stop();
			high_c.get()->stop();
			// move.y =-1.0f;
			low_c = Sound::play_3D(*low_c_sample, 1.0f, get_leg_tip_position(), 10.0f);
			player_note = 0;
			//CHECK CORRECT NOTE
			size_t note_count = 0;
			match = false;
			wrong_sequence = false;
			while(note_count<4 && !match && !wrong_sequence){
				
				if(!song[note_count].correct){ //if next note
					
					if(player_note == song[note_count].note){ //if match
						song[note_count].correct = true;	//correct
						match = true; //exit out of check loop	
					} else {
						wrong_sequence = true; //exit loop
						wrong = Sound::play_3D(*wrong_sample, 1.0f, get_leg_tip_position(), 10.0f);
					}

				}
				

				note_count++;
			}
		} 
		if (up.downs==1){
			low_c.get()->stop();
			mid_e.get()->stop();
			mid_g.get()->stop();
			// move.y = 1.0f;
			high_c = Sound::play_3D(*high_c_sample, 1.0f, get_leg_tip_position(), 10.0f);
			player_note = 3;
			//CHECK CORRECT NOTE
			size_t note_count = 0;
			match = false;
			wrong_sequence = false;
			while(note_count<4 && !match && !wrong_sequence){
				if(!song[note_count].correct){ //if next note
					if(player_note == song[note_count].note){ //if match
						song[note_count].correct = true;	//correct
						match = true; //exit out of check loop	
					} else {
						wrong_sequence = true; //exit loop
						wrong = Sound::play_3D(*wrong_sample, 1.0f, get_leg_tip_position(), 10.0f);
					}

				}
				

				note_count++;
			}
		} //TODOFORME: replay on space

		
		//check everything and also set if wrong

		bool all_done = true;
		for (size_t i = 0; i < 4; i++){
			if(!song[i].correct){
				all_done = false;
			}
			if(wrong_sequence){//have to reset
				song[i].correct = false;
			}
		}
		
		if(all_done){
			correct = Sound::play_3D(*correct_sample, 1.0f, get_leg_tip_position(), 10.0f);
			for (uint32_t x = 0; x < 4; ++x) {
				song[x] = Note{1,false}; //just populate with default note TODOFORME
			}
		}
		

		//if playing the song:
		if(playing){
			//update note_elapsed
			note_elapsed+=elapsed;
			if(note_elapsed>2.0f){
				bool note_played = false;
				for ( size_t i = 0; i < 4; i++){
					if(!note_played){
						if(!song[i].done){
							uint8_t current_note = song[i].note;
							if(current_note == 0){
								low_c_c = Sound::play_3D(*low_c_choir_sample, 1.0f, get_leg_tip_position(), 10.0f);
							} else if(current_note ==1){
								mid_e_c = Sound::play_3D(*mid_e_choir_sample, 1.0f, get_leg_tip_position(), 10.0f);
							} else if(current_note==2){
								mid_g_c = Sound::play_3D(*mid_g_choir_sample, 1.0f, get_leg_tip_position(), 10.0f);
							}else{
								high_c_c = Sound::play_3D(*high_c_choir_sample, 1.0f, get_leg_tip_position(), 10.0f);
							}
							song[i].done = true;
							note_played = true;
						}
					}
					
				}
				if(!note_played){
					//song done playing
					playing = false;
				}
				note_elapsed=0; //reset for next note, wait 4 seconds
			}
		}



		

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	// if (leg_tip_loop)
	// {
	// 	{ //update listener to camera position:
	// 	glm::mat4x3 frame = camera->transform->make_local_to_parent();
	// 	glm::vec3 frame_right = frame[0];
	// 	glm::vec3 frame_at = frame[3];
	// 	Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	// 	}
	// }
	

	// { //update listener to camera position:
	// 	glm::mat4x3 frame = camera->transform->make_local_to_parent();
	// 	glm::vec3 frame_right = frame[0];
	// 	glm::vec3 frame_at = frame[3];
	// 	Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	// }

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
