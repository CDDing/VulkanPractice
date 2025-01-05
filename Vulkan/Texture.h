#pragma once
class Texture
{
public:
	Texture() {};
	Texture(Device& device);
	void destroy(Device& device);
	Image image;
	ImageView imageView;
	Sampler sampler;
private:

	VkImageLayout _imageLayout;
	uint32_t _width, _height;
	uint32_t _mipLevels;
	uint32_t _layerCount;
	uint32_t index;
};

