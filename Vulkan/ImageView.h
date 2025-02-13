#pragma once
class ImageView
{
public:
    ImageView();
    ImageView(std::shared_ptr<Device> device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layerCount=1);
    operator vk::ImageView& () {
        return _imageView;
    }
    //복사
    ImageView(const ImageView& other) : _imageView(other._imageView), _device(other._device){

    }
    ImageView& operator=(const ImageView& other) noexcept{
        _imageView = other._imageView;
        _device = other._device;

        return *this;
    }
    //이동
    ImageView(ImageView&& other) noexcept
        : _imageView(other._imageView), _device(std::move(other._device))
    {
        other._imageView = VK_NULL_HANDLE;
    }

    ImageView& operator=(ImageView&& other) noexcept
    {
        if (this != &other)
        {
            if (_imageView != VK_NULL_HANDLE)
                _device->logical.destroyImageView(_imageView);

            _imageView = other._imageView;
            _device = std::move(other._device);
            other._imageView = VK_NULL_HANDLE;
        }
        return *this;
    }
    ~ImageView() {
        if (_imageView != VK_NULL_HANDLE) {
            std::cout << "이미지 뷰 해제" << std::endl;
            _device->logical.destroyImageView(_imageView);
            _imageView = VK_NULL_HANDLE;
        }
    }
private:
    vk::ImageView _imageView = VK_NULL_HANDLE;
    std::shared_ptr<Device> _device;
};

