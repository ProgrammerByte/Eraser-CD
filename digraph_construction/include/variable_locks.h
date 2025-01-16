#pragma once
#include "set_operations.h"
#include <set>
#include <string>
#include <unordered_map>

struct VariableLocks
    : public std::unordered_map<std::string, std::set<std::string>> {
  VariableLocks &operator*=(const VariableLocks &other) {
    for (auto &pair : other) {
      if (find(pair.first) != end()) {
        // If the key exists in both sets, intersect the sets
        at(pair.first) *= pair.second;
      } else {
        // If the key only exists in the other set, add it to this set
        insert(pair);
      }
    }
    return *this;
  }

  VariableLocks operator*(const VariableLocks &other) const {
    VariableLocks result = *this;
    return result *= other;
  }

  VariableLocks &operator+=(const VariableLocks &other) {
    for (auto it = begin(); it != end();) {
      if (other.find(it->first) != end()) {
        it->second += other.at(it->first);
        ++it;
      } else {
        erase(it->first);
      }
    }
    return *this;
  }

  VariableLocks operator+(const VariableLocks &other) const {
    VariableLocks result = *this;
    return result += other;
  }
};
