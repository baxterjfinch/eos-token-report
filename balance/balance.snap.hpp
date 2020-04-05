#pragma once
#include <eosio/asset.hpp>
#include <eosio/block_select.hpp>

struct base_request {
    uint32_t    block         = {};
    eosio::name first_account = {};
};

STRUCT_REFLECT(base_request) {
    STRUCT_MEMBER(base_request, block)
    STRUCT_MEMBER(base_request, first_account)
}

struct get_liquid_request : base_request {};
struct get_staked_request : base_request {};
struct get_refund_request : base_request {};

struct balance {
    eosio::name account = {};
    int64_t     amount  = {};
};

STRUCT_REFLECT(balance) {
    STRUCT_MEMBER(balance, account)
    STRUCT_MEMBER(balance, amount)
}

struct base_response {
    std::vector<balance>       balances = {};
    std::optional<eosio::name> more     = {};

    EOSLIB_SERIALIZE(base_response, (balances)(more))
};

STRUCT_REFLECT(base_response) {
    STRUCT_MEMBER(base_response, balances)
    STRUCT_MEMBER(base_response, more)
}

struct get_liquid_response : base_response {};
struct get_staked_response : base_response {};
struct get_refund_response : base_response {};

using balance_snap_query_request = eosio::tagged_variant<
    eosio::serialize_tag_as_name,
    eosio::tagged_type<"get.liquid"_n, get_liquid_request>,
    eosio::tagged_type<"get.staked"_n, get_staked_request>,
    eosio::tagged_type<"get.refund"_n, get_refund_request>>;

using balance_snap_query_response = eosio::tagged_variant<
    eosio::serialize_tag_as_name,
    eosio::tagged_type<"get.liquid"_n, get_liquid_response>,
    eosio::tagged_type<"get.staked"_n, get_staked_response>,
    eosio::tagged_type<"get.refund"_n, get_refund_response>>;
