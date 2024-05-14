#include "Trashcan.hpp"
#include "TrashLevelEvent.hpp"

static ByteVector timeToBytes(trashcan::TimePoint time) {
	return toByteArray(std::chrono::duration_cast<trashcan::Unit>(time.time_since_epoch()).count());
}

Trashcan* Trashcan::get() {
    static auto inst = new Trashcan();
    return inst;
}

ghc::filesystem::path Trashcan::getPath() const {
    return save::getLevelsSaveDir() / "trashcan";
}
void Trashcan::onLoad(ghc::filesystem::path const& dir, GJGameLevel* level) {
    auto timeBin = file::readBinary(dir / ".trashtime").unwrapOr(timeToBytes(trashcan::Clock::now()));
    trashcan::Unit dur(*reinterpret_cast<trashcan::Unit::rep*>(timeBin.data()));
    m_levels.push_back(Level {
        .level = level,
        .trashTime = trashcan::TimePoint(dur)
    });
}
void Trashcan::onAdd(GJGameLevel* level, bool isNew) {
	// Save the trashing time
	auto time = trashcan::Clock::now();
	(void)file::writeBinary(CategoryInfo::from(level)->getSaveDir() / ".trashtime", timeToBytes(time));
    m_levels.push_back(Level {
        .level = level,
        .trashTime = time
    });

    if (!isNew) {
        auto ev = TrashLevelEvent();
        ev.m_impl->level = level;
        ev.m_impl->mode = TrashLevelEvent::Impl::Trash;
        ev.post();
    }
}
void Trashcan::onRemove(GJGameLevel* level) {
	ranges::remove(m_levels, [&](auto trashed) { return trashed.level == level; });
}
std::vector<Trashcan::Level> Trashcan::getAllLevels() const {
    return m_levels;
}

Result<> Trashcan::clear() {
	if (!ghc::filesystem::exists(this->getPath())) {
		return Ok();
	}
	
	std::error_code ec;
	ghc::filesystem::remove_all(this->getPath(), ec);
	if (ec) {
		return Err("Unable to delete the trashcan directory: {} (error code {})", ec.message(), ec.value());
	}
    m_levels.clear();

	auto ev = TrashLevelEvent();
	ev.m_impl->level = nullptr;
	ev.m_impl->mode = TrashLevelEvent::Impl::Delete;
	ev.post();

	return Ok();
}
