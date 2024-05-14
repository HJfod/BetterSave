#include "TrashLevelEvent.hpp"

TrashLevelEvent::TrashLevelEvent() : m_impl(std::make_unique<TrashLevelEvent::Impl>()) {}
TrashLevelEvent::~TrashLevelEvent() {}

GJGameLevel* TrashLevelEvent::getLevel() const {
	return m_impl->level;
}
bool TrashLevelEvent::isPermanentDelete() const {
	return m_impl->mode == Impl::Delete;
}
bool TrashLevelEvent::isTrash() const {
	return m_impl->mode == Impl::Trash;
}
bool TrashLevelEvent::isUntrash() const {
	return m_impl->mode == Impl::Untrash;
}
