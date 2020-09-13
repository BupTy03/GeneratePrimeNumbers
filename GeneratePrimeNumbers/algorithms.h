#pragma once
#include <random>
#include <cassert>
#include <functional>


template<typename I>
struct modulo_multiply
{
    explicit modulo_multiply(const I& i) : modulus_{ i } {}
    I modulus() const { return modulus; }
    I operator()(const I& n, const I& m) const { return (n * m) % modulus_; }

private:
    const I modulus_;
};


template<typename T> T identity_element(std::plus<T>) { return T{ 0 }; }
template<typename T> T identity_element(std::multiplies<T>) { return T{ 1 }; }
template<typename T> bool identity_element(std::logical_and<T>) { return true; }
template<typename T> bool identity_element(std::logical_or<T>) { return false; }
template<typename I> I identity_element(const modulo_multiply<I>&) { return I{ 1 }; }


template<typename T, typename N, typename BinaryOperation>
T power(T x, N n, BinaryOperation op)
{
    assert(n >= 0);
    if (n == 0) return identity_element(op);

    while ((n & 1) == 0)
    {
        n >>= 1;
        x = op(x, x);
    }

    T result = x;
    n >>= 1;
    while (n != 0)
    {
        x = op(x, x);
        if ((n & 1) != 0)
            result = op(result, x);

        n >>= 1;
    }

    return result;
}

template<typename I>
bool miller_rabin_test(const I& n, const I& q, const I& k, const I& w)
{
    // assert(n > 1 && n - 1 == power(I{2}, k, std::multiplies<>()) * q && odd(q));
    modulo_multiply<I> mmult{ n };
    I x = power(w, q, mmult);
    if (x == I{ 1 } || x == n - I{ 1 })
        return true;

    for (I i{ 1 }; i < k; ++i)
    {
        // invariant: x = w**(2**(i - 1)q)
        x = mmult(x, x);
        if (x == n - I{ 1 }) return true;
        if (x == I{ 1 }) return false;
    }

    return false;
}

template<typename I>
std::pair<I, I> find_q_and_k(I n)
{
    I k{ 0 };
    while ((n & 1) == 0)
    {
        n >>= 1;
        ++k;
    }

    return { n, k };
}


template<typename I, std::size_t CountChecks = 100>
bool is_prime(const I& n, std::mt19937& gen)
{
    assert((n & 1) == 1);
    assert(n > 251);

    constexpr int prime_numbers[] = {
            5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107,
            109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
            229, 233, 239, 241, 251
    };

    for (int num : prime_numbers)
    {
        if (n % num == 0)
            return false;
    }

    const I maxN = n - 1;
    const std::uint64_t maxVal = maxN > std::numeric_limits<std::uint64_t>::max() ? std::numeric_limits<std::uint64_t>::max() : std::uint64_t{ maxN };
    std::uniform_int_distribution<std::uint64_t> distrib(3, maxVal);
    const auto [q, k] = find_q_and_k(maxN);
    for (std::size_t i = 0; i < CountChecks; ++i)
    {
        const I w = distrib(gen);
        if (!miller_rabin_test(n, q, k, w))
            return false;
    }

    return true;
}

