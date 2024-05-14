#pragma once

#include <Geode/loader/Log.hpp>

using namespace geode::prelude;

template <class... Args>
[[noreturn]]
void terminate(std::string const& reason, Args&&... args) {
    log::error(
        "{}! Causing a deliberate null-read to prevent data loss!",
        fmt::vformat(reason, fmt::make_format_args(args...))
    );

    // todo: use geode::utils::terminate once it's available on main
    volatile int* addr = nullptr;
    volatile int p = *std::launder(addr);
}

#define ASSERT_OR_TERMINATE(msg, ...) \
    auto GEODE_CONCAT(assert_res_, __LINE__) = (__VA_ARGS__); \
    if (GEODE_CONCAT(assert_res_, __LINE__).isErr()) { \
        terminate(msg " failed due to \"{}\"", std::move(GEODE_CONCAT(assert_res_, __LINE__).unwrapErr())); \
    }
