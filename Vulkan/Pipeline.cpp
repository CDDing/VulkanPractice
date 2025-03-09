#include "pch.h"
#include "Pipeline.h"


Pipeline::Pipeline(DContext& context, vk::Extent2D& swapChainExtent, std::vector<std::vector<vk::DescriptorSetLayout>>& descriptorSetLayouts, vk::raii::RenderPass& renderPass, const std::string& vsShaderPath, const std::string& psShaderPath, ShaderType type)
	: pipeline(nullptr), pipelineLayout(nullptr)
{
    vk::raii::ShaderModule vertShaderModule = createShader(context, vsShaderPath);
    vk::raii::ShaderModule fragShaderModule = createShader(context, psShaderPath);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ {},
        vk::ShaderStageFlagBits::eVertex,vertShaderModule,"main"};

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ {},
        vk::ShaderStageFlagBits::eFragment,fragShaderModule,"main"};
    
    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,fragShaderStageInfo };

    //1.3.3 Fixed functions
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{ {},dynamicStates };


    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::inputAttributeDescriptions(0, { VertexComponent::Position,VertexComponent::Normal,VertexComponent::UV,VertexComponent::Tangent });

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{{},
        bindingDescription, attributeDescription};

    if (type == ShaderType::DEFAULT) {
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
    }
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {},vk::PrimitiveTopology::eTriangleList};
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport{0.0f,0.0f,
    (float)swapChainExtent.width,(float)swapChainExtent.height,
    0.0f,1.0f};

    vk::Rect2D scissor{ {0,0},swapChainExtent };

    vk::PipelineViewportStateCreateInfo viewportState{
        {},viewport,scissor };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        {}, vk::False,vk::False,
    vk::PolygonMode::eFill,vk::CullModeFlagBits::eBack,vk::FrontFace::eCounterClockwise};
    rasterizer.lineWidth = 1.0f;
    if (type == ShaderType::SKYBOX) {
        rasterizer.frontFace = vk::FrontFace::eClockwise;
    }
    else if (type == ShaderType::DEFERRED) {
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    }
    else if (type == ShaderType::DEFAULT) {
        rasterizer.cullMode = vk::CullModeFlagBits::eFront;
    }
    rasterizer.depthBiasEnable = vk::False;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling{
        {},vk::SampleCountFlagBits::e1,vk::False,1.0f,nullptr,vk::False,vk::False };


    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR 
        | vk::ColorComponentFlagBits::eG 
        | vk::ColorComponentFlagBits::eB 
        | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::False;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    std::array<vk::PipelineColorBlendAttachmentState, 6> blendAttachmentStates = {
        colorBlendAttachment,
        colorBlendAttachment,
        colorBlendAttachment,
        colorBlendAttachment,
        colorBlendAttachment,
        colorBlendAttachment
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    if (type == ShaderType::DEFERRED) {
        colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
        colorBlending.pAttachments = blendAttachmentStates.data();
    }

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.depthTestEnable = vk::True;
    depthStencil.depthWriteEnable = vk::True;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = vk::False;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = vk::False;
    depthStencil.front = vk::StencilOpState{};
    depthStencil.back = vk::StencilOpState{};


    //VKPipelineLayout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts[static_cast<int>(type)].size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts[static_cast<int>(type)].data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    vk::PushConstantRange range = {};
    if (type == ShaderType::DEFERRED) {
        range.stageFlags = vk::ShaderStageFlagBits::eFragment;
        range.offset = 0;
        range.size = 20;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &range;
    }

    //

    pipelineLayout = context.logical.createPipelineLayout(pipelineLayoutInfo);
    

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    
	pipeline = vk::raii::Pipeline(context.logical, nullptr, pipelineInfo);
    
}
