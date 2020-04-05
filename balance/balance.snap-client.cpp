#include "balance.snap.hpp"
#include <eosio/input_output.hpp>
#include <eosio/parse_json.hpp>
#include <eosio/schema.hpp>

extern "C" __attribute__((eosio_wasm_entry)) void initialize() {}
extern "C" void describe_query_request() { eosio::set_output_data(eosio::make_json_schema<balance_snap_query_request>()); }
extern "C" void describe_query_response() { eosio::set_output_data(eosio::make_json_schema<balance_snap_query_response>()); }

extern "C" void create_query_request() {
    eosio::set_output_data(
        pack(std::make_tuple("local"_n, "balance.snap"_n, eosio::parse_json<balance_snap_query_request>(eosio::get_input_data()))));
}

extern "C" void decode_query_response() {
    eosio::set_output_data(to_json(eosio::unpack<balance_snap_query_response>(eosio::get_input_data())));
}
