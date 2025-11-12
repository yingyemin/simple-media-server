-- ----------------------------
-- Table structure for sms_cloud_record
-- ----------------------------
DROP TABLE IF EXISTS `sms_cloud_record`;
CREATE TABLE `sms_cloud_record`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `app` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `stream` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `call_id` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `start_time` bigint(20) NULL DEFAULT NULL,
  `end_time` bigint(20) NULL DEFAULT NULL,
  `media_server_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `file_name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `folder` varchar(500) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `file_path` varchar(500) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `collect` tinyint(1) NULL DEFAULT 0,
  `file_size` bigint(20) NULL DEFAULT NULL,
  `time_len` bigint(20) NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 127 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_cloud_record
-- ----------------------------

-- ----------------------------
-- Table structure for sms_common_group
-- ----------------------------
DROP TABLE IF EXISTS `sms_common_group`;
CREATE TABLE `sms_common_group`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `parent_id` int(11) NULL DEFAULT NULL,
  `parent_device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `business_group` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `civil_code` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_common_group_device_platform`(`device_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 5 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of sms_common_group
-- ----------------------------
INSERT INTO `sms_common_group` VALUES (4, '11010101002157000001', '测试', NULL, NULL, '11010101002157000001', '2025-04-18 11:26:34', '2025-04-18 11:26:34', '44011801');

-- ----------------------------
-- Table structure for sms_common_region
-- ----------------------------
DROP TABLE IF EXISTS `sms_common_region`;
CREATE TABLE `sms_common_region`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `parent_id` int(11) NULL DEFAULT NULL,
  `parent_device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_common_region_device_id`(`device_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 7 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of sms_common_region
-- ----------------------------
INSERT INTO `sms_common_region` VALUES (6, '44011801', '测试门口', NULL, NULL, '2025-04-18 11:23:40', '2025-04-18 11:23:40');

-- ----------------------------
-- Table structure for sms_device
-- ----------------------------
DROP TABLE IF EXISTS `sms_device`;
CREATE TABLE `sms_device`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `dept_id` bigint(20) NULL DEFAULT NULL COMMENT '部门ID',
  `device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `manufacturer` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `model` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `firmware` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `transport` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `stream_mode` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `on_line` tinyint(1) NULL DEFAULT 0,
  `register_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `keepalive_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `port` int(11) NULL DEFAULT NULL,
  `expires` int(11) NULL DEFAULT NULL,
  `subscribe_cycle_for_catalog` int(11) NULL DEFAULT 0,
  `subscribe_cycle_for_mobile_position` int(11) NULL DEFAULT 0,
  `mobile_position_submission_interval` int(11) NULL DEFAULT 5,
  `subscribe_cycle_for_alarm` int(11) NULL DEFAULT 0,
  `host_address` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `charset` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `ssrc_check` tinyint(1) NULL DEFAULT 0,
  `geo_coord_sys` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `media_server_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT 'auto',
  `custom_name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `sdp_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `local_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `password` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `as_message_channel` tinyint(1) NULL DEFAULT 0,
  `heart_beat_interval` int(11) NULL DEFAULT NULL,
  `heart_beat_count` int(11) NULL DEFAULT NULL,
  `position_capability` int(11) NULL DEFAULT NULL,
  `broadcast_push_after_ack` tinyint(1) NULL DEFAULT 0,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_device_device`(`device_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 19 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_device
-- ----------------------------
INSERT INTO `sms_device` VALUES (17, NULL, '34020000001320000001', NULL, 'Hikvision', 'DS-2CD2110FDV2-IS', 'V5.5.92', 'UDP', 'UDP', 1, '2025-08-28 12:35:23', '2025-08-28 12:52:24', '192.168.2.204', '2025-04-18 10:25:34', '2025-08-28 12:52:24', 5060, 3600, 0, 0, 5, 0, '192.168.2.204:5060', 'GB2312', 0, 'WGS84', 'auto', NULL, NULL, '192.168.2.199', NULL, 0, 60, 3, 0, 0);
INSERT INTO `sms_device` VALUES (18, NULL, '34020000001330000001', '', 'UNIVIEW', 'IPC-S362-IR@DACP-IR3-M28-F', 'IPC_Q1201-B5022P30D1711C31 build 2018-06-19 22:03:52', 'UDP', 'TCP-PASSIVE', 1, '2025-08-28 12:35:23', '2025-08-28 12:52:24', '192.168.2.101', '2025-08-28 12:35:23', '2025-08-28 12:52:24', 5060, 3600, 0, 0, 5, 0, '192.168.2.101:5060', 'GB2312', 0, 'WGS84', 'auto', NULL, NULL, '192.168.2.199', NULL, 0, 60, 3, 0, 0);

-- ----------------------------
-- Table structure for sms_device_alarm
-- ----------------------------
DROP TABLE IF EXISTS `sms_device_alarm`;
CREATE TABLE `sms_device_alarm`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `channel_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `alarm_priority` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `alarm_method` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `alarm_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `alarm_description` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `longitude` double NULL DEFAULT NULL,
  `latitude` double NULL DEFAULT NULL,
  `alarm_type` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_device_alarm
-- ----------------------------

-- ----------------------------
-- Table structure for sms_device_channel
-- ----------------------------
DROP TABLE IF EXISTS `sms_device_channel`;
CREATE TABLE `sms_device_channel`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `manufacturer` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `model` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `owner` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `civil_code` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `block` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `address` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `parental` int(11) NULL DEFAULT NULL,
  `parent_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `safety_way` int(11) NULL DEFAULT NULL,
  `register_way` int(11) NULL DEFAULT NULL,
  `cert_num` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `certifiable` int(11) NULL DEFAULT NULL,
  `err_code` int(11) NULL DEFAULT NULL,
  `end_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `secrecy` int(11) NULL DEFAULT NULL,
  `ip_address` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `port` int(11) NULL DEFAULT NULL,
  `password` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `status` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `longitude` double NULL DEFAULT NULL,
  `latitude` double NULL DEFAULT NULL,
  `ptz_type` int(11) NULL DEFAULT NULL,
  `position_type` int(11) NULL DEFAULT NULL,
  `room_type` int(11) NULL DEFAULT NULL,
  `use_type` int(11) NULL DEFAULT NULL,
  `supply_light_type` int(11) NULL DEFAULT NULL,
  `direction_type` int(11) NULL DEFAULT NULL,
  `resolution` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `business_group_id` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `download_speed` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `svc_space_support_mod` int(11) NULL DEFAULT NULL,
  `svc_time_support_mode` int(11) NULL DEFAULT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `sub_count` int(11) NULL DEFAULT NULL,
  `stream_id` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `has_audio` tinyint(1) NULL DEFAULT 0,
  `gps_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `stream_identification` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `channel_type` int(11) NOT NULL DEFAULT 0,
  `gb_device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_manufacturer` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_model` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_owner` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_civil_code` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_block` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_address` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_parental` int(11) NULL DEFAULT NULL,
  `gb_parent_id` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_safety_way` int(11) NULL DEFAULT NULL,
  `gb_register_way` int(11) NULL DEFAULT NULL,
  `gb_cert_num` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_certifiable` int(11) NULL DEFAULT NULL,
  `gb_err_code` int(11) NULL DEFAULT NULL,
  `gb_end_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_secrecy` int(11) NULL DEFAULT NULL,
  `gb_ip_address` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_port` int(11) NULL DEFAULT NULL,
  `gb_password` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_status` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_longitude` double NULL DEFAULT NULL,
  `gb_latitude` double NULL DEFAULT NULL,
  `gb_business_group_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_ptz_type` int(11) NULL DEFAULT NULL,
  `gb_position_type` int(11) NULL DEFAULT NULL,
  `gb_room_type` int(11) NULL DEFAULT NULL,
  `gb_use_type` int(11) NULL DEFAULT NULL,
  `gb_supply_light_type` int(11) NULL DEFAULT NULL,
  `gb_direction_type` int(11) NULL DEFAULT NULL,
  `gb_resolution` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_download_speed` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `gb_svc_space_support_mod` int(11) NULL DEFAULT NULL,
  `gb_svc_time_support_mode` int(11) NULL DEFAULT NULL,
  `record_plan_id` int(11) NULL DEFAULT NULL,
  `data_type` int(11) NOT NULL,
  `data_device_id` int(11) NOT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_sms_unique_channel`(`gb_device_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 31 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_device_channel
-- ----------------------------
INSERT INTO `sms_device_channel` VALUES (29, '34020000001320000001', 'Camera 01', 'Hikvision', 'IP Camera', 'Owner', NULL, NULL, 'Address', 0, NULL, 0, 1, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, 'ON', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '2025-04-18 10:25:36', '2025-04-18 11:25:48', 0, NULL, 0, NULL, NULL, 0, '34020000001320000001', 'Camera 01', 'Hikvision', 'IP Camera', 'Owner', '44011801', NULL, 'Address', 0, '11010101002157000001', 0, 1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'ON', NULL, NULL, '11010101002157000001', NULL, NULL, NULL, NULL, NULL, NULL, NULL, '', NULL, NULL, NULL, 1, 17);
INSERT INTO `sms_device_channel` VALUES (30, '34020000001330000001', '', 'UNIVIEW', 'IPC-S360', 'IPC-B5022P30D1711C31', NULL, NULL, 'Address', 1, NULL, 0, 1, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, 'ON', 0, 0, 3, NULL, NULL, NULL, NULL, NULL, '5/4/2', NULL, '0', NULL, NULL, '2025-08-28 12:35:24', '2025-08-28 12:35:24', 0, NULL, 0, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1, 18);

-- ----------------------------
-- Table structure for sms_device_mobile_position
-- ----------------------------
DROP TABLE IF EXISTS `sms_device_mobile_position`;
CREATE TABLE `sms_device_mobile_position`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `channel_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `device_name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `longitude` double NULL DEFAULT NULL,
  `latitude` double NULL DEFAULT NULL,
  `altitude` double NULL DEFAULT NULL,
  `speed` double NULL DEFAULT NULL,
  `direction` double NULL DEFAULT NULL,
  `report_source` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_device_mobile_position
-- ----------------------------

-- ----------------------------
-- Table structure for sms_media_server
-- ----------------------------
DROP TABLE IF EXISTS `sms_media_server`;
CREATE TABLE `sms_media_server`  (
  `id` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `hook_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `sdp_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `stream_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `http_port` int(11) NULL DEFAULT NULL,
  `http_ssl_port` int(11) NULL DEFAULT NULL,
  `rtmp_port` int(11) NULL DEFAULT NULL,
  `rtmp_ssl_port` int(11) NULL DEFAULT NULL,
  `jt1078_port` int(11) NULL DEFAULT NULL,
  `rtp_proxy_port` int(11) NULL DEFAULT NULL,
  `rtsp_port` int(11) NULL DEFAULT NULL,
  `rtsp_ssl_port` int(11) NULL DEFAULT NULL,
  `flv_port` int(11) NULL DEFAULT NULL,
  `flv_ssl_port` int(11) NULL DEFAULT NULL,
  `ws_flv_port` int(11) NULL DEFAULT NULL,
  `ws_flv_ssl_port` int(11) NULL DEFAULT NULL,
  `auto_config` tinyint(1) NULL DEFAULT 0,
  `secret` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `type` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT 'zlm',
  `rtp_enable` tinyint(1) NULL DEFAULT 0,
  `rtp_port_range` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `send_rtp_port_range` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `record_assist_port` int(11) NULL DEFAULT NULL,
  `default_server` tinyint(1) NULL DEFAULT 0,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `hook_alive_interval` int(11) NULL DEFAULT NULL,
  `record_path` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `record_day` int(11) NULL DEFAULT 7,
  `transcode_suffix` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `uk_media_server_unique_ip_http_port`(`ip`, `http_port`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_media_server
-- ----------------------------
INSERT INTO `sms_media_server` VALUES ('hxkj_zlm', '192.168.2.199', '192.168.2.199', '192.168.2.199', '192.168.2.199', 8092, 443, 1935, 0, 10000, 554, 0, 8092, 443, 8092, 443, 1, 'hxkj_zlm', 'zlm', 1, '30000,30500', '30000,30500', 0, 1, '2025-08-28 12:30:31', '2025-08-28 12:35:23', 10, '', 3, NULL);

-- ----------------------------
-- Table structure for sms_gb28181_sip_server
-- ----------------------------
DROP TABLE IF EXISTS `sms_gb28181_sip_server`;
CREATE TABLE `sms_gb28181_sip_server`  (
  `id` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `hook_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `http_port` int(11) NULL DEFAULT NULL,
  `http_ssl_port` int(11) NULL DEFAULT NULL,
  `auto_config` tinyint(1) NULL DEFAULT 0,
  `secret` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `type` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT 'sms',
  `gb28181_sip_port` int(11) NULL DEFAULT NULL,
  `default_server` tinyint(1) NULL DEFAULT 0,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `hook_alive_interval` int(11) NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `uk_media_server_unique_ip_http_port`(`ip`, `http_port`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_gb28181_sip_server
-- ----------------------------
INSERT INTO `sms_gb28181_sip_server` VALUES ('hxkj_zlm', '192.168.2.199', '192.168.2.199', 8092, 443, 1, 'hxkj_sms', 'sms', 0, 1, '2025-08-28 12:30:31', '2025-08-28 12:35:23', 10);


-- ----------------------------
-- Table structure for sms_platform
-- ----------------------------
DROP TABLE IF EXISTS `sms_platform`;
CREATE TABLE `sms_platform`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `enable` tinyint(1) NULL DEFAULT 0,
  `name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `server_gb_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `server_gb_domain` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `server_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `server_port` int(11) NULL DEFAULT NULL,
  `device_gb_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `device_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `device_port` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `username` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `password` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `expires` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `keep_timeout` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `transport` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `civil_code` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `manufacturer` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `model` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `address` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `character_set` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `ptz` tinyint(1) NULL DEFAULT 0,
  `rtcp` tinyint(1) NULL DEFAULT 0,
  `status` tinyint(1) NULL DEFAULT 0,
  `catalog_group` int(11) NULL DEFAULT NULL,
  `register_way` int(11) NULL DEFAULT NULL,
  `secrecy` int(11) NULL DEFAULT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `as_message_channel` tinyint(1) NULL DEFAULT 0,
  `catalog_with_platform` int(11) NULL DEFAULT 1,
  `catalog_with_group` int(11) NULL DEFAULT 1,
  `catalog_with_region` int(11) NULL DEFAULT 1,
  `auto_push_channel` tinyint(1) NULL DEFAULT 1,
  `send_stream_ip` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_platform_unique_server_gb_id`(`server_gb_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 3 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_platform
-- ----------------------------

-- ----------------------------
-- Table structure for sms_platform_channel
-- ----------------------------
DROP TABLE IF EXISTS `sms_platform_channel`;
CREATE TABLE `sms_platform_channel`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `platform_id` int(11) NULL DEFAULT NULL,
  `device_channel_id` int(11) NULL DEFAULT NULL,
  `custom_device_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_manufacturer` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_model` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_owner` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_civil_code` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_block` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_address` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_parental` int(11) NULL DEFAULT NULL,
  `custom_parent_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_safety_way` int(11) NULL DEFAULT NULL,
  `custom_register_way` int(11) NULL DEFAULT NULL,
  `custom_cert_num` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_certifiable` int(11) NULL DEFAULT NULL,
  `custom_err_code` int(11) NULL DEFAULT NULL,
  `custom_end_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_secrecy` int(11) NULL DEFAULT NULL,
  `custom_ip_address` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_port` int(11) NULL DEFAULT NULL,
  `custom_password` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_status` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_longitude` double NULL DEFAULT NULL,
  `custom_latitude` double NULL DEFAULT NULL,
  `custom_ptz_type` int(11) NULL DEFAULT NULL,
  `custom_position_type` int(11) NULL DEFAULT NULL,
  `custom_room_type` int(11) NULL DEFAULT NULL,
  `custom_use_type` int(11) NULL DEFAULT NULL,
  `custom_supply_light_type` int(11) NULL DEFAULT NULL,
  `custom_direction_type` int(11) NULL DEFAULT NULL,
  `custom_resolution` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_business_group_id` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_download_speed` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `custom_svc_space_support_mod` int(11) NULL DEFAULT NULL,
  `custom_svc_time_support_mode` int(11) NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_platform_gb_channel_platform_id_catalog_id_device_channel_id`(`platform_id`, `device_channel_id`) USING BTREE,
  UNIQUE INDEX `uk_platform_gb_channel_device_id`(`custom_device_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 33 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_platform_channel
-- ----------------------------

-- ----------------------------
-- Table structure for sms_platform_group
-- ----------------------------
DROP TABLE IF EXISTS `sms_platform_group`;
CREATE TABLE `sms_platform_group`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `platform_id` int(11) NULL DEFAULT NULL,
  `group_id` int(11) NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_sms_platform_group_platform_id_group_id`(`platform_id`, `group_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 4 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_platform_group
-- ----------------------------

-- ----------------------------
-- Table structure for sms_platform_region
-- ----------------------------
DROP TABLE IF EXISTS `sms_platform_region`;
CREATE TABLE `sms_platform_region`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `platform_id` int(11) NULL DEFAULT NULL,
  `region_id` int(11) NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_sms_platform_region_platform_id_group_id`(`platform_id`, `region_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 14 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_platform_region
-- ----------------------------

-- ----------------------------
-- Table structure for sms_record_plan
-- ----------------------------
DROP TABLE IF EXISTS `sms_record_plan`;
CREATE TABLE `sms_record_plan`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `snap` tinyint(1) NULL DEFAULT 0,
  `name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 5 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_record_plan
-- ----------------------------

-- ----------------------------
-- Table structure for sms_record_plan_item
-- ----------------------------
DROP TABLE IF EXISTS `sms_record_plan_item`;
CREATE TABLE `sms_record_plan_item`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `start` int(11) NULL DEFAULT NULL,
  `stop` int(11) NULL DEFAULT NULL,
  `week_day` int(11) NULL DEFAULT NULL,
  `plan_id` int(11) NULL DEFAULT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 14 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_record_plan_item
-- ----------------------------

-- ----------------------------
-- Table structure for sms_resources_tree
-- ----------------------------
DROP TABLE IF EXISTS `sms_resources_tree`;
CREATE TABLE `sms_resources_tree`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `is_catalog` tinyint(1) NULL DEFAULT 1,
  `device_channel_id` int(11) NULL DEFAULT NULL,
  `gb_stream_id` int(11) NULL DEFAULT NULL,
  `name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `parentId` int(11) NULL DEFAULT NULL,
  `path` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_resources_tree
-- ----------------------------

-- ----------------------------
-- Table structure for sms_stream_proxy
-- ----------------------------
DROP TABLE IF EXISTS `sms_stream_proxy`;
CREATE TABLE `sms_stream_proxy`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `type` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `app` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `stream` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `src_url` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `timeout` int(11) NULL DEFAULT NULL,
  `ffmpeg_cmd_key` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `rtsp_type` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `media_server_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `enable_audio` tinyint(1) NULL DEFAULT 0,
  `enable_mp4` tinyint(1) NULL DEFAULT 0,
  `pulling` tinyint(1) NULL DEFAULT 0,
  `enable` tinyint(1) NULL DEFAULT 0,
  `enable_remove_none_reader` tinyint(1) NULL DEFAULT 0,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `name` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `stream_key` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `enable_disable_none_reader` tinyint(1) NULL DEFAULT 0,
  `relates_media_server_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_stream_proxy_app_stream`(`app`, `stream`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 5 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_stream_proxy
-- ----------------------------

-- ----------------------------
-- Table structure for sms_stream_push
-- ----------------------------
DROP TABLE IF EXISTS `sms_stream_push`;
CREATE TABLE `sms_stream_push`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `app` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `stream` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `media_server_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `server_id` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `push_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `status` tinyint(1) NULL DEFAULT 0,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `pushing` tinyint(1) NULL DEFAULT 0,
  `self` tinyint(1) NULL DEFAULT 0,
  `start_offline_push` tinyint(1) NULL DEFAULT 1,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE,
  UNIQUE INDEX `uk_stream_push_app_stream`(`app`, `stream`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 17 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_stream_push
-- ----------------------------

-- ----------------------------
-- Table structure for sms_user_api_key
-- ----------------------------
DROP TABLE IF EXISTS `sms_user_api_key`;
CREATE TABLE `sms_user_api_key`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) NULL DEFAULT NULL,
  `app` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `api_key` text CHARACTER SET utf8 COLLATE utf8_bin NULL,
  `expired_at` bigint(20) NULL DEFAULT NULL,
  `remark` varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `enable` tinyint(1) NULL DEFAULT 1,
  `create_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `update_time` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of sms_user_api_key
-- ----------------------------

SET FOREIGN_KEY_CHECKS = 1;

-- Table structure for sms_user
create table sms_user (
                          id serial primary key,
                          username character varying(255),
                          password character varying(255),
                          role_id integer,
                          create_time character varying(50),
                          update_time character varying(50),
                          push_key character varying(50),
                          constraint uk_user_username unique (username)
);

-- Table structure for sms_user_role
create table sms_user_role (
                               id serial primary key,
                               name character varying(50),
                               authority character varying(50),
                               create_time character varying(50),
                               update_time character varying(50)
);

-- Table structure for sms_log
create table sms_log (
                         id serial primary key ,
                         name character varying(50),
                         type character varying(50),
                         uri character varying(200),
                         address character varying(50),
                         result character varying(50),
                         timing bigint,
                         username character varying(50),
                         create_time character varying(50)
);