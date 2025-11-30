To build

```
mkdir build
cd build
cmake ..
make
```

Running examples

```
./examples/kbgdb_service_example --rules_file ../examples/rules.txt

```

Then from another terminal

```
curl -X POST http://localhost:8080/api/query \                                    
     -H "Content-Type: application/json" \
     -d '{"query": "parent(john, ?X)"}'
[{"X":"bob"},{"X":"mary"}]

"program": "${workspaceFolder}/build/examples/simple_query_example",
"args": [
     "--rules_file", "../examples/rules.txt", "--query", "grandparent(john, ?X)"
],
```