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

#ifndef _OCEABASE_COMMON_OB_SRV_RPC_PROXY_H_
#define _OCEABASE_COMMON_OB_SRV_RPC_PROXY_H_

#include "sql/engine/cmd/ob_kill_session_arg.h"
#include "storage/tablelock/ob_table_lock_rpc_struct.h"
#include "rpc/obrpc/ob_rpc_proxy.h"
#include "share/ob_rpc_struct.h"
#include "observer/ob_server_struct.h"

namespace oceanbase
{
namespace observer
{
class ObGlobalContext;
}
namespace obrpc
{

class ObSrvRpcProxy
    : public ObRpcProxy
{
public:
  DEFINE_TO(ObSrvRpcProxy);

  // special usage when can't deliver request.
  RPC_S(PR5 nouse, OB_ERROR_PACKET);

  RPC_S(PR5 set_config, OB_SET_CONFIG, (common::ObString));
  RPC_S(PR5 get_config, OB_GET_CONFIG, common::ObString);
  RPC_S(PR5 set_tenant_config, OB_SET_TENANT_CONFIG, (obrpc::ObTenantConfigArg));
  RPC_S(PR1 get_diagnose_args, OB_GET_DIAGNOSE_ARGS, common::ObString);


  RPC_AP(PR5 notify_tenant_server_unit_resource, OB_NOTIFY_TENANT_SERVER_UNIT_RESOURCE, (obrpc::TenantServerUnitConfig));
  RPC_AP(PR5 get_wrs_info, OB_GET_WRS_INFO, (ObGetWRSArg), ObGetWRSResult);
  RPC_AP(PR5 stop_write, OB_PARTITION_STOP_WRITE, (obrpc::Int64), obrpc::Int64);
  RPC_AP(PR5 check_log, OB_PARTITION_CHECK_LOG, (obrpc::Int64), obrpc::Int64);
  RPC_AP(PR5 check_frozen_scn, OB_CHECK_FROZEN_SCN, (obrpc::ObCheckFrozenScnArg));
  RPC_AP(PR5 get_min_sstable_schema_version, OB_GET_MIN_SSTABLE_SCHEMA_VERSION,
      (obrpc::ObGetMinSSTableSchemaVersionArg), obrpc::ObGetMinSSTableSchemaVersionRes);
  RPC_AP(PR5 init_tenant_config, OB_INIT_TENANT_CONFIG,
      (obrpc::ObInitTenantConfigArg), obrpc::ObInitTenantConfigRes);
  RPC_AP(PR3 get_leader_locations, OB_GET_LEADER_LOCATIONS,
      (obrpc::ObGetLeaderLocationsArg), obrpc::ObGetLeaderLocationsResult);
  RPC_S(PR5 fetch_sys_ls, OB_FETCH_SYS_LS,
        share::ObLSReplica);
  RPC_S(PR5 broadcast_rs_list, OB_BROADCAST_RS_LIST, (obrpc::ObRsListArg));
  // Rpc bulk interface for replication, migration, etc.
  RPC_S(PR5 delete_backup_ls_task, OB_DELETE_BACKUP_LS_TASK, (ObLSBackupCleanArg));
  RPC_S(PR5 backup_ls_data, OB_BACKUP_LS_DATA, (ObBackupDataArg));
  RPC_S(PR5 backup_completing_log, OB_BACKUP_COMPL_LOG, (ObBackupComplLogArg));
  RPC_S(PR5 backup_build_index, OB_BACKUP_BUILD_INDEX, (ObBackupBuildIdxArg));
  RPC_S(PR5 backup_meta, OB_BACKUP_META, (ObBackupMetaArg));
  RPC_S(PR5 check_not_backup_tablet_create_scn, OB_BACKUP_CHECK_TABLET_CREATE_TS, (ObBackupCheckTabletArg));
  RPC_S(PR5 check_backup_task_exist, OB_CHECK_BACKUP_TASK_EXIST, (ObBackupCheckTaskArg), obrpc::Bool);
  // ls disaster recovery rpc
  RPC_S(PR5 ls_migrate_replica, OB_LS_MIGRATE_REPLICA, (ObLSMigrateReplicaArg));
  RPC_S(PR5 ls_add_replica, OB_LS_ADD_REPLICA, (ObLSAddReplicaArg));
  RPC_S(PR5 ls_type_transform, OB_LS_TYPE_TRANSFORM, (ObLSChangeReplicaArg));
  RPC_S(PR5 ls_remove_paxos_replica, OB_LS_REMOVE_PAXOS_REPLICA, (ObLSDropPaxosReplicaArg));
  RPC_S(PR5 ls_remove_nonpaxos_replica, OB_LS_REMOVE_NONPAXOS_REPLICA, (ObLSDropNonPaxosReplicaArg));
  RPC_S(PR5 ls_modify_paxos_replica_number, OB_LS_MODIFY_PAXOS_REPLICA_NUMBER, (ObLSModifyPaxosReplicaNumberArg));
  RPC_S(PR5 ls_check_dr_task_exist, OB_LS_CHECK_DR_TASK_EXIST, (ObDRTaskExistArg), obrpc::Bool);
  RPC_S(PR5 checkpoint_slog, OB_CHECKPOINT_SLOG, (ObCheckpointSlogArg));

  RPC_AP(PR5 minor_freeze, OB_MINOR_FREEZE, (ObMinorFreezeArg), obrpc::Int64);
  RPC_AP(PR5 check_schema_version_elapsed, OB_CHECK_SCHEMA_VERSION_ELAPSED, (ObCheckSchemaVersionElapsedArg), ObCheckSchemaVersionElapsedResult);
  RPC_AP(PR5 check_modify_time_elapsed, OB_CHECK_MODIFY_TIME_ELAPSED, (ObCheckModifyTimeElapsedArg), ObCheckModifyTimeElapsedResult);

  RPC_AP(PR5 check_ddl_tablet_merge_status, OB_DDL_CHECK_TABLET_MERGE_STATUS, (ObDDLCheckTabletMergeStatusArg), ObDDLCheckTabletMergeStatusResult);

  RPC_S(PR5 switch_leader, OB_SWITCH_LEADER, (ObSwitchLeaderArg));
  RPC_S(PR5 batch_switch_rs_leader, OB_BATCH_SWITCH_RS_LEADER, (ObAddr));
  RPC_S(PR5 get_partition_count, OB_GET_PARTITION_COUNT,
        ObGetPartitionCountResult);
  RPC_AP(PR5 switch_schema, OB_SWITCH_SCHEMA, (ObSwitchSchemaArg), obrpc::ObSwitchSchemaResult);
  RPC_S(PR5 refresh_memory_stat, OB_REFRESH_MEMORY_STAT);
  RPC_S(PR5 wash_memory_fragmentation, OB_WASH_MEMORY_FRAGMENTATION);
  RPC_S(PR5 bootstrap, OB_BOOTSTRAP, (ObBootstrapArg));
  RPC_S(PR5 is_empty_server, OB_IS_EMPTY_SERVER, (ObCheckServerEmptyArg), Bool);
  RPC_S(PR5 check_deployment_mode_match, OB_CHECK_DEPLOYMENT_MODE, (ObCheckDeploymentModeArg), Bool);
  RPC_S(PR5 report_replica, OB_REPORT_REPLICA);
  RPC_S(PR5 recycle_replica, OB_RECYCLE_REPLICA);
  RPC_S(PR5 clear_location_cache, OB_CLEAR_LOCATION_CACHE);
  RPC_S(PR5 refresh_sync_value, OB_REFRESH_SYNC_VALUE, (ObAutoincSyncArg));
  RPC_S(PR5 sync_auto_increment, OB_SYNC_AUTO_INCREMENT, (ObAutoincSyncArg));
  RPC_S(PR5 clear_autoinc_cache, OB_CLEAR_AUTOINC_CACHE, (ObAutoincSyncArg));
  RPC_S(PR5 dump_memtable, OB_DUMP_MEMTABLE, (ObDumpMemtableArg));
  RPC_S(PR5 dump_tx_data_memtable, OB_DUMP_TX_DATA_MEMTABLE, (ObDumpTxDataMemtableArg));
  RPC_S(PR5 dump_single_tx_data, OB_DUMP_SINGLE_TX_DATA, (ObDumpSingleTxDataArg));
  RPC_S(PR5 halt_all_prewarming, OB_FORCE_PURGE_MEMTABLE);
  RPC_AP(PR5 halt_all_prewarming_async, OB_FORCE_PURGE_MEMTABLE_ASYNC, (obrpc::UInt64));
  RPC_S(PR5 set_debug_sync_action, OB_SET_DS_ACTION, (obrpc::ObDebugSyncActionArg));
  RPC_S(PR5 request_heartbeat, OB_REQUEST_HEARTBEAT, share::ObLeaseRequest);
  RPC_S(PR5 check_partition_table, OB_CHECK_PARTITION_TABLE);
  RPC_S(PR5 execute_io_benchmark, OB_EXECUTE_IO_BENCHMARK);
  RPC_S(PR5 refresh_io_calibration, OB_REFRESH_IO_CALIBRATION, (obrpc::ObRefreshIOCalibrationArg));
  RPC_S(PR5 update_baseline_schema_version, OB_UPDATE_BASELINE_SCHEMA_VERSION, (obrpc::Int64));
  RPC_AP(PR1 detect_master_rs_ls, OB_DETECT_MASTER_RS_LS,
        (obrpc::ObDetectMasterRsArg), obrpc::ObDetectMasterRsLSResult);
  RPC_AP(PR1 get_root_server_status, OB_GET_ROOT_SERVER_ROLE,
        (obrpc::ObDetectMasterRsArg), obrpc::ObGetRootserverRoleResult);
  RPC_S(PR5 sync_partition_table, OB_SYNC_PARTITION_TABLE, (obrpc::Int64));
  RPC_S(PR5 flush_cache, OB_FLUSH_CACHE, (ObFlushCacheArg));
  RPC_S(PR5 set_tracepoint, OB_SET_TP, (obrpc::ObAdminSetTPArg));
  RPC_S(PR5 kill_session, OB_KILL_SESSION, (sql::ObKillSessionArg));
  RPC_S(PR5 cancel_sys_task, OB_CANCEL_SYS_TASK, (obrpc::ObCancelTaskArg));
  RPC_S(PR5 set_disk_valid, OB_SET_DISK_VALID, (ObSetDiskValidArg));
  RPC_S(PR5 add_disk, OB_ADD_DISK, (ObAdminAddDiskArg));
  RPC_S(PR5 drop_disk, OB_DROP_DISK, (ObAdminDropDiskArg));
  RPC_S(PR5 force_switch_ilog_file, OB_FORCE_SWITCH_ILOG_FILE, (ObForceSwitchILogFileArg));
  RPC_S(PR5 force_set_all_as_single_replica, OB_FORCE_SET_ALL_AS_SINGLE_REPLICA, (ObForceSetAllAsSingleReplicaArg));
  RPC_S(PR5 force_set_server_list, OB_FORCE_SET_SERVER_LIST, (ObForceSetServerListArg));
  RPC_S(PR5 calc_column_checksum_request, OB_CALC_COLUMN_CHECKSUM_REQUEST, (ObCalcColumnChecksumRequestArg), obrpc::ObCalcColumnChecksumRequestRes);
  RPC_AP(PR5 build_ddl_single_replica_request, OB_DDL_BUILD_SINGLE_REPLICA_REQUEST, (obrpc::ObDDLBuildSingleReplicaRequestArg), obrpc::ObDDLBuildSingleReplicaRequestResult);
  RPC_S(PR5 fetch_tablet_autoinc_seq_cache, OB_FETCH_TABLET_AUTOINC_SEQ_CACHE, (obrpc::ObFetchTabletSeqArg), obrpc::ObFetchTabletSeqRes);
  RPC_AP(PR5 batch_get_tablet_autoinc_seq, OB_BATCH_GET_TABLET_AUTOINC_SEQ, (obrpc::ObBatchGetTabletAutoincSeqArg), obrpc::ObBatchGetTabletAutoincSeqRes);
  RPC_AP(PR5 batch_set_tablet_autoinc_seq, OB_BATCH_SET_TABLET_AUTOINC_SEQ, (obrpc::ObBatchSetTabletAutoincSeqArg), obrpc::ObBatchSetTabletAutoincSeqRes);
  RPC_S(PRD force_create_sys_table, OB_FORCE_CREATE_SYS_TABLE, (ObForceCreateSysTableArg));
  RPC_S(PRD schema_revise, OB_SCHEMA_REVISE, (ObSchemaReviseArg));
  RPC_S(PRD force_set_locality, OB_FORCE_SET_LOCALITY, (ObForceSetLocalityArg));
  RPC_S(PR5 force_disable_blacklist, OB_FORCE_DISABLE_BLACKLIST);
  RPC_S(PR5 force_enable_blacklist, OB_FORCE_ENABLE_BLACKLIST);
  RPC_S(PR5 force_clear_srv_blacklist, OB_FORCE_CLEAR_BLACKLIST);

  RPC_S(PR5 update_local_stat_cache, obrpc::OB_SERVER_UPDATE_STAT_CACHE, (ObUpdateStatCacheArg));
  // The optimizer estimates the number of rows
  RPC_S(PR5 estimate_partition_rows, OB_ESTIMATE_PARTITION_ROWS, (ObEstPartArg), ObEstPartRes);

  RPC_AP(PR1 ha_gts_ping_request, OB_HA_GTS_PING_REQUEST, (ObHaGtsPingRequest), ObHaGtsPingResponse);
  RPC_AP(PR1 ha_gts_get_request, OB_HA_GTS_GET_REQUEST, (ObHaGtsGetRequest));
  RPC_AP(PR1 ha_gts_get_response, OB_HA_GTS_GET_RESPONSE, (ObHaGtsGetResponse));
  RPC_AP(PR1 ha_gts_heartbeat, OB_HA_GTS_HEARTBEAT, (ObHaGtsHeartbeat));
  RPC_S(PR1 ha_gts_update_meta, OB_HA_GTS_UPDATE_META, (ObHaGtsUpdateMetaRequest),
      ObHaGtsUpdateMetaResponse);
  RPC_S(PR1 ha_gts_change_member, OB_HA_GTS_CHANGE_MEMBER, (ObHaGtsChangeMemberRequest),
      ObHaGtsChangeMemberResponse);
  RPC_S(PR5 get_tenant_refreshed_schema_version, OB_GET_TENANT_REFRESHED_SCHEMA_VERSION,
        (ObGetTenantSchemaVersionArg), ObGetTenantSchemaVersionResult);
  RPC_S(PR5 update_tenant_memory, OB_UPDATE_TENANT_MEMORY, (obrpc::ObTenantMemoryArg));
  RPC_S(PR5 renew_in_zone_hb, OB_RENEW_IN_ZONE_HB, (share::ObInZoneHbRequest), share::ObInZoneHbResponse);
  RPC_S(PR5 pre_process_server_status, OB_PRE_PROCESS_SERVER, (obrpc::ObPreProcessServerArg));
  RPC_S(PR5 pre_bootstrap_create_server_working_dir, OB_PRE_BOOTSTRAP_CREATE_SERVER_WORKING_DIR, (ObPreBootstrapCreateServerWorkingDirArg));
  RPC_S(PR5 handle_part_trans_ctx, OB_HANDLE_PART_TRANS_CTX, (obrpc::ObTrxToolArg), ObTrxToolRes);
  RPC_S(PR5 flush_local_opt_stat_monitoring_info, obrpc::OB_SERVER_FLUSH_OPT_STAT_MONITORING_INFO, (obrpc::ObFlushOptStatArg));
  RPC_AP(PR5 set_member_list, OB_SET_MEMBER_LIST, (obrpc::ObSetMemberListArgV2), obrpc::ObSetMemberListResult);
  RPC_AP(PR5 create_ls, OB_CREATE_LS, (obrpc::ObCreateLSArg), obrpc::ObCreateLSResult);
  RPC_AP(PR5 create_tablet, OB_CREATE_TABLET, (obrpc::ObBatchCreateTabletArg), obrpc::ObCreateTabletBatchRes);
  RPC_AP(PR5 batch_broadcast_schema, OB_BATCH_BROADCAST_SCHEMA, (obrpc::ObBatchBroadcastSchemaArg), obrpc::ObBatchBroadcastSchemaResult);
  RPC_AP(PR5 drop_tablet, OB_DROP_TABLET, (obrpc::ObBatchRemoveTabletArg), obrpc::ObRemoveTabletRes);
  RPC_AP(PR5 lock_table, OB_TABLE_LOCK_TASK, (transaction::tablelock::ObTableLockTaskRequest),
         transaction::tablelock::ObTableLockTaskResult);
  RPC_AP(PR4 unlock_table, OB_HIGH_PRIORITY_TABLE_LOCK_TASK, (transaction::tablelock::ObTableLockTaskRequest),
         transaction::tablelock::ObTableLockTaskResult);
  RPC_AP(PR5 batch_lock_obj, OB_BATCH_TABLE_LOCK_TASK, (transaction::tablelock::ObLockTaskBatchRequest),
         transaction::tablelock::ObTableLockTaskResult);
  RPC_AP(PR4 batch_unlock_obj, OB_HIGH_PRIORITY_BATCH_TABLE_LOCK_TASK, (transaction::tablelock::ObLockTaskBatchRequest),
         transaction::tablelock::ObTableLockTaskResult);
  RPC_S(PR4 admin_remove_lock_op, OB_REMOVE_OBJ_LOCK, (transaction::tablelock::ObAdminRemoveLockOpArg));
  RPC_S(PR4 admin_update_lock_op, OB_UPDATE_OBJ_LOCK, (transaction::tablelock::ObAdminUpdateLockOpArg));
  RPC_S(PR5 remote_write_ddl_redo_log, OB_REMOTE_WRITE_DDL_REDO_LOG, (obrpc::ObRpcRemoteWriteDDLRedoLogArg));
  RPC_S(PR5 remote_write_ddl_commit_log, OB_REMOTE_WRITE_DDL_COMMIT_LOG, (obrpc::ObRpcRemoteWriteDDLCommitLogArg), obrpc::Int64);
  RPC_S(PR5 check_ls_can_offline, OB_CHECK_LS_CAN_OFFLINE, (obrpc::ObCheckLSCanOfflineArg));
  RPC_S(PR5 clean_sequence_cache, obrpc::OB_CLEAN_SEQUENCE_CACHE, (obrpc::UInt64));
  RPC_S(PR5 register_tx_data, OB_REGISTER_TX_DATA, (ObRegisterTxDataArg), ObRegisterTxDataResult);
  RPC_S(PR5 query_ls_is_valid_member, OB_QUERY_LS_IS_VALID_MEMBER, (ObQueryLSIsValidMemberRequest),
      ObQueryLSIsValidMemberResponse);
  RPC_S(PR5 check_backup_dest_connectivity, OB_CHECK_BACKUP_DEST_CONNECTIVITY, (ObCheckBackupConnectivityArg));
  RPC_AP(PR1 get_ls_access_mode, OB_GET_LS_ACCESS_MODE, (obrpc::ObGetLSAccessModeInfoArg), obrpc::ObLSAccessModeInfo);
  RPC_AP(PR1 change_ls_access_mode, OB_CHANGE_LS_ACCESS_MODE, (obrpc::ObLSAccessModeInfo), obrpc::ObChangeLSAccessModeRes);
  RPC_S(PR5 estimate_tablet_block_count, OB_ESTIMATE_TABLET_BLOCK_COUNT, (ObEstBlockArg), ObEstBlockRes);
  RPC_AP(PR1 get_ls_sync_scn, OB_GET_LS_SYNC_SCN, (obrpc::ObGetLSSyncScnArg), obrpc::ObGetLSSyncScnRes);
  RPC_AP(PR5 refresh_tenant_info, OB_REFRESH_TENANT_INFO, (obrpc::ObRefreshTenantInfoArg), obrpc::ObRefreshTenantInfoRes);
  RPC_S(PR5 sync_rewrite_rules, OB_SYNC_REWRITE_RULES, (ObSyncRewriteRuleArg));
}; // end of class ObSrvRpcProxy

} // end of namespace rpc
} // end of namespace oceanbase

#endif /* _OCEABASE_COMMON_OB_SRV_RPC_PROXY_H_ */
