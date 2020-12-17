esimdjson
=====

An Erlang NIF wrapper for the [simdjson](https://github.com/simdjson/simdjson) library.

**WARNING**: The software contained in this repository is a work in progress.

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

The `load/2` and `parse/` functions can return an error of the form
`{error, {Reason, Msg}}`, like this:
```erlang
4> esimdjson:parse(Parser, <<"[1, ">>).
{error,{tape_error,"The JSON document has an improper structure: missing or superfluous commas, braces, missing keys, etc."}}
```

The `simdjson` library will automatically allocate more memory for a parser as
required. You can control this behaviour by passing options to `new/1`.
To fix the capacity of the parser, use `{fixed_capacity, N}` where `N` is in bytes:
```erlang
1> {ok, Parser} = esimdjson:new([{fixed_capacity, 10}]).           
{ok,#Ref<0.1487449649.1034027009.156176>}                          
2> esimdjson:parse(Parser, <<"[1,2,3,4,5,6,7,8,9,10]">>).          
{error,{capacity,"This parser can't support a document that big"}} 
```
If you would like `simdjson` to expand the capacity when necessary, but not beyond
`M` bytes, use the `{max_capacity, M}` option.


Build
-----
```bash
$ rebar3 compile
```

Emit debugging information for GDB:
```bash
$ pushd c_src; make clean; CXX_DEBUG=true make; popd
```

**NOTE**: Your compiler will have to support [C++17](https://en.wikipedia.org/wiki/C%2B%2B17) if you want to build the NIF binaries,
since `simdjson` uses the `std::string_view` class.

Features
--------
- [x] Basic error handling
- [ ] Benchmarks
- [x] DOM API
    - [x] Arrays
    - [x] Objects
    - [x] String type
    - [x] Number types
    - [x] Null type
    - [x] Boolean type
    - [x] Parse binary
    - [x] Parse file
    - [x] Parser options
- [ ] [On-demand API](https://github.com/simdjson/simdjson/blob/master/doc/ondemand.md)
- [ ] Tests
