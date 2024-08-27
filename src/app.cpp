#include "RootSignature.h"
#include "rhi_sc.h"
#include <argparse/argparse.hpp>
#include <memory>
#include <ostream>
#include <string>
#include <ranges>
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
void AddMacroDefns(argparse::ArgumentParser& parser, std::unique_ptr<RHI::ShaderCompiler::CompileOptions>& args)
{
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
}
int main(int argc, char** argv) 
{
    argparse::ArgumentParser parser("rhi_sc");
    parser.add_description("Shader Compiler For Pistachio's RHI");
    parser.add_argument("-i", "--inputs")
        .required()
        .nargs(argparse::nargs_pattern::at_least_one)
        .help("The HLSL source files (-i {FILENAMES})");
    parser.add_argument("-o", "--outputs")
        .required()
        .nargs(argparse::nargs_pattern::at_least_one)
        .help("Output Filename (-o {FILENAMES})");
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
    if (parser["-g"] == true)
    {
        args->EnableDebuggingSymbols();
    }
    AddMacroDefns(parser, args);
    args->SetOptimizationLevel(GetOptimizationLevel(parser));
    auto out_files = parser.get<std::vector<std::string>>("-o");
    auto names = parser.get<std::vector<std::string>>("-i");
    if(names.size() != out_files.size())
    {
        std::cerr << "Input and Output files must be the same length" << std::endl;
        return 1;
    }
    size_t num_files = names.size();
    auto cmp = RHI::ShaderCompiler::Compiler::New();
    for(auto i : std::views::iota(static_cast<decltype(num_files)>(0), num_files))
    {
        RHI::ShaderCompiler::ShaderSource src;
        src.source = std::filesystem::path(names[i]);
        src.stage = GetShaderStage(parser);
        auto res = cmp->CompileToFile(src, args, out_files[i]);
        if (res.error != RHI::ShaderCompiler::CompilationError::None)
        {
            std::cerr << res.messages;
            return 1;
        }
    }
    
    return 0;
}