#pragma once
class Camera
{
public:
	Camera();
	glm::mat4 GetView();
	glm::mat4 GetProj();
	glm::vec3 GetPos() { return _position;}
	glm::vec3 GetDir() { return _viewDir; }
	glm::vec3 GetUp() { return _upDir; }
	void Update(float dt, bool* keyPressed,double mouse_dx,double mouse_dy);


private:
	void moveForward(float dt);
	void moveRight(float dt);
	void rotate(double mouse_dx, double mouse_dy);

	glm::vec3 _position = glm::vec3(0.0f,0.0f,2.0f);
	glm::vec3 _viewDir = glm::vec3(0.0f,0.0f,-1.0f);
	glm::vec3 _upDir = glm::vec3(0.0f,1.0f,0.0f);
	glm::vec3 _rightDir = glm::vec3(1.0f,0.0f,0.0f);

	float _speed = 1.0f;
	float _rotateSpeed = 0.5f;

	float _nearZ = 0.01f;
	float _farZ = 10000.0f;
	float _aspect = 800.0f / 600.0f;

};

