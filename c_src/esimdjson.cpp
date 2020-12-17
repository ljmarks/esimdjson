#include "esimdjson.h"

int load(ErlNifEnv *env, void **priv_data, const ERL_NIF_TERM load_info) {
  ErlNifResourceFlags flags =
      ErlNifResourceFlags(ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER);
  ErlNifResourceType *res_type = enif_open_resource_type(
      env, nullptr, "esimdjson_dom_parser", dom_parser_dtor, flags, nullptr);
  if (!res_type)
    return -1;
  *priv_data = (void *)res_type;

  // Make atoms
  // Static variables are used here to avoid making atoms in the NIF callbacks.
  // See:
  // https://github.com/erlang/otp/blob/master/erts/emulator/nifs/common/erl_tracer_nif.c
  atom_ok = enif_make_atom(env, "ok");
  atom_error = enif_make_atom(env, "error");
  atom_null = enif_make_atom(env, "null");
  atom_true = enif_make_atom(env, "true");
  atom_false = enif_make_atom(env, "false");
  atom_fixed_capacity = enif_make_atom(env, "fixed_capacity");
  atom_max_capacity = enif_make_atom(env, "max_capacity");

  return 0;
}

ERL_NIF_TERM make_atom(ErlNifEnv *env, const char *atom) {
  ERL_NIF_TERM ret;

  if (!enif_make_existing_atom(env, atom, &ret, ERL_NIF_LATIN1)) {
    return enif_make_atom(env, atom);
  }

  return ret;
}

ERL_NIF_TERM make_ok_result(ErlNifEnv *env, const ERL_NIF_TERM result) {
  return enif_make_tuple2(env, atom_ok, result);
}

ERL_NIF_TERM make_error(ErlNifEnv *env, const ERL_NIF_TERM reason) {
  return enif_make_tuple2(env, atom_error, reason);
}

ERL_NIF_TERM make_simdjson_error(ErlNifEnv *env,
                                 const simdjson::error_code error) {
  ERL_NIF_TERM reason_atom = make_atom(env, error_code_txt[error].txt);
  std::string error_str = simdjson::error_message(error);
  ERL_NIF_TERM reason_str =
      enif_make_string(env, error_str.data(), ERL_NIF_LATIN1);
  ERL_NIF_TERM reason = enif_make_tuple2(env, reason_atom, reason_str);

  return make_error(env, reason);
}

ERL_NIF_TERM nif_new(ErlNifEnv *env, const int argc,
                     const ERL_NIF_TERM argv[]) {
  /*TODO:
   * - max_capacity option
   * (https://github.com/simdjson/simdjson/blob/master/doc/performance.md#reusing-the-parser-for-maximum-efficiency)
   * - fixed_capacity option (ditto)
   */
  ERL_NIF_TERM opt_cdr;
  ERL_NIF_TERM opt_car;
  size_t max_cap = 0;
  size_t fixed_cap = 0;

  if (argc != 1 || !enif_is_list(env, (opt_cdr = argv[0])))
    return enif_make_badarg(env);

  while (enif_get_list_cell(env, opt_cdr, &opt_car, &opt_cdr)) {
    if (get_max_capacity(env, opt_car, &max_cap))
      continue;
    else if (get_fixed_capacity(env, opt_car, &fixed_cap))
      continue;
    else
      return enif_make_badarg(env);
  }

  // It only makes sense to specify one of fixed_capacity or max_capacity
  if (max_cap && fixed_cap)
    return enif_make_badarg(env);

  ErlNifResourceType *res_type = (ErlNifResourceType *)enif_priv_data(env);

  void *parser_res =
      enif_alloc_resource(res_type, sizeof(simdjson::dom::parser));

  simdjson::dom::parser *parser;
  if (max_cap)
    parser = new (parser_res) simdjson::dom::parser(max_cap);
  else if (fixed_cap) {
    parser = new (parser_res) simdjson::dom::parser(0);
    auto error = parser->allocate(fixed_cap);
    if (error)
      return enif_make_badarg(env);
  } else
    parser = new (parser_res) simdjson::dom::parser();

  ERL_NIF_TERM res_term = enif_make_resource(env, parser_res);
  enif_release_resource(parser_res);

  return make_ok_result(env, res_term);
}

ERL_NIF_TERM nif_load(ErlNifEnv *env, const int argc,
                      const ERL_NIF_TERM argv[]) {
  if (argc != 2)
    return enif_make_badarg(env);

  ErlNifResourceType *res_type = (ErlNifResourceType *)enif_priv_data(env);
  simdjson::dom::parser *pparser;
  if (!enif_get_resource(env, argv[0], res_type, (void **)&pparser))
    return enif_make_badarg(env);

  unsigned int path_size;
  if (!enif_get_list_length(env, argv[1], &path_size))
    return enif_make_badarg(env);
  std::unique_ptr<char[]> path{new char[path_size + 1]};
  if (!enif_get_string(env, argv[1], path.get(), path_size + 1, ERL_NIF_LATIN1))
    return enif_make_badarg(env);

  simdjson::dom::element element;

  auto error = pparser->load(path.get()).get(element);
  if (error) {
    return make_simdjson_error(env, error);
  }

  ERL_NIF_TERM result;
  make_term_from_dom(env, element, &result);

  return make_ok_result(env, result);
}

ERL_NIF_TERM nif_parse(ErlNifEnv *env, const int argc,
                       const ERL_NIF_TERM argv[]) {
  if (argc != 2)
    return enif_make_badarg(env);

  ErlNifResourceType *res_type = (ErlNifResourceType *)enif_priv_data(env);
  simdjson::dom::parser *pparser;
  if (!enif_get_resource(env, argv[0], res_type, (void **)&pparser))
    return enif_make_badarg(env);

  ErlNifBinary bin;
  if (!enif_inspect_binary(env, argv[1], &bin))
    return enif_make_badarg(env);

  simdjson::dom::element element;
  auto error = pparser->parse((char *)bin.data, bin.size).get(element);
  if (error)
    return make_simdjson_error(env, error);

  ERL_NIF_TERM result;
  make_term_from_dom(env, element, &result);

  return make_ok_result(env, result);
}

ERL_NIF_TERM nif_max_capacity(ErlNifEnv *env, const int argc,
                              const ERL_NIF_TERM argv[]) {
  if (argc != 1)
    return enif_make_badarg(env);

  ErlNifResourceType *res_type = (ErlNifResourceType *)enif_priv_data(env);
  simdjson::dom::parser *pparser;
  if (!enif_get_resource(env, argv[0], res_type, (void **)&pparser))
    return enif_make_badarg(env);

  ERL_NIF_TERM result = enif_make_uint64(env, pparser->max_capacity());

  return make_ok_result(env, result);
}

int get_max_capacity(ErlNifEnv *env, const ERL_NIF_TERM opt, size_t *max_cap) {
  int arity = 0;
  int ret = 0;
  const ERL_NIF_TERM *tuple_array;
  if (enif_get_tuple(env, opt, &arity, &tuple_array) && arity == 2 &&
      enif_is_identical(tuple_array[0], atom_max_capacity) &&
      enif_get_uint64(env, tuple_array[1], max_cap))
    ret = 1;

  return ret;
}

int get_fixed_capacity(ErlNifEnv *env, const ERL_NIF_TERM opt,
                       size_t *fixed_cap) {
  int arity = 0;
  int ret = 0;
  const ERL_NIF_TERM *tuple_array;
  if (enif_get_tuple(env, opt, &arity, &tuple_array) && arity == 2 &&
      enif_is_identical(tuple_array[0], atom_fixed_capacity) &&
      enif_get_uint64(env, tuple_array[1], fixed_cap))
    ret = 1;

  return ret;
}

int make_term_from_dom(ErlNifEnv *env, const simdjson::dom::element element,
                       ERL_NIF_TERM *term) {
  switch (element.type()) {
  case simdjson::dom::element_type::INT64:
    *term = enif_make_int64(env, int64_t(element));
    break;
  case simdjson::dom::element_type::UINT64:
    *term = enif_make_uint64(env, uint64_t(element));
    break;
  case simdjson::dom::element_type::DOUBLE:
    *term = enif_make_double(env, double(element));
    break;
  case simdjson::dom::element_type::BOOL: {
    *term = bool(element) ? atom_true : atom_false;
  } break;
  case simdjson::dom::element_type::NULL_VALUE: {
    *term = atom_null;
  } break;
  case simdjson::dom::element_type::STRING: {
    std::string_view str = std::string_view(element);
    char *str_bin = (char *)enif_make_new_binary(env, str.size(), term);
    str.copy(str_bin, str.size());
  } break;
  case simdjson::dom::element_type::OBJECT: {
    simdjson::dom::object obj = simdjson::dom::object(element);
    // TODO: VLAs are (probably?) a bad idea. Use something
    // which is more explicit about asking for stack memory.
    ERL_NIF_TERM keys[obj.size()];
    ERL_NIF_TERM values[obj.size()];

    size_t i = 0;
    for (auto [key, value] : obj) {
      ERL_NIF_TERM k;
      char *k_bin = (char *)enif_make_new_binary(env, key.size(), &k);
      key.copy(k_bin, key.size());

      ERL_NIF_TERM v;
      make_term_from_dom(env, value, &v);
      keys[i] = k;
      values[i] = v;
      i++;
    }
    enif_make_map_from_arrays(env, keys, values, obj.size(), term);

  } break;
  case simdjson::dom::element_type::ARRAY: {
    ERL_NIF_TERM cons = enif_make_list(env, 0);

    simdjson::dom::array array = simdjson::dom::array(element);

    for (size_t i = array.size(); i > 0; i--) {
      simdjson::dom::element e = array.at(i - 1);
      ERL_NIF_TERM car;
      make_term_from_dom(env, e, &car);
      cons = enif_make_list_cell(env, car, cons);
    }
    *term = cons;

  } break;
  default:
    break;
  }

  return 0;
}

void dom_parser_dtor(ErlNifEnv *env, void *obj) {
  // Memory deallocation is done by Erlang GC since we released the resource
  // with `enif_release_resource`, so we only need to do object destruction.
  simdjson::dom::parser *pparser = (simdjson::dom::parser *)obj;
  pparser->~parser();
}

static ErlNifFunc nif_funcs[] = {
    {"parse", 2, nif_parse, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"load", 2, nif_load, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"new", 1, nif_new},
    {"max_capacity", 1, nif_max_capacity},
};

ERL_NIF_INIT(esimdjson, nif_funcs, load, nullptr, nullptr, nullptr)
