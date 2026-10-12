#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace nrtransition {
using Id = std::uint64_t;
enum class Stage { Pre, Post };
enum class State { Initializing, Active, Closing, WaitingForSubmission,
                   WaitingForCompletion, Quiescing, Resetting, FailedFsrOnly };
enum class Kind { RecordedList, SubmissionBegin, SubmitList, JobPublished,
                  SubmissionEnd, EnqueuersClosed, SubmissionsClosed,
                  HipCompleted, FenceSignaled, FenceCompleted, BeginReset,
                  ResetFinished, InitChecked, EvidenceLost, BeginQuiesce,
                  GenerationHipCompleted, RetireCompleted, RetireGraphicsCompleted };
struct Token { Id generation{}; Id serial{}; };
struct Event {
    Kind kind{};
    Id generation{};
    Id device{};
    Id queue{};
    Id id{};
    Id related{};
    bool success{true};
};
struct Config {
    Id device{1}; Id queue{1}; Stage initial{Stage::Pre}; bool enabled{true};
    Id first_generation{1}; Id deadline_ms{5000};
};
class Controller {
public:
    static constexpr std::size_t Capacity = 128;
    explicit Controller(Config config, Id now_ms = 0);
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(Controller&&) = delete;
    void request(Stage stage, Id now_ms);
    void request_rebuild(Stage stage, Id request_id, Id now_ms);
    void request_mode_change(Id now_ms);
    void set_enabled(bool enabled) noexcept;
    void accept(const Event& event, Id now_ms);
    void tick(Id now_ms);
    State state() const noexcept;
    Id generation() const noexcept;
    Stage requested() const noexcept;
    Stage initializing_stage() const noexcept;
    std::optional<Stage> effective() const noexcept;
    std::optional<Token> reset_authorization() const noexcept;
    bool neural_allowed() const noexcept;
    bool initialization_capture_allowed() const noexcept;
    bool retain_resources() const noexcept;
    const char* failure() const noexcept;
private:
    struct Entry { Id id{}; Id related{}; bool done{}; };
    Config config_;
    State state_{State::Initializing};
    Stage requested_; Stage initializing_;
    std::optional<Stage> effective_;
    std::optional<Token> token_;
    Id generation_; Id serial_{}; Id last_time_{}; Id started_{}; Id fence_{};
    Id rebuild_request_{};
    bool enabled_; bool recording_closed_{}; bool submissions_closed_{};
    bool fence_done_{}; const char* failure_{""};
    bool generation_hip_done_{};
    bool initialization_started_{};
    bool initialization_deadline_armed_{};
    Id last_list_{}, last_submission_{}, last_job_{}, last_retire_fence_{};
    std::array<Entry, Capacity> lists_{}, submissions_{}, jobs_{};
    std::size_t list_count_{}, submission_count_{}, job_count_{};
    void fail(const char* reason) noexcept;
    void close(Id now_ms);
    void advance();
    Entry* find(std::array<Entry, Capacity>& entries, std::size_t count, Id id);
    Entry* add(std::array<Entry, Capacity>& entries, std::size_t& count, Id id);
};
}
