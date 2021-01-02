#!/bin/bash
curl -X GET -i -H "Accept: application/json" -d '{"jsonrpc": "2.0"}' http://127.0.0.1:30159/stop_daemon
