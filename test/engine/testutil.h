/**
 * @file testutil.h
 * @brief common test utilities header
 * @copyright 2019 Scalewatch (www.scalewatch.com). All rights reserved.
 * Please see accompanying LICENSE file for licensing terms.
 */
 
#pragma once

#include <cstdio>
#include <cassert>
#include <cstring>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <cstddef>
#include <cstdint>

#include "gtest/gtest.h"

#include "simplx_core/platform.h"
#include "simplx_core/engine.h"
#include "simplx_core/internal/thread.h"

static const char CHAIN_TEST_PREFIX = '+';

inline
std::string errorToString(const char*fileName, int sourceFileLine)
{
	std::stringstream s;
	s << fileName << ':' << sourceFileLine << std::ends;
	return s.str();
}

#define _SIMPLX_TEST_EXIT_EXCEPTION_CATCH_BEGIN_ \
	try {
#define _SIMPLX_TEST_EXIT_EXCEPTION_CATCH_END_ \
	} catch (std::exception& e) { \
		ADD_FAILURE() << e.what(); \
	} catch (...) { \
		ADD_FAILURE() << "Unknown exception"; \
	}

template<class _Memory> class TestAllocatorBase {
public:
	inline _Memory* getMemory() const noexcept {
		return memory;
	}

protected:
	_Memory* memory;
	inline TestAllocatorBase() noexcept :
			memory(0) {
	}
	inline TestAllocatorBase(_Memory& pMemory) noexcept :
			memory(&pMemory) {
	}
	inline TestAllocatorBase(const TestAllocatorBase& copy) noexcept :
			memory(copy.memory) {
	}
};

template<class T, class _Memory> struct TestAllocator: TestAllocatorBase<_Memory> {
	typedef TestAllocatorBase<_Memory> Base;
	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;

	template<class U> struct rebind {
		typedef TestAllocator<U, _Memory> other;
	};

	inline pointer address(reference r) const {
		return &r;
	}
	inline const_pointer address(const_reference r) const {
		return &r;
	}

	inline TestAllocator() noexcept {
	}
	inline TestAllocator(_Memory& m) noexcept : Base(m) {
	}
	inline TestAllocator(const TestAllocatorBase<_Memory>& copy) noexcept : Base(copy) {
	}

	/**
	 * throw (std::bad_alloc)
	 */
	inline pointer allocate(size_type n, const void* = 0) {
		if (Base::memory == 0) {
			return static_cast<pointer>(malloc(n * sizeof(T)));
		}
		return static_cast<pointer>(Base::memory->acquire(n * sizeof(T)));
	}
	inline void deallocate(pointer p, size_type n) {
		if (Base::memory == 0) {
			free(p);
		} else {
			Base::memory->release(n * sizeof(T), p);
		}
	}

	inline void construct(pointer p, const T& val);
	inline void destroy(pointer p);

	inline size_type max_size() const noexcept {
		return std::numeric_limits<size_type>::max();
	}
};

template<class _Memory> struct TestAllocator<void, _Memory> : TestAllocatorBase<_Memory> {
	typedef TestAllocatorBase<_Memory> Base;
	template<class U> struct rebind {
		typedef TestAllocator<U, _Memory> other;
	};
	inline TestAllocator() noexcept {
	}
	inline TestAllocator(_Memory& pMemory) noexcept : Base(pMemory) {
	}
	inline TestAllocator(const TestAllocatorBase<_Memory>& copy) noexcept : Base(copy) {
	}
};

template<class T, class _Memory> inline bool operator==(const TestAllocator<T, _Memory>& left, const TestAllocator<T, _Memory>& right) {
	return left.getMemory() == right.getMemory();
}

template<class T, class _Memory> inline bool operator!=(const TestAllocator<T, _Memory>& left, const TestAllocator<T, _Memory>& right) {
	return left.getMemory() != right.getMemory();
}

struct TestMemory
{
	TestMemory()
        : inUse(0), badAlloc(std::numeric_limits<size_t>::max())
    {
	}
	~TestMemory()
    {
		EXPECT_EQ(0u, inUse);
	}
	void* acquire(size_t sz)
    {
		if (badAlloc <= inUse + sz) {
			throw std::bad_alloc();
		}
		inUse += sz;
		return malloc(sz);
	}
	void release(size_t sz, void* p)
    {
		inUse -= sz;
		free(p);
	}
    
    size_t inUse;
	size_t badAlloc;
};

class WaitCondition
{
public:
	WaitCondition()
        : signal(mutex), flag(false)
    {
	}
	~WaitCondition()
    {
		simplx::Mutex::Lock lock(mutex);
	}
	void wait()
    {
		simplx::Mutex::Lock lock(mutex);
		while (flag == false) {
			signal.wait();
		}
	}
	void notify()
    {
		simplx::Mutex::Lock lock(mutex);
		ASSERT_FALSE(flag);
		flag = true;
		signal.notify();
	}
	void reset()
    {
		flag = false;
	}

private:
	simplx::Mutex mutex;
	simplx::Signal signal;
	bool flag;
};

namespace simplx
{

class TestAsyncExceptionHandler: public AsyncExceptionHandler
{
protected:

	void onEventException(Actor*,
			const std::type_info& asyncActorTypeInfo,
			const char* onXXX_FunctionName, const Actor::Event& event,
			const char* whatException) noexcept override
    {
		std::cout << "TestEventLoop::onEventException(), "
				<< cppDemangledTypeInfoName(asyncActorTypeInfo) << "::"
				<< onXXX_FunctionName << '(' << event << ") threw ("
				<< whatException << ')' << std::endl;
		FAIL();
	}
    
	void onUnreachableException(Actor&,
			const std::type_info& asyncActorTypeInfo,
			const Actor::ActorId::RouteIdComparable& /*routeIdComparable*/,
			const char* whatException) noexcept override
    {
		std::cout << "TestEventLoop::onUnreachableException(), "
				<< cppDemangledTypeInfoName(asyncActorTypeInfo)
				<< "::onUnreachable(" << /*routeIdComparable*/"" << ") threw ("
				<< whatException << ')' << std::endl;
		FAIL();
	}
};

struct TestEngine: TestAsyncExceptionHandler, Engine
{
	inline TestEngine(StartSequence& startSequence) : Engine((startSequence.setExceptionHandler(*this), startSequence))
    {
	}
};

} // namespace


