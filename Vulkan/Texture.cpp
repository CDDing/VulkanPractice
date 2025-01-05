#include "pch.h"
#include "Texture.h"

Texture::Texture(Device& device)
{
}


void Texture::destroy(Device& device)
{
	vkDestroyImageView(device.Get(), imageView.Get(), nullptr);
	vkDestroyImage(device.Get(), image.Get(), nullptr);
	vkFreeMemory(device.Get(), image.GetMemory(), nullptr);
	vkDestroySampler(device.Get(), sampler.Get(), nullptr);

}
