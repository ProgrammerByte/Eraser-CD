#pragma once
#include "set_operations.h"
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

// varName -> tid
struct ActiveThreads
    : public std::unordered_map<std::string, std::set<std::string>> {
  ActiveThreads &operator+=(const ActiveThreads &other) {
    for (auto &pair : other) {
      if (find(pair.first) != end()) {
        at(pair.first) += pair.second;
      } else {
        insert(pair);
      }
    }
    return *this;
  }

  ActiveThreads operator+(const ActiveThreads &other) const {
    ActiveThreads result = *this;
    return result += other;
  }

  ActiveThreads &operator-=(const std::set<std::string> &other) {
    for (auto &varName : other) {
      erase(varName);
    }
    return *this;
  }

  ActiveThreads operator-(const std::set<std::string> &other) const {
    ActiveThreads result = *this;
    return result -= other;
  }
};

// Q: (tid -> pending writes)
struct QueuedWrites
    : public std::unordered_map<std::string, std::set<std::string>> {
  std::set<std::string> values() {
    std::set<std::string> result;
    for (auto &pair : *this) {
      result += pair.second;
    }
    return result;
  }

  QueuedWrites removeVars(std::set<std::string> vars) {
    for (auto it = begin(); it != end();) {
      it->second -= vars;
      if (it->second.empty()) {
        it = erase(it);
      } else {
        ++it;
      }
    }
    return *this;
  }

  QueuedWrites removeVar(std::string var) {
    for (auto it = begin(); it != end();) {
      it->second.erase(var);
      if (it->second.empty()) {
        it = erase(it);
      } else {
        ++it;
      }
    }
    return *this;
  }

  QueuedWrites &operator+=(const QueuedWrites &other) {
    for (auto &pair : other) {
      if (find(pair.first) != end()) {
        at(pair.first) += pair.second;
      } else {
        insert(pair);
      }
    }
    return *this;
  }

  QueuedWrites operator+(const QueuedWrites &other) const {
    QueuedWrites result = *this;
    return result += other;
  }

  QueuedWrites &operator-=(const std::set<std::string> &other) {
    for (auto &tid : other) {
      erase(tid);
    }
    return *this;
  }

  QueuedWrites operator-(const std::set<std::string> &other) const {
    QueuedWrites result = *this;
    return result -= other;
  }
};

struct EraserSets {
  // currently held locks
  std::set<std::string> locks;
  std::set<std::string> unlocks;

  // state machine state representation
  std::set<std::string> externalReads;
  std::set<std::string> internalReads;
  std::set<std::string> externalWrites;
  std::set<std::string> internalWrites;
  std::set<std::string> internalShared;
  std::set<std::string> externalShared;
  std::set<std::string> sharedModified;
  QueuedWrites queuedWrites;

  // threads
  std::set<std::string> finishedThreads;
  ActiveThreads activeThreads;

  bool locksEqual(const EraserSets &other) const {
    return locks == other.locks && unlocks == other.unlocks;
  }

  bool varsEqual(const EraserSets &other) const {
    return externalReads == other.externalReads &&
           internalReads == other.internalReads &&
           externalWrites == other.externalWrites &&
           internalWrites == other.internalWrites &&
           internalShared == other.internalShared &&
           externalShared == other.externalShared &&
           sharedModified == other.sharedModified &&
           queuedWrites == other.queuedWrites &&
           finishedThreads == other.finishedThreads &&
           activeThreads == other.activeThreads;
  }

  bool operator==(const EraserSets &other) const {
    return locksEqual(other) && varsEqual(other);
  }

  bool operator!=(const EraserSets &other) const { return !(*this == other); }

  static const EraserSets defaultValue;
};

const inline EraserSets EraserSets::defaultValue = {{}, {}, {}, {}, {}, {},
                                                    {}, {}, {}, {}, {}, {}};