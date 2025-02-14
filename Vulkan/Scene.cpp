#include "pch.h"
#include "Scene.h"

void Scene::destroy(std::shared_ptr<Device> device)
{
	for (auto& model : models) {
		model.destroy(device);
	}
	skybox.destroy(device);
}
