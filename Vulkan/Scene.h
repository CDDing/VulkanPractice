#pragma once
class Scene
{
public:
	Model skybox;
	std::vector<Model> models;

	void destroy(Device& device);
};

