#include "rhi_sc.h"
#include <argparse/argparse.hpp>
#include <string>
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
int main(int argc, char** argv) 
{
    argparse::ArgumentParser parser("rhi_sc");
    parser.add_argument("-o", "--output")
        .required()
        .help("Output Filename (-o {FILENAME})");
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
        std::cout << macro << std::endl;
    }
    args->SetOptimizationLevel(GetOptimizationLevel(parser));
    
    std::cout << out_file << std::endl;
}