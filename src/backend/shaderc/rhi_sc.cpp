#include "include/rhi_sc.h"
#include "FormatsAndTypes.h"
#include "RootSignature.h"
#include "shaderc/shaderc.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
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
        void CompileOptions::EnableDebuggingSymbols()
        {
            auto opt = (ShaderCCompileOptions*)this;
            opt->options.SetGenerateDebugInfo();
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
        shaderc::SpvCompilationResult Compile(ShaderCCompiler* cmp, const ShaderSource& source, const CompileOptions& opt, CompilationResult& ret_val)
        {
            shaderc::SpvCompilationResult result;
            auto sc_opt = static_cast<const ShaderCCompileOptions*>(&opt);
            auto kind = ShaderKind(source.stage);
            if(std::holds_alternative<std::filesystem::path>(source.source))
            {
                auto& path = std::get<std::filesystem::path>(source.source);
                if(!std::filesystem::exists(path))
                {
                    ret_val.error = CompilationError::NonExistentFile;
                    ret_val.messages = "File passed in was not found";
                    return result;
                }
                std::ifstream file(path);
                std::string str;
                str.resize(std::filesystem::file_size(path));
                str.assign(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
                result = cmp->compiler.CompileGlslToSpv(str, kind, path.c_str(), sc_opt->options);
            }
            else
            {
                auto& src = std::get<ShaderSource::StringSource>(source.source);
                result = cmp->compiler.CompileGlslToSpv(src.shader.data(), src.shader.size(), kind, std::string(src.filename).c_str());
            }
            ret_val.messages = result.GetErrorMessage();
            if(result.GetCompilationStatus() != shaderc_compilation_status_success)
            {
                ret_val.error = CompilationError::Error;
                return result;
            }
            ret_val.error = CompilationError::None;
            return result;
        }
        CompilationResult Compiler::CompileToFile(const ShaderSource& source, const CompileOptions& opt, const std::filesystem::path& output)
        {
            CompilationResult ret_val;
            auto result = Compile(static_cast<ShaderCCompiler*>(this), source, opt, ret_val);
            if(ret_val.error != CompilationError::None) return ret_val;
            
            std::ofstream file(output);
            uint32_t spvSize = (result.end() - result.begin()) * sizeof(decltype(result)::element_type);
            std::span spvSizeBytes = std::as_writable_bytes(std::span(&spvSize, 1));
            if(std::endian::native != std::endian::little)
            {
                std::reverse(spvSizeBytes.begin(), spvSizeBytes.end());
            }
            file.write((char*)&spvSize, sizeof(spvSize));
            file.write((const char*)result.begin(), spvSize);
            uint32_t zero[4] = {0,0,0,0};
            file.write((char*)zero, sizeof(uint32_t) * 4);
            return ret_val;
        }
        [[nodiscard]] CompilationResult Compiler::CompileToBuffer(RHI::API api, const ShaderSource& source, const CompileOptions& opt, std::vector<char>& output)
        {
            CompilationResult ret_val;
            if(api != RHI::API::Vulkan)
            {
                ret_val.messages = "Only Vulkan API shaders supported";
                ret_val.error = CompilationError::APINotAvailable;
                return ret_val;
            }
            auto result = Compile(static_cast<ShaderCCompiler*>(this), source, opt, ret_val);
            if(ret_val.error != CompilationError::None) return ret_val;

            uint32_t spvSize = (result.end() - result.begin()) * sizeof(decltype(result)::element_type);
            output.resize(spvSize);
            std::memcpy(output.data(), result.begin(), spvSize);
            return ret_val;
        }
    }
}
