#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>
#include "Core.h"
#include "RootSignature.h"
namespace RHI
{
    namespace ShaderCompiler
    {
        enum class CompilationError
        {
            None,
            NonExistentFile
        };
        enum class OptimizationLevel
        {
            None, _1, _2, _3, Max=_3
        };
        class CompileOptions {
        protected:
            DECL_CLASS_CONSTRUCTORS(CompileOptions);
        public:
            static std::unique_ptr<CompileOptions> New();
            void AddMacroDefinition(std::string_view name, std::optional<std::string_view> value);
            void SetOptimizationLevel(OptimizationLevel level);
        };
        class ShaderSource
        {
        public:
            struct StringSource
            {
                std::string_view shader;
                std::string_view filename;
            };
            std::variant<std::filesystem::path, StringSource> source;
            ShaderStage stage;
        };
        class Compiler {
        protected:
            DECL_CLASS_CONSTRUCTORS(Compiler);
        public:
            static std::unique_ptr<Compiler> New();
            [[nodiscard]] CompilationError CompileToFile(const ShaderSource& source, const CompileOptions& opt, const std::filesystem::path& output);
            void CompileToBuffer(const ShaderSource& source, const CompileOptions& opt, std::vector<char>& output);
        };
    }
}
