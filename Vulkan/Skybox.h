#pragma once
class Model;
class Skybox :
    public Model
{
public:
    virtual void InitDescriptorSet(std::shared_ptr<Device> device);
};


Skybox makeSkyBox(std::shared_ptr<Device> device);