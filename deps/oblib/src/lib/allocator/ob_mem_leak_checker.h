/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef __OB_MEM_LEAK_CHECKER_H__
#define __OB_MEM_LEAK_CHECKER_H__
#include "lib/utility/ob_defer.h"
#include "lib/hash/ob_hashmap.h"
#include "lib/alloc/alloc_struct.h"
#include "lib/utility/ob_simple_rate_limiter.h"

namespace oceanbase {
namespace common {
class ObMemLeakChecker {
  static constexpr const char MOD_INFO_MAP_STR[] =  "leakInfoMap";
  struct PtrKey {
    void* ptr_;
    uint64_t hash() const
    {
      return murmurhash(&ptr_, sizeof(ptr_), 0);
    }
    bool operator==(const PtrKey& other) const
    {
      return (other.ptr_ == this->ptr_);
    }
  };
  struct Info {
    Info() : bt_(), bytes_()
    {}

    uint64_t hash() const
    {
      return murmurhash(bt_, static_cast<int32_t>(strlen(bt_)), 0);
    }
    bool operator==(const Info& other) const
    {
      return (0 == STRNCMP(bt_, other.bt_, static_cast<int32_t>(strlen(bt_))));
    }

    char bt_[512];
    int64_t bytes_;
  };

  typedef hash::ObHashMap<PtrKey, Info> mod_alloc_info_t;

public:

  class mod_info_map_t
  {
    public: 
      typedef hash::ObHashMap<Info, std::pair<int64_t, int64_t>> hashmap;
      int create(int64_t bucket_num)
      {
        ObMemAttr attr(common::OB_SERVER_TENANT_ID, MOD_INFO_MAP_STR, ObCtxIds::DEFAULT_CTX_ID,
                      lib::OB_HIGH_ALLOC);
        return map_.create(bucket_num, attr, attr);
      }
      
      hashmap *operator->()
      {
        return &map_;
      }
    private:
      hashmap map_;
  };

  using TCharArray = char[2 * lib::AOBJECT_LABEL_SIZE + 1];

  ObMemLeakChecker()
  {
    origin_str_[0] = '\0';
    ct_ = NOCHECK;
    static_id_ = -1;
    is_wildcard_ = false;
    len_ = 0;
  }
  static ObMemLeakChecker& get_instance()
  {
    static ObMemLeakChecker one;
    return one;
  }
  int init()
  {
    ObMemAttr attr(common::OB_SERVER_TENANT_ID, "leakMap", ObCtxIds::DEFAULT_CTX_ID, lib::OB_HIGH_ALLOC);
    int ret = malloc_info_.create(512, attr, attr);
    if (OB_FAIL(ret)) {
      _OB_LOG(ERROR, "failed to create hashmap, err=%d", ret);
    } else {
      _OB_LOG(INFO, "leak checker init succ");
    }
    return ret;
  }

  bool is_context_check() const
  {
    return CONTEXT_CHECK == ct_;
  }
  bool is_label_check() const
  {
    return LABEL_CHECK == ct_;
  }
  int64_t get_static_id() const
  {
    return static_id_;
  }
  bool is_wildcard() const
  {
    return is_wildcard_;
  }

  // Will generate two buffers
  // A copy of the original string is used for the outer layer to check for status changes
  // One copy is encoded according to the same rules as AObject, used for memory compare to accelerate matching
  void set_str(const char* str)
  {
    // disable leak check under current function
    ct_ = NOCHECK;
    CheckType tmp_ct = NOCHECK;
    DEFER(ct_ = tmp_ct);

    _OB_LOG(INFO, "leak mod to check: %s", str);
    if (nullptr == str || 0 == STRLEN(str) || 0 == STRNCMP(str, "NONE", STRLEN("NONE"))) {
      origin_str_[0] = '\0';
      tmp_ct = NOCHECK;
    } else if (0 == STRNCMP(str, MOD_INFO_MAP_STR, strlen(MOD_INFO_MAP_STR))) {
       // ensure leak_mod is different from the mod_info_map's label
    } else {
      STRNCPY(origin_str_, str, sizeof(origin_str_));
      origin_str_[sizeof(origin_str_) - 1] = '\0';
      char cpy[sizeof(origin_str_)];
      MEMCPY(cpy, origin_str_, sizeof(origin_str_));
      char* end = (char*)memchr(cpy, '@', strlen(cpy));
      if (end != nullptr) {
        tmp_ct = CONTEXT_CHECK;
        static_id_ = 0;
        sscanf(end + 1, "0x%lx", &static_id_);
        *end = '\0';
        is_wildcard_ = 0 == STRCMP("*", cpy);
      } else {
        tmp_ct = LABEL_CHECK;
      }
      const int64_t mod_id = ObModSet::instance().get_mod_id(cpy);
      if (is_valid_mod_id(mod_id)) {
        ident_char_ = lib::INVISIBLE_CHARACTER;
        mod_id_ = mod_id;
        len_ = offsetof(ObMemLeakChecker, data_) - offsetof(ObMemLeakChecker, label_);
      } else {
        STRNCPY(label_, cpy, sizeof(label_));
        label_[sizeof(label_) - 1] = '\0';
        len_ = strlen(label_);
      }
    }
  }

  const char* get_str() const
  {
    return origin_str_;
  }
  const char* get_label() const
  {
    return label_;
  }
  int get_check_type() const
  {
    return ct_;
  }

  void reset()
  {
    malloc_info_.reuse();
  }

  void on_alloc(lib::AObject &obj)
  {
    obj.on_leak_check_ = false;
    if (is_label_check() &&
        label_match(obj) &&
        malloc_info_.size() < MAP_SIZE_LIMIT) {
      Info info;
      info.bytes_ = obj.alloc_bytes_;
      lbt(info.bt_, sizeof(info.bt_));
      PtrKey ptr_key;
      ptr_key.ptr_ = obj.data_;
      int ret = OB_SUCCESS;

      if (OB_FAIL(malloc_info_.set_refactored(ptr_key, info))) {
        _OB_LOG(WARN, "failed to insert leak checker(ret=%d), ptr=%p bt=%s", ret, ptr_key.ptr_, lbt());
      } else {
        obj.on_leak_check_ = true;
      }
    }
  }

  void on_free(lib::AObject& obj)
  {
    if (is_label_check() && obj.on_leak_check_ && label_match(obj)) {
      PtrKey ptr_key;
      ptr_key.ptr_ = obj.data_;
      int ret = OB_SUCCESS;
      if (OB_FAIL(malloc_info_.erase_refactored(ptr_key))) {
        if (REACH_TIME_INTERVAL(1000 * 1000)) {
          _OB_LOG(WARN, "failed to erase leak checker(ret=%d), ptr=%p, bt=%s", ret, ptr_key.ptr_, lbt());
        }
      }
    }
  }

  int load_leak_info_map(mod_info_map_t &info_map)
  {
    int ret = OB_SUCCESS;
    using hashtable = mod_alloc_info_t::hashtable;
    auto bucket_it = malloc_info_.bucket_begin();
    while (bucket_it != malloc_info_.bucket_end()) {
      hashtable::bucket_lock_cond blc(*bucket_it);
      hashtable::readlocker locker(blc.lock());
      auto node_it = bucket_it->node_begin();
      while (node_it != bucket_it->node_end()) {
        std::pair<int64_t, int64_t> item;
        //_OB_LOG(INFO, "hash value, bt=%s, hash=%lu", it->second.bt_, it->second.hash());
        ret = info_map->get_refactored(node_it->second, item);
        if (OB_FAIL(ret) && OB_HASH_NOT_EXIST != ret) {
          _OB_LOG(INFO, "LEAK_CHECKER, ptr=%p bt=%s", node_it->first.ptr_, node_it->second.bt_);
        } else {
          if (OB_SUCC(ret)) {
            item.first += 1;
            item.second += node_it->second.bytes_;
            if (OB_FAIL(info_map->set_refactored(node_it->second, item, 1, 0, 1))) {
              _OB_LOG(WARN, "failed to aggregate memory size, ret=%d", ret);
            } else {
              _OB_LOG(DEBUG, "LEAK_CHECKER hash updated");
            }
          } else {
            item.first = 1;
            item.second = node_it->second.bytes_;
            if (OB_FAIL(info_map->set_refactored(node_it->second, item, 1, 0, 0))) {
              _OB_LOG(WARN, "failed to aggregate memory size, ret=%d", ret);
            } else {
              _OB_LOG(DEBUG, "LEAK_CHECKER hash inserted");
            }
          }
        }
        node_it++;
      }
      bucket_it++;
    }
    return ret;
  }
  void print()
  {
    mod_info_map_t tmp_map;
    int ret = tmp_map.create(10000);
    if (OB_FAIL(ret)) {
      _OB_LOG(ERROR, "failed to create hashmap, err=%d", ret);
    } else if (OB_FAIL(load_leak_info_map(tmp_map))) {
      _OB_LOG(INFO, "failed to collection leak info, ret=%d", ret);
    } else {
      _OB_LOG(INFO, "######## LEAK_CHECKER (str = %s)########", origin_str_);

      mod_info_map_t::hashmap::const_iterator jt = tmp_map->begin();
      for (; jt != tmp_map->end(); ++jt)
      {
        _OB_LOG(INFO, "[LC] bt=%s, count=%ld, bytes=%ld",
                jt->first.bt_, jt->second.first, jt->second.second);
      }
      _OB_LOG(INFO, "######## LEAK_CHECKER (END) ########");
    }
  }
  bool label_match(lib::AObject& obj)
  {
    return 0 == MEMCMP(&obj.label_[0], label_, len_);
  }

private:
  // Limit the memory used by hashmap
  static constexpr int MEMORY_LIMIT = 1L << 30;
  static constexpr int MAP_SIZE_LIMIT = MEMORY_LIMIT / sizeof(Info);
  enum CheckType {
    NOCHECK,
    LABEL_CHECK,
    CONTEXT_CHECK,
  };

private:
  TCharArray origin_str_;
  union {
    struct {
      struct {
        int64_t ident_char_ : 8;
        int64_t mod_id_ : 56;
      };
      char data_[0];
    };
    char label_[lib::AOBJECT_LABEL_SIZE + 1];
  };
  CheckType ct_;
  int64_t static_id_;
  bool is_wildcard_;
  int len_;
  mod_alloc_info_t malloc_info_;
};
};  // end namespace common
};  // end namespace oceanbase

#endif /* __OB_MEM_LEAK_CHECKER_H__ */
