workon tabular_predDB
pkill -f python\.\*server_jsonrpc.py
nohup python -u tabular_predDB/jsonrpc_http/server_jsonrpc.py >server_jsonrpc.out 2>server_jsonrpc.err &
