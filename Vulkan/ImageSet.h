#pragma once
class ImageSet
{
public:
	ImageSet() {};
	ImageSet(std::shared_ptr<Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::ImageAspectFlags aspectFlags,uint32_t arrayLayers = 1);

    // 복사
    ImageSet(const ImageSet& other)
        : image(other.image), imageView(other.imageView) {
    }
    ImageSet& operator=(const ImageSet& other) {
        if (this != &other) {
            image = other.image;
            imageView = other.imageView;
        }
        return *this;
    }

    // 이동
    ImageSet(ImageSet&& other) noexcept
        : image(std::move(other.image)), imageView(std::move(other.imageView)) {
    }
    ImageSet& operator=(ImageSet&& other) noexcept {
        if (this != &other) {
            image = std::move(other.image);
            imageView = std::move(other.imageView);
        }
        return *this;
    }

    ~ImageSet() = default;

	Image image;
	ImageView imageView;
};

