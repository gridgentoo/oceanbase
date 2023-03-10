/**
 * Copyright (c) 2022 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX OBLOG_FETCHER

#include "ob_log_part_trans_resolver_factory.h"

#include "lib/allocator/ob_mod_define.h"            // ObNewModIds
#include "storage/transaction/ob_trans_define.h"    // MAX_ELR_TRANS_INTERVAL


using namespace oceanbase::common;

namespace oceanbase
{
namespace liboblog
{

ObLogPartTransResolverFactory::ObLogPartTransResolverFactory() :
    inited_(false),
    task_pool_(NULL),
    log_entry_task_pool_(NULL),
    dispatcher_(NULL),
    cluster_id_filter_(NULL),
    allocator_(),
    task_map_(),
    trans_commit_map_()
{}

ObLogPartTransResolverFactory::~ObLogPartTransResolverFactory()
{
  destroy();
}

int ObLogPartTransResolverFactory::init(TaskPool &task_pool,
    IObLogEntryTaskPool &log_entry_task_pool,
    IObLogFetcherDispatcher &dispatcher,
    IObLogClusterIDFilter &cluster_id_filter)
{
  int ret = OB_SUCCESS;
  const int64_t obj_size = sizeof(ObLogPartTransResolver);

  if (OB_UNLIKELY(inited_)) {
    LOG_ERROR("init twice");
    ret = OB_INIT_TWICE;
  } else if (OB_FAIL(allocator_.init(obj_size, ObModIds::OB_LOG_PART_TRANS_RESOLVER,
      OB_SERVER_TENANT_ID, DEFAULT_BLOCK_SIZE))) {
    LOG_ERROR("init allocator fail", KR(ret), K(obj_size));
  } else if (OB_FAIL(task_map_.init(ObModIds::OB_LOG_PART_TRANS_RESOLVER))) {
    LOG_ERROR("init task map fail", KR(ret));
  } else if (OB_FAIL(trans_commit_map_.init(ObModIds::OB_LOG_PART_TRANS_RESOLVER))) {
    LOG_ERROR("init abort task map fail", KR(ret));
  } else {
    task_pool_ = &task_pool;
    log_entry_task_pool_ = &log_entry_task_pool;
    dispatcher_ = &dispatcher;
    cluster_id_filter_ = &cluster_id_filter;
    inited_ = true;
  }
  return ret;
}

void ObLogPartTransResolverFactory::destroy()
{
  inited_ = false;
  task_pool_ = NULL;
  log_entry_task_pool_ = NULL;
  dispatcher_ = NULL;
  cluster_id_filter_ = NULL;
  (void)allocator_.destroy();
  (void)task_map_.destroy();
  (void)trans_commit_map_.destroy();
}

int ObLogPartTransResolverFactory::alloc(const char *pkey_str,
    IObLogPartTransResolver *&ptr)
{
  int ret = OB_SUCCESS;

  if (OB_ISNULL(task_pool_) || OB_ISNULL(dispatcher_) || OB_ISNULL(cluster_id_filter_)) {
    LOG_ERROR("not init", K(task_pool_), K(dispatcher_), K(cluster_id_filter_));
    ret = OB_NOT_INIT;
  } else {
    void *obj = allocator_.alloc();

    if (OB_ISNULL(obj)) {
      LOG_ERROR("allocate memory for ObLogPartTransResolver fail", K(obj));
      ret = OB_ALLOCATE_MEMORY_FAILED;
    } else {
      ptr = new(obj) ObLogPartTransResolver(pkey_str, *task_pool_, task_map_, trans_commit_map_, *dispatcher_, *cluster_id_filter_);
    }
  }

  return ret;
}

void ObLogPartTransResolverFactory::free(IObLogPartTransResolver *ptr)
{
  if (OB_LIKELY(inited_) && OB_NOT_NULL(ptr)) {
    ObLogPartTransResolver *resolver = dynamic_cast<ObLogPartTransResolver *>(ptr);
    ptr->~IObLogPartTransResolver();

    allocator_.free(resolver);
    ptr = NULL;
    resolver = NULL;
  }
}

bool ObLogPartTransResolverFactory::TransInfoClearerByCheckpoint::operator()(const transaction::ObTransID &key,
    TransCommitInfo &trans_commit_info)
{
  const int64_t log_ts = trans_commit_info.log_ts_;
  const uint64_t log_id = trans_commit_info.log_id_;
  bool need_purge = (log_ts < (checkpoint_ - transaction::MAX_ELR_TRANS_INTERVAL));

  if (need_purge) {
		purge_count_++;
    _LOG_DEBUG("[STAT] [TRANS_COMMIT_INFO] [PURGE] PART_TRANS_ID=%s CHECKPOINT=%ld/%ld(%lu) DELTA=%ld/%ld",
        to_cstring(key), log_ts, checkpoint_, log_id,
        checkpoint_ - log_ts, transaction::MAX_ELR_TRANS_INTERVAL);
  }

  return need_purge;
}

void ObLogPartTransResolverFactory::gc_commit_trans_info(const int64_t checkpoint)
{
  int ret = OB_SUCCESS;
  TransInfoClearerByCheckpoint purger(checkpoint);

  if (OB_FAIL(trans_commit_map_.remove_if(purger))) {
    LOG_ERROR("trans_commit_map_ remove_if fail", K(checkpoint));
  } else {
    _LOG_INFO("[STAT] [TRANS_COMMIT_INFO] [PURGE] PURGE_COUNT=%ld CUR_COUNT=%ld CHECKPOINT=%ld",
        purger.purge_count_, trans_commit_map_.count(), checkpoint);
  }
}

}
}
