#include "RootSignature.h"
#include "rhi_sc.h"
#include <argparse/argparse.hpp>
#include <ostream>
#include <string>
#include <string_view>
RHI::ShaderCompiler::OptimizationLevel GetOptimizationLevel(argparse::ArgumentParser& parser)
{
    if(parser["-ONone"] == true)
    {
        return RHI::ShaderCompiler::OptimizationLevel::None;
    }
    else if (parser["-O1"] == true)
    {
        return RHI::ShaderCompiler::OptimizationLevel::_1;
    }
    else if (parser["-O2"] == true)
    {
        return RHI::ShaderCompiler::OptimizationLevel::_2;
    }
    else if (parser["-O3"] == true)
    {
        return RHI::ShaderCompiler::OptimizationLevel::_3;
    }
    else if (parser["-OFast"] == true)
    {
        return RHI::ShaderCompiler::OptimizationLevel::Max;
    }
    return RHI::ShaderCompiler::OptimizationLevel::None;
}
RHI::ShaderStage GetShaderStage(argparse::ArgumentParser& parser)
{
    auto stg = parser.get("-t");
    std::transform(stg.begin(), stg.end(), stg.begin(),
    [](unsigned char c){ return std::tolower(c); });
    if (stg == "pixel")
        return RHI::ShaderStage::Pixel;
    else if (stg == "vertex")
        return RHI::ShaderStage::Vertex;
    else if (stg == "compute")
        return RHI::ShaderStage::Compute;
    else if (stg == "hull")
        return RHI::ShaderStage::Hull;
    else if (stg == "domain")
        return RHI::ShaderStage::Domain;
    else if (stg == "geometry")
        return RHI::ShaderStage::Geometry;
    else
        return RHI::ShaderStage::None;
}
int main(int argc, char** argv) 
{
    argparse::ArgumentParser parser("rhi_sc");
    parser.add_description("Shader Compiler For Pistachio's RHI");
    parser.add_argument("filename").help("The HLSL source file");
    parser.add_argument("-o", "--output")
        .required()
        .help("Output Filename (-o {FILENAME})");
    parser.add_argument("-t", "--target")
        .required()
        .help("Target Shader Stage (-t [pixel | vertex | compute | hull | domain | geometry ] - case insensitive)");
    parser.add_argument("-g", "--embed-debug")
        .flag()
        .help("Embed Debug Info");
    parser.add_argument("-D")
        .help("Define Macro (-D name or -D name=value)")
        .append();
    auto& grp = parser.add_mutually_exclusive_group();
    grp.add_argument("-ONone").flag().help("Disable Optimization");
    grp.add_argument("-O1").flag().help("Optimization Level 1");
    grp.add_argument("-O2").flag().help("Optimization Level 2");
    grp.add_argument("-O3").flag().help("Optimization Level 3");
    grp.add_argument("-OFast").flag().help("Same as -O3");
    parser.parse_args(argc, argv);
    auto args = RHI::ShaderCompiler::CompileOptions::New();
    auto out_file = parser.get("-o");
    if (parser["-g"] == true)
    {
        args->EnableDebuggingSymbols();
    }
    auto macros = parser.get<std::vector<std::string>>("-D");
    for(auto& macro : macros)
    {
        auto name = macro.find('=');
        if(name == std::string::npos)
        {
            std::cout << "name = " << macro << std::endl;
            args->AddMacroDefinition(macro, std::nullopt);
        }
        else {
            auto mac_name = std::string_view(macro.data(), name);
            auto mac_value = std::string_view(macro.data() + name + 1, macro.size() - name -1);
            args->AddMacroDefinition(mac_name, mac_value);
        }
    }
    args->SetOptimizationLevel(GetOptimizationLevel(parser));
    auto name = parser.get("filename");
    auto cmp = RHI::ShaderCompiler::Compiler::New();
    RHI::ShaderCompiler::ShaderSource src;
    src.source = std::filesystem::path(name);
    src.stage = GetShaderStage(parser);
    auto res = cmp->CompileToFile(src, args, out_file);
    if (res.error != RHI::ShaderCompiler::CompilationError::None)
    {
        std::cerr << res.messages;
        return 1;
    }
    return 0;
}