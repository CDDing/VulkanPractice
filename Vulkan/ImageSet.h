#pragma once
class ImageSet
{
public:
	ImageSet() {};
	ImageSet(Image img, ImageView imgView) :image(img), imageView(imgView) {};
	ImageSet(Device& device, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags,uint32_t arrayLayers = 1);

	Image image;
	ImageView imageView;

	void destroy(Device& device) {
		image.destroy(device);
		imageView.destroy(device);
	}
	operator Image& () {
		return image;
	}
	operator ImageView& () {
		return imageView;
	}
	operator VkImage& () {
		return image;
	}
	operator VkImageView& () {
		return imageView;
	}
	void operator=(Image img) {
		image = img;
	}
	void operator=(ImageView imgView) {
		imageView = imgView;
	}
};

