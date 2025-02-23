#pragma once
class Model;
class Skybox :
    public Model
{
public:
	Skybox(std::nullptr_t) : Model(nullptr) {};
    Skybox(Device& device);

    virtual void InitDescriptorSet(Device& device);
};
