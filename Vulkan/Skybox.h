#pragma once
class Model;
class Skybox :
    public Model
{
public:
	Skybox(std::nullptr_t) : Model(nullptr) {};
    Skybox(DContext& context);

    virtual void InitDescriptorSet(DContext& context);
};
