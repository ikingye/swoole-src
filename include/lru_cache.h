#pragma once

#include <unordered_map>
#include <list>
#include <utility>
#include <memory>

#include "swoole.h"

namespace swoole
{
/**
 * This cache isn't thread safe
 */
class LRUCache
{
private:
    typedef std::pair<uint64_t, std::shared_ptr<void>> cache_node_t;
    typedef std::list<std::pair<std::string, cache_node_t>> cache_list_t;

    std::unordered_map<std::string, cache_list_t::iterator> cache_map;
    cache_list_t cache_list;
    size_t cache_capacity;

public:
    explicit LRUCache(size_t capacity)
    {
        cache_capacity = capacity;
    }

    inline std::shared_ptr<void> get(const std::string &key)
    {
        auto iter = cache_map.find(key);
        if (iter == cache_map.end())
        {
            return nullptr;
        }

        if (iter->second->second.first < swTimer_get_absolute_msec())
        {
            return nullptr;
        }

        cache_list.splice(cache_list.begin(), cache_list, iter->second);
        return iter->second->second.second; // iter -> list::iter -> cache_node_t -> value
    }

    inline void set(const std::string &key, std::shared_ptr<void> val, double expire = 0)
    {
        uint64_t expire_time;

        if (expire <= 0)
        {
            expire_time = UINT64_MAX;
        }
        else
        {
            expire_time = swTimer_get_absolute_msec() + (uint64_t) (expire * 1000);
        }

        auto iter = cache_map.find(key);
        if (iter != cache_map.end())
        {
            iter->second->second.first = expire_time;
            iter->second->second.second = val;
            cache_list.splice(cache_list.begin(), cache_list, iter->second);
            return;
        }

        size_t size = cache_list.size();
        if (size == cache_capacity && size > 0)
        {
            auto del = cache_list.back();
            cache_map.erase(del.first);
            cache_list.pop_back();
        }

        cache_list.emplace_front(key, cache_node_t{expire_time, val});
        cache_map[key] = cache_list.begin();
    }

    inline void del(const std::string &key)
    {
        auto iter = cache_map.find(key);
        if (iter == cache_map.end())
        {
            return;
        }

        cache_list.erase(iter->second);
        cache_map.erase(iter);
    }

    inline void clear()
    {
        cache_list.clear();
        cache_map.clear();
    }
};
}