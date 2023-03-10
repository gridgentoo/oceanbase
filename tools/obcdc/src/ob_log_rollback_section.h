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

#ifndef OCEANBASE_LIBOBLOG_ROLLBACK_SECTION_H_
#define OCEANBASE_LIBOBLOG_ROLLBACK_SECTION_H_

#include "ob_log_lighty_list.h"                     // LightyList
#include "lib/queue/ob_link.h"                      // ObLink
#include "lib/container/ob_array_iterator.h"        // ObArray

namespace oceanbase
{
namespace liboblog
{
// One log(have savepoint) correspond to one RollbackNode, One RollbackNode may contains one or more rollback savepoint
class RollbackNode : public common::ObLink
{
public:
  RollbackNode(uint64_t log_id);
  ~RollbackNode();

public:
  int push(int64_t rollback_seq);
  void set_next(RollbackNode* next) { next_ = next; }
  uint64_t get_log_id() { return log_id_; }
  const common::ObArray<int64_t>& get_rollback_seqs() const { return rollback_seqs_; }

  int64_t to_string(char* buf, const int64_t buf_len) const;

private:
  uint64_t log_id_;

  // rollback to savepoint seq array
  common::ObArray<int64_t> rollback_seqs_;
};

// Rollback algorithm:
// We will rollback statements based the total rollback to savepoint seq through traversal the all RollbackNode.
//
// EG: as following, one Participant have 4 RedoLog, and the third/fourth of them exist rollback to savepoint
// 1. We should find all RollbackNodes that are greater than the current log when DmlParser handle the RedoLog,
//    eg, for log_id=1, [1, 100]
//    RollbackNode (150) and (350, 380) all meet the conditions, so we can rollback statement through traversal the
//    all rollback to savepoint seq:
//    (150) -> do nothing
//    (350, 380) -> do nothing
//
//    for log_id=5, [101, 200]
//    (150) -> rollback, [101, 150)
//    (350, 380) -> do nothing
// 2. We should deal specifically with logs that contain rollback to savepoint, rollback statements based on the information contained in the log.
//    eg, log_id=15 [300, 400]
//    (350, 380) -> [300, 350) and (380, 400]
// 3. RollbackList will sort by LogId to increase the rollback speed
//
// ParticipantRedoLog:    [log_id=1] -----> [log_id=5] -----> [log_id=10] -----> [log_id=15] -----> NULL
// StmtSeqRange:          [1, 100]          [101, 200]        [201, 300]         [300, 400]
// rollback to savepoint:   None              None             150                350, 380
// RollbackList:          (150) -----> (350, 380) -----> NULL
//
typedef LightyList<RollbackNode> RollbackList;

}; // end namespace liboblog
}; // end namespace oceanbase
#endif
