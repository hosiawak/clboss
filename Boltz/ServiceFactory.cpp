#include"Boltz/Detail/ServiceImpl.hpp"
#include"Boltz/Service.hpp"
#include"Boltz/ServiceFactory.hpp"
#include"Ev/Io.hpp"
#include"Ev/ThreadPool.hpp"
#include"Ev/yield.hpp"
#include"Secp256k1/SignerIF.hpp"
#include"Sqlite3.hpp"
#include"Util/make_unique.hpp"

namespace Boltz {

class ServiceFactory::Impl {
private:
	Ev::ThreadPool& threadpool;
	Sqlite3::Db db;
	Secp256k1::SignerIF& signer;
	Boltz::EnvIF& env;
	std::string proxy;

	/* nullptr if we are currently initializing,
	 * pointer to false if currently uninitialized,
	 * pointer to true if already initialized.  */
	std::unique_ptr<bool> initted_flag;

	Ev::Io<void> try_initialize() {
		return Ev::yield().then([this]() {
			if (!initted_flag)
				/* Busy wait.  */
				return Ev::yield().then([this]() {
					return try_initialize();
				});
			if (*initted_flag)
				/* Do nothing.  */
				return Ev::lift();
			/* Start initializing.  */
			initted_flag = nullptr;
			return do_initialize();
		});
	}
	Ev::Io<void> do_initialize() {
		/* Perform actual initialization.  */
		return db.transact().then([this](Sqlite3::Tx tx) {
			tx.query_execute(R"QRY(
			-- reverse submarine swaps.
			CREATE TABLE IF NOT EXISTS "BoltzServiceFactory_rsub"
			     ( id INTEGER PRIMARY KEY

			     -- The api access point that generated
			     -- this row.
			     , apiAccess TEXT NOT NULL

			     -- We generated these.
			     -- privkey used to tweak the signer.
			     -- these, and other data, are in hex.
			     , tweak TEXT NOT NULL
			     -- LN preimage.
			     , preimage TEXT NOT NULL
			     -- onchain address the C-Lightning node
			     -- controls.
			     , destinationAddress TEXT NOT NULL

			     -- from exchange, before paying invoice.
			     -- id from exchange.
			     , swapId TEXT NOT NULL
			     , redeemScript TEXT NOT NULL
			     , timeoutBlockheight INTEGER NOT NULL
			     -- in satoshi.
			     , onchainAmount INTEGER NOT NULL

			     -- whether lockup fields have been set.
			     -- 0 == false, 1 == true
			     , lockedUp INTEGER NOT NULL

			     -- from the exchange, after paying invoice.
			     -- These are the details of the lockup
			     -- transaction output.
			     , lockupTxid TEXT NULL
			     -- The output of the lockupTxid that contains
			     -- the value spendable by the redeem script.
			     , lockupOut INTEGER NULL
			     -- The blockheight at which we believe the
			     -- tx was confirmed.
			     , lockupConfirmedHeight INTEGER NULL

			     -- We generated these on seeing that
			     -- the transaction from the service
			     -- confirmed.
			     -- in satoshi.
			     , lockupClaimFees INTEGER NULL

			     -- additional data we might need.
			     , comment TEXT
			     );
			CREATE INDEX IF NOT EXISTS
			       "BoltzServiceFactory_rsub_swapid"
			    ON "BoltzServiceFactory_rsub"
			     ( swapId
			     , apiAccess
			     );
			)QRY");
			tx.commit();

			initted_flag = Util::make_unique<bool>(true);

			return Ev::yield();
		});
	}

public:
	Impl() =delete;

	Impl( Ev::ThreadPool& threadpool_
	    , Sqlite3::Db db_
	    , Secp256k1::SignerIF& signer_
	    , Boltz::EnvIF& env_
	    , std::string proxy_
	    ) : threadpool(threadpool_)
	      , db(std::move(db_))
	      , signer(signer_)
	      , env(env_)
	      , proxy(std::move(proxy_))
	      , initted_flag(Util::make_unique<bool>(false))
	      { }

	Ev::Io<std::unique_ptr<Service>>
	create_service(std::string const& api_endpoint) {
		return try_initialize().then([this, api_endpoint]() {
			auto ptr = std::unique_ptr<Service>();
			ptr = Util::make_unique<Detail::ServiceImpl>
				( threadpool
				, db
				, signer
				, env
				, proxy
				, api_endpoint
				);
			return Ev::lift(std::move(ptr));
		});
	}
};

ServiceFactory::ServiceFactory(ServiceFactory&&) =default;
ServiceFactory& ServiceFactory::operator=(ServiceFactory&&) =default;
ServiceFactory::~ServiceFactory() =default;

ServiceFactory::ServiceFactory( Ev::ThreadPool& threadpool
			      , Sqlite3::Db db
			      , Secp256k1::SignerIF& signer
			      , Boltz::EnvIF& env
			      , std::string proxy
			      ) : pimpl(Util::make_unique<Impl>( threadpool
							       , std::move(db)
							       , signer
							       , env
							       , std::move(proxy)
							       ))
				{ }

Ev::Io<std::unique_ptr<Service>>
ServiceFactory::create_service(std::string const& api_endpoint) {
	return pimpl->create_service(api_endpoint);
}

}
