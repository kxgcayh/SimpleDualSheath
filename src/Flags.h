#pragma once

// Minimal replacement for the old framework's stl::flag + enum bitwise helpers.
namespace SDS
{
	template <class E>
	class flag
	{
	public:
		using underlying_type = std::underlying_type_t<E>;

		constexpr flag() noexcept = default;
		constexpr flag(E a_value) noexcept :
			_bits(static_cast<underlying_type>(a_value))
		{}

		[[nodiscard]] constexpr bool test(E a_value) const noexcept
		{
			return (_bits & static_cast<underlying_type>(a_value)) == static_cast<underlying_type>(a_value);
		}

		[[nodiscard]] constexpr bool test_any(E a_value) const noexcept
		{
			return (_bits & static_cast<underlying_type>(a_value)) != 0;
		}

		constexpr flag& operator|=(E a_value) noexcept
		{
			_bits |= static_cast<underlying_type>(a_value);
			return *this;
		}

		constexpr flag& operator|=(flag a_rhs) noexcept
		{
			_bits |= a_rhs._bits;
			return *this;
		}

		[[nodiscard]] constexpr underlying_type underlying() const noexcept { return _bits; }

		friend constexpr bool operator==(flag a_lhs, flag a_rhs) noexcept { return a_lhs._bits == a_rhs._bits; }
		friend constexpr bool operator!=(flag a_lhs, flag a_rhs) noexcept { return !(a_lhs == a_rhs); }
		friend constexpr bool operator==(flag a_lhs, E a_rhs) noexcept
		{
			return a_lhs._bits == static_cast<underlying_type>(a_rhs);
		}
		friend constexpr bool operator!=(flag a_lhs, E a_rhs) noexcept { return !(a_lhs == a_rhs); }

	private:
		underlying_type _bits{ 0 };
	};

#define SDS_DEFINE_ENUM_BITWISE(E)                                                                               \
	inline constexpr E operator|(E a_lhs, E a_rhs) noexcept                                                      \
	{                                                                                                            \
		return static_cast<E>(static_cast<std::underlying_type_t<E>>(a_lhs) | static_cast<std::underlying_type_t<E>>(a_rhs)); \
	}                                                                                                            \
	inline constexpr E operator&(E a_lhs, E a_rhs) noexcept                                                      \
	{                                                                                                            \
		return static_cast<E>(static_cast<std::underlying_type_t<E>>(a_lhs) & static_cast<std::underlying_type_t<E>>(a_rhs)); \
	}                                                                                                            \
	inline E& operator|=(E& a_lhs, E a_rhs) noexcept                                                             \
	{                                                                                                            \
		return a_lhs = a_lhs | a_rhs;                                                                            \
	}

	namespace Data
	{
		enum class Flags : std::uint32_t
		{
			kNone = 0,

			kNPC         = 1u << 0,
			kPlayer      = 1u << 1,
			kRight       = 1u << 2,
			kSwap        = 1u << 3,
			kFirstPerson = 1u << 4,
			kMountOnly   = 1u << 7,

			kEnabled = (kPlayer | kNPC)
		};

		SDS_DEFINE_ENUM_BITWISE(Flags);
	}
}
