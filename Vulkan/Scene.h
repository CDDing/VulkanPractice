#pragma once
class Scene
{
public:
	Skybox skybox;
	std::vector<Model> models;

	void destroy(std::shared_ptr<Device> device);
};

