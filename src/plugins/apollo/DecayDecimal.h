#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_PLUGINS_APOLLO_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_PLUGINS_APOLLO_H

#include "gradido_blockchain/lib/DecayDecimal.h"

#define MAGIC_NUMBER_DIFF_TO_NODE_DECAY_CALCULATION_THRESHOLD 0.0005
#define MAGIC_NUMBER_GREGORIAN_CALENDER_SECONDS_PER_YEAR 31556952
#define MAGIC_NUMBER_APOLLO_PRECISION 25

namespace plugin {
	namespace apollo {
		class DecayDecimal : public ::DecayDecimal
		{
		public: 
			using ::DecayDecimal::DecayDecimal;
			
			/*!
			 *  use alternative algorithms and round after each step down to MAGIC_NUMBER_APOLLO_PRECISION to mimic apollo calculation as closely as possible
			 *  gradido/(2^(duration.totalSeconds()/MAGIC_NUMBER_GREGORIAN_CALENDER_SECONDS_PER_YEAR))
			 *  wolframalpha has shown me this way: https://www.wolframalpha.com/input?i=%28e%5E%28%28-ln+2%29+%2F+31556952%29%29%5E31556951
		     */
			void applyDecay(Poco::Timespan duration);

			bool isSimilarEnough(const Decimal& b);
		};
	}
}


#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_PLUGINS_APOLLO_H