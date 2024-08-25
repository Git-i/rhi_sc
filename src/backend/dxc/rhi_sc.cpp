#include "include/rhi_sc.h"
#include "FormatsAndTypes.h"
#include "RootSignature.h"
#include "WinAdapter.h"
#include "dxcapi.h"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <variant>
namespace RHI
{
    namespace ShaderCompiler
    {
        std::wstring to_wstring(const std::string& stringToConvert)
        {
            std::wstring wideString = 
                std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(stringToConvert);
            return wideString;
        }
        class DXCCompileOptions : public CompileOptions
        {
        public:
            bool debug;
            OptimizationLevel level;
            std::vector<std::wstring> macros;
            DXCCompileOptions()
            {
            }
            std::wstring StageToString(RHI::ShaderStage stg)
            {
                switch (stg)
                {
                    using enum ShaderStage;
                    case Vertex: return L"vs_6_0";
                    case Pixel: return L"ps_6_0";
                    case Geometry: return L"gs_6_0";
                    case Domain: return L"ds_6_0";
                    case Hull: return L"hs_6_0";
                    case Compute: return L"cs_6_0";
                    default: return L"";
                }
            }
            std::wstring OptimizationToString()
            {
                switch (level)
                {
                    using enum OptimizationLevel;
                    case None: return L"-Od";
                    case _1: return L"-O1";
                    case _2: return L"-O2";
                    case _3: return L"-O3";
                }
            }
            std::vector<std::wstring> AsArgs(RHI::ShaderStage stg)
            {
                std::vector<std::wstring> args = {};
                args.emplace_back(L"-T");
                args.emplace_back(StageToString(stg));
                if(debug)
                {
                    args.emplace_back(L"-Zi");
                    args.emplace_back(L"-Qembed_debug");
                }
                args.emplace_back(OptimizationToString());
                for(auto& macro: macros)
                {
                    args.emplace_back(L"-D");
                    args.emplace_back(macro.data());
                }
                return args;
            }
            std::vector<const wchar_t*> AsRawArgs(std::vector<std::wstring>& args)
            {
                std::vector<const wchar_t*> raw;
                for(auto& arg: args)
                    raw.push_back(arg.c_str());
                return raw;
            }
        };
        class DXCCompiler : public Compiler
        {
        public:
            CComPtr<IDxcCompiler3> compiler;
            DXCCompiler()
            {
                DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
            }
        };
        std::unique_ptr<CompileOptions> CompileOptions::New()
        {
            return std::make_unique<DXCCompileOptions>();
        }
        std::unique_ptr<Compiler> Compiler::New()
        {
            return std::make_unique<DXCCompiler>();
        }
        void CompileOptions::AddMacroDefinition(std::string_view name, std::optional<std::string_view> value)
        {
            auto opt = (DXCCompileOptions*)this;
            std::wstring& wname = opt->macros.emplace_back(name.size(), L' ');
            wname.reserve(mbstowcs(wname.data(), name.data(), name.size()));
            if(value)
            {
                wname += L'=';
                wname += to_wstring(std::string(*value));   
            }
        }
        void CompileOptions::EnableDebuggingSymbols()
        {
            auto opt = (DXCCompileOptions*)this;
            opt->debug = true;
        }
        void CompileOptions::SetOptimizationLevel(OptimizationLevel level)
        {
            auto opt = (DXCCompileOptions*)this;
            opt->level = level;
        }
        /*
        shaderc::SpvCompilationResult Compile(ShaderCCompiler* cmp, const ShaderSource& source, const std::unique_ptr<CompileOptions>& opt, CompilationResult& ret_val)
        {
            shaderc::SpvCompilationResult result;
            auto sc_opt = static_cast<const ShaderCCompileOptions*>(opt.get());
            auto kind = ShaderKind(source.stage);
            if(kind == shaderc_glsl_infer_from_source)
            {
                ret_val.error = CompilationError::InvalidStage;
                ret_val.messages = "Invalid Shader Stage Specified";
            }
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
        CompilationResult Compiler::CompileToFile(const ShaderSource& source, const std::unique_ptr<CompileOptions>& opt, const std::filesystem::path& output)
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
        [[nodiscard]] CompilationResult Compiler::CompileToBuffer(RHI::API api, const ShaderSource& source, const std::unique_ptr<CompileOptions>& opt, std::vector<char>& output, bool memory_repr)
        {
            CompilationResult ret_val;
            if(memory_repr && api != RHI::API::Vulkan)
            {
                ret_val.messages = "Only Vulkan API shaders supported";
                ret_val.error = CompilationError::APINotAvailable;
                return ret_val;
            }
            auto result = Compile(static_cast<ShaderCCompiler*>(this), source, opt, ret_val);
            if(ret_val.error != CompilationError::None) return ret_val;

            uint32_t spvSize = (result.end() - result.begin()) * sizeof(decltype(result)::element_type);
            uint32_t extraBytes = memory_repr ? 0 : sizeof(uint32_t) * 5;
            output.resize(spvSize + extraBytes);
            uint32_t offset = 0;
            if(!memory_repr)
            {
                uint32_t spvSizeCpy = spvSize;
                std::span spvSizeBytes = std::as_writable_bytes(std::span(&spvSizeCpy, 1));
                if(std::endian::native != std::endian::little)
                {
                    std::reverse(spvSizeBytes.begin(), spvSizeBytes.end());
                }
                std::memcpy(output.data(), spvSizeBytes.data(), sizeof(uint32_t));
            }
            std::memcpy(output.data() + offset, result.begin(), spvSize);
            if(!memory_repr)
            {
                std::memset(output.data() + offset + spvSize, 0, sizeof(uint32_t) * 4);
            }
            return ret_val;
        }
        */
    }
}
