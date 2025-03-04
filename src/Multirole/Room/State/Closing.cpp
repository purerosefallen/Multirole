#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::Closing& /*unused*/) noexcept
{
	for(const auto& kv : duelists)
		kv.second->Disconnect();
	{
		std::scoped_lock lock(mDuelists);
		duelists.clear();
	}
	for(const auto& c : spectators)
		c->Disconnect();
	spectators.clear();
	return std::nullopt;
}

StateOpt Context::operator()(State::Closing& /*unused*/, const Event::Join& e) noexcept
{
	e.client.Disconnect();
	return std::nullopt;
}

} // namespace Ignis::Multirole::Room
