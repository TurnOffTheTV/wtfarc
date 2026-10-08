#ifndef WTFARC_ERROR_HPP
#define WTFARC_ERROR_HPP

#include <stdexcept>

namespace WTFarc {

	class wtfarc_error : public std::runtime_error {
		public:
			using std::runtime_error::runtime_error; 
	};

}

#endif