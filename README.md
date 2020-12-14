esimdjson
=====

An Erlang NIF wrapper for the [simdjson](https://github.com/simdjson/simdjson) library.

**WARNING**: The software contained in this repository is a work in progress.

Features
--------
- [ ] Exception handling
- [ ] Benchmarks
- [ ] DOM API
    - [x] parse binary
    - [x] parse file
    - [ ] parser options
- [ ] [On-demand API](https://github.com/simdjson/simdjson/blob/master/doc/ondemand.md)
- [ ] Tests


Usage
-----
Allocate buffers to re-use across multiple documents by creating a new parser:
```erlang
1> {ok, Parser} = esimdjson:new([]).
{ok,#Ref<0.1676207467.4139122690.98770>}
```
Parse a binary:
```erlang
2> esimdjson:parse(Parser, <<"[1,2,3]">>).
{ok,[1,2,3]}
```
And another one:
```erlang
3> esimdjson:parse(Parser, <<"{\"name\": \"B. E. Muser\", \"age\": 23}">>).
{ok,#{<<"age">> => 23,<<"name">> => <<"B. E. Muser">>}}
```

You can also load and parse from a file:
```erlang
3> esimdjson:load(Parser, "employees.json").
{ok,[#{<<"age">> => 23,<<"name">> => <<"B. E. Muser">>},
     #{<<"age">> => 30,<<"name">> => <<"Al O. Cater">>},
     #{<<"age">> => 52,<<"name">> => <<"Joe Armstrong">>}]}
```

Build
-----
```bash
$ rebar3 compile
```

**NOTE**: Your compiler will have to support [C++17](https://en.wikipedia.org/wiki/C%2B%2B17) if you want to build the NIF binaries,
since `simdjson` uses the `std::string_view` class.
