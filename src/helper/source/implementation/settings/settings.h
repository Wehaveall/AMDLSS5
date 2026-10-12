#pragma once
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

namespace nr {
struct Settings { bool pre_upscale = false; unsigned percent = 100; };
inline bool operator==(Settings a, Settings b) { return a.pre_upscale == b.pre_upscale && a.percent == b.percent; }
inline bool operator!=(Settings a, Settings b) { return !(a == b); }
struct Extent { unsigned width, height; };
enum class Control { PreUpscale, Scale };
enum class Key { Left, Right, Enter };
enum class Readiness { Uninitialised, Pending, Ready, Failed };
struct Snapshot {
    Settings requested{};
    std::optional<Settings> effective;
    std::uint64_t generation = 1;
    std::uint64_t effective_generation = 0;
    Readiness readiness = Readiness::Uninitialised;
    std::string diagnostic;
};
struct Operation { bool ok; bool persisted; bool changed; std::string diagnostic; };
std::optional<Extent> scaled_extent(Extent stage, unsigned percent);

class SettingsStore {
public:
    explicit SettingsStore(std::filesystem::path selected_ini);
    Snapshot snapshot() const;
    Operation request(Settings value);
    Operation control(Control control, Key key);
    Operation reload();

    bool commit_if_current(std::uint64_t generation);
    bool fail_if_current(std::uint64_t generation, std::string diagnostic);
private:
    Operation request_locked(Settings value);
    void accept_locked(Settings value);
    const std::filesystem::path selected_ini_;
    mutable std::mutex mutex_;
    Snapshot state_;
};
}
