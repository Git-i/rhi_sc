#pragma once
#include <filesystem>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>
namespace RHI
{
    namespace ShaderCompiler
    {
        enum class OptimizationLevel
        {
            None, _1, _2, _3, Max=_3
        };
        class CompileOptions {
            void AddMacroDefinition(std::string_view name, std::optional<std::string_view> value);
            void SetOptimizationLevel(OptimizationLevel level);
        };
        class ShaderSource
        {
            struct StringSource
            {
                std::string shader;
                std::string_view filename;
            };
            std::variant<std::filesystem::path, StringSource> source;
        };
        class Compiler {
            void CompileToFile(const ShaderSource& source, const std::filesystem::path& output);
            void CompileToBuffer(const ShaderSource& source, std::vector<char>& output);
            [[deprecated("Use the std::vector implementation")]]
            void CompileToBuffer(const ShaderSource& source, char* output);
        };
    }
}
