#ifndef SHV_CHAINPACK_SHAREDDATA_H
#define SHV_CHAINPACK_SHAREDDATA_H

#include <atomic>
#include <algorithm>

namespace shv {
namespace chainpack {

class SharedData {
	/// https://www.boost.org/doc/libs/1_57_0/doc/html/atomic/usage_examples.html#boost_atomic.usage_examples.example_reference_counters.implementation
public:
	SharedData() : m_refcount(0) {}
	virtual ~SharedData() {}

	void ref()
	{
		m_refcount.fetch_add(1, std::memory_order_relaxed);
	}
	void deref()
	{
		if (m_refcount.fetch_sub(1, std::memory_order_release) == 1) {
			std::atomic_thread_fence(std::memory_order_acquire);
			delete this;
		}
	}
private:
	//mutable
	std::atomic<int> m_refcount;
};

template <class T> class ExplicitlySharedDataPointer
{
public:
	typedef T Type;
	typedef T *pointer;
	inline T &operator*() const { return *d; }
	inline T *operator->() { return d; }
	inline T *operator->() const { return d; }
	inline T *data() const { return d; }
	inline const T *constData() const { return d; }
	//inline void detach() { if (d && d->ref.load() != 1) detach_helper(); }
	inline void reset()
	{
		if(d)
			d->deref();
		d = nullptr;
	}
	inline operator bool () const { return d != nullptr; }
	inline bool operator==(const ExplicitlySharedDataPointer<T> &other) const { return d == other.d; }
	inline bool operator!=(const ExplicitlySharedDataPointer<T> &other) const { return d != other.d; }
	inline bool operator==(const T *ptr) const { return d == ptr; }
	inline bool operator!=(const T *ptr) const { return d != ptr; }
	inline ExplicitlySharedDataPointer() { d = nullptr; }
	inline ~ExplicitlySharedDataPointer() { if (d) d->deref(); }
	explicit ExplicitlySharedDataPointer(T *data) noexcept;
	inline ExplicitlySharedDataPointer(const ExplicitlySharedDataPointer<T> &o) : d(o.d) { if (d) d->ref(); }
	template<class X>
	inline ExplicitlySharedDataPointer(const ExplicitlySharedDataPointer<X> &o)
		: d(o.data())
	{
		if(d)
			d->ref();
	}
	inline ExplicitlySharedDataPointer<T> & operator=(const ExplicitlySharedDataPointer<T> &o) {
		if (o.d != d) {
			if (o.d)
				o.d->ref();
			T *old = d;
			d = o.d;
			if (old)
				old->deref();
		}
		return *this;
	}
	inline ExplicitlySharedDataPointer &operator=(T *o)
	{
		if (o != d) {
			if (o)
				o->ref();
			T *old = d;
			d = o;
			if (old)
				old->deref();
		}
		return *this;
	}
	inline ExplicitlySharedDataPointer(ExplicitlySharedDataPointer &&o) noexcept : d(o.d) { o.d = nullptr; }
	inline ExplicitlySharedDataPointer<T> &operator=(ExplicitlySharedDataPointer<T> &&other) noexcept
	{
		ExplicitlySharedDataPointer moved(std::move(other));
		swap(moved);
		return *this;
	}
	inline bool operator!() const { return !d; }
	inline void swap(ExplicitlySharedDataPointer &other) noexcept { std::swap(d, other.d); }
protected:
	//T *clone();
private:
	//void detach_helper();
	T *d;
};

template <class T>
inline ExplicitlySharedDataPointer<T>::ExplicitlySharedDataPointer(T *adata) noexcept
	: d(adata)
{
	if (d)
		d->ref();
}

} // namespace chainpack
} // namespace shv

#endif // SHV_CHAINPACK_SHAREDDATA_H
