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
            std::wstring StageToString(RHI::ShaderStage stg) const
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
            std::wstring OptimizationToString() const
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
            std::vector<std::wstring> AsArgs(RHI::ShaderStage stg) const
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
                args.push_back(L"-spirv");
                return args;
            }
            std::vector<const wchar_t*> AsRawArgs(std::vector<std::wstring>& args) const
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
            CComPtr<IDxcIncludeHandler> includeHandler;
            CComPtr<IDxcUtils> utils;
            DXCCompiler()
            {
                DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
                DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
                utils->CreateDefaultIncludeHandler(&includeHandler);
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
        DxcBuffer MakeBuffer(IDxcBlobEncoding** enc, DXCCompiler* cmp, const ShaderSource& src)
        {
            DxcBuffer buff;
            if(std::holds_alternative<std::filesystem::path>(src.source))
            {
                auto& path = std::get<std::filesystem::path>(src.source);
                auto res = cmp->utils->LoadFile(to_wstring(path.string()).c_str(), nullptr, enc);
                if(!SUCCEEDED(res))
                {
                    memset(&buff, 0, sizeof(buff));
                    return buff;
                }
                buff.Size = (*enc)->GetBufferSize();
                buff.Ptr = (*enc)->GetBufferPointer();
                buff.Encoding = DXC_CP_ACP;
            }  
            else
            {
                auto& str = std::get<ShaderSource::StringSource>(src.source);
                buff.Ptr = str.shader.data();
                buff.Size = str.shader.size();
                buff.Encoding = DXC_CP_ACP;
            }
            return buff;
        }
        CComPtr<IDxcResult> Compile(DXCCompiler* cmp, const ShaderSource& source, const std::unique_ptr<CompileOptions>& opt, CompilationResult& ret_val)
        {
            auto sc_opt = static_cast<const DXCCompileOptions*>(opt.get());
            auto args = sc_opt->AsArgs(source.stage);
            CComPtr<IDxcBlobEncoding> enc;
            auto buffer = MakeBuffer(&enc, cmp, source);
            if(!buffer.Ptr) 
            {
                ret_val.error = CompilationError::Error;
                ret_val.messages = "Internal Error Occurred, Check that the filename is valid";
                return nullptr;
            }
            CComPtr<IDxcResult> result;
            cmp->compiler->Compile(&buffer, sc_opt->AsRawArgs(args).data(), args.size(), cmp->includeHandler, IID_PPV_ARGS(&result));
            return result;
        }
        CompilationResult Compiler::CompileToFile(const ShaderSource& source, const std::unique_ptr<CompileOptions>& opt, const std::filesystem::path& output)
        {
            CompilationResult ret_val;
            auto result = Compile(static_cast<DXCCompiler*>(this), source, opt, ret_val);
            if(ret_val.error != CompilationError::None) return ret_val;
            CComPtr<IDxcBlobUtf8> pErrors = nullptr;
            result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
            if (pErrors != nullptr && pErrors->GetStringLength() != 0)
                ret_val.messages = pErrors->GetStringPointer();
            HRESULT status;result->GetStatus(&status);
            if(FAILED(status))
            {
                ret_val.error = CompilationError::Error;   
                return ret_val;
            }
            CComPtr<IDxcBlob> pShader = nullptr;
            CComPtr<IDxcBlobWide> pShaderName = nullptr;
            result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
            if(!pShader)
            {
                ret_val.error = CompilationError::Error;   
                return ret_val;
            }
            std::ofstream file(output);
            uint32_t spvSize = pShader->GetBufferSize();
            std::span spvSizeBytes = std::as_writable_bytes(std::span(&spvSize, 1));
            if(std::endian::native != std::endian::little)
            {
                std::reverse(spvSizeBytes.begin(), spvSizeBytes.end());
            }
            file.write((char*)&spvSize, sizeof(spvSize));
            file.write((const char*)pShader->GetBufferPointer(), spvSize);
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
            auto result = Compile(static_cast<DXCCompiler*>(this), source, opt, ret_val);
            if(ret_val.error != CompilationError::None) return ret_val;

            CComPtr<IDxcBlobUtf8> pErrors = nullptr;
            result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
            if (pErrors != nullptr && pErrors->GetStringLength() != 0)
                ret_val.messages = pErrors->GetStringPointer();
            HRESULT status;result->GetStatus(&status);
            if(FAILED(status))
            {
                ret_val.error = CompilationError::Error;   
                return ret_val;
            }
            CComPtr<IDxcBlob> pShader = nullptr;
            CComPtr<IDxcBlobWide> pShaderName = nullptr;
            result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
            if(!pShader)
            {
                ret_val.error = CompilationError::Error;   
                return ret_val;
            }

            uint32_t spvSize = pShader->GetBufferSize();
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
                offset = sizeof(uint32_t);
            }
            std::memcpy(output.data() + offset, pShader->GetBufferPointer(), spvSize);
            if(!memory_repr)
            {
                std::memset(output.data() + offset + spvSize, 0, sizeof(uint32_t) * 4);
            }
            return ret_val;
        }
    }
}
