#pragma once
class Scene
{
public:
	Model skybox;
	std::vector<Model> models;

	void destroy(std::shared_ptr<Device> device);
};

