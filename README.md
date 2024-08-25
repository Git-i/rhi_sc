# rhi_sc

rhi_sc is a shader compiler for [Pistachio's RHI](), it offers a library and an executable to perform shader compilation

## Platforms

It is actively developed and tested on linux but should work (maybe even better) on windows, as it uses shaderc (and possibly dxc in the future)

## How to Include

Add it as a subproject to an existing meson project and fetch the `rhi_sc_dep` variable to use the dependency, the preferred backend can be selected via meson options

## Example

The following code shows an example of using the library to compile and create a shader

```cpp
#include "rhi_sc.h"
#include "Device.h" //from the RHI

int main()
{
    RHI::Ptr<RHI::Device> device;
    //Initialize the RHI
    auto options = RHI::ShaderCompiler::CompileOptions::New();
    options->AddMacroDefinition(macro_name, std::nullopt);
    options->SetOptimizationLevel(RHI::ShaderCompiler::OptimizationLevel::Max);

    RHI::ShaderCompiler::ShaderSource src = std::filesystem::path("filepath.ext");
    auto compiler = RHI::ShaderCompiler::Compiler::New()
}
```

## Backend Support

### ShaderC

| API    | GLSL                                       | HLSL                                         |
| ------ | ------------------------------------------ | -------------------------------------------- |
| Vulkan | Planned                                    | <span style="color:green">Implemented</span> |
| DX12   | <span style="color:red">Not Planned</span> | <span style="color:red">Not Planned</span>   |

### DXC

| API    | GLSL                                       | HLSL    |
| ------ | ------------------------------------------ | ------- |
| Vulkan | <span style="color:red">Not Planned</span> | Planned |
| DX12   | <span style="color:red">Not Planned</span> | Planned |

## Using DXC without Windows SDK (linux or some other reason)

If you're on linux (or for some reason don't have dxc on windows), you should make sure the dxcompiler library and dxcapi, and WinAdapter header's are visible, this should be the case if you have the vulkan sdk, you can also place these files in the dxc_lib directory if you downloaded them from Mircrosoft's repo.

## DXC sounds like work (on linux). why set it up?

the DXC backend provides several benefits like:

- DX12 support (windows only)
- HLSL 2021
