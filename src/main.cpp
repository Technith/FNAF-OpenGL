#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <filesystem>
#include <numbers>
#include <map>
#include <string>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Graphics.hpp>

#include "AssimpImport.h"
#include "Mesh.h"
#include "Object3D.h"
#include "Animator.h"
#include "ShaderProgram.h"
#include <TranslationAnimation.h>

#define M_PI std::numbers::pi_v<float>
//#define LOG_FPS

// We use a structure to track all the elements of a scene, including a list of objects,
// a list of animators, and a shader program to use to render those objects.
struct Scene {
	ShaderProgram program{};
	std::vector<Object3D> objects{};
	std::vector<Animator> animators{};
};

/**
 * @brief Constructs a shader program that applies the Phong reflection model.
 */
ShaderProgram phongLightingShader() {
	ShaderProgram shader{};
	try {
		// These shaders are INCOMPLETE.
		shader.load("shaders/light_perspective.vert", "shaders/lighting.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Constructs a shader program that performs texture mapping with no lighting.
 */
ShaderProgram texturingShader() {
	ShaderProgram shader{};
	try {
		shader.load("shaders/texture_perspective.vert", "shaders/texturing.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Loads an image from the given path into an OpenGL texture.
 */
Texture loadTexture(const std::filesystem::path& path, const std::string& samplerName = "baseTexture") {
	StbImage i{};
	i.loadFromFile(path.string());
	return Texture::loadImage(i, samplerName);
}

/*****************************************************************************************
*  DEMONSTRATION SCENES
*****************************************************************************************/
Scene bunny() {
	Scene scene{ phongLightingShader() };

	// We assume that (0,0) in texture space is the upper left corner, but some artists use (0,0) in the lower
	// left corner. In that case, we have to flip the V-coordinate of each UV texture location. The last parameter
	// to assimpLoad controls this. If you load a model and it looks very strange, try changing the last parameter.
	auto bunny{ assimpLoad("models/bunny_textured.obj", true) };
	bunny.grow(glm::vec3{ 9, 9, 9 });
	bunny.move(glm::vec3{ 0.2, -1, 0 });

	// Move all objects into the scene's objects list.
	scene.objects.push_back(std::move(bunny));
	// Now the "bunny" variable is empty; if we want to refer to the bunny object, we need to reference 
	// scene.objects[0]

	Animator spinBunny{};
	// Spin the bunny 360 degrees over 10 seconds.
	spinBunny.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0f, glm::vec3{ 0, 1, 0 }));

	// Move all animators into the scene's animators list.
	scene.animators.push_back(std::move(spinBunny));

	return scene;
}


/**
 * @brief Demonstrates loading a square, oriented as the "floor", with a manually-specified texture
 * that does not come from Assimp.
 */
Scene marbleSquare() {
	Scene scene{ texturingShader() };

	std::vector<Texture> textures{
		loadTexture("models/White_marble_03/Textures_2K/white_marble_03_2k_baseColor.tga", "baseTexture"),
	};
	auto mesh{ Mesh::square(textures) };
	Object3D floor{ std::vector<Mesh>{mesh} };
	floor.grow(glm::vec3{ 5, 5, 5 });
	floor.move(glm::vec3{ 0, -1.5, 0 });
	floor.rotate(glm::vec3{ -M_PI / 2, 0, 0 });

	scene.objects.push_back(std::move(floor));
	return scene;
}

/**
 * @brief Loads a cube with a cube map texture.
 */
Scene cube() {
	Scene scene{ texturingShader() };

	auto cube{ assimpLoad("models/cube.obj", true) };

	scene.objects.push_back(std::move(cube));

	Animator spinCube{};
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0f, glm::vec3{ 0, 2 * M_PI, 0 }));
	// Then spin around the x axis.
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0f, glm::vec3{ 2 * M_PI, 0, 0 }));

	scene.animators.push_back(std::move(spinCube));

	return scene;
}

/**
 * @brief Constructs a scene of a tiger sitting in a boat, where the tiger is the child object
 * of the boat.
 * @return
 */
Scene lifeOfPi() {
	// This scene is more complicated; it has child objects, as well as animators.
	Scene scene{ phongLightingShader() };

	scene.program.setUniform("directionalLight", glm::vec3(0, -1, 0));

	auto boat{ assimpLoad("models/boat/boat.fbx", true) };
	boat.move(glm::vec3{ 0, -0.7, 0 });
	boat.grow(glm::vec3{ 0.01, 0.01, 0.01 });
	auto tiger{ assimpLoad("models/tiger/scene.gltf", true) };
	tiger.move(glm::vec3{ 0, -5, 10 });
	tiger.setMaterial(glm::vec4{ 1, 1, 1, 1 });
	// Move the tiger to be a child of the boat.
	boat.addChild(std::move(tiger));

	// Move the boat into the scene list.
	scene.objects.push_back(std::move(boat));

	// We want these animations to referenced the *moved* objects, which are no longer
	// in the variables named "tiger" and "boat". "boat" is now in the "objects" list at
	// index 0, and "tiger" is the index-1 child of the boat.
	Animator animBoat{};
	animBoat.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0f, glm::vec3{ 0, 2 * M_PI, 0 }));
	Animator animTiger{};
	animTiger.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0].getChild(1), 10.0f, glm::vec3{ 0, 0, 2 * M_PI }));

	// The Animators will be destroyed when leaving this function, so we move them into
	// a list to be returned.
	scene.animators.push_back(std::move(animBoat));
	scene.animators.push_back(std::move(animTiger));

	scene.program.activate();
	scene.program.setUniform("directionalLight", glm::vec3(0, -1, 0));

	// Transfer ownership of the objects and animators back to the main.
	return scene;
}

Scene freddy() {
	Scene scene{ phongLightingShader() };

	auto freddy{ assimpLoad("models/freddy_fazbear/scene.gltf", true) };
	freddy.move(glm::vec3{ 0, 0, -20.0 });

	scene.objects.push_back(std::move(freddy));

	auto bright_freddy{ assimpLoad("models/freddy_fazbear/scene.gltf", true) };
	bright_freddy.setMaterial(glm::vec4{ 1, 1, 1, 1 });
	bright_freddy.move(glm::vec3{ 0, 5, -30 });

	scene.objects.push_back(std::move(bright_freddy));

	Animator animFreddy{};
	//animFreddy.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0f, glm::vec3{ 0, 2 * M_PI, 0 }));
	scene.animators.push_back(std::move(animFreddy));

	return scene;
}

Scene fnaf(std::vector<Object3D> extra) {
	Scene scene{ phongLightingShader() };

	auto freddy{ assimpLoad("models/fnaf_movie/freddy/scene.gltf", true) };
	freddy.move(glm::vec3{ 0, -.5, -29 });
	freddy.grow(glm::vec3{ .55, .55, .55 });
	scene.objects.push_back(std::move(freddy));

	auto bonnie{ assimpLoad("models/fnaf_movie/bonnie/scene.gltf", true) };
	bonnie.move(glm::vec3{ -.5, -.5, -29.5 });
	bonnie.grow(glm::vec3{ .05, .05, .05 });
	scene.objects.push_back(std::move(bonnie));
	
	auto chica{ assimpLoad("models/fnaf_movie/chica/scene.gltf", true) };
	chica.move(glm::vec3{ .5, -.5, -29.5 });
	chica.grow(glm::vec3{ .05, .05, .05 });
	scene.objects.push_back(std::move(chica));

	auto foxy{ assimpLoad("models/fnaf_movie/foxy/scene.gltf", true) };
	//foxy.move(glm::vec3{-9, -1.6, -28});
	foxy.move(glm::vec3{ -9, -.55, -28 });
	foxy.grow(glm::vec3{ .05, .05, .05 });
	foxy.rotate(glm::vec3{ 0, M_PI / 4, 0 });
	scene.objects.push_back(std::move(foxy));

	auto stage{ assimpLoad("models/fnaf_movie/stage/scene.gltf", true) };
	stage.move(glm::vec3{ 0, .55, -30 });
	stage.grow(glm::vec3{ 0.336, 0.336, 0.336 });
	stage.rotate(glm::vec3{ 0, M_PI, 0 });
	scene.objects.push_back(std::move(stage));

	auto office{ assimpLoad("models/fnaf_movie/office/scene.gltf", true) };
	office.move(glm::vec3{ 0, -.5, 4.5 });
	scene.objects.push_back(std::move(office));

	auto rightOfficeDoor{ assimpLoad("models/fnaf_movie/office_door/scene.gltf", true) };
	// Closed
	//rightOfficeDoor.move(glm::vec3{ .85, -.5, 4.25 });
	// Open
	rightOfficeDoor.move(glm::vec3{ .85, .65, 4.25 });
	rightOfficeDoor.grow(glm::vec3{ .2, .2, .2 });
	scene.objects.push_back(std::move(rightOfficeDoor));

	auto leftOfficeDoor{ assimpLoad("models/fnaf_movie/office_door/scene.gltf", true) };
	// Closed
	//leftOfficeDoor.move(glm::vec3{ -.525, -.5, 4.25 });
	// Open
	leftOfficeDoor.move(glm::vec3{ -.525, .65, 4.25 });
	leftOfficeDoor.grow(glm::vec3{ .2, .2, .2 });
	scene.objects.push_back(std::move(leftOfficeDoor));

	auto cove{ assimpLoad("models/fnaf_movie/pirate_cove/scene.gltf", true) };
	cove.move(glm::vec3{ -9, -.8, -28 });
	cove.grow(glm::vec3{ .84, .84, .84 });
	cove.rotate(glm::vec3{ 0, (5 * M_PI) / 4, 0 });
	scene.objects.push_back(std::move(cove));

	for (auto obj : extra) scene.objects.push_back(std::move(obj));

	Animator animRightDoorDown{};
	animRightDoorDown.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[6], 1.0f, glm::vec3{ 0, -1.15, 0 }));
	scene.animators.push_back(std::move(animRightDoorDown));

	Animator animLeftDoorDown{};
	animLeftDoorDown.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[7], 1.0f, glm::vec3{ 0, -1.15, 0 }));
	scene.animators.push_back(std::move(animLeftDoorDown));

	Animator animRightDoorUp{};
	animRightDoorUp.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[6], 2.0f, glm::vec3{ 0, 1.15, 0 }));
	scene.animators.push_back(std::move(animRightDoorUp));

	Animator animLeftDoorUp{};
	animLeftDoorUp.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[7], 2.0f, glm::vec3{ 0, 1.15, 0 }));
	scene.animators.push_back(std::move(animLeftDoorUp));

	return scene;
}

void movement(glm::vec3& cameraPos, glm::vec3& cameraForwards, float& yaw, float deltaTime, float moveSpeed, float rotationSpeed) {
	auto deltaYaw = 0;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
		yaw -= rotationSpeed * deltaTime;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
		yaw += rotationSpeed * deltaTime;
	}

	//yaw = glm::clamp(yaw + deltaYaw, -(3*M_PI)/4, -M_PI/4);

	cameraForwards = glm::normalize(glm::vec3{ std::cos(yaw), 0, std::sin(yaw) });

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
		cameraPos += cameraForwards * deltaTime * moveSpeed;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
		cameraPos -= cameraForwards * deltaTime * moveSpeed;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
		cameraPos.y += moveSpeed * deltaTime;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
		cameraPos.y -= moveSpeed * deltaTime;
	}
}

void doorAction(Scene& scene, bool& leftDoorClosed, bool& rightDoorClosed, const std::optional<sf::Event>& event) {
	if (event->is<sf::Event::KeyPressed>()) {
		auto key = event->getIf<sf::Event::KeyPressed>()->code;
		if (key == sf::Keyboard::Key::Q) {
			if (!leftDoorClosed) {
				scene.animators[1].start();
				leftDoorClosed = true;
			}
			else if (leftDoorClosed) {
				scene.animators[3].start();
				leftDoorClosed = false;
			}
		}
	}

	if (event->is<sf::Event::KeyPressed>()) {
		auto key = event->getIf<sf::Event::KeyPressed>()->code;
		if (key == sf::Keyboard::Key::E) {
			if (!rightDoorClosed) {
				scene.animators[0].start();
				rightDoorClosed = true;
			}
			else if (rightDoorClosed) {
				scene.animators[2].start();
				rightDoorClosed = false;
			}
		}
	}

	scene.objects[6].setPosition(glm::clamp(scene.objects[6].getPosition(), glm::vec3{ .85, -.5, 4.25 }, glm::vec3{ .85, .65, 4.25 }));
	scene.objects[7].setPosition(glm::clamp(scene.objects[7].getPosition(), glm::vec3{ -.525, -.5, 4.25 }, glm::vec3{ -.525, .65, 4.25 }));
}

void cameraAction(std::map<std::string, glm::vec3>& activeCamInfo, int& activeCam, std::map<std::string, glm::vec3> stageCamera, std::map<std::string, glm::vec3> coveCamera, std::map<std::string, glm::vec3> hallCamera, const std::optional<sf::Event>& event) {
	if (event->is<sf::Event::KeyPressed>()) {
		auto key = event->getIf<sf::Event::KeyPressed>()->code;
		if (key == sf::Keyboard::Key::Num1) {
			activeCam = 0;
			activeCamInfo = stageCamera;
		}
		if (key == sf::Keyboard::Key::Num2) {
			activeCam = 1;
			activeCamInfo = coveCamera;
		}
		if (key == sf::Keyboard::Key::Num3) {
			activeCam = 2;
			activeCamInfo = hallCamera;
		}
	}
}

int main() {
	std::cout << std::filesystem::current_path() << std::endl;

	// Initialize the window and OpenGL.
	sf::ContextSettings settings;
	settings.depthBits = 24; // Request a 24 bits depth buffer
	settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	settings.majorVersion = 3; // You might have to change these on Mac.
	settings.minorVersion = 3;

	sf::Window window{
		sf::VideoMode::getFullscreenModes().at(0), "Modern OpenGL",
		sf::Style::Resize | sf::Style::Close,
		sf::State::Windowed, settings
	};


	gladLoadGL();
	glEnable(GL_DEPTH_TEST);
	// Enable Backface Culling (Cull triangles whihc normal is not towards the camera)
	//glEnable(GL_CULL_FACE);

	auto width = 256;
	auto height = 256;

	uint32_t myFbo;
	glGenFramebuffers(1, &myFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, myFbo);

	uint32_t colorBufferId;
	glGenTextures(1, &colorBufferId);
	glBindTexture(GL_TEXTURE_2D, colorBufferId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	Mesh cam = Mesh::square({});
	Texture camtext{};
	camtext.textureId = colorBufferId;
	cam.addTexture(camtext);

	uint32_t depthBufferId;
	glGenTextures(1, &depthBufferId);
	glBindTexture(GL_TEXTURE_2D, depthBufferId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBufferId, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// Inintialize scene objects.

	// Player Camera
	std::map<std::string, glm::vec3> playerCamera;
	playerCamera["cameraPos"] = glm::vec3{ 0, 0, 5 };
	playerCamera["cameraForwards"] = glm::vec3{ 0, 0, -1 };
	playerCamera["cameraUp"] = glm::vec3{ 0, 1, 0 };
	// Stage Camera 
	std::map<std::string, glm::vec3> stageCamera;
	stageCamera["cameraPos"] = glm::vec3{ 0, 1, -28 };
	stageCamera["cameraForwards"] = glm::vec3{ 0, 0, -1 };
	stageCamera["cameraUp"] = glm::vec3{ 0, 1, 0 };
	// Pirate Cove Camera
	std::map<std::string, glm::vec3> coveCamera;
	coveCamera["cameraPos"] = glm::vec3{ -9, .6, -27.15};
	coveCamera["cameraForwards"] = glm::vec3{ -1, 0, -1 };
	coveCamera["cameraUp"] = glm::vec3{ 0, 1, 0 };
	// Left Hall Camera
	std::map<std::string, glm::vec3> hallCamera;
	hallCamera["cameraPos"] = glm::vec3{ -1, .7, 3 };
	hallCamera["cameraForwards"] = glm::vec3{ -1, 0, -1 };
	hallCamera["cameraUp"] = glm::vec3{ 0, 1, 0 };
	//Security Camera
	std::map<std::string, glm::vec3> securityCamera = stageCamera;
	// Camera Screen
	std::vector<Mesh> camMesh{ cam };
	Object3D camObj = Object3D(camMesh);
	camObj.move(glm::vec3{ .25, .1, 3.85 });
	camObj.grow(glm::vec3{ -.5, .5, .5 });
	camObj.rotate(glm::vec3{ 0, 0, M_PI });

	std::vector<Object3D> extraObj{ camObj }; // Add objects outside of function
	

	auto yaw = -M_PI / 2;
	auto cameraYaw = -M_PI / 2;
	auto deltaYaw = M_PI / 8;
	auto pitch = -M_PI / 4;
	auto moveSpeed = 3.0f;
	auto rotationSpeed = 2.0f;
	auto leftDoorClosed = false;
	auto rightDoorClosed = false;
	int activeCam = 0; // 0 - Stage, 1 - Cove
	auto distanceMoved = 0.0f;
	auto foxyVelocity = 2.0f;
	auto foxyAcceleration = 1.4f;
	bool alive = true;
	bool foxyEvent = false;
	bool foxyReset = false;
	sf::Time foxyTrigger = sf::seconds(30.0f);

	auto myScene{ fnaf(extraObj) };
	// You can directly access specific objects in the scene using references.
	auto& firstObject{ myScene.objects[0] };
	auto& foxy{ myScene.objects[3] };

	// Activate the shader program.
	myScene.program.activate();

	// Start the animators.
	//for (auto& anim : myScene.animators) {
	//	anim.start();
	//}

	// Ready, set, go!
	bool running{ true };
	sf::Clock c;

	auto last{ c.getElapsedTime() };

	
	while (window.isOpen()) {
		// Check for events.
		while (const std::optional event{ window.pollEvent() }) {
			if (event->is<sf::Event::Closed>()) {
				window.close();
			}
			doorAction(myScene, leftDoorClosed, rightDoorClosed, event);
			cameraAction(securityCamera, activeCam, stageCamera, coveCamera, hallCamera, event);
		}
		auto now{ c.getElapsedTime() };
		auto diff{ now - last };
		last = now;

		auto deltaTime = diff.asSeconds();
		movement(playerCamera["cameraPos"], playerCamera["cameraForwards"], yaw, deltaTime, moveSpeed, rotationSpeed);

#ifdef LOG_FPS
		// FPS calculation.
		std::cout << 1 / diff.asSeconds() << " FPS " << std::endl;
#endif

		std::cout << playerCamera["cameraPos"].x << playerCamera["cameraPos"].y << playerCamera["cameraPos"].z << std::endl;

		// Foxy running down hallway
		if (c.getElapsedTime() > foxyTrigger && c.getElapsedTime() < foxyTrigger + sf::seconds(1.0f)) {
			foxyEvent = true;
		}
		auto deltaMove = (foxyAcceleration * foxyVelocity) * deltaTime;
		if (foxyEvent && distanceMoved < 8) {
			foxy.move(glm::vec3{ deltaMove, 0, deltaMove });
			distanceMoved += deltaMove;
		}
		if (foxyEvent && distanceMoved > 8 && distanceMoved < 32) {
			foxy.move(glm::vec3{ 0, 0, deltaMove });
			distanceMoved += deltaMove;
		}
		if (foxyEvent && distanceMoved > 32) { // If Foxy reaches office and door isn't closed
			if (!leftDoorClosed && alive) {
				foxy.rotate(glm::vec3{ 0, 0, -M_PI / 8 });
				alive = false;
			}
			if (leftDoorClosed) {
				foxyEvent = false;
				foxyReset = true;
			}
		}
		if (foxyReset) {
			foxy.setPosition(glm::vec3{ -9, -.55, -28 });
			foxy.setOrientation(glm::vec3{ 0, M_PI / 4, 0 });
			foxyReset = false;
		}

		// Tilt the Stage Camera down
		glm::vec3 front;
		front.x = cos(pitch) * cos(cameraYaw);
		front.y = sin(pitch);
		front.z = cos(pitch) * sin(cameraYaw);
		securityCamera["cameraForwards"] = glm::normalize(front);

		cameraYaw += deltaYaw * deltaTime;
		if (cameraYaw > -M_PI / 4) {
			cameraYaw = -M_PI / 4;
			deltaYaw = -glm::abs(deltaYaw);
		}
		if (cameraYaw < -3 * M_PI / 4) {
			cameraYaw = -3 * M_PI / 4;
			deltaYaw = glm::abs(deltaYaw);
		}

		// Security Camera
		glBindFramebuffer(GL_FRAMEBUFFER, myFbo);

		glViewport(0, 0, width, height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glm::mat4 securityCameraMat{ glm::lookAt(securityCamera["cameraPos"], securityCamera["cameraPos"] + securityCamera["cameraForwards"], securityCamera["cameraUp"]) };
		glm::mat4 securityPerspective{ glm::perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f)};

		myScene.program.setUniform("projection", securityPerspective);
		myScene.program.setUniform("view", securityCameraMat);
		myScene.program.setUniform("cameraPos", securityCamera["cameraPos"]);

		myScene.program.setUniform("directionalLight", glm::vec3(0, 1, -1));
		myScene.program.setUniform("ambientColor", glm::vec3(1, 1, 1));
		//myScene.program.setUniform("directionalLight", glm::vec3(-1, -1, -1));


		for (auto& o : myScene.objects) {
			o.render(myScene.program);
		}

		// Player Camera
	    glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, window.getSize().x, window.getSize().y);
	
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glm::mat4 playerCameraMat{ glm::lookAt(playerCamera["cameraPos"], playerCamera["cameraPos"] + playerCamera["cameraForwards"], playerCamera["cameraUp"]) };
		glm::mat4 playerPerspective{ glm::perspective(glm::radians(45.0f), static_cast<float>(window.getSize().x) / window.getSize().y, 0.1f, 100.0f) };
		myScene.program.setUniform("view", playerCameraMat);
		myScene.program.setUniform("projection", playerPerspective);
		myScene.program.setUniform("cameraPos", playerCamera["cameraPos"]);

		myScene.program.setUniform("ambientColor", glm::vec3(1, 1, 1));
		//myScene.program.setUniform("directionalLight", glm::vec3(-1, -1, -1));
		myScene.program.setUniform("directionalLight", glm::vec3(0, -1, -1));
		myScene.program.setUniform("directionalColor", glm::vec3(1, 1, 1));

		// Update the scene.
		for (auto& anim : myScene.animators) {
			anim.tick(diff.asSeconds());
		}

		// Clear the OpenGL "context".
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Render the scene objects.
		for (auto& o : myScene.objects) {
			o.render(myScene.program);
		}


		window.display();
	}

	return 0;
}


