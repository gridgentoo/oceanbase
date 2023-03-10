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

#include "ob_archive_round_mgr.h"
#include "lib/ob_define.h"
#include "lib/atomic/ob_atomic.h"
#include "observer/ob_server_event_history_table_operator.h"

namespace oceanbase {
namespace archive {
using namespace oceanbase::common;
using namespace oceanbase::share;

ObArchiveRoundMgr::ObArchiveRoundMgr()
    : add_pg_finish_(false),
      total_pg_count_(0),
      started_pg_count_(0),
      incarnation_(-1),
      current_archive_round_(-1),
      cur_piece_id_(OB_BACKUP_INVALID_PIECE_ID),
      cur_piece_create_date_(OB_INVALID_TIMESTAMP),
      compatible_(false),
      is_oss_(false),
      has_handle_error_(false),
      has_encount_error_(false),
      log_archive_status_(LOG_ARCHIVE_INVALID_STATUS),
      rwlock_()
{}

ObArchiveRoundMgr::~ObArchiveRoundMgr()
{
  destroy();
}

int ObArchiveRoundMgr::init()
{
  ARCHIVE_LOG(INFO, "ObArchiveRoundMgr init succ");
  return OB_SUCCESS;
}

void ObArchiveRoundMgr::destroy()
{
  WLockGuard guard(rwlock_);

  add_pg_finish_ = false;
  total_pg_count_ = 0;
  started_pg_count_ = 0;
  incarnation_ = -1;
  current_archive_round_ = -1;
  cur_piece_id_ = OB_BACKUP_INVALID_PIECE_ID;
  cur_piece_create_date_ = OB_INVALID_TIMESTAMP;
  compatible_ = false;
  is_oss_ = false;
  has_encount_error_ = false;

  memset(root_path_, 0, share::OB_MAX_BACKUP_PATH_LENGTH);
  memset(storage_info_, 0, share::OB_MAX_BACKUP_STORAGE_INFO_LENGTH);
}

void ObArchiveRoundMgr::inc_started_pg()
{
  ATOMIC_INC(&started_pg_count_);

  if (started_pg_count_ >= total_pg_count_) {
    set_add_pg_finish_flag();
  }
}

void ObArchiveRoundMgr::set_add_pg_finish_flag()
{
  ATOMIC_STORE(&add_pg_finish_, true);
}

void ObArchiveRoundMgr::inc_total_pg_count()
{
  ATOMIC_INC(&total_pg_count_);
}

void ObArchiveRoundMgr::dec_total_pg_count()
{
  if (OB_UNLIKELY(0 == total_pg_count_)) {
    ARCHIVE_LOG(ERROR, "total_pg_count_ is zero");
  } else {
    ATOMIC_DEC(&total_pg_count_);
  }
}

int ObArchiveRoundMgr::mark_fatal_error(
    const ObPartitionKey& pg_key, const int64_t incarnation, const int64_t archive_round)
{
  int ret = OB_SUCCESS;
  WLockGuard guard(rwlock_);

  if (incarnation_ != incarnation || current_archive_round_ != archive_round) {
    ret = OB_EAGAIN;
    ARCHIVE_LOG(WARN,
        "encount error with different incarnation or round, just retry",
        KR(ret),
        K(incarnation_),
        K(incarnation),
        K(current_archive_round_),
        K(archive_round),
        K(pg_key));
  } else {
    if (!has_encount_error_) {
      has_encount_error_ = true;
      SERVER_EVENT_ADD("log_archive", "mark_fatal_error", "partition", pg_key);
    }
    ARCHIVE_LOG(ERROR, "mark_fatal_error succ", K(incarnation), K(archive_round));
  }
  return ret;
}

// used for observer report archive_status to rs
bool ObArchiveRoundMgr::has_encounter_fatal_error(const int64_t incarnation, const int64_t archive_round)
{
  bool b_ret = false;
  RLockGuard guard(rwlock_);
  if (incarnation_ == incarnation && current_archive_round_ == archive_round) {
    b_ret = has_encount_error_;
  }
  return b_ret;
}

int ObArchiveRoundMgr::set_archive_start(const int64_t incarnation, const int64_t archive_round, const int64_t piece_id,
    const int64_t piece_create_date, const bool is_oss, const share::ObTenantLogArchiveStatus::COMPATIBLE compatible)
{
  int ret = OB_SUCCESS;
  WLockGuard guard(rwlock_);

  if (OB_UNLIKELY(0 >= incarnation || 0 > archive_round || 0 > piece_id)) {
    ret = OB_INVALID_ARGUMENT;
    ARCHIVE_LOG(
        WARN, "invalid arguments", K(incarnation), K(archive_round), K(piece_id), K(piece_create_date), K(compatible));
  } else if (share::ObTenantLogArchiveStatus::COMPATIBLE::NONE != compatible &&
             share::ObTenantLogArchiveStatus::COMPATIBLE::COMPATIBLE_VERSION_1 != compatible &&
             share::ObTenantLogArchiveStatus::COMPATIBLE::COMPATIBLE_VERSION_2 != compatible) {
    ret = OB_NOT_SUPPORTED;
    ARCHIVE_LOG(ERROR, "compatible not support", K(incarnation), K(archive_round), K(compatible));
  } else {
    incarnation_ = incarnation;
    current_archive_round_ = archive_round;
    cur_piece_id_ = piece_id;
    cur_piece_create_date_ = piece_create_date;
    compatible_ = compatible >= share::ObTenantLogArchiveStatus::COMPATIBLE::COMPATIBLE_VERSION_2;
    log_archive_status_ = LOG_ARCHIVE_BEGINNING;
    is_oss_ = is_oss;
    has_handle_error_ = false;
    has_encount_error_ = false;
  }

  return ret;
}

int ObArchiveRoundMgr::set_archive_force_stop(
    const int64_t incarnation, const int64_t archive_round, const int64_t piece_id)
{
  int ret = OB_SUCCESS;
  WLockGuard guard(rwlock_);

  if (OB_UNLIKELY(0 >= incarnation || 0 > archive_round || piece_id < 0)) {
    ret = OB_INVALID_ARGUMENT;
    ARCHIVE_LOG(WARN, "invalid arguments", K(incarnation), K(archive_round), K(piece_id), K(ret));
  } else {
    incarnation_ = incarnation;
    current_archive_round_ = archive_round;
    cur_piece_id_ = piece_id;
    log_archive_status_ = LOG_ARCHIVE_STOPPED;
  }
  return ret;
}

int ObArchiveRoundMgr::update_cur_piece_info(const int64_t incarnation, const int64_t archive_round,
    const int64_t new_piece_id, const int64_t new_piece_create_date)
{
  int ret = OB_SUCCESS;
  WLockGuard guard(rwlock_);
  if (OB_UNLIKELY(incarnation != incarnation_ || current_archive_round_ != archive_round || 0 == cur_piece_id_ ||
                  new_piece_id <= cur_piece_id_)) {
    ret = OB_ERR_UNEXPECTED;
    ARCHIVE_LOG(WARN,
        "invalid arguments",
        K(incarnation),
        K(archive_round),
        K(new_piece_id),
        K(new_piece_create_date),
        K(incarnation_),
        K(current_archive_round_),
        K(cur_piece_id_));
  } else {
    cur_piece_id_ = new_piece_id;
    cur_piece_create_date_ = new_piece_create_date;
  }
  return ret;
}

void ObArchiveRoundMgr::get_archive_round_info(int64_t& incarnation, int64_t& archive_round, int64_t& cur_piece_id,
    int64_t& cur_piece_create_date, bool& is_oss, LogArchiveStatus& log_archive_status, bool& has_encount_error) const
{
  RLockGuard guard(rwlock_);
  incarnation = incarnation_;
  archive_round = current_archive_round_;
  cur_piece_id = cur_piece_id_;
  cur_piece_create_date = cur_piece_create_date_;
  is_oss = is_oss_;
  log_archive_status = log_archive_status_;
  has_encount_error = has_encount_error_;
}

void ObArchiveRoundMgr::get_archive_round_compatible(int64_t& incarnation, int64_t& archive_round, bool& compatible)
{
  RLockGuard guard(rwlock_);
  incarnation = incarnation_;
  archive_round = current_archive_round_;
  compatible = compatible_;
}

bool ObArchiveRoundMgr::need_handle_error()
{
  RLockGuard guard(rwlock_);
  return !has_handle_error_ && has_encount_error_;
}

void ObArchiveRoundMgr::set_has_handle_error(bool has_handle)
{
  WLockGuard guard(rwlock_);
  has_handle_error_ = has_handle;
}

bool ObArchiveRoundMgr::is_in_archive_invalid_status() const
{
  RLockGuard guard(rwlock_);
  return LOG_ARCHIVE_INVALID_STATUS == ATOMIC_LOAD(&log_archive_status_);
}

bool ObArchiveRoundMgr::is_in_archive_status() const
{
  RLockGuard guard(rwlock_);
  return LOG_ARCHIVE_BEGINNING == ATOMIC_LOAD(&log_archive_status_) ||
         LOG_ARCHIVE_DOING == ATOMIC_LOAD(&log_archive_status_);
}

bool ObArchiveRoundMgr::is_in_archive_beginning_status() const
{
  RLockGuard guard(rwlock_);
  return LOG_ARCHIVE_BEGINNING == ATOMIC_LOAD(&log_archive_status_);
}

bool ObArchiveRoundMgr::is_in_archive_doing_status() const
{
  RLockGuard guard(rwlock_);
  return LOG_ARCHIVE_DOING == ATOMIC_LOAD(&log_archive_status_);
}

bool ObArchiveRoundMgr::is_in_archive_stopping_status() const
{
  RLockGuard guard(rwlock_);
  return LOG_ARCHIVE_IN_STOPPING == ATOMIC_LOAD(&log_archive_status_);
}

bool ObArchiveRoundMgr::is_in_archive_stopped_status() const
{
  RLockGuard guard(rwlock_);
  return LOG_ARCHIVE_STOPPED == ATOMIC_LOAD(&log_archive_status_);
}

void ObArchiveRoundMgr::update_log_archive_status(const LogArchiveStatus status)
{
  WLockGuard guard(rwlock_);
  ATOMIC_STORE(&log_archive_status_, status);
}

ObArchiveRoundMgr::LogArchiveStatus ObArchiveRoundMgr::get_log_archive_status()
{
  RLockGuard guard(rwlock_);
  return ATOMIC_LOAD(&log_archive_status_);
}

bool ObArchiveRoundMgr::is_server_archive_stop(const int64_t incarnation, const int64_t round)
{
  RLockGuard guard(rwlock_);
  return LOG_ARCHIVE_STOPPED == ATOMIC_LOAD(&log_archive_status_) && incarnation == incarnation_ &&
         round == current_archive_round_;
}

}  // namespace archive
}  // namespace oceanbase
