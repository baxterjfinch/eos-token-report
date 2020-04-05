const HistoryTools = require('./src/HistoryTools.js');
const fs = require('fs');
const { TextDecoder, TextEncoder } = require('util');
const fetch = require('node-fetch');
const encoder = new TextEncoder('utf8');
const decoder = new TextDecoder('utf8');

// Run one of the balance.snap queries and merge the results
// into accounts.
async function query(block, wasm, type, accounts, field) {
    let first_account = '';
    let num_found = 0;
    while (first_account != null) {
        // Create request
        const request = wasm.createQueryRequest(JSON.stringify(
            [type, {
                block,
                first_account,
            }]
        ));

        // Fetch reply
        const queryReply = await fetch('http://127.0.0.1:8880/wasmql/v1/query', {
            method: 'POST',
            body: HistoryTools.combineRequests([
                request,
            ]),
        });
        if (queryReply.status !== 200)
            throw new Error(queryReply.status + ': ' + queryReply.statusText + ': ' + await queryReply.text());

        // Convert from binary
        const responses = HistoryTools.splitResponses(new Uint8Array(await queryReply.arrayBuffer()));
        const response = JSON.parse(wasm.decodeQueryResponse(responses[0]))[1];

        // Merge result into accounts
        for (let bal of response.balances) {
            let account = accounts[bal.account] = accounts[bal.account] || {};
            account[field] = bal.amount;
        }
        num_found += response.balances.length;
        console.log(field, num_found, 'more=' + response.more);

        // Results span multiple queries
        first_account = response.more;
    }
}

if (process.argv.length != 3) {
    console.log('usage: nodejs balance.snap.js block_num');
} else {
    const block = +process.argv[2];
    (async () => {
        try {
            const myClientWasm = await HistoryTools.createClientWasm({
                mod: new WebAssembly.Module(fs.readFileSync('balance.snap-client.wasm')),
                encoder, decoder
            });
            let accounts = {};
            await query(block, myClientWasm, 'get.liquid', accounts, 'liquid');
            await query(block, myClientWasm, 'get.staked', accounts, 'staked');
            await query(block, myClientWasm, 'get.refund', accounts, 'refund');
            let csv = '';
            for (let name of Object.keys(accounts).sort()) {
                const acc = accounts[name];
                csv += `"${name}",${acc.liquid || 0},${acc.staked || 0},${acc.refund || 0}` + '\n';
            }
            fs.writeFileSync(block + '.csv', csv);
            console.log('wrote to ' + block + '.csv');
        } catch (e) {
            console.log(e);
        }
    })()
}
