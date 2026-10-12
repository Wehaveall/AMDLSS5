#pragma once
#include <cstddef>
#include <cstdint>

namespace nrliveimage {
inline constexpr std::size_t image_size = 0x73d000;
inline constexpr std::size_t file_size = 7417344;
inline constexpr std::uint32_t callback_rva = 0x72CFF0;
inline constexpr char file_sha256[] = "28609e75a8917e47f370a67745a359d6605bcf6f1eb10153c45e94fa80e040dc";
enum class VerificationStage : std::uint32_t {
    Verified, InvalidInput, HeaderProtection, ModuleIdentity, ModulePath,
    DiskOpen, DiskSize, DiskRead, DiskDigest, SpanProtection, SpanRead,
    SpanMismatch, CallbackProtection, CallbackRead, CallbackPublished, Exception
};
struct VerificationDiagnostic {
    VerificationStage stage{VerificationStage::InvalidInput};
    std::uint32_t span_rva{};
    std::uint32_t win32_error{};
    std::uint32_t actual_protection{};
    std::uint32_t mismatch_rva{};
    std::uint32_t expected_byte{};
    std::uint32_t observed_byte{};
    std::uint32_t window_size{};
    std::uint64_t expected_window{};
    std::uint64_t observed_window{};
};
const char* verification_stage_name(VerificationStage stage) noexcept;

bool inspect_mapped_snapshot(const void* data, std::size_t size) noexcept;
bool inspect_mapped_snapshot(const void* data, std::size_t size, VerificationDiagnostic* diagnostic) noexcept;

bool inspect_mapped_snapshot(const void* data, std::size_t size, VerificationDiagnostic* diagnostic, std::uint64_t validated_load_base) noexcept;
bool inspect_disk_snapshot(const void* data, std::size_t size) noexcept;

bool verify_live_image(void* base) noexcept;
bool verify_live_image(void* base, VerificationDiagnostic* diagnostic) noexcept;
namespace detail {

bool sha256_matches(const void* data, std::size_t size, const std::uint8_t* digest32) noexcept;
}
}
