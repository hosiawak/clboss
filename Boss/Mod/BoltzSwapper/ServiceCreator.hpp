#ifndef BOSS_MOD_BOLTZSWAPPER_SERVICECREATOR_HPP
#define BOSS_MOD_BOLTZSWAPPER_SERVICECREATOR_HPP

#include"Boss/Mod/BoltzSwapper/ServiceModule.hpp"
#include"Boss/Msg/Network.hpp"
#include<map>
#include<memory>
#include<string>
#include<vector>

namespace Boltz { class EnvIF; }
namespace Ev { class ThreadPool; }
namespace S { class Bus; }

namespace Boss { namespace Mod { namespace BoltzSwapper {

/** class Boss::Mod::BoltzSwapper::ServiceCreator
 *
 * @brief Constructs `ServiceModule` objects and
 * keeps them alive.
 */
class ServiceCreator {
private:
	S::Bus& bus;
	Ev::ThreadPool& threadpool;
	Boltz::EnvIF& env;

	/* Known BOLTZ instances and the networks they are for.  */
	std::map<Boss::Msg::Network, std::vector<std::string>> const&
	instances;

	/* The actual services we are maintaining.  */
	std::vector<std::unique_ptr<Boss::Mod::BoltzSwapper::ServiceModule>>
	services;

	/* Construction is performed via this object.  */
	class Core;
	void start();

public:
	ServiceCreator() =delete;
	ServiceCreator(ServiceCreator&&) =delete;
	ServiceCreator(ServiceCreator const&) =delete;

	ServiceCreator( S::Bus& bus_
		      , Ev::ThreadPool& threadpool_
		      , Boltz::EnvIF& env_
		      , std::map<Boss::Msg::Network, std::vector<std::string>> const& instances_
		      ) : bus(bus_)
			, threadpool(threadpool_)
			, env(env_)
			, instances(instances_)
			, services()
			{ start(); }
};

}}}

#endif /* !defined(BOSS_MOD_BOLTZSWAPPER_SERVICECREATOR_HPP) */
