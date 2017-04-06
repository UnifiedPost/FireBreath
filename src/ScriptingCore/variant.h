/**********************************************************\
Created:    2008-2012
License:    Dual license model; choose one of two:
            New BSD License
            http://www.opensource.org/licenses/bsd-license.php
            - or -
            GNU Lesser General Public License, version 2.1
            http://www.gnu.org/licenses/lgpl-2.1.html

Copyright 2008-2012 Richard Bateman, Firebreath development team
\**********************************************************/

#pragma once
#ifndef FB_VARIANT_H
#define FB_VARIANT_H

//#define ANY_IMPLICIT_CASTING    // added to enable implicit casting

#include <cstdint>
#include <boost/any.hpp>
#include <stdexcept>
#include <typeinfo>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/variant/variant_fwd.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/logic/tribool.hpp>



#include "Util/meta_util.h"
#include "utf8_tools.h"
#include "variant_conversions.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning( disable : 4800 )
#endif



namespace FB
{
	class JSObject;

	namespace variant_detail {
		struct null {
			bool operator<(const null&) const {
				return false;
			}
		};

		struct empty {
			bool operator<(const empty&) const {
				return false;
			}
		};

		template<typename T>
		struct lessthan {
			static bool impl(const boost::any& l, const boost::any& r) {
				return boost::any_cast<T>(l) < boost::any_cast<T>(r);
			}
		};

		template<typename T>
		struct lessthan < std::weak_ptr<T> > {
			static bool impl(const boost::any& l, const boost::any& r) {
				return boost::any_cast<std::weak_ptr<T>>(l).owner_before(boost::any_cast<std::weak_ptr<T>>(r));
			}
		};


	} // namespace variant_detail

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// @exception bad_variant_cast
	///
	/// @brief  Thrown when variant::cast<type> or variant::convert_cast<type> fails because the
	///         type of the value stored in the variant is not compatible with the operation
	////////////////////////////////////////////////////////////////////////////////////////////////////
	struct bad_variant_cast : std::bad_cast {
		bad_variant_cast(const std::type_info& src, const std::type_info& dest)
			: from(src.name()), to(dest.name())
		{ }
		virtual const char* what() const throw() {
			return "bad cast";
		}
		const char* from;
		const char* to;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// @class  variant
	///
	/// @brief  Accepts any datatype, used in all interactions with javascript.  Provides tools for
	///         getting back out the type you put in or for coercing that type into another type
	///         (if possible).
	///
	/// variant is the most versatile and fundamental class in FireBreath.  Any type can be assigned
	/// to a variant, and you can get that type back out using cast(), like so:
	/// @code
	///      variant a = 5;
	///      int i_a = a.cast<int>();
	/// @endcode
	///         
	/// Basic type conversion can be handled using convert_cast(), like so:
	/// @code
	///      variant str = "5";
	///      int i_a = a.convert_cast<int>();
	/// @endcode
	/// 
	/// JSAPIAuto relies heavily on the ability of variant to convert_cast effectively for all type
	/// conversion.  If the type conversion fails, a FB::bad_variant_cast exception will be thrown.
	/// 
	/// @note If you assign a char* to variant it will be automatically converted to a std::string before
	///       the assignment.
	/// @note If you assign a wchar_t* to variant it will be automatically converted to a std::wstring
	///       before the assignment
	////////////////////////////////////////////////////////////////////////////////////////////////////
	class variant
	{
	public:
		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn template <typename T> variant::variant(const T& x)
		///
		/// @brief  Templated constructor to allow any arbitrary type
		///
		/// @param  x   The value 
		////////////////////////////////////////////////////////////////////////////////////////////////////
		template <typename T>
		variant(const T& x, bool);

		template <typename T>
		variant(const T& x);

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn variant::variant()
		///
		/// @brief  Default constructor initializes the variant to an empty value
		////////////////////////////////////////////////////////////////////////////////////////////////////
		variant()
			: lessthan(&FB::variant::lessthan_default) {
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn variant& variant::assign(const variant& x)
		///
		/// @brief  Assigns a new value from another variant
		///
		/// @param  x   The variant to copy. 
		///
		/// @return *this
		////////////////////////////////////////////////////////////////////////////////////////////////////
		variant& assign(const variant& x);

		template<class T>
		variant& assign(const T& x);

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn template <typename T> variant& variant::assign(const T& x, bool)
		///
		/// @brief  Assigns a value of arbitrary type
		///
		/// @param  x   The new value 
		///
		/// @return *this
		////////////////////////////////////////////////////////////////////////////////////////////////////
		template <typename T>
		variant& assign(const T& x, bool);

		// assignment operator 
		template<typename T>
		variant& operator=(T const& x);

		// utility functions
		variant& swap(variant& x);

		// comparison function
		bool operator<(const variant& rh) const;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn const std::type_info& variant::get_type() const
		///
		/// @brief  Gets the type of the value stored in variant
		///
		/// @return The type that can be compared with typeid()
		////////////////////////////////////////////////////////////////////////////////////////////////////
		const std::type_info& get_type() const;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn template<typename T> bool variant::is_of_type() const
		///
		/// @brief  Query if this object is of a particular type. 
		///         
		/// Example:
		/// @code
		///      if (value.is_of_type<int>())
		///      {
		///         // Do something
		///      }
		/// @endcode
		///
		/// @return true if of type, false if not. 
		////////////////////////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		bool can_be_type() const;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn template<typename T> bool variant::is_of_type() const
		///
		/// @brief  Query if this object is of a particular type. 
		///         
		/// Example:
		/// @code
		///      if (value.is_of_type<int>())
		///      {
		///         // Do something
		///      }
		/// @endcode
		///
		/// @return true if of type, false if not. 
		////////////////////////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		bool is_of_type() const;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn template<typename T> T variant::cast() const
		///
		/// @brief  returns the value cast as the given type; throws bad_variant_type if that type is
		///         not the type of the value stored in variant
		///
		/// @exception  bad_variant_cast    Thrown when bad variant cast. 
		///
		/// @return value of type T
		////////////////////////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		T cast() const;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn template<typename T> typename FB::meta::disable_for_containers<T, const T>::type variant::convert_cast() const
		///
		/// @brief  Converts the stored value to the requested type *if possible* and returns the resulting
		///         value.  If the conversion is not possible, throws bad_variant_cast
		///         
		/// Supported destination types include:
		///   - all numeric types
		///   - std::string (stored in UTF8 if needed)
		///   - std::wstring (converted from UTF8 if needed)
		///   - bool
		///   - STL container types (from a FB::JSObjectPtr containing an array javascript object)
		///   - STL dict types (from a FB::JSObjectPtr containing a javascript object)
		///
		/// @return converted value of the specified type
		////////////////////////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		typename FB::meta::disable_for_containers<T, const T>::type
			convert_cast() const;

		template<typename T>
		typename FB::meta::enable_for_containers<T, Promise<T>>::type
			convert_cast() const;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn bool variant::empty() const
		///
		/// @brief  Returns true if the variant is empty (has not been assigned a value or has been reset)
		///
		/// @return true if empty, false if not 
		////////////////////////////////////////////////////////////////////////////////////////////////////
		bool empty() const;

		bool is_null() const;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// @fn void variant::reset()
		///
		/// @brief  Frees any value assigned and resets the variant to empty state
		////////////////////////////////////////////////////////////////////////////////////////////////////
		void reset();

	private:
		template<typename T>
		const T convert_cast_impl() const;

		static bool variant::lessthan_default(const boost::any& l, const boost::any& r) {
			return false;
		}

		// fields
		boost::any object;
		bool(*lessthan)(const boost::any&, const boost::any&);

	};

	template <class T>
	variant make_variant(T);
}
#endif // FB_VARIANT_H

