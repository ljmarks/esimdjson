-module(esimdjson).
-export([new/0, new/1, load/2, parse/2, max_capacity/1]).
-on_load(init/0).

-define(APPNAME, esimdjson).
-define(LIBNAME, esimdjson).

-type esimdjson_option() :: {max_capacity, integer()} | {fixed_capacity, integer()}.
-type esimdjson_options() :: [esimdjson_option()].
-type esimdjson_parser() :: any().
-type esimdjson_error_reason() :: capacity
                                | memalloc
                                | depth_error
                                | string_error
                                | t_atom_error
                                | f_atom_error
                                | n_atom_error
                                | utf8_error
                                | uninitialized
                                | empty
                                | unescaped_chars
                                | unclosed_string
                                | unsupported_architecture
                                | incorrect_type
                                | number_out_of_range
                                | index_out_of_bounds
                                | no_such_field
                                | io_error
                                | invalid_json_pointer
                                | invalid_uri_fragment
                                | unexpected_error
                                | parser_in_use.
-type esimdjson_error() :: {error, {esimdjson_error_reason(), string()}}.

-spec new() -> {ok, any()} | esimdjson_error().
new() ->
    new([]).

-spec new(Opts :: esimdjson_options()) -> {ok, esimdjson_parser()} | esimdjson_error().
new(_) ->
    not_loaded(?LINE).

-spec load(Parser :: esimdjson_parser(),
           Path :: string()) -> {ok, term()} | esimdjson_error().
load(_, _) ->
    not_loaded(?LINE).

-spec parse(Parser :: esimdjson_parser(),
            Binary :: binary()) -> {ok, term()} | esimdjson_error().
parse(_, _) ->
    not_loaded(?LINE).

-spec max_capacity(Parser :: esimdjson_parser()) -> {ok, integer()}.
max_capacity(_) ->
    not_loaded(?LINE).

init() ->
    SoName = case code:priv_dir(?APPNAME) of
        {error, bad_name} ->
            case filelib:is_dir(filename:join(["..", priv])) of
                true ->
                    filename:join(["..", priv, ?LIBNAME]);
                _ ->
                    filename:join([priv, ?LIBNAME])
            end;
        Dir ->
            filename:join(Dir, ?LIBNAME)
    end,
    erlang:load_nif(SoName, 0).

not_loaded(Line) ->
    erlang:nif_error({not_loaded, [{module, ?MODULE}, {line, Line}]}).
