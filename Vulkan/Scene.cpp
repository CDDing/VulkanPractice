#include "pch.h"
#include "Scene.h"

void Scene::destroy(Device& device)
{
	for (auto& model : models) {
		model.destroy(device);
	}
	skybox.destroy(device);
}
