#pragma once

#include <string>
#include <filesystem>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

struct UpdateTrashEvent : public Event {};

class Trashed : public CCObject {
protected:
    std::filesystem::path m_path;
    std::variant<Ref<GJGameLevel>, Ref<GJLevelList>> m_value;

    Trashed(std::filesystem::path const& path, GJGameLevel* level);
    Trashed(std::filesystem::path const& path, GJLevelList* list);

public:
    using Clock = std::chrono::file_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Unit = std::chrono::minutes;

    static std::vector<Ref<Trashed>> load();
    static Result<> trash(GJGameLevel* level);
    static Result<> trash(GJLevelList* list);
    
    GJGameLevel* asLevel() const;
    GJLevelList* asList() const;

    std::string getName() const;

    TimePoint getTrashTime() const;

    Result<> untrash();
    Result<> KABOOM();
};

std::filesystem::path getTrashDir();
std::string getFreeIDInDir(std::string const& name, std::filesystem::path const& dir, std::string const& ext);
