#include "include/download_whisper_model.h"
#include "include/get_exec_path.h"

void download_whisper_model(int model_index)
{
    static constexpr std::array<const char*, 5> k_models = {
        "tiny",
        "base",
        "small",
        "medium",
        "large"
    };

    if (model_index < 0 || model_index >= static_cast<int>(k_models.size()))
        throw std::out_of_range("modelIndex must be 0–4 (tiny/base/small/medium/large)");

    const std::string model = k_models[model_index];

    const std::filesystem::path exe_dir = get_executable_path().parent_path();

#ifdef _WIN32
    const std::filesystem::path models_dir = exe_dir / "whisper.cpp" / "models";
    const std::string cmd = "cd /d \"" + models_dir.string() + "\" && \"" +
                            (models_dir / "download-ggml-model.cmd").string() + "\" " + model;
#else
    const std::filesystem::path models_dir = exe_dir / "whisper.cpp" / "models";
    const std::string cmd = "cd \"" + models_dir.string() + "\" && bash \"" +
                            (models_dir / "download-ggml-model.sh").string() + "\" " + model;
#endif

    const int ret = std::system(cmd.c_str());

    if (ret != 0)
        throw std::runtime_error("Download failed for model '" + model +
                                 "' (exit code " + std::to_string(ret) + ")");
}