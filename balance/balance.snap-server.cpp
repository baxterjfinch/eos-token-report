#include "balance.snap.hpp"
#include <eosio/database.hpp>
#include <eosio/input_output.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/types.h>
#include <eosio/crypto.hpp>

#include <eosio.system/eosio.system.hpp>

// refunds table in eosio.system
struct eosiosystem_refund_request {
    eosio::name           owner;
    eosio::time_point_sec request_time;
    eosio::asset          net_amount;
    eosio::asset          cpu_amount;
};

// Identify system token
eosio::symbol get_system_token(uint32_t block) {
    auto s = query_database(eosio::query_contract_row_range_code_table_scope_pk{
        .snapshot_block = block,
        .first =
            {
                .code        = "eosio"_n,
                .table       = "rammarket"_n,
                .scope       = "eosio"_n.value,
                .primary_key = 0,
            },
        .last =
            {
                .code        = "eosio"_n,
                .table       = "rammarket"_n,
                .scope       = "eosio"_n.value,
                .primary_key = 0xffff'ffff'ffff'ffff,
            },
        .max_results = 100,
    });

    eosio::symbol symbol;
    bool          found = false;
    eosio::for_each_contract_row<eosiosystem::exchange_state>(s, [&](eosio::contract_row& r, auto* state) {
        if (r.present && state) {
            eosio::check(!found, "Found more than 1 rammarket row");
            symbol = state->quote.balance.symbol;
            found  = true;
        }
        return true;
    });
    eosio::check(found, "Didn't find a rammarket row");
    return symbol;
}

// Fetch liquid balances
void process(get_liquid_request& req, const eosio::database_status& status) {
    if (req.block < status.first || req.block > status.irreversible)
        eosio::check(false, "requested block is out of range");
    auto system_token = get_system_token(req.block);

    auto s = query_database(eosio::query_contract_row_range_code_table_pk_scope{
        .snapshot_block = req.block,
        .first =
            {
                .code        = "eosio.token"_n,
                .table       = "accounts"_n,
                .primary_key = system_token.code().raw(),
                .scope       = req.first_account.value,
            },
        .last =
            {
                .code        = "eosio.token"_n,
                .table       = "accounts"_n,
                .primary_key = system_token.code().raw(),
                .scope       = 0xffff'ffff'ffff'ffff,
            },
        .max_results = 100,
    });

    get_liquid_response response;
    eosio::for_each_contract_row<eosio::asset>(s, [&](eosio::contract_row& r, eosio::asset* a) {
        if (r.present && a)
            response.balances.push_back(balance{eosio::name{r.scope}, a->amount});
        return true;
    });
    if (!response.balances.empty())
        response.more = eosio::name{response.balances.back().account.value + 1};
    eosio::set_output_data(pack(balance_snap_query_response{std::move(response)}));
}

// Fetch staked balances
void process(get_staked_request& req, const eosio::database_status& status) {
    if (req.block < status.first || req.block > status.irreversible)
        eosio::check(false, "requested block is out of range");
    auto system_token = get_system_token(req.block);

    auto s = query_database(eosio::query_contract_row_range_code_table_scope_pk{
        .snapshot_block = req.block,
        .first =
            {
                .code        = "eosio"_n,
                .table       = "voters"_n,
                .scope       = "eosio"_n.value,
                .primary_key = req.first_account.value,
            },
        .last =
            {
                .code        = "eosio"_n,
                .table       = "voters"_n,
                .scope       = "eosio"_n.value,
                .primary_key = 0xffff'ffff'ffff'ffff,
            },
        .max_results = 100,
    });

    get_staked_response response;
    eosio::for_each_contract_row<eosiosystem::voter_info>(s, [&](eosio::contract_row& r, eosiosystem::voter_info* info) {
        if (r.present && info)
            response.balances.push_back(balance{info->owner, info->staked});
        return true;
    });
    if (!response.balances.empty())
        response.more = eosio::name{response.balances.back().account.value + 1};
    eosio::set_output_data(pack(balance_snap_query_response{std::move(response)}));
}

// Fetch refund balances
void process(get_refund_request& req, const eosio::database_status& status) {
    if (req.block < status.first || req.block > status.irreversible)
        eosio::check(false, "requested block is out of range");
    auto system_token = get_system_token(req.block);

    auto s = query_database(eosio::query_contract_row_range_code_table_scope_pk{
        .snapshot_block = req.block,
        .first =
            {
                .code        = "eosio"_n,
                .table       = "refunds"_n,
                .scope       = req.first_account.value,
                .primary_key = 0,
            },
        .last =
            {
                .code        = "eosio"_n,
                .table       = "refunds"_n,
                .scope       = 0xffff'ffff'ffff'ffff,
                .primary_key = 0xffff'ffff'ffff'ffff,
            },
        .max_results = 100,
    });

    get_refund_response response;
    eosio::for_each_contract_row<eosiosystem_refund_request>(s, [&](eosio::contract_row& r, eosiosystem_refund_request* refund) {
        if (r.present && refund)
            response.balances.push_back(balance{refund->owner, refund->net_amount.amount + refund->cpu_amount.amount});
        return true;
    });
    if (!response.balances.empty())
        response.more = eosio::name{response.balances.back().account.value + 1};
    eosio::set_output_data(pack(balance_snap_query_response{std::move(response)}));
}

extern "C" __attribute__((eosio_wasm_entry)) void initialize() {}

extern "C" void run_query() {
    auto request = eosio::unpack<balance_snap_query_request>(eosio::get_input_data());
    std::visit([](auto& x) { process(x, eosio::get_database_status()); }, request.value);
}
