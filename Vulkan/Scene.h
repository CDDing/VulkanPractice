#pragma once
class Scene
{
public:
	Scene(std::nullptr_t) : skybox(nullptr) {};
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	Scene(Scene&&) = default;
	Scene& operator=(Scene&&) = default;
	Skybox skybox;
	std::vector<Model> models = {};

};

