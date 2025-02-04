// TODO -  CREDIT:
// https://stackoverflow.com/questions/13448064/how-to-find-the-intersection-of-two-stl-sets

#pragma once
#include <algorithm>
#include <iterator>
#include <set>

template <class T, class CMP = std::less<T>, class ALLOC = std::allocator<T>>
std::set<T, CMP, ALLOC> operator*(const std::set<T, CMP, ALLOC> &s1,
                                  const std::set<T, CMP, ALLOC> &s2) {
  std::set<T, CMP, ALLOC> s;
  std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
                        std::inserter(s, s.begin()));
  return s;
}

template <class T, class CMP = std::less<T>, class ALLOC = std::allocator<T>>
std::set<T, CMP, ALLOC> operator+(const std::set<T, CMP, ALLOC> &s1,
                                  const std::set<T, CMP, ALLOC> &s2) {
  std::set<T, CMP, ALLOC> s;
  std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(),
                 std::inserter(s, s.begin()));
  return s;
}

template <class T, class CMP = std::less<T>, class ALLOC = std::allocator<T>>
std::set<T, CMP, ALLOC> operator-(const std::set<T, CMP, ALLOC> &s1,
                                  const std::set<T, CMP, ALLOC> &s2) {
  std::set<T, CMP, ALLOC> s;
  std::set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(),
                      std::inserter(s, s.begin()));
  return s;
}

template <class T, class CMP = std::less<T>, class ALLOC = std::allocator<T>>
std::set<T, CMP, ALLOC> &operator*=(std::set<T, CMP, ALLOC> &s1,
                                    const std::set<T, CMP, ALLOC> &s2) {
  auto iter1 = s1.begin();
  for (auto iter2 = s2.begin(); iter1 != s1.end() && iter2 != s2.end();) {
    if (*iter1 < *iter2)
      iter1 = s1.erase(iter1);
    else {
      if (!(*iter2 < *iter1))
        ++iter1;
      ++iter2;
    }
  }
  while (iter1 != s1.end())
    iter1 = s1.erase(iter1);
  return s1;
}

template <class T, class CMP = std::less<T>, class ALLOC = std::allocator<T>>
std::set<T, CMP, ALLOC> &operator+=(std::set<T, CMP, ALLOC> &s1,
                                    const std::set<T, CMP, ALLOC> &s2) {
  s1.insert(s2.begin(), s2.end());
  return s1;
}

template <class T, class CMP = std::less<T>, class ALLOC = std::allocator<T>>
std::set<T, CMP, ALLOC> &operator-=(std::set<T, CMP, ALLOC> &s1,
                                    const std::set<T, CMP, ALLOC> &s2) {
  auto iter1 = s1.begin();
  for (auto iter2 = s2.begin(); iter1 != s1.end() && iter2 != s2.end();) {
    if (*iter1 < *iter2)
      ++iter1;
    else {
      if (*iter2 == *iter1)
        iter1 = s1.erase(iter1);
      ++iter2;
    }
  }
  return s1;
}