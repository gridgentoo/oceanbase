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

#ifndef OCEANBASE_LIBOBLOG_OB_LOG_FETCH_STREAM_CONTAINER_H__
#define OCEANBASE_LIBOBLOG_OB_LOG_FETCH_STREAM_CONTAINER_H__

#include "lib/lock/ob_spin_rwlock.h"      // SpinRWLock
#include "lib/net/ob_addr.h"              // ObAddr

#include "ob_log_fetch_stream_type.h"     // FetchStreamType
#include "ob_log_fetch_stream.h"          // FSList, FetchStream

namespace oceanbase
{
namespace liboblog
{
class IFetchStreamPool;
class IObLogRpc;
class IObLogStreamWorker;
class IFetchLogARpcResultPool;
class IObLogSvrFinder;
class IObLogFetcherHeartbeatWorker;
class PartProgressController;
class PartFetchCtx;

class FetchStreamContainer
{
public:
  explicit FetchStreamContainer(const FetchStreamType stype);
  virtual ~FetchStreamContainer();

public:
  void reset();
  void reset(const common::ObAddr &svr,
      IObLogRpc &rpc,
      IFetchStreamPool &fs_pool,
      IObLogSvrFinder &svr_finder,
      IObLogFetcherHeartbeatWorker &heartbeater,
      IObLogStreamWorker &stream_worker,
      IFetchLogARpcResultPool &rpc_result_pool,
      PartProgressController &progress_controller);

public:
  // Assign the fetch log task to a FetchStream
  // If the target is a "new fetch stream task", assign it to a worker thread for processing
  int dispatch(PartFetchCtx &task);

  void do_stat();

private:
  void free_fs_list_();
  int find_fs_and_add_task_(PartFetchCtx &task,
      bool &succeed,
      FSList &base_fs_list);
  int alloc_fetch_stream_(FetchStream *&fs);
  int try_create_new_fs_and_add_task_(PartFetchCtx &task,
      FSList &base_fs_list,
      bool &succeed);

public:
  TO_STRING_KV("stype", print_fetch_stream_type(stype_),
      K_(svr),
      K_(fs_list));

private:
  // Constants: Stream types for all tasks
  const FetchStreamType           stype_;

  // Target server
  common::ObAddr                  svr_;

  IObLogRpc                       *rpc_;                    // RPC Processor
  IFetchStreamPool                *fs_pool_;                // Fetch log stream task object pool
  IObLogSvrFinder                 *svr_finder_;             // SvrFinder
  IObLogFetcherHeartbeatWorker    *heartbeater_;            // Heartbeat Manager
  IObLogStreamWorker              *stream_worker_;          // Stream master
  IFetchLogARpcResultPool         *rpc_result_pool_;        // RPC result pool
  PartProgressController          *progress_controller_;    // Progress controller

  // Fetch log stream task
  // Use read/write locks to control the reading and writing of tasks
  FSList                  fs_list_;
  common::SpinRWLock      lock_;

private:
  DISALLOW_COPY_AND_ASSIGN(FetchStreamContainer);
};

}
}

#endif
