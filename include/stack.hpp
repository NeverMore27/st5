
#include <iostream>
#include <string>
#include <mutex>
#include <future>
#include <memory>
#include <ctime>
#include <condition_variable>

template <typename T>
class stack
{
public:
	stack() /*noexcept*/ noexcept;
	~stack()  /*noexcept*/ noexcept;
	stack(const stack<T>&) /*strong*/;
	stack<T>& operator =(const stack<T>&) noexcept;
	void push(T const &) /*strong*/;
	auto try_pop()->std::shared_ptr<T>;
	auto wait_and_pop()->std::shared_ptr<T>;
	size_t size() const /*noexcept*/ noexcept;
	bool empty() const /*noexcept*/ noexcept;
private:
	mutable std::mutex mutex_;
	std::condition_variable cond_;
	T * array_;
	size_t array_size_;
	size_t count_;
	void swap(stack<T>&) /*noexcept*/ noexcept;
};

template <typename T>
void stack<T>::swap(stack<T>& object) noexcept
{
	std::lock(mutex_, object.mutex_);
	std::swap(object.array_size_, array_size_);
	std::swap(count_, object.count_);
	std::swap(object.array_, array_);
	mutex_.unlock();
	object.mutex_.unlock();
}

template <typename T>
stack<T>::stack() noexcept: count_{ 0 }, array_size_{ 0 }, array_{ nullptr }
{
}

template <typename T>
stack<T>::~stack() noexcept
{
	delete[] array_;
}

template <typename T>
stack<T>::stack(const stack& object)
{
	std::lock_guard<std::mutex> lock(object.mutex_);
	array_size_ = object.array_size_;
	count_ = object.count_;
	array_ = new T[count_];
	try
	{
		std::copy(object.array_, object.array_ + count_, array_);
	}
	catch (...)
	{
		delete[] array_;
		throw;
	}
}

template <typename T>
stack<T>& stack<T>:: operator =(const stack<T>&object) noexcept
{
	if (this != &object)
	{
		stack{ object }.swap(*this);
	}
	return *this;
}

template <typename T>
void stack<T>::push(T const &value)
{
	std::unique_lock<std::mutex> lock(mutex_);
	if (array_size_ == count_)
	{
		size_t array_size = array_size_ == 0 ? 1 : array_size_ * 2;
		T *ptr = new T[array_size];
		try
		{
			std::copy(array_, array_ + count_, ptr);
		}
		catch (...)
		{
			delete[] ptr;
			throw;
		}

		array_size_ = array_size;
		delete[] array_;
		array_ = ptr;
	}

	array_[count_] = value;
	++count_;
	cond_.notify_one();
}

template <typename T>
auto stack<T>::try_pop() -> std::shared_ptr<T>
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (count_ == 0)
	{
		return nullptr;
	}
	auto ar = std::make_shared<T>(array_[count_ - 1]);
	--count_;
	return ar;
}
template <typename T>
auto stack<T>::wait_and_pop() -> std::shared_ptr<T>
{
	std::unique_lock<std::mutex> locker(mutex_);
	while (!size())
	{
		cond_.wait(loker);
	}
	auto ar = std::make_shared<T>(array_[count_-1]);
	--count_;
	return ar;
}
template <typename T>
size_t stack<T>::size() const noexcept
{
	std::lock_guard<std::mutex> lock(mutex_);

	return count_;
}

template <typename T>
bool stack<T>::empty() const noexcept
{
	std::lock_guard<std::mutex> lock(mutex_);
	return count_ == 0;
}
