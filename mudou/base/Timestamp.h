#pragma once

#include <stdint.h>
#include <algorithm>
#include <string>

using namespace std;
	
	class Timestamp
	{
	public:
		Timestamp() : microSecondsSinceEpoch_(0)
		{
		}
		explicit Timestamp(int64_t microSecondsSinceEpoch);
		// 重载运算符可以实现对象的相加减
        Timestamp& operator+=(Timestamp lhs)
        {
            this->microSecondsSinceEpoch_ += lhs.microSecondsSinceEpoch_;
            return *this;
        }
        Timestamp& operator+=(int64_t lhs)
        {
            this->microSecondsSinceEpoch_ += lhs;
            return *this;
        }
        Timestamp& operator-=(Timestamp lhs)
        {
            this->microSecondsSinceEpoch_ -= lhs.microSecondsSinceEpoch_;
            return *this;
        }
        Timestamp& operator-=(int64_t lhs)
        {
            this->microSecondsSinceEpoch_ -= lhs;
            return *this;
        }

		void swap(Timestamp& that)
		{
			std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
		}
		// 将时间格以字符方式显示
		string toString() const;
		string toFormattedString(bool showMicroseconds = true) const;

		bool valid() const { return microSecondsSinceEpoch_ > 0; }

		// 返回微妙和秒
		int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
		time_t secondsSinceEpoch() const
		{
			return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
		}
		// 系统当前时间和返回一个无效的时间对象
		static Timestamp now();
		static Timestamp invalid();

		static const int kMicroSecondsPerSecond = 1000 * 1000;

	private:
		int64_t     microSecondsSinceEpoch_;
	};

	inline bool operator<(Timestamp lhs, Timestamp rhs)
	{
		return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
	}

	inline bool operator>(Timestamp lhs, Timestamp rhs)
	{
		return rhs < lhs;
	}

	inline bool operator<=(Timestamp lhs, Timestamp rhs)
	{
		return !(lhs > rhs);
	}

	inline bool operator>=(Timestamp lhs, Timestamp rhs)
	{
		return !(lhs < rhs);
	}

	inline bool operator==(Timestamp lhs, Timestamp rhs)
	{
		return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
	}

	inline bool operator!=(Timestamp lhs, Timestamp rhs)
	{
		return !(lhs == rhs);
	}

	///
	/// Gets time difference of two timestamps, result in seconds.
	///
	/// @param high, low
	/// @return (high-low) in seconds
	/// @c double has 52-bit precision, enough for one-microsecond
	/// resolution for next 100 years.
	inline double timeDifference(Timestamp high, Timestamp low)
	{
		int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
		return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
	}

	///
	/// Add @c seconds to given timestamp.
	///
	/// @return timestamp+seconds as Timestamp
	///
	inline Timestamp addTime(Timestamp timestamp, int64_t microseconds)
	{
		//int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
		return Timestamp(timestamp.microSecondsSinceEpoch() + microseconds);
	}

