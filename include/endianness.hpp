#ifndef ENDIANNESS_HPP
#define ENDIANNESS_HPP

#include <bit>

namespace WTFarc {

	int inline littleToSystem(int num){
		if constexpr(std::endian::native==std::endian::big){
			return std::byteswap(num);
		}else{
			return num;
		}
	}

	int inline bigToSystem(int num){
		if constexpr(std::endian::native==std::endian::little){
			return std::byteswap(num);
		}else{
			return num;
		}
	}

};

#endif