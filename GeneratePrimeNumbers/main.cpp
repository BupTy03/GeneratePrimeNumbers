#include "algorithms.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <functional>
#include <mutex>
#include <cassert>
#include <vector>
#include <future>
#include <chrono>

#include <boost/multiprecision/cpp_int.hpp>


constexpr auto CountBitsInBlock = 2048;
using big_int = boost::multiprecision::cpp_int;


static const big_int MIN_PRIME_NUMBER = power(big_int(2), CountBitsInBlock, std::multiplies<big_int>()) + 1;
static const big_int MAX_PRIME_NUMBER = MIN_PRIME_NUMBER + 10000;


struct PrimesWriter
{
    explicit PrimesWriter(const std::string& filename, std::int64_t countPrimes)
        : primesWritten_(0)
        , primesToWrite_(countPrimes)
        , m_()
        , os_(filename)
    {
    }

    bool EnoughPrimes() const { return (primesWritten_.load() >= primesToWrite_); }
    void Write(const big_int& prime) 
    {
        std::lock_guard lock{ m_ };
        os_ << prime << '\n';
        primesWritten_++;
    }

private:
    std::atomic_int64_t primesWritten_;
    const std::int64_t primesToWrite_;
    std::mutex m_;
    std::ofstream os_;
};


struct WritePrimes
{
    explicit WritePrimes(std::random_device& rd, big_int from, big_int to, PrimesWriter& writer)
        : writer_(&writer), gen_(rd()), from_(std::move(from)), to_(std::move(to))
    {
        assert(write_);
    }

    void operator()() 
    {
        big_int p = from_;
        if ((p & 1) == 0) ++p;

        while (p < to_ && !writer_->EnoughPrimes())
        {
            if (is_prime<big_int, 100>(p, gen_))
            {
                if(!writer_->EnoughPrimes())
                    writer_->Write(p);
            }

            p += 2;
        }
    }

private:
    PrimesWriter* writer_;
    std::mt19937 gen_;
    big_int from_;
    big_int to_;
};


int main()
{
    constexpr std::size_t countPrimeNumbers = 10;

    std::random_device rd;

    const auto hardwareThreads = std::thread::hardware_concurrency() - 1;
    const auto numbersPerThread = (MAX_PRIME_NUMBER - MIN_PRIME_NUMBER) / hardwareThreads;

    PrimesWriter primesWriter("very_big_primes.txt", countPrimeNumbers);
    std::vector<std::future<void>> futures;

    big_int from = MIN_PRIME_NUMBER;
    while(from < MAX_PRIME_NUMBER)
    {
        big_int to = from + numbersPerThread;
        futures.emplace_back(std::async(std::launch::async, WritePrimes(rd, from, to, primesWriter)));
        from = std::move(to);
    }

    const auto beginTime = std::chrono::system_clock::now();

    for (auto& f : futures)
        f.get();

    const auto endTime = std::chrono::system_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::minutes>(endTime - beginTime).count() << " minutes." << std::endl;
    return 0;
}
