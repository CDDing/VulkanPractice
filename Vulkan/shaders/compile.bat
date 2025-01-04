@echo off
setlocal enabledelayedexpansion

:: Vulkan SDK glslc 경로 설정
set GLSLC_PATH=C:\VulkanSDK\1.3.261.1\Bin\glslc.exe

:: 현재 디렉토리에서 모든 .vert와 .frag 파일 처리
for %%f in (*.vert *.frag) do (
    echo Compiling %%f to %%f.spv...
    "%GLSLC_PATH%" %%f -o %%f.spv
)

echo All shaders compiled successfully.
pause