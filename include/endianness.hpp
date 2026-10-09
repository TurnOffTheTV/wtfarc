#ifndef ENDIANNESS_HPP
#define ENDIANNESS_HPP

#include <bit>
#include <concepts>

namespace WTFarc {

	template <std::integral T>
	constexpr T inline littleToSystem(T num){
		if constexpr(std::endian::native==std::endian::big){
			return std::byteswap(num);
		}else{
			return num;
		}
	}

	template <std::integral T>
	constexpr T inline bigToSystem(T num){
		if constexpr(std::endian::native==std::endian::little){
			return std::byteswap(num);
		}else{
			return num;
		}
	}

};

#endif