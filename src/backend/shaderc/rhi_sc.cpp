#include "include/rhi_sc.h"
#include "RootSignature.h"
#include "shaderc/shaderc.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <shaderc/shaderc.h>
#include <variant>
namespace RHI
{
    namespace ShaderCompiler
    {
        class ShaderCCompileOptions : public CompileOptions
        {
        public:
            ShaderCCompileOptions()
            {
                options.SetSourceLanguage(shaderc_source_language_hlsl);
            }
            shaderc::CompileOptions options;
        };
        class ShaderCCompiler : public Compiler
        {
        public:
            shaderc::Compiler compiler;
        };
        std::unique_ptr<CompileOptions> CompileOptions::New()
        {
            return std::make_unique<ShaderCCompileOptions>();
        }
        std::unique_ptr<Compiler> Compiler::New()
        {
            return std::make_unique<ShaderCCompiler>();
        }
        void CompileOptions::AddMacroDefinition(std::string_view name, std::optional<std::string_view> value)
        {
            auto opt = (ShaderCCompileOptions*)this;
            if (value)
            {
                opt->options.AddMacroDefinition(name.data(), name.size(), value->data(), value->size());
            }
            else
            {
                opt->options.AddMacroDefinition(std::string(name));
            }
        }
        void CompileOptions::SetOptimizationLevel(OptimizationLevel level)
        {
            auto opt = (ShaderCCompileOptions*)this;
            shaderc_optimization_level lv;
            switch(level)
            {
                using enum OptimizationLevel;   
                case None: lv = shaderc_optimization_level_zero; break;    
                case _1: lv = shaderc_optimization_level_size; break;    
                case _2: lv = shaderc_optimization_level_size; break;    
                case _3: lv = shaderc_optimization_level_performance; break;    
            }
            opt->options.SetOptimizationLevel(lv);
        }
        static shaderc_shader_kind ShaderKind(ShaderStage stg)
        {
            switch (stg)
            {
                using enum ShaderStage;
                case Vertex: return shaderc_vertex_shader;
                case Pixel: return shaderc_fragment_shader;
                case Geometry: return shaderc_geometry_shader;
                case Domain: return shaderc_tess_control_shader;
                case Hull: return shaderc_tess_evaluation_shader;
                case Compute: return shaderc_compute_shader;
                default: return shaderc_glsl_infer_from_source;
            }
        }
        CompilationError Compiler::CompileToFile(const ShaderSource& source, const CompileOptions& opt, const std::filesystem::path& output)
        {
            auto sc_opt = static_cast<const ShaderCCompileOptions*>(&opt);
            auto kind = ShaderKind(source.stage);
            auto cmp = (ShaderCCompiler*)this;
            if(std::holds_alternative<std::filesystem::path>(source.source))
            {
                auto& path = std::get<std::filesystem::path>(source.source);
                if(!std::filesystem::exists(path))
                {
                    return CompilationError::NonExistentFile;
                }
                std::ifstream file(path);
                std::string str;
                str.resize(std::filesystem::file_size(path));
                str.assign(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
                auto res = cmp->compiler.CompileGlslToSpv(str, kind, path.c_str(), sc_opt->options);
                
            }
            else
            {
                auto& src = std::get<ShaderSource::StringSource>(source.source);
                cmp->compiler.CompileGlslToSpv(src.shader.data(), src.shader.size(), kind, std::string(src.filename).c_str());
            }
        }
        void Compiler::CompileToBuffer(const ShaderSource& source, const CompileOptions& opt, std::vector<char>& output)
        {
            auto sc_opt = static_cast<const ShaderCCompileOptions*>(&opt);
        }
    }
}
