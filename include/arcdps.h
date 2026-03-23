// arcdps.h - Minimal Safe Version
#pragma once
#include <stdint.h>

// 基础类型
typedef int32_t id_t;
typedef uint64_t evtc_time_t;

// 简化的事件结构
struct cbtevent {
    evtc_time_t time;
    uint16_t src_agent;
    uint16_t dst_agent;
    int32_t value;
    int32_t buff_dmg;
    uint32_t overstack_value;
    int32_t result;
    int32_t is_shield_damage;
    id_t skill_id;
    id_t src_instid;
    id_t dst_instid;
    id_t src_master_instid;
    id_t dst_master_instid;
    uint8_t result_flags;
    uint8_t is_flanking_src;
    uint8_t is_flanking_dst;
    uint8_t phase;
    id_t src_masterid;
    id_t dst_masterid;
};

// Agent 结构
struct agent {
    char* name;
    uint64_t address;
    uint64_t profession;
    uint64_t elite;
    uint64_t team;
    uint64_t type;
    uint64_t flags;
    uint64_t self;
    uint64_t name_len;
    char* profession_name;
    char* elite_name;
};

// 技能信息
struct skill_info {
    char* name;
    uint64_t icon;
    uint64_t range;
    uint64_t type;
    uint64_t flags;
    uint64_t ground;
    uint64_t flip;
    uint64_t recharge;
    uint64_t duration;
    uint64_t name_len;
};

// 函数指针类型
typedef uint32_t (*gv_version_t)();
typedef bool (*gv_init_t)(const char* version, uint32_t ver);
typedef void (*gv_combat_local_function_t)(cbtevent* ev, agent* src, agent* dst, int64_t skill_name, skill_info* skill);
typedef void (*gv_combat_local_ex_function_t)(cbtevent* ev, agent* src, agent* dst, int64_t skill_name, skill_info* skill, uint64_t time);
