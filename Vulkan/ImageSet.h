#pragma once
class ImageSet
{
	Image image;
	ImageView imageView;

	void destroy(Device& device) {
		image.destroy(device);
		imageView.destroy(device);
	}
};

